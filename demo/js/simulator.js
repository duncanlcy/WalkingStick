// Simulates all three WalkingStick nodes and BLE data flow.

import { config } from './config.js';
import {
  AlertLevel,
  AlertType,
  BLE_NAMES,
  DeviceRole,
  PROTOCOL_VERSION,
} from './protocol.js';
import { SafetyMonitor, leftTotal, rightTotal } from './safety.js';

const SCENARIOS = {
  normal: 'normal',
  gait_imbalance: 'gait_imbalance',
  fall: 'fall',
  impact: 'impact',
  sos: 'sos',
  low_battery: 'low_battery',
};

export class WalkingStickSimulator {
  constructor() {
    this.safety = new SafetyMonitor();
    this.scenario = SCENARIOS.normal;
    this.running = false;
    this.tick = 0;
    this.startedAt = Date.now();
    this.listeners = new Set();

    this.shoe = this.createNode(DeviceRole.SHOE_PAD);
    this.waist = this.createNode(DeviceRole.WAIST_SAFETY_PAD);
    this.stick = this.createNode(DeviceRole.WALKING_STICK);

    this.logger = [];
    this.alerts = [];
    this.bleLinks = {
      shoeToWaist: true,
      waistToStick: true,
    };
    this.stickAlertActive = false;
    this.lastScanMs = 0;
    this.detectedNodes = [];
  }

  createNode(role) {
    return {
      role,
      name: BLE_NAMES[role],
      online: true,
      battery: 92,
      lastPacket: null,
    };
  }

  onUpdate(listener) {
    this.listeners.add(listener);
    return () => this.listeners.delete(listener);
  }

  notify() {
    const state = this.getState();
    for (const listener of this.listeners) {
      listener(state);
    }
  }

  start() {
    if (this.running) {
      return;
    }
    this.running = true;
    this.loop();
  }

  stop() {
    this.running = false;
  }

  setScenario(scenario) {
    this.scenario = scenario;
    if (scenario === SCENARIOS.sos) {
      this.triggerSos();
      return;
    }
    this.notify();
  }

  reset() {
    this.scenario = SCENARIOS.normal;
    this.tick = 0;
    this.startedAt = Date.now();
    this.logger = [];
    this.alerts = [];
    this.stickAlertActive = false;
    this.detectedNodes = [];
    this.shoe.battery = 92;
    this.waist.battery = 88;
    this.stick.battery = 95;
    this.notify();
  }

  triggerSos() {
    const timestampMs = Date.now() - this.startedAt;
    const alert = {
      timestamp_ms: timestampMs,
      level: AlertLevel.CRITICAL,
      type: AlertType.SOS,
      source: DeviceRole.WALKING_STICK,
      message: 'SOS pressed on handle',
    };
    this.stickAlertActive = true;
    this.pushAlert(alert);
    this.notify();
  }

  loop() {
    if (!this.running) {
      return;
    }

    const now = Date.now();
    const timestampMs = now - this.startedAt;

    const pressure = this.simulatePressure(timestampMs);
    const accel = this.simulateAccel(timestampMs);

    const gaitAlert = this.safety.evaluateGait(pressure, DeviceRole.SHOE_PAD, timestampMs);
    const shoePacket = this.makePacket(DeviceRole.SHOE_PAD, timestampMs, {
      pressure_left: leftTotal(pressure),
      pressure_right: rightTotal(pressure),
      battery_percent: this.shoe.battery,
    }, gaitAlert);

    const safetyAlert = this.safety.evaluate(accel, DeviceRole.WAIST_SAFETY_PAD, timestampMs);
    const waistPacket = this.makePacket(DeviceRole.WAIST_SAFETY_PAD, timestampMs, {
      accel_x: accel.x,
      accel_y: accel.y,
      accel_z: accel.z,
      battery_percent: this.waist.battery,
    }, safetyAlert);

    if (this.bleLinks.shoeToWaist && gaitAlert.level !== AlertLevel.NONE) {
      this.forwardToWaist(gaitAlert);
    }

    if (this.bleLinks.waistToStick) {
      const forwardAlert = safetyAlert.level !== AlertLevel.NONE ? safetyAlert : gaitAlert;
      if (forwardAlert.level >= AlertLevel.WARNING) {
        this.forwardToStick(forwardAlert);
      }
    }

    this.shoe.lastPacket = shoePacket;
    this.waist.lastPacket = waistPacket;

    this.logPacket(shoePacket);
    this.logPacket(waistPacket);

    if (gaitAlert.level !== AlertLevel.NONE) {
      this.pushAlert(gaitAlert);
    }
    if (safetyAlert.level !== AlertLevel.NONE) {
      this.pushAlert(safetyAlert);
    }

    if (timestampMs - this.lastScanMs >= config.BLE_SCAN_INTERVAL_MS) {
      this.lastScanMs = timestampMs;
      this.runBleScan();
    }

  if (this.scenario === SCENARIOS.low_battery) {
      this.stick.battery = Math.max(8, this.stick.battery - 0.02);
      if (this.stick.battery <= config.LOW_BATTERY_PERCENT && !this.stickAlertActive) {
        this.stickAlertActive = true;
        this.pushAlert({
          timestamp_ms: timestampMs,
          level: AlertLevel.WARNING,
          type: AlertType.LOW_BATTERY,
          source: DeviceRole.WALKING_STICK,
          message: `Low battery: ${Math.round(this.stick.battery)}%`,
        });
      }
    }

    this.tick += 1;
    this.notify();

    setTimeout(() => this.loop(), config.SENSOR_SAMPLE_MS);
  }

