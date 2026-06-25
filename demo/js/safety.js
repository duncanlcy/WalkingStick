// Mirrors include/safety.h

import { config } from './config.js';
import {
  AlertLevel,
  AlertType,
  DeviceRole,
} from './protocol.js';

function emptyAlert(source) {
  return {
    timestamp_ms: 0,
    level: AlertLevel.NONE,
    type: AlertType.NONE,
    source,
    message: '',
  };
}

export class SafetyMonitor {
  constructor(
    fallThresholdG = config.FALL_ACCEL_THRESHOLD_G,
    impactThresholdG = config.IMPACT_THRESHOLD_G,
  ) {
    this.fallThresholdG = fallThresholdG;
    this.impactThresholdG = impactThresholdG;
  }

  evaluate(accel, source, timestampMs) {
    const event = emptyAlert(source);
    event.timestamp_ms = timestampMs;

    const magnitude = Math.sqrt(accel.x ** 2 + accel.y ** 2 + accel.z ** 2);

    if (magnitude >= this.impactThresholdG) {
      event.level = AlertLevel.CRITICAL;
      event.type = AlertType.IMPACT;
      event.message = `Impact detected: ${magnitude.toFixed(2)}g`;
      return event;
    }

    if (magnitude >= this.fallThresholdG) {
      event.level = AlertLevel.WARNING;
      event.type = AlertType.FALL_DETECTED;
      event.message = `Possible fall: ${magnitude.toFixed(2)}g`;
      return event;
    }

    return event;
  }

  evaluateGait(pressure, source, timestampMs) {
    const event = emptyAlert(source);
    event.timestamp_ms = timestampMs;

    const left = pressure.leftTotal();
    const right = pressure.rightTotal();
    const total = left + right;

    if (total < config.MIN_PRESSURE_THRESHOLD) {
      return event;
    }

    const imbalance = Math.abs(left - right) / total;

    if (imbalance > 0.6) {
      event.level = AlertLevel.WARNING;
      event.type = AlertType.GAIT_IRREGULAR;
      event.message = `Gait imbalance: ${Math.round(imbalance * 100)}%`;
    }

    return event;
  }
}

export function leftTotal(pressure) {
  return pressure.left_heel + pressure.left_toe;
}

export function rightTotal(pressure) {
  return pressure.right_heel + pressure.right_toe;
}
