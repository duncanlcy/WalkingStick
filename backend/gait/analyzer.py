import json
import math
from pathlib import Path

import numpy as np

from backend.gait.types import (
    AlertSeverity,
    AnalysisResult,
    GaitFeatures,
    InstabilityFlag,
    InstabilityType,
)
from backend.sensors.models import SensorReading


def _load_thresholds() -> dict:
    path = Path(__file__).resolve().parents[2] / "shared" / "gait_thresholds.json"
    with open(path) as f:
        return json.load(f)


class GaitAnalyzer:
    """Extract gait features from IMU sensor readings and detect instability."""

    def __init__(self, config: dict | None = None):
        self.config = config or _load_thresholds()
        self.thresholds = self.config["thresholds"]
        self.window_size = self.config["window_size"]
        self.min_samples = self.config["min_samples"]
        self.step_threshold = self.config["step_detection_threshold"]
        self._baseline_cadence: float | None = None
        self._history: list[SensorReading] = []

    def reset(self) -> None:
        self._baseline_cadence = None
        self._history.clear()

    def add_reading(self, reading: SensorReading) -> AnalysisResult | None:
        self._history.append(reading)
        if len(self._history) > self.window_size * 3:
            self._history = self._history[-self.window_size * 2 :]
        if len(self._history) < self.min_samples:
            return None
        window = self._history[-self.window_size :]
        return self.analyze(window)

    def analyze(self, readings: list[SensorReading]) -> AnalysisResult:
        if len(readings) < self.min_samples:
            return AnalysisResult(
                features=GaitFeatures(),
                overall_severity=AlertSeverity.NONE,
                is_unstable=False,
            )

        features = self._extract_features(readings)
        flags = self._evaluate_instability(features)
        overall = self._compute_overall_severity(flags)
        is_unstable = overall in (AlertSeverity.CAUTION, AlertSeverity.CRITICAL) or (
            overall == AlertSeverity.WATCH and len(flags) >= 2
        )

        return AnalysisResult(
            features=features,
            flags=flags,
            overall_severity=overall,
            is_unstable=is_unstable,
        )

    def _extract_features(self, readings: list[SensorReading]) -> GaitFeatures:
        accel_mag = np.array(
            [math.sqrt(r.accel_x**2 + r.accel_y**2 + r.accel_z**2) for r in readings]
        )
        lateral = np.array([r.accel_x for r in readings])
        timestamps = np.array([r.timestamp for r in readings])

        step_intervals = self._detect_steps(accel_mag, timestamps)
        step_var = float(np.std(step_intervals) / (np.mean(step_intervals) + 1e-6)) if len(step_intervals) >= 3 else 0.0

        cadence = 0.0
        if len(step_intervals) >= 2:
            mean_interval = float(np.mean(step_intervals))
            cadence = 60.0 / mean_interval if mean_interval > 0 else 0.0

        cadence_drop = 0.0
        if self._baseline_cadence is None and cadence > 0:
            self._baseline_cadence = cadence
        elif self._baseline_cadence and cadence > 0:
            cadence_drop = max(0.0, (self._baseline_cadence - cadence) / self._baseline_cadence)
            self._baseline_cadence = 0.95 * self._baseline_cadence + 0.05 * cadence

        asymmetry = self._compute_asymmetry(step_intervals)
        lateral_sway = float(np.sqrt(np.mean(lateral**2)))

        pitch_vals = []
        roll_vals = []
        for r in readings:
            pitch = math.degrees(math.atan2(r.accel_y, math.sqrt(r.accel_x**2 + r.accel_z**2 + 1e-6)))
            roll = math.degrees(math.atan2(r.accel_x, math.sqrt(r.accel_y**2 + r.accel_z**2 + 1e-6)))
            pitch_vals.append(pitch)
            roll_vals.append(roll)

        pitch_var = float(np.var(pitch_vals))
        roll_var = float(np.var(roll_vals))

        impact = float(np.max(accel_mag) - np.mean(accel_mag))

        stability = self._compute_stability_score(
            step_var, lateral_sway, cadence_drop, asymmetry, pitch_var, roll_var, impact
        )

        return GaitFeatures(
            step_time_variability=step_var,
            lateral_sway_rms=lateral_sway,
            cadence_steps_per_min=cadence,
            cadence_drop_ratio=cadence_drop,
            step_asymmetry=asymmetry,
            pitch_variance=pitch_var,
            roll_variance=roll_var,
            impact_spike=impact,
            stability_score=stability,
        )

    def _detect_steps(self, accel_mag: np.ndarray, timestamps: np.ndarray) -> list[float]:
        intervals: list[float] = []
        last_peak_time: float | None = None
        mean_mag = float(np.mean(accel_mag))

        for i in range(1, len(accel_mag) - 1):
            if accel_mag[i] > accel_mag[i - 1] and accel_mag[i] > accel_mag[i + 1]:
                if accel_mag[i] > mean_mag * self.step_threshold:
                    t = float(timestamps[i])
                    if last_peak_time is not None:
                        interval = t - last_peak_time
                        if 0.3 < interval < 2.0:
                            intervals.append(interval)
                    last_peak_time = t
        return intervals

    def _compute_asymmetry(self, intervals: list[float]) -> float:
        if len(intervals) < 4:
            return 0.0
        left = intervals[0::2]
        right = intervals[1::2]
        if not left or not right:
            return 0.0
        mean_left = float(np.mean(left))
        mean_right = float(np.mean(right))
        return abs(mean_left - mean_right) / (mean_left + mean_right + 1e-6)

    def _compute_stability_score(
        self,
        step_var: float,
        lateral_sway: float,
        cadence_drop: float,
        asymmetry: float,
        pitch_var: float,
        roll_var: float,
        impact: float,
    ) -> float:
        penalties = [
            min(30, step_var * 80),
            min(25, lateral_sway * 15),
            min(20, cadence_drop * 50),
            min(15, asymmetry * 60),
            min(15, (pitch_var + roll_var) / 8),
            min(20, impact * 5),
        ]
        return max(0.0, 100.0 - sum(penalties))

    def _evaluate_instability(self, features: GaitFeatures) -> list[InstabilityFlag]:
        flags: list[InstabilityFlag] = []
        checks = [
            (InstabilityType.STEP_VARIABILITY, features.step_time_variability, "step_time_variability"),
            (InstabilityType.LATERAL_SWAY, features.lateral_sway_rms, "lateral_sway_rms"),
            (InstabilityType.CADENCE_DROP, features.cadence_drop_ratio, "cadence_drop_ratio"),
            (InstabilityType.ASYMMETRY, features.step_asymmetry, "step_asymmetry"),
            (InstabilityType.NEAR_FALL, max(features.pitch_variance, features.roll_variance), "pitch_variance"),
            (InstabilityType.RAPID_DECELERATION, features.impact_spike, "impact_spike"),
        ]

        messages = self.config.get("messages", {})

        for inst_type, value, key in checks:
            thresh = self.thresholds.get(key, {})
            severity = self._classify_severity(value, thresh)
            if severity != AlertSeverity.NONE:
                msg_key = inst_type.value
                msg_map = messages.get(msg_key, {})
                message = msg_map.get(severity.value, f"Instability detected: {inst_type.value}")
                flags.append(
                    InstabilityFlag(
                        type=inst_type,
                        severity=severity,
                        value=value,
                        threshold=thresh.get(severity.value, 0),
                        message=message,
                    )
                )

        return flags

    def _classify_severity(self, value: float, thresholds: dict) -> AlertSeverity:
        if value >= thresholds.get("critical", float("inf")):
            return AlertSeverity.CRITICAL
        if value >= thresholds.get("caution", float("inf")):
            return AlertSeverity.CAUTION
        if value >= thresholds.get("watch", float("inf")):
            return AlertSeverity.WATCH
        return AlertSeverity.NONE

    def _compute_overall_severity(self, flags: list[InstabilityFlag]) -> AlertSeverity:
        if not flags:
            return AlertSeverity.NONE
        severity_order = [AlertSeverity.CRITICAL, AlertSeverity.CAUTION, AlertSeverity.WATCH]
        for sev in severity_order:
            if any(f.severity == sev for f in flags):
                return sev
        return AlertSeverity.NONE
