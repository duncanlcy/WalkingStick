import { WalkingStickSimulator, SCENARIOS } from './simulator.js';
import {
  AlertLevelLabel,
  AlertTypeLabel,
  DeviceNames,
} from './protocol.js';

if (window.location.protocol === 'file:') {
  document.body.innerHTML = `
    <main style="max-width:640px;margin:3rem auto;padding:1.5rem;font-family:system-ui,sans-serif;line-height:1.5;">
      <h1>WalkingStick demo must run through a local server</h1>
      <p>Opening <code>index.html</code> directly will not work. Start the server first, then open the URL it prints.</p>
      <h2>Windows (PowerShell)</h2>
      <pre style="background:#111;color:#eee;padding:1rem;border-radius:8px;">cd E:\\WalkingStick
.\\scripts\\run_demo.ps1</pre>
      <h2>Windows (Command Prompt)</h2>
      <pre style="background:#111;color:#eee;padding:1rem;border-radius:8px;">cd E:\\WalkingStick
scripts\\run_demo.bat</pre>
      <p>Then open <a href="http://localhost:8080">http://localhost:8080</a></p>
    </main>
  `;
  throw new Error('Demo must be served over http://localhost');
}

const simulator = new WalkingStickSimulator();

const els = {
  scenarioButtons: document.getElementById('scenario-buttons'),
  resetBtn: document.getElementById('reset-btn'),
  uptime: document.getElementById('uptime'),
  loggerCount: document.getElementById('logger-count'),
  alertFeed: document.getElementById('alert-feed'),
  bleNodes: document.getElementById('ble-nodes'),
  shoeCard: document.getElementById('shoe-card'),
  waistCard: document.getElementById('waist-card'),
  stickCard: document.getElementById('stick-card'),
  pressureBars: document.getElementById('pressure-bars'),
  accelDisplay: document.getElementById('accel-display'),
  linkShoeWaist: document.getElementById('link-shoe-waist'),
  linkWaistStick: document.getElementById('link-waist-stick'),
  stickStatus: document.getElementById('stick-status'),
};

const scenarios = [
  { id: SCENARIOS.normal, label: 'Normal walk', desc: 'Balanced gait, ~1g at waist' },
  { id: SCENARIOS.gait_imbalance, label: 'Gait imbalance', desc: 'Left/right pressure skew > 60%' },
  { id: SCENARIOS.fall, label: 'Possible fall', desc: 'Waist accel above 2.5g' },
  { id: SCENARIOS.impact, label: 'Impact', desc: 'Waist accel above 4.0g' },
  { id: SCENARIOS.sos, label: 'SOS button', desc: 'Handle alert triggered' },
  { id: SCENARIOS.low_battery, label: 'Low battery', desc: 'Handle battery drains below 15%' },
];

function formatUptime(ms) {
  const totalSec = Math.floor(ms / 1000);
  const min = Math.floor(totalSec / 60);
  const sec = totalSec % 60;
  return `${String(min).padStart(2, '0')}:${String(sec).padStart(2, '0')}`;
}

function alertClass(level) {
  if (level === 3) return 'critical';
  if (level === 2) return 'warning';
  if (level === 1) return 'info';
  return 'none';
}

function renderScenarioButtons(activeScenario) {
  if (els.scenarioButtons.dataset.active !== activeScenario) {
    els.scenarioButtons.dataset.active = activeScenario;
    els.scenarioButtons.innerHTML = scenarios
      .map(
        (s) => `
        <button
          class="scenario-btn ${activeScenario === s.id ? 'active' : ''}"
          data-scenario="${s.id}"
          title="${s.desc}"
        >
          <span class="scenario-label">${s.label}</span>
          <span class="scenario-desc">${s.desc}</span>
        </button>
      `,
      )
      .join('');

    els.scenarioButtons.querySelectorAll('[data-scenario]').forEach((btn) => {
      btn.addEventListener('click', () => {
        simulator.setScenario(btn.dataset.scenario);
      });
    });
  } else {
    els.scenarioButtons.querySelectorAll('[data-scenario]').forEach((btn) => {
      btn.classList.toggle('active', btn.dataset.scenario === activeScenario);
    });
  }
}

function renderPressureBars(pressure) {
  if (!pressure) {
    els.pressureBars.innerHTML = '<p class="muted">Waiting for shoe pad telemetry…</p>';
    return;
  }

  const channels = [
    { label: 'L heel', value: pressure.left_heel },
    { label: 'L toe', value: pressure.left_toe },
    { label: 'R heel', value: pressure.right_heel },
    { label: 'R toe', value: pressure.right_toe },
  ];

  const max = Math.max(...channels.map((c) => c.value), 1);

  els.pressureBars.innerHTML = channels
    .map(
      (c) => `
      <div class="bar-row">
        <span class="bar-label">${c.label}</span>
        <div class="bar-track"><div class="bar-fill" style="width:${(c.value / max) * 100}%"></div></div>
        <span class="bar-value">${c.value}</span>
      </div>
    `,
    )
    .join('');
}

