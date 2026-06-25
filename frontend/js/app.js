/**
 * Main application — wires sensor input → backend analysis → visual/voice alerts.
 */
(() => {
  const SESSION_ID = `session-${Math.random().toString(36).slice(2, 9)}`;
  let ws = null;
  let monitoring = false;

  const $ = (id) => document.getElementById(id);

  const els = {
    connectionStatus: $("connection-status"),
    btnStart: $("btn-start"),
    btnStop: $("btn-stop"),
    btnReset: $("btn-reset"),
    behaviorSelect: $("behavior-select"),
    voiceToggle: $("voice-enabled"),
    visualToggle: $("visual-enabled"),
    gaugeFill: $("gauge-fill"),
    gaugeValue: $("gauge-value"),
    metricCadence: $("metric-cadence"),
    metricStepVar: $("metric-step-var"),
    metricSway: $("metric-sway"),
    metricAsymmetry: $("metric-asymmetry"),
    severityValue: $("severity-value"),
    flagsList: $("flags-list"),
    alertLog: $("alert-log"),
  };

  function connect() {
    const protocol = location.protocol === "https:" ? "wss:" : "ws:";
    ws = new WebSocket(`${protocol}//${location.host}/ws/${SESSION_ID}`);

    ws.onopen = () => {
      els.connectionStatus.classList.add("connected");
      els.connectionStatus.querySelector("span:last-child").textContent = "Connected";
    };

    ws.onclose = () => {
      els.connectionStatus.classList.remove("connected");
      els.connectionStatus.querySelector("span:last-child").textContent = "Disconnected";
      if (monitoring) {
        setTimeout(connect, 2000);
      }
    };

    ws.onmessage = (event) => {
      const data = JSON.parse(event.data);
      if (data.type === "update") {
        handleUpdate(data);
      } else if (data.type === "alert") {
        handleAlert(data.alert);
      }
    };
  }

  function send(data) {
    if (ws && ws.readyState === WebSocket.OPEN) {
      ws.send(JSON.stringify(data));
    }
  }

  function handleUpdate(data) {
    if (!data.analysis) return;
    updateDashboard(data.analysis);
    if (data.alert) {
      handleAlert(data.alert);
    }
  }

  function handleAlert(alert) {
    AlertOutput.trigger(alert);
    addLogEntry(alert);
  }

  function updateDashboard(analysis) {
    const f = analysis.features;
    const score = f.stability_score;

    els.gaugeFill.style.width = `${score}%`;
    els.gaugeFill.style.background =
      score >= 70 ? "var(--stable)" : score >= 40 ? "var(--caution)" : "var(--critical)";
    els.gaugeValue.textContent = score.toFixed(0);

    els.metricCadence.textContent = f.cadence_steps_per_min.toFixed(0);
    els.metricStepVar.textContent = f.step_time_variability.toFixed(3);
    els.metricSway.textContent = f.lateral_sway_rms.toFixed(3);
    els.metricAsymmetry.textContent = f.step_asymmetry.toFixed(3);

    const sev = analysis.overall_severity;
    const labels = { none: "Normal", watch: "Watch", caution: "Caution", critical: "Critical" };
    els.severityValue.textContent = labels[sev] || "Normal";
    els.severityValue.className = `severity-value ${sev === "none" ? "normal" : sev}`;

    updateFlags(analysis.flags);
  }

  function updateFlags(flags) {
    els.flagsList.innerHTML = "";
    if (!flags || flags.length === 0) {
      els.flagsList.innerHTML = '<li class="flag-empty">No instability detected</li>';
      return;
    }
    flags.forEach((flag) => {
      const li = document.createElement("li");
      li.className = `flag-item ${flag.severity}`;
      li.innerHTML = `<strong>${flag.type.replace(/_/g, " ")}</strong> — ${flag.message}`;
      els.flagsList.appendChild(li);
    });
  }

  function addLogEntry(alert) {
    const empty = els.alertLog.querySelector(".log-empty");
    if (empty) empty.remove();

    const li = document.createElement("li");
    li.className = "log-item";
    const time = new Date(alert.timestamp * 1000).toLocaleTimeString();
    li.innerHTML = `
      <div class="log-time">${time} — ${alert.severity.toUpperCase()}</div>
      <div>${alert.message}</div>
    `;
    els.alertLog.prepend(li);

    while (els.alertLog.children.length > 20) {
      els.alertLog.removeChild(els.alertLog.lastChild);
    }
  }

  function startMonitoring() {
    monitoring = true;
    els.btnStart.disabled = true;
    els.btnStop.disabled = false;

    if (!ws || ws.readyState !== WebSocket.OPEN) {
      connect();
    }

    const behavior = els.behaviorSelect.value;
    SensorSimulator.start(behavior, (reading) => {
      send({ type: "sensor", reading });
    });
  }

  function stopMonitoring() {
    monitoring = false;
    SensorSimulator.stop();
    els.btnStart.disabled = false;
    els.btnStop.disabled = true;
  }

  function reset() {
    stopMonitoring();
    send({ type: "reset" });
    AlertOutput.hideVisual();

    els.gaugeFill.style.width = "100%";
    els.gaugeValue.textContent = "—";
    els.metricCadence.textContent = "—";
    els.metricStepVar.textContent = "—";
    els.metricSway.textContent = "—";
    els.metricAsymmetry.textContent = "—";
    els.severityValue.textContent = "Normal";
    els.severityValue.className = "severity-value normal";
    els.flagsList.innerHTML = '<li class="flag-empty">No instability detected</li>';
    els.alertLog.innerHTML = '<li class="log-empty">No alerts yet</li>';
  }

  els.btnStart.addEventListener("click", startMonitoring);
  els.btnStop.addEventListener("click", stopMonitoring);
  els.btnReset.addEventListener("click", reset);

  els.voiceToggle.addEventListener("change", (e) => {
    AlertOutput.setVoiceEnabled(e.target.checked);
  });

  els.visualToggle.addEventListener("change", (e) => {
    AlertOutput.setVisualEnabled(e.target.checked);
  });

  connect();
})();
