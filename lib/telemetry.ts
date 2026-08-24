export type Packet = {
  nodeId: string;
  role?: "gateway" | "field";
  pitch: number;
  roll: number;
  vibration: number;
  timestamp?: string;
};

export type Node = Packet & {
  id: string;
  role: string;
  online: boolean;
  level: number;
  lastSeen: string;
  baselineReady: boolean;
  tiltStartedAt?: number;
  baselinePitch: number;
  baselineRoll: number;
  deltaPitch: number;
  deltaRoll: number;
  deltaTilt: number;
};

type State = {
  nodes: Map<string, Node>;
  events: { time: string; level: number; text: string }[];
  baseline: { samples: number; meanVib: number; m2Vib: number; ready: boolean };
  suppressBlastUntil: number;
  mode: string;
};

const state: State = (globalThis as any).__surakshaState ?? {
  nodes: new Map(),
  events: [],
  baseline: { samples: 0, meanVib: 0.04, m2Vib: 0, ready: false },
  suppressBlastUntil: 0,
  mode: "live"
};
(globalThis as any).__surakshaState = state;

const now = () => Date.now();

const addEvent = (level: number, text: string) => {
  const time = new Date().toLocaleTimeString();
  if (state.events[0]?.text === text) return;
  state.events.unshift({ time, level, text });
  if (state.events.length > 50) state.events.pop();
};

export function triggerBlastSuppression(seconds = 60) {
  const time = now();
  if (time < state.suppressBlastUntil) {
    state.suppressBlastUntil = 0;
    addEvent(0, "Scheduled blast suppression deactivated manually. Full vibration alarms restored.");
  } else {
    state.suppressBlastUntil = time + seconds * 1000;
    addEvent(1, `SCHEDULED BLAST SUPPRESSION ACTIVE (${seconds}s). Transient vibration alarms muted.`);
  }
  return status();
}

export function resetSystem() {
  state.events = [];
  state.suppressBlastUntil = 0;
  
  // Reset statistical baseline learning counter to 0 so it re-learns live
  state.baseline.samples = 0;
  state.baseline.meanVib = 0.04;
  state.baseline.m2Vib = 0;
  state.baseline.ready = false;
  
  state.nodes.forEach(node => {
    node.baselinePitch = node.pitch;
    node.baselineRoll = node.roll;
    node.deltaPitch = 0;
    node.deltaRoll = 0;
    node.deltaTilt = 0;
    node.level = 0;
    node.tiltStartedAt = undefined;
    node.baselineReady = false;
  });

  addEvent(0, "SYSTEM RESET: Alarms cleared, baseline zeroed, and 60-sample learning restarted.");
  return status();
}

export function calibrate(nodeId?: string) {
  const nodesToCalibrate = nodeId ? [state.nodes.get(nodeId)].filter(Boolean) : Array.from(state.nodes.values());
  
  nodesToCalibrate.forEach(node => {
    if (!node) return;
    node.baselinePitch = node.pitch;
    node.baselineRoll = node.roll;
    node.deltaPitch = 0;
    node.deltaRoll = 0;
    node.deltaTilt = 0;
    node.level = 0;
    node.tiltStartedAt = undefined;
    node.baselineReady = true;
  });

  state.baseline.samples = 60;
  state.baseline.ready = true;
  addEvent(0, `Baseline calibrated: zero-offset set for ${nodesToCalibrate.length} node(s).`);
  return status();
}

