/**
 * SurakshaMesh Multi-Node Telemetry Simulator
 * 
 * Simulates real-time telemetry from 3 mesh nodes (NODE-01 Gateway, NODE-02 Field, NODE-03 Field)
 * to test baseline calibration, spatial coherence warnings, and critical subsidence alerts.
 * 
 * Usage:
 *   node tools/simulate.mjs [normal | watch | warning | critical | interactive] [BASE_URL]
 * 
 * Examples:
 *   node tools/simulate.mjs normal
 *   node tools/simulate.mjs warning
 *   node tools/simulate.mjs critical
 *   node tools/simulate.mjs interactive
 */

import readline from "node:readline";

const mode = process.argv[2] || "normal";
const baseUrl = process.argv[3] || process.env.BASE_URL || "http://localhost:3000";

console.log("=========================================================");
console.log("       SurakshaMesh Multi-Node Telemetry Simulator       ");
console.log("=========================================================");
console.log(`Endpoint:  ${baseUrl}/api/telemetry`);
console.log(`Mode:      ${mode.toUpperCase()}`);
console.log("=========================================================\n");

let currentScenario = mode.toLowerCase();
let eventStartTick = 0;

// Node state generator
function getPacketSet(scenario, tick) {
  const noise = () => (Math.random() - 0.5) * 0.08;
  const vibNoise = () => Math.max(0.015, 0.035 + (Math.random() - 0.5) * 0.015);

  let n1 = { nodeId: "NODE-01", role: "gateway", pitch: noise(), roll: noise(), vibration: vibNoise(), stalta: 1.02, temp: 26.8, evt: 0, seq: tick };
  let n2 = { nodeId: "NODE-02", role: "field", pitch: noise(), roll: noise(), vibration: vibNoise(), stalta: 1.05, temp: 27.2, evt: 0, seq: tick };
  let n3 = { nodeId: "NODE-03", role: "field", pitch: noise(), roll: noise(), vibration: vibNoise(), stalta: 0.98, temp: 26.9, evt: 0, seq: tick };

  switch (scenario) {
    case "watch":
      // Single node isolated tilt
      n2.pitch = 3.2 + noise();
      n2.roll = -0.6 + noise();
      n2.vibration = 0.05 + noise();
      n2.stalta = 2.8;
      n2.evt = 1;
      break;

    case "warning":
    case "shift":
      // Multi-node spatial coherence: both NODE-02 and NODE-03 tilting coherently (>5°)
      n1.pitch = 1.2 + noise();
      n2.pitch = 5.8 + noise();
      n2.roll = -0.8 + noise();
      n3.pitch = 5.4 + noise();
      n3.roll = 0.5 + noise();
      n2.vibration = 0.08 + noise();
      n3.vibration = 0.07 + noise();
      n1.stalta = 2.1;
      n2.stalta = 5.4;
      n3.stalta = 4.9;
      n1.evt = 1;
      n2.evt = 2;
      n3.evt = 2;
      break;

    case "critical":
    case "collapse":
      // Severe multi-node ground subsidence (>8° severe tilt + elevated vibration)
      n1.pitch = 3.5 + noise();
      n2.pitch = 9.8 + noise();
      n2.roll = -4.2 + noise();
      n3.pitch = 10.5 + noise();
      n3.roll = 3.8 + noise();
      n1.vibration = 0.12 + vibNoise();
      n2.vibration = 0.32 + vibNoise();
      n3.vibration = 0.28 + vibNoise();
      n1.stalta = 4.2;
      n2.stalta = 8.6;
      n3.stalta = 7.9;
      n1.evt = 2;
      n2.evt = 2;
      n3.evt = 2;
      break;

    case "blast":
      // High vibration spike across all nodes without permanent tilt
      n1.vibration = 0.38 + vibNoise();
      n2.vibration = 0.44 + vibNoise();
      n3.vibration = 0.41 + vibNoise();
      n1.stalta = 6.5;
      n2.stalta = 7.8;
      n3.stalta = 7.1;
      n1.evt = 3;
      n2.evt = 3;
      n3.evt = 3;
      break;

    case "event": {
      // Dynamic Event Lifecycle demo:
      // Ticks 0-4: Seismic trigger tremor (STA/LTA spike, PENDING)
      // Ticks 5-14: Ground settling into permanent tilt (>5°, SUBSIDENCE CONFIRMED)
      // Ticks 15+: Vibration recedes, permanent offset remains
      const eTick = tick - eventStartTick;
      if (eTick < 5) {
        n1.vibration = 0.28 + vibNoise();
        n2.vibration = 0.42 + vibNoise();
        n3.vibration = 0.39 + vibNoise();
        n1.stalta = 3.8;
        n2.stalta = 7.2;
        n3.stalta = 6.5;
        n2.evt = 1; // Pending
        n3.evt = 1;
      } else if (eTick < 15) {
        const progress = Math.min(1.0, (eTick - 4) / 10);
        n2.pitch = 6.2 * progress + noise();
        n2.roll = -1.2 * progress + noise();
        n3.pitch = 5.8 * progress + noise();
        n3.roll = 0.8 * progress + noise();
        n2.vibration = 0.12 * (1 - progress) + vibNoise();
        n3.vibration = 0.10 * (1 - progress) + vibNoise();
        n2.stalta = 3.5;
        n3.stalta = 3.1;
        n2.evt = 2; // Subsidence confirmed
        n3.evt = 2;
      } else {
        n2.pitch = 6.2 + noise();
        n2.roll = -1.2 + noise();
        n3.pitch = 5.8 + noise();
        n3.roll = 0.8 + noise();
        n2.stalta = 1.05;
        n3.stalta = 1.02;
        n2.evt = 2;
        n3.evt = 2;
      }
      break;
    }

    case "normal":
    default:
      // Steady baseline resting state
      break;
  }

  return [n1, n2, n3];
}

