from dataclasses import dataclass, field
from enum import Enum
from typing import Optional


class AlertSeverity(str, Enum):
    NONE = "none"
    WATCH = "watch"
    CAUTION = "caution"
    CRITICAL = "critical"


class InstabilityType(str, Enum):
    STEP_VARIABILITY = "step_variability"
    LATERAL_SWAY = "lateral_sway"
    CADENCE_DROP = "cadence_drop"
    ASYMMETRY = "asymmetry"
    NEAR_FALL = "near_fall"
    RAPID_DECELERATION = "rapid_deceleration"


@dataclass
class GaitFeatures:
    step_time_variability: float = 0.0
    lateral_sway_rms: float = 0.0
    cadence_steps_per_min: float = 0.0
    cadence_drop_ratio: float = 0.0
    step_asymmetry: float = 0.0
    pitch_variance: float = 0.0
    roll_variance: float = 0.0
    impact_spike: float = 0.0
    stability_score: float = 100.0

    def to_dict(self) -> dict:
        return {
            "step_time_variability": round(self.step_time_variability, 4),
            "lateral_sway_rms": round(self.lateral_sway_rms, 4),
            "cadence_steps_per_min": round(self.cadence_steps_per_min, 2),
            "cadence_drop_ratio": round(self.cadence_drop_ratio, 4),
            "step_asymmetry": round(self.step_asymmetry, 4),
            "pitch_variance": round(self.pitch_variance, 4),
            "roll_variance": round(self.roll_variance, 4),
            "impact_spike": round(self.impact_spike, 4),
            "stability_score": round(self.stability_score, 2),
        }


@dataclass
class InstabilityFlag:
    type: InstabilityType
    severity: AlertSeverity
    value: float
    threshold: float
    message: str

    def to_dict(self) -> dict:
        return {
            "type": self.type.value,
            "severity": self.severity.value,
            "value": round(self.value, 4),
            "threshold": self.threshold,
            "message": self.message,
        }


@dataclass
class AnalysisResult:
    features: GaitFeatures
    flags: list[InstabilityFlag] = field(default_factory=list)
    overall_severity: AlertSeverity = AlertSeverity.NONE
    is_unstable: bool = False

    def to_dict(self) -> dict:
        return {
            "features": self.features.to_dict(),
            "flags": [f.to_dict() for f in self.flags],
            "overall_severity": self.overall_severity.value,
            "is_unstable": self.is_unstable,
        }


@dataclass
class AlertEvent:
    severity: AlertSeverity
    message: str
    voice_message: str
    instability_types: list[InstabilityType]
    timestamp: float
    stability_score: float
  # cooldown key for deduplication
    alert_key: str = ""

    def to_dict(self) -> dict:
        return {
            "severity": self.severity.value,
            "message": self.message,
            "voice_message": self.voice_message,
            "instability_types": [t.value for t in self.instability_types],
            "timestamp": self.timestamp,
            "stability_score": round(self.stability_score, 2),
            "alert_key": self.alert_key,
        }