export function ingest(p: Packet) {
  if (!p.nodeId || !Number.isFinite(p.pitch) || !Number.isFinite(p.roll) || !Number.isFinite(p.vibration)) {
    throw new Error("nodeId, pitch, roll and vibration are required finite numbers");
  }
  const time = now();
  const old = state.nodes.get(p.nodeId);

  // Initialize or maintain baseline tare
  let baselinePitch = old?.baselinePitch ?? p.pitch;
  let baselineRoll = old?.baselineRoll ?? p.roll;

  if (old === undefined) {
    baselinePitch = p.pitch;
    baselineRoll = p.roll;
  }

  const deltaPitch = Number((p.pitch - baselinePitch).toFixed(2));
  const deltaRoll = Number((p.roll - baselineRoll).toFixed(2));
  const deltaTilt = Math.max(Math.abs(deltaPitch), Math.abs(deltaRoll));

  let tiltStartedAt = old?.tiltStartedAt;
  if (deltaTilt >= 2.0) {
    if (!tiltStartedAt) tiltStartedAt = time;
  } else {
    if (deltaTilt < 1.5) tiltStartedAt = undefined;
  }

  // Baseline statistical accumulation while ground is quiet
  const b = state.baseline;
  if (deltaTilt < 1.5 && p.vibration < 0.2) {
    b.samples = Math.min(60, b.samples + 1);
    const d = p.vibration - b.meanVib;
    b.meanVib += d / b.samples;
    b.m2Vib += d * (p.vibration - b.meanVib);
    if (b.samples >= 20) b.ready = true;
  }

  const isBlastSuppressed = time < state.suppressBlastUntil;

  // Risk Classification based on Relative Deviation from Baseline
  let level = 0;
  const isPersistent = tiltStartedAt && (time - tiltStartedAt >= 3000);

  if (deltaTilt >= 8.0) {
    level = 3; // Critical immediately on severe tilt
  } else if (deltaTilt >= 5.0 && isPersistent) {
    level = 2; // Warning on persistent >5 deg
  } else if (deltaTilt >= 2.0) {
    level = 1; // Watch on >2 deg
  } else if (!isBlastSuppressed && p.vibration >= 0.15) {
    level = 1; // Watch on vibration only if not during scheduled blast
  }

  const n: Node = {
    ...p,
    id: p.nodeId,
    nodeId: p.nodeId,
    role: p.role ?? old?.role ?? (p.nodeId === "NODE-01" ? "gateway" : "field"),
    online: true,
    level,
    lastSeen: new Date().toISOString(),
    baselineReady: b.ready,
    tiltStartedAt,
    baselinePitch,
    baselineRoll,
    deltaPitch,
    deltaRoll,
    deltaTilt
  };

  state.nodes.set(p.nodeId, n);
  return n;
}

export function status() {
  const time = now();
  const nodes = Array.from(state.nodes.values());
  const isBlastSuppressed = time < state.suppressBlastUntil;
  const blastRemaining = isBlastSuppressed ? Math.max(0, Math.ceil((state.suppressBlastUntil - time) / 1000)) : 0;

  // Update online status: offline if no packet received for >5 seconds
  nodes.forEach(n => {
    n.id = n.nodeId;
    const ageMs = time - new Date(n.lastSeen).getTime();
    n.online = ageMs < 5000;
    if (!n.online) {
      n.level = 0;
    }
  });

  const onlineNodes = nodes.filter(n => n.online);
  const activeTilted = onlineNodes.filter(n => n.deltaTilt >= 2.0);
  const severeTilted = onlineNodes.filter(n => n.deltaTilt >= 8.0);

  let overall = 0;
  let rationale = isBlastSuppressed 
    ? `BLAST SUPPRESSION ACTIVE (${blastRemaining}s remaining) — Transient vibration alarms muted.` 
    : state.baseline.ready
      ? "Ground is stable. Tilt and vibration are within normal baseline."
      : `Learning ambient ground baseline (${state.baseline.samples}/60 quiet samples)...`;

  if (onlineNodes.length === 0) {
    overall = 0;
    rationale = "Waiting for live nodes to connect over ESP-NOW mesh...";
  } else if (severeTilted.length >= 1) {
    overall = 3;
    rationale = `CRITICAL: Severe ground tilt (${severeTilted.map(n => `${n.nodeId}: ${n.deltaTilt.toFixed(1)}°`).join(", ")}) detected! Trigger site emergency SOP.`;
  } else if (activeTilted.length >= 2) {
    const samePitchSign = activeTilted.every(n => n.deltaPitch > 1.0) || activeTilted.every(n => n.deltaPitch < -1.0);
    const sameRollSign = activeTilted.every(n => n.deltaRoll > 1.0) || activeTilted.every(n => n.deltaRoll < -1.0);
    const coherent = samePitchSign || sameRollSign;

    if (coherent) {
      overall = 2;
      rationale = `SPATIAL COHERENCE WARNING: ${activeTilted.length} adjacent nodes tilting synchronously — active ground subsidence confirmed.`;
    } else {
      overall = 1;
      rationale = `WATCH: ${activeTilted.length} nodes show local tilt deviation. Verify zone stability.`;
    }
  } else if (activeTilted.length === 1) {
    overall = 1;
    rationale = `WATCH: Isolated tilt on ${activeTilted[0].nodeId} (Δ ${activeTilted[0].deltaTilt.toFixed(1)}°). Monitoring for multi-node spread.`;
  }

  if (overall >= 2) {
    addEvent(overall, rationale);
  }

  return {
    mode: state.mode,
    overallLevel: overall,
    rationale,
    nodes,
    onlineCount: onlineNodes.length,
    events: state.events,
    baseline: { samples: state.baseline.samples, ready: state.baseline.ready },
    blastSuppression: { active: isBlastSuppressed, remainingSeconds: blastRemaining }
  };
}