  simulatePressure(timestampMs) {
    const phase = timestampMs / 600;
    const walk = (Math.sin(phase) + 1) / 2;

    let leftScale = 0.45 + walk * 0.35;
    let rightScale = 0.45 + (1 - walk) * 0.35;

    if (this.scenario === SCENARIOS.gait_imbalance) {
      leftScale *= 2.2;
      rightScale *= 0.25;
    }

    const base = 180 + walk * 420;
    return {
      left_heel: Math.round(base * leftScale * 0.55),
      left_toe: Math.round(base * leftScale * 0.45),
      right_heel: Math.round(base * rightScale * 0.55),
      right_toe: Math.round(base * rightScale * 0.45),
    };
  }

  simulateAccel(timestampMs) {
    const noise = Math.sin(timestampMs / 120) * 0.05;

    if (this.scenario === SCENARIOS.impact) {
      const spike = Math.sin(timestampMs / 80) > 0.92 ? 4.8 : 1.0;
      return { x: 0.1, y: 0.2, z: spike };
    }

    if (this.scenario === SCENARIOS.fall) {
      const spike = Math.sin(timestampMs / 100) > 0.85 ? 3.1 : 1.0;
      return { x: 0.3, y: spike * 0.4, z: spike };
    }

    return { x: 0.05 + noise, y: 0.03, z: 1.0 + noise * 0.5 };
  }

  makePacket(source, timestampMs, sampleFields, alert) {
    return {
      protocol_version: PROTOCOL_VERSION,
      source,
      sample: {
        timestamp_ms: timestampMs,
        accel_x: 0,
        accel_y: 0,
        accel_z: 0,
        pressure_left: 0,
        pressure_right: 0,
        battery_percent: 100,
        source,
        ...sampleFields,
      },
      alert,
      has_alert: alert.level !== AlertLevel.NONE,
    };
  }

  forwardToWaist(alert) {
    this.waist.lastForwarded = alert;
  }

  forwardToStick(alert) {
    this.stickAlertActive = true;
    this.stick.lastForwarded = alert;
    this.pushAlert({
      ...alert,
      message: `Forwarded to handle: ${alert.message}`,
    });
  }

  logPacket(packet) {
    this.logger.push(packet);
    if (this.logger.length > 64) {
      this.logger.shift();
    }
  }

  pushAlert(alert) {
    const key = `${alert.type}-${alert.message}`;
    const recent = this.alerts[this.alerts.length - 1];
    if (recent && `${recent.type}-${recent.message}` === key) {
      return;
    }

    this.alerts.unshift({ ...alert, id: `${Date.now()}-${this.alerts.length}` });
    if (this.alerts.length > 30) {
      this.alerts.pop();
    }
  }

  runBleScan() {
    this.detectedNodes = [
      { name: BLE_NAMES[DeviceRole.SHOE_PAD], rssi: -58 + Math.round(Math.random() * 6) },
      { name: BLE_NAMES[DeviceRole.WAIST_SAFETY_PAD], rssi: -52 + Math.round(Math.random() * 6) },
    ];
  }

  getState() {
    const pressure = this.shoe.lastPacket?.sample;
    const accel = this.waist.lastPacket?.sample;

    return {
      scenario: this.scenario,
      running: this.running,
      tick: this.tick,
      uptimeMs: Date.now() - this.startedAt,
      nodes: {
        shoe: this.shoe,
        waist: this.waist,
        stick: this.stick,
      },
      bleLinks: this.bleLinks,
      detectedNodes: this.detectedNodes,
      stickAlertActive: this.stickAlertActive,
      pressure: pressure
        ? {
            left_heel: Math.round(pressure.pressure_left * 0.55),
            left_toe: Math.round(pressure.pressure_left * 0.45),
            right_heel: Math.round(pressure.pressure_right * 0.55),
            right_toe: Math.round(pressure.pressure_right * 0.45),
            left: pressure.pressure_left,
            right: pressure.pressure_right,
          }
        : null,
      accel: accel
        ? {
            x: accel.accel_x,
            y: accel.accel_y,
            z: accel.accel_z,
            magnitude: Math.sqrt(accel.accel_x ** 2 + accel.accel_y ** 2 + accel.accel_z ** 2),
          }
        : null,
      loggerCount: this.logger.length,
      alerts: this.alerts,
      thresholds: {
        fall: config.FALL_ACCEL_THRESHOLD_G,
        impact: config.IMPACT_THRESHOLD_G,
        minPressure: config.MIN_PRESSURE_THRESHOLD,
        lowBattery: config.LOW_BATTERY_PERCENT,
      },
    };
  }
}

export { SCENARIOS };
