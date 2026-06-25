// Mirrors include/protocol.h and include/device_roles.h

export const PROTOCOL_VERSION = 1;

export const DeviceRole = {
  WAIST_SAFETY_PAD: 0,
  SHOE_PAD: 1,
  WALKING_STICK: 2,
};

export const DeviceNames = {
  [DeviceRole.WAIST_SAFETY_PAD]: 'waist_safety_pad',
  [DeviceRole.SHOE_PAD]: 'shoe_pad',
  [DeviceRole.WALKING_STICK]: 'walking_stick',
};

export const BLE_NAMES = {
  [DeviceRole.WAIST_SAFETY_PAD]: 'WalkingStick-Waist',
  [DeviceRole.SHOE_PAD]: 'WalkingStick-Shoe',
  [DeviceRole.WALKING_STICK]: 'WalkingStick-Handle',
};

export const WALKING_STICK_SERVICE_UUID = 'a1b2c3d4-e5f6-7890-abcd-ef1234567890';

export const AlertLevel = {
  NONE: 0,
  INFO: 1,
  WARNING: 2,
  CRITICAL: 3,
};

export const AlertLevelLabel = {
  [AlertLevel.NONE]: 'none',
  [AlertLevel.INFO]: 'info',
  [AlertLevel.WARNING]: 'warning',
  [AlertLevel.CRITICAL]: 'critical',
};

export const AlertType = {
  NONE: 0,
  FALL_DETECTED: 1,
  IMPACT: 2,
  GAIT_IRREGULAR: 3,
  LOW_BATTERY: 4,
  SOS: 5,
};

export const AlertTypeLabel = {
  [AlertType.NONE]: 'none',
  [AlertType.FALL_DETECTED]: 'fall_detected',
  [AlertType.IMPACT]: 'impact',
  [AlertType.GAIT_IRREGULAR]: 'gait_irregular',
  [AlertType.LOW_BATTERY]: 'low_battery',
  [AlertType.SOS]: 'sos',
};
