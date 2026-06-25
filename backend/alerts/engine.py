import time

from backend.gait.analyzer import GaitAnalyzer
from backend.gait.types import AlertEvent, AlertSeverity, AnalysisResult, InstabilityType
from backend.sensors.models import SensorReading


SEVERITY_RANK = {
    AlertSeverity.NONE: 0,
    AlertSeverity.WATCH: 1,
    AlertSeverity.CAUTION: 2,
    AlertSeverity.CRITICAL: 3,
}

VOICE_PREFIX = {
    AlertSeverity.WATCH: "",
    AlertSeverity.CAUTION: "Caution. ",
    AlertSeverity.CRITICAL: "Warning! ",
}


class AlertEngine:
    """Decides when to emit visual and voice warnings based on gait analysis."""

    def __init__(self, analyzer: GaitAnalyzer | None = None):
        self.analyzer = analyzer or GaitAnalyzer()
        self._consecutive_unstable = 0
        self._last_alert_time: dict[str, float] = {}
        self._last_voice_time = 0.0
        policy = self.analyzer.config.get("alert_policy", {})
        self.consecutive_required = policy.get("consecutive_unstable_windows", 2)
        self.voice_cooldown = policy.get("voice_cooldown_seconds", 8.0)
        self.cooldowns = {
            AlertSeverity.WATCH: policy.get("watch_cooldown_seconds", 3.0),
            AlertSeverity.CAUTION: policy.get("caution_cooldown_seconds", 6.0),
            AlertSeverity.CRITICAL: policy.get("critical_cooldown_seconds", 4.0),
        }

    def reset(self) -> None:
        self.analyzer.reset()
        self._consecutive_unstable = 0
        self._last_alert_time.clear()
        self._last_voice_time = 0.0

    def process_reading(self, reading: SensorReading) -> tuple[AnalysisResult | None, AlertEvent | None]:
        result = self.analyzer.add_reading(reading)
        if result is None:
            return None, None
        alert = self._evaluate_alert(result, reading.timestamp)
        return result, alert

    def process_batch(self, readings: list[SensorReading]) -> tuple[AnalysisResult, AlertEvent | None]:
        result = self.analyzer.analyze(readings)
        ts = readings[-1].timestamp if readings else time.time()
        alert = self._evaluate_alert(result, ts)
        return result, alert

    def _evaluate_alert(self, result: AnalysisResult, timestamp: float) -> AlertEvent | None:
        if result.overall_severity == AlertSeverity.NONE:
            self._consecutive_unstable = 0
            return None

        if result.is_unstable:
            self._consecutive_unstable += 1
        else:
            self._consecutive_unstable = 0
            return None

        if self._consecutive_unstable < self.consecutive_required:
            return None

        severity = result.overall_severity
        alert_key = self._build_alert_key(result)
        cooldown = self.cooldowns.get(severity, 5.0)
        last = self._last_alert_time.get(alert_key, 0.0)
        if timestamp - last < cooldown:
            return None

        self._last_alert_time[alert_key] = timestamp

        primary = max(result.flags, key=lambda f: SEVERITY_RANK[f.severity])
        voice_msg = self._build_voice_message(severity, primary.message, timestamp)

        return AlertEvent(
            severity=severity,
            message=primary.message,
            voice_message=voice_msg,
            instability_types=[f.type for f in result.flags],
            timestamp=timestamp,
            stability_score=result.features.stability_score,
            alert_key=alert_key,
        )

    def _build_alert_key(self, result: AnalysisResult) -> str:
        types = sorted(f.type.value for f in result.flags)
        return f"{result.overall_severity.value}:{','.join(types)}"

    def _build_voice_message(self, severity: AlertSeverity, message: str, timestamp: float) -> str:
        if timestamp - self._last_voice_time < self.voice_cooldown and severity != AlertSeverity.CRITICAL:
            return ""
        prefix = VOICE_PREFIX.get(severity, "")
        self._last_voice_time = timestamp
        return f"{prefix}{message}"