function renderAccel(accel, thresholds) {
  if (!accel) {
    els.accelDisplay.innerHTML = '<p class="muted">Waiting for waist IMU…</p>';
    return;
  }

  const magClass =
    accel.magnitude >= thresholds.impact
      ? 'critical'
      : accel.magnitude >= thresholds.fall
        ? 'warning'
        : 'ok';

  els.accelDisplay.innerHTML = `
    <div class="accel-grid">
      <div><span>X</span><strong>${accel.x.toFixed(2)}g</strong></div>
      <div><span>Y</span><strong>${accel.y.toFixed(2)}g</strong></div>
      <div><span>Z</span><strong>${accel.z.toFixed(2)}g</strong></div>
      <div class="magnitude ${magClass}"><span>|a|</span><strong>${accel.magnitude.toFixed(2)}g</strong></div>
    </div>
    <p class="threshold-hint">
      Fall ≥ ${thresholds.fall}g · Impact ≥ ${thresholds.impact}g
    </p>
  `;
}

function renderNodeCard(cardEl, node, stickAlertActive, extraHtml = '') {
  const online = node.online ? 'online' : 'offline';
  cardEl.className = `device-card ${online} ${node.role === 2 && stickAlertActive ? 'alerting' : ''}`;
  cardEl.querySelector('.device-name').textContent = node.name;
  cardEl.querySelector('.battery').textContent = `${Math.round(node.battery)}%`;
  const telemetry = cardEl.querySelector('.telemetry');
  if (node.lastPacket) {
    const s = node.lastPacket.sample;
    telemetry.innerHTML = `
      <div>ts: ${s.timestamp_ms} ms</div>
      ${extraHtml}
      ${node.lastPacket.has_alert ? `<div class="packet-alert">Alert in packet</div>` : ''}
    `;
  }
}

function renderBle(state) {
  els.linkShoeWaist.className = `flow-link ${state.bleLinks.shoeToWaist ? 'connected' : 'disconnected'}`;
  els.linkWaistStick.className = `flow-link ${state.bleLinks.waistToStick ? 'connected' : 'disconnected'}`;

  els.bleNodes.innerHTML = state.detectedNodes.length
    ? state.detectedNodes
        .map((n) => `<li><span>${n.name}</span><span class="rssi">${n.rssi} dBm</span></li>`)
        .join('')
    : '<li class="muted">Scanning for WalkingStick nodes…</li>';

  els.stickStatus.textContent = state.stickAlertActive
    ? 'Vibrator ON — safety event active'
    : 'Vibrator off — monitoring';
  els.stickStatus.className = `stick-status ${state.stickAlertActive ? 'active' : ''}`;
}

function renderAlerts(alerts) {
  if (!alerts.length) {
    els.alertFeed.innerHTML = '<p class="muted">No alerts yet. Try a scenario above.</p>';
    return;
  }

  els.alertFeed.innerHTML = alerts
    .map(
      (a) => `
      <article class="alert-item ${alertClass(a.level)}">
        <div class="alert-meta">
          <span class="alert-level">${AlertLevelLabel[a.level]}</span>
          <span class="alert-type">${AlertTypeLabel[a.type]}</span>
          <span class="alert-source">${DeviceNames[a.source]}</span>
          <span class="alert-time">${a.timestamp_ms} ms</span>
        </div>
        <p>${a.message}</p>
      </article>
    `,
    )
    .join('');
}

function render(state) {
  renderScenarioButtons(state.scenario);
  els.uptime.textContent = formatUptime(state.uptimeMs);
  els.loggerCount.textContent = String(state.loggerCount);

  renderPressureBars(state.pressure);
  renderAccel(state.accel, state.thresholds);
  renderNodeCard(els.shoeCard, state.nodes.shoe, state.stickAlertActive, state.pressure
    ? `<div>L/R: ${state.pressure.left} / ${state.pressure.right}</div>`
    : '');
  renderNodeCard(els.waistCard, state.nodes.waist, state.stickAlertActive, state.accel
    ? `<div>|a|: ${state.accel.magnitude.toFixed(2)}g</div>`
    : '');
  renderNodeCard(els.stickCard, state.nodes.stick, state.stickAlertActive);
  renderBle(state);
  renderAlerts(state.alerts);
}

els.resetBtn.addEventListener('click', () => simulator.reset());

simulator.onUpdate(render);
simulator.start();
render(simulator.getState());
