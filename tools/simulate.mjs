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

// Node state generator
function getPacketSet(scenario, tick) {
  const noise = () => (Math.random() - 0.5) * 0.08;
  const vibNoise = () => Math.max(0.015, 0.035 + (Math.random() - 0.5) * 0.015);

  let n1 = { nodeId: "NODE-01", role: "gateway", pitch: noise(), roll: noise(), vibration: vibNoise() };
  let n2 = { nodeId: "NODE-02", role: "field", pitch: noise(), roll: noise(), vibration: vibNoise() };
  let n3 = { nodeId: "NODE-03", role: "field", pitch: noise(), roll: noise(), vibration: vibNoise() };

  switch (scenario) {
    case "watch":
      // Single node isolated tilt
      n2.pitch = 3.2 + noise();
      n2.roll = -0.6 + noise();
      n2.vibration = 0.05 + noise();
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
      break;

    case "blast":
      // High vibration spike across all nodes without permanent tilt
      n1.vibration = 0.38 + vibNoise();
      n2.vibration = 0.44 + vibNoise();
      n3.vibration = 0.41 + vibNoise();
      break;

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
      .map(p => `${p.nodeId}: P=${p.pitch.toFixed(1)}° R=${p.roll.toFixed(1)}° V=${p.vibration.toFixed(3)}g`)
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
  console.log("  [b] Blast Vibration Spike");
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
      console.log("\n--> Switched to: BLAST Vibration Spike");
    }
  });
}

process.on("SIGINT", () => {
  clearInterval(interval);
  console.log("\nSimulator terminated.");
  process.exit();
});
