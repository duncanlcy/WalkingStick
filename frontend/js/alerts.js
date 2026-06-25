/**
 * Visual and voice alert output for WalkingStick gait monitor.
 */
const AlertOutput = (() => {
  const overlay = () => document.getElementById("alert-overlay");
  const titleEl = () => document.getElementById("alert-title");
  const messageEl = () => document.getElementById("alert-message");
  const iconEl = () => document.getElementById("alert-icon");
  const badgeEl = () => document.getElementById("alert-severity-badge");
  const scoreEl = () => document.getElementById("alert-score");

  let voiceEnabled = true;
  let visualEnabled = true;
  let hideTimer = null;
  let synth = window.speechSynthesis;
  let currentUtterance = null;

  const SEVERITY_CONFIG = {
    watch: {
      title: "Notice",
      icon: "👀",
      displayMs: 4000,
      rate: 0.95,
      pitch: 1.0,
      volume: 0.7,
    },
    caution: {
      title: "Caution",
      icon: "⚠️",
      displayMs: 6000,
      rate: 0.9,
      pitch: 1.05,
      volume: 0.9,
    },
    critical: {
      title: "Warning!",
      icon: "🚨",
      displayMs: 10000,
      rate: 0.85,
      pitch: 1.1,
      volume: 1.0,
    },
  };

  function setVoiceEnabled(enabled) {
    voiceEnabled = enabled;
    if (!enabled && currentUtterance) {
      synth.cancel();
    }
  }

  function setVisualEnabled(enabled) {
    visualEnabled = enabled;
    if (!enabled) {
      hideVisual();
    }
  }

  function speak(text, severity) {
    if (!voiceEnabled || !text || !synth) return;

    synth.cancel();
    const config = SEVERITY_CONFIG[severity] || SEVERITY_CONFIG.caution;
    const utterance = new SpeechSynthesisUtterance(text);
    utterance.rate = config.rate;
    utterance.pitch = config.pitch;
    utterance.volume = config.volume;

    const voices = synth.getVoices();
    const preferred = voices.find(
      (v) => v.lang.startsWith("en") && (v.name.includes("Female") || v.name.includes("Samantha"))
    ) || voices.find((v) => v.lang.startsWith("en"));
    if (preferred) utterance.voice = preferred;

    currentUtterance = utterance;
    synth.speak(utterance);
  }

  function showVisual(alert) {
    if (!visualEnabled) return;

    const config = SEVERITY_CONFIG[alert.severity] || SEVERITY_CONFIG.caution;
    const el = overlay();

    el.className = `alert-overlay severity-${alert.severity}`;
    iconEl().textContent = config.icon;
    titleEl().textContent = config.title;
    messageEl().textContent = alert.message;
    badgeEl().textContent = alert.severity;
    badgeEl().className = `badge ${alert.severity}`;
    scoreEl().textContent = `Stability: ${alert.stability_score}`;

    if (hideTimer) clearTimeout(hideTimer);
    hideTimer = setTimeout(hideVisual, config.displayMs);
  }

  function hideVisual() {
    const el = overlay();
    el.classList.add("hidden");
    el.className = "alert-overlay hidden";
  }

  function trigger(alert) {
    if (!alert) return;
    showVisual(alert);
    if (alert.voice_message) {
      speak(alert.voice_message, alert.severity);
    } else if (alert.message) {
      speak(alert.message, alert.severity);
    }
  }

  if (synth) {
    synth.onvoiceschanged = () => synth.getVoices();
  }

  return {
    trigger,
    hideVisual,
    setVoiceEnabled,
    setVisualEnabled,
  };
})();
