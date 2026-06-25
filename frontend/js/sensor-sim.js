/**
 * Simulates IMU sensor data for different walking behaviors.
 * In production, this would be replaced by BLE/device sensor streams.
 */
const SensorSimulator = (() => {
  let intervalId = null;
  let startTime = 0;
  let sampleIndex = 0;
  let behavior = "unstable";
  const sampleRate = 20;

  function generateReading(t, behavior, i) {
    const phase = t * 2.0;

    if (behavior === "stable") {
      return {
        timestamp: Date.now() / 1000,
        accel_x: 0.1 * Math.sin(phase * 4 * Math.PI),
        accel_y: 0.05 * Math.cos(phase * 2 * Math.PI),
        accel_z: 9.8 + 0.3 * Math.sin(phase * 4 * Math.PI),
        gyro_x: 0.02,
        gyro_y: 0.01,
        gyro_z: 0.01,
        behavior: "stable",
      };
    }

    if (behavior === "unstable") {
      const wobble = 0.8 * Math.sin(phase * 1.2);
      const stepFreq = 3.5 + wobble;
      const rand = () => (Math.random() - 0.5) * 0.7;
      let ax = 1.2 * Math.sin(phase * stepFreq * Math.PI + 0.3) + rand();
      if (i % 7 < 3) ax *= 1.6;
      return {
        timestamp: Date.now() / 1000,
        accel_x: ax,
        accel_y: 0.9 * Math.cos(phase * (stepFreq * 0.7) * Math.PI) + rand(),
        accel_z: 9.8 + 1.4 * Math.sin(phase * stepFreq * Math.PI) + rand(),
        gyro_x: 0.35 * Math.sin(phase * 2.5),
        gyro_y: 0.25 * Math.cos(phase * 1.8),
        gyro_z: rand() * 0.2,
        behavior: "unstable",
      };
    }

    if (behavior === "near_fall") {
      const progress = i / (sampleRate * 10);
      let ax = 0.3 * Math.sin(phase * 2 * Math.PI);
      let ay = 0.2 * Math.cos(phase * 2 * Math.PI);
      let az = 9.8 + 0.5 * Math.sin(phase * 3 * Math.PI);
      if (progress > 0.7) {
        const spike = (progress - 0.7) / 0.3;
        ax += spike * 3.0;
        ay += spike * 2.5;
        az -= spike * 4.0;
      }
      return {
        timestamp: Date.now() / 1000,
        accel_x: ax,
        accel_y: ay,
        accel_z: az,
        gyro_x: 0.3,
        gyro_y: 0.4,
        gyro_z: 0.2,
        behavior: "near_fall",
      };
    }

    return {
      timestamp: Date.now() / 1000,
      accel_x: 0, accel_y: 0, accel_z: 9.8,
      gyro_x: 0, gyro_y: 0, gyro_z: 0,
      behavior: "stable",
    };
  }

  function start(selectedBehavior, onReading) {
    stop();
    behavior = selectedBehavior;
    startTime = Date.now() / 1000;
    sampleIndex = 0;

    intervalId = setInterval(() => {
      const t = (Date.now() / 1000) - startTime;
      const reading = generateReading(t, behavior, sampleIndex++);
      onReading(reading);
    }, 1000 / sampleRate);
  }

  function stop() {
    if (intervalId) {
      clearInterval(intervalId);
      intervalId = null;
    }
  }

  function isRunning() {
    return intervalId !== null;
  }

  async function fetchServerSample(behavior) {
    const resp = await fetch(`/api/sample/${behavior}`);
    const data = await resp.json();
    return data.readings;
  }

  return { start, stop, isRunning, fetchServerSample };
})();