async function sendTelemetry(packets) {
  try {
    const res = await fetch(`${baseUrl}/api/telemetry`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ packets })
    });
    if (!res.ok) {
      console.warn(`[API WARN] ${res.status}: ${res.statusText}`);
      return;
    }
    const data = await res.json();
    const summary = packets
      .map(p => `${p.nodeId}: P=${p.pitch.toFixed(1)}° R=${p.roll.toFixed(1)}° V=${p.vibration.toFixed(3)}g S=${(p.stalta ?? 1.0).toFixed(1)} E=${p.evt ?? 0}`)
      .join(" | ");
    
    const levelLabel = ["NORMAL", "WATCH", "WARNING", "CRITICAL"][data.overallLevel ?? 0] || "UNKNOWN";
    process.stdout.write(`\r[${new Date().toLocaleTimeString()}] [STATUS: ${levelLabel}] ${summary}`);
  } catch (err) {
    console.error(`\n[API ERROR] Failed to connect to ${baseUrl}:`, err.message);
  }
}

let tick = 0;
const interval = setInterval(async () => {
  tick++;
  const packets = getPacketSet(currentScenario, tick);
  await sendTelemetry(packets);
}, 1000);

if (mode === "interactive") {
  console.log("\n🎮 Interactive Controls (Press key to switch scenario):");
  console.log("  [0] Normal baseline (Green)");
  console.log("  [1] Watch (Yellow - Single Node Tilt)");
  console.log("  [2] Warning / Shift (Orange - Multi-Node Spatial Coherence)");
  console.log("  [3] Critical Collapse (Red - Severe Failure + Web Audio Siren)");
  console.log("  [b] Blast Vibration Spike (Auto-Suppressed)");
  console.log("  [e] Event Lifecycle (Tremor -> Pending -> Subsidence Confirmed)");
  console.log("  [q] Quit\n");

  readline.emitKeypressEvents(process.stdin);
  if (process.stdin.isTTY) {
    process.stdin.setRawMode(true);
  }

  process.stdin.on("keypress", (str, key) => {
    if (key.ctrl && key.name === "c" || key.name === "q") {
      clearInterval(interval);
      console.log("\nSimulator stopped.");
      process.exit();
    }
    if (str === "0" || str === "n") {
      currentScenario = "normal";
      console.log("\n--> Switched to: NORMAL Baseline");
    } else if (str === "1" || str === "w") {
      currentScenario = "watch";
      console.log("\n--> Switched to: WATCH (Single Node Tilt)");
    } else if (str === "2" || str === "s") {
      currentScenario = "warning";
      console.log("\n--> Switched to: WARNING (Multi-Node Spatial Coherence)");
    } else if (str === "3" || str === "c") {
      currentScenario = "critical";
      console.log("\n--> Switched to: CRITICAL (Severe Ground Subsidence)");
    } else if (str === "b") {
      currentScenario = "blast";
      console.log("\n--> Switched to: BLAST Vibration Spike (Auto-Suppressed)");
    } else if (str === "e") {
      currentScenario = "event";
      eventStartTick = tick;
      console.log("\n--> Switched to: EVENT LIFECYCLE (Tremor -> Pending -> Subsidence Confirmed)");
    }
  });
}

process.on("SIGINT", () => {
  clearInterval(interval);
  console.log("\nSimulator terminated.");
  process.exit();
});
