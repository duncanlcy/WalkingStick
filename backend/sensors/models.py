from dataclasses import dataclass, field
from enum import Enum
from typing import Optional


class WalkingBehavior(str, Enum):
    STABLE = "stable"
    UNSTABLE = "unstable"
    NEAR_FALL = "near_fall"
    FALL = "fall"


@dataclass
class SensorReading:
    timestamp: float
    accel_x: float
    accel_y: float
    accel_z: float
    gyro_x: float = 0.0
    gyro_y: float = 0.0
    gyro_z: float = 0.0
    behavior: Optional[WalkingBehavior] = None

    @classmethod
    def from_dict(cls, data: dict) -> "SensorReading":
        behavior = data.get("behavior")
        return cls(
            timestamp=float(data["timestamp"]),
            accel_x=float(data["accel_x"]),
            accel_y=float(data["accel_y"]),
            accel_z=float(data["accel_z"]),
            gyro_x=float(data.get("gyro_x", 0.0)),
            gyro_y=float(data.get("gyro_y", 0.0)),
            gyro_z=float(data.get("gyro_z", 0.0)),
            behavior=WalkingBehavior(behavior) if behavior else None,
        )

    def to_dict(self) -> dict:
        return {
            "timestamp": self.timestamp,
            "accel_x": self.accel_x,
            "accel_y": self.accel_y,
            "accel_z": self.accel_z,
            "gyro_x": self.gyro_x,
            "gyro_y": self.gyro_y,
            "gyro_z": self.gyro_z,
            "behavior": self.behavior.value if self.behavior else None,
        }


@dataclass
class SensorBatch:
    readings: list[SensorReading] = field(default_factory=list)

    @classmethod
    def from_dict(cls, data: dict) -> "SensorBatch":
        readings = [SensorReading.from_dict(r) for r in data.get("readings", [])]
        return cls(readings=readings)
