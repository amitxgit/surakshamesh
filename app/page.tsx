"use client";

import { useEffect, useMemo, useRef, useState } from "react";
import { HexMeshCanvas, type MeshNode } from "./components/HexMeshCanvas";

type Event = { time: string; level: number; text: string };
type Status = {
  overallLevel: number;
  rationale: string;
  nodes: MeshNode[];
  onlineCount?: number;
  events: Event[];
  baseline: { samples: number; ready: boolean };
  blastSuppression?: { active: boolean; remainingSeconds: number };
};

const label = ["NORMAL", "WATCH", "WARNING", "CRITICAL"];
const actionText = [
  "No ground deformation detected. Baseline stable.",
  "Isolated tilt/vibration detected. Monitor zone.",
  "Multi-node subsidence confirmed. Inspect area.",
  "CRITICAL: Severe ground failure. Evacuate zone immediately!"
];

export default function Dashboard() {
  const [status, setStatus] = useState<Status | null>(null);
  const [busy, setBusy] = useState(false);
  const [selectedId, setSelectedId] = useState<string | null>(null);
  const [expanded, setExpanded] = useState(false);
  const [audioEnabled, setAudioEnabled] = useState(true);
  const audioContextRef = useRef<AudioContext | null>(null);
  const lastBeepRef = useRef<number>(0);

  const refresh = async () => {
    try {
      const next = await fetch("/api/status", { cache: "no-store" }).then(res => res.json());
      setStatus(next);
      setSelectedId(curr => curr ?? next.nodes[0]?.nodeId ?? next.nodes[0]?.id ?? null);
    } catch {
      // Ignore transient network errors
    }
  };

  useEffect(() => {
    refresh();
    const timer = setInterval(refresh, 1000);
    return () => clearInterval(timer);
  }, []);

  // Web Audio Synth for Critical Alarm
  useEffect(() => {
    if (!audioEnabled || !status || status.overallLevel < 3) return;
    const now = Date.now();
    if (now - lastBeepRef.current < 600) return;
    lastBeepRef.current = now;

    try {
      const ctx = audioContextRef.current || new (window.AudioContext || (window as any).webkitAudioContext)();
      audioContextRef.current = ctx;
      if (ctx.state === "suspended") ctx.resume();

      const osc = ctx.createOscillator();
      const gain = ctx.createGain();
      osc.type = "sawtooth";
      osc.frequency.setValueAtTime(880, ctx.currentTime);
      osc.frequency.exponentialRampToValueAtTime(1320, ctx.currentTime + 0.25);
      gain.gain.setValueAtTime(0.15, ctx.currentTime);
      gain.gain.exponentialRampToValueAtTime(0.01, ctx.currentTime + 0.3);
      osc.connect(gain);
      gain.connect(ctx.destination);
      osc.start();
      osc.stop(ctx.currentTime + 0.3);
    } catch {
      // Ignore audio context auto-play restrictions before user gesture
    }
  }, [status, audioEnabled]);

  const calibrate = async () => {
    setBusy(true);
    await fetch("/api/calibrate", { method: "POST" });
    await refresh();
    setBusy(false);
  };

  const toggleBlast = async () => {
    setBusy(true);
    await fetch("/api/blast", { method: "POST" });
    await refresh();
    setBusy(false);
  };

  const resetAll = async () => {
    setBusy(true);
    await fetch("/api/reset", { method: "POST" });
    await refresh();
    setBusy(false);
  };

  const simulate = async (kind: string) => {
    setBusy(true);
    await fetch(`/api/demo/${kind}`, { method: "POST" });
    await refresh();
    setBusy(false);
  };

  const exportCSV = () => {
    if (!status) return;
    const headers = "Timestamp,NodeID,Role,DeltaPitch_deg,DeltaRoll_deg,Vibration_g,RiskLevel,Status\n";
    const rows = status.nodes.map(n => {
      const dP = n.deltaPitch ?? n.pitch;
      const dR = n.deltaRoll ?? n.roll;
      return `"${new Date().toISOString()}","${n.nodeId || n.id}","${n.role}",${dP.toFixed(2)},${dR.toFixed(2)},${n.vibration.toFixed(4)},${n.level},"${n.online ? "ONLINE" : "OFFLINE"}"`;
    }).join("\n");
    const blob = new Blob([headers + rows], { type: "text/csv" });
    const url = URL.createObjectURL(blob);
    const a = document.createElement("a");
    a.href = url;
    a.download = `SurakshaMesh_Audit_${Date.now()}.csv`;
    a.click();
  };

  const level = status?.overallLevel ?? 0;
  const selected = useMemo(() => {
    return (
      status?.nodes.find(n => (n.nodeId || n.id) === selectedId) ??
      status?.nodes[0]
    );
  }, [status, selectedId]);

  const samplePercent = Math.min(100, ((status?.baseline.samples ?? 0) / 60) * 100);
  const onlineCount = status?.nodes.filter(n => n.online).length ?? 0;
  const isBlastActive = status?.blastSuppression?.active ?? false;
  const blastRemaining = status?.blastSuppression?.remainingSeconds ?? 0;

  return (
    <main className={`level-${level}`}>
      {/* Blast Suppression Warning Banner */}
      {isBlastActive && (
        <div style={{
          background: "#F59E0B",
          color: "#000",
          textAlign: "center",
          padding: "7px 16px",
          fontWeight: 700,
          fontSize: "13px",
          letterSpacing: "0.05em",
          display: "flex",
          justifyContent: "center",
          alignItems: "center",
          gap: "12px"
        }}>
          <span>⚡ SCHEDULED MINE BLAST IN PROGRESS ({blastRemaining}s remaining) — Transient vibration alarms muted</span>
          <button 
            onClick={toggleBlast}
            style={{
              padding: "2px 8px",
              background: "#000",
              color: "#FFF",
              border: "0",
              borderRadius: "4px",
              cursor: "pointer",
              fontSize: "11px"
            }}
          >
            End Now
          </button>
        </div>
      )}

      <header className="topbar">
        <div className="brand">
          <span className="brand-mark">S</span>
          <div>
            <h1>SurakshaMesh</h1>
            <p>SUBSIDENCE EARLY-WARNING NETWORK</p>
          </div>
        </div>
        <div className="top-meta" style={{ display: "flex", gap: "10px", alignItems: "center" }}>
          <span>SECTOR / DEMO-01</span>
          
          <button 
            onClick={toggleBlast}
            disabled={busy}
            title="Suppress false alarms during planned mine detonation"
            style={{
              padding: "5px 10px",
              background: isBlastActive ? "#F59E0B" : "rgba(245, 158, 11, 0.15)",
              border: "1px solid #F59E0B",
              borderRadius: "6px",
              color: isBlastActive ? "#000" : "#FBBF24",
              cursor: "pointer",
              fontSize: "11px",
              fontWeight: 600
            }}
          >
            {isBlastActive ? `💥 BLAST (${blastRemaining}s)` : "💥 BLAST MODE (60s)"}
          </button>

          <button 
            onClick={calibrate}
            disabled={busy}
            title="Set current physical sensor position as 0.0° level ground"
            style={{
              padding: "5px 10px",
              background: "rgba(59, 130, 246, 0.15)",
              border: "1px solid #3B82F6",
              borderRadius: "6px",
              color: "#60A5FA",
              cursor: "pointer",
              fontSize: "11px",
              fontWeight: 600
            }}
          >
            🎯 TARE / ZERO
          </button>

          <button 
            onClick={resetAll}
            disabled={busy}
            title="Clear all active alarms, reset logs and re-zero"
            style={{
              padding: "5px 10px",
              background: "rgba(239, 68, 68, 0.15)",
              border: "1px solid #EF4444",
              borderRadius: "6px",
              color: "#F87171",
              cursor: "pointer",
              fontSize: "11px",
              fontWeight: 600
            }}
          >
            🔄 RESET
          </button>

          <button 
            onClick={() => setAudioEnabled(a => !a)}
            title={audioEnabled ? "Mute Web Audio Alarm" : "Enable Web Audio Alarm"}
            style={{
              padding: "5px 8px",
              background: "var(--color-bg-elevated)",
              border: "1px solid var(--color-border)",
              borderRadius: "6px",
              color: audioEnabled ? "#34D399" : "#94A3B8",
              cursor: "pointer",
              fontSize: "11px"
            }}
          >
            {audioEnabled ? "🔊 SOUND ON" : "🔇 MUTED"}
          </button>

          <span className="gateway">
            <i style={{ background: onlineCount > 0 ? "var(--color-normal)" : "#64748B" }} />
            {onlineCount > 0 ? `${onlineCount} NODE(S) ONLINE` : "WAITING FOR GATEWAY"}
          </span>
        </div>
      </header>

      <section className="status-banner" role="alert" aria-live={level === 3 ? "assertive" : "polite"}>
        <div>
          <p className="eyebrow">PANEL STATUS / REAL-TIME SUBSIDENCE RISK</p>
          <strong>{label[level]}</strong>
          <p className="status-action">{actionText[level]}</p>
        </div>
        <div className="decision-summary">
          <small>DECISION MODE</small>
          <b>{status?.baseline.ready ? "ACTIVE MONITORING (TARED)" : "CALIBRATING BASELINE"}</b>
          <span>{status?.rationale}</span>
          <div className="progress">
            <i style={{ width: `${samplePercent}%` }} />
          </div>
        </div>
      </section>

      <section className="command-layout">
        <aside className="left-sidebar">
          <p className="panel-label">CONNECTED NODES ({onlineCount}/{status?.nodes.length ?? 0})</p>
          <div className="node-list">
            {status?.nodes.map(node => {
              const id = node.nodeId || node.id || "NODE";
              const isSelected = (selected?.nodeId || selected?.id) === id;
              const isOnline = node.online;
              const dP = node.deltaPitch ?? node.pitch;
              const dR = node.deltaRoll ?? node.roll;

              return (
                <button
                  className={`node-row level-${node.level} ${isSelected ? "selected" : ""} ${!isOnline ? "offline" : ""}`}
                  key={id}
                  onClick={() => setSelectedId(id)}
                  style={{ opacity: isOnline ? 1 : 0.45 }}
                >
                  <span className="state-dot" style={{ background: !isOnline ? "#64748B" : undefined }} />
                  <span>
                    <b>{id}</b>
                    <small>{(node.role || "field").toUpperCase()} · {isOnline ? "ONLINE" : "OFFLINE"}</small>
                  </span>
                  <code>
                    {isOnline ? (
                      <>
                        ΔP {dP > 0 ? "+" : ""}{dP.toFixed(1)}°<br />
                        ΔR {dR > 0 ? "+" : ""}{dR.toFixed(1)}° · {node.vibration.toFixed(3)}g
                      </>
                    ) : (
                      <>
                        DISCONNECTED<br />
                        NO SIGNAL
                      </>
                    )}
                  </code>
                </button>
              );
            })}
          </div>
          <div className="decision-panel">
            <p className="panel-label">DECISION ENGINE</p>
            <b className="mode-badge">{status?.baseline.ready ? "ACTIVE" : "LEARNING"}</b>
            <div className="sample-count">
              <strong>{status?.baseline.samples ?? 0}</strong>
              <span>/ 60 SAMPLES</span>
            </div>
            <div className="meter">
              <i style={{ strokeDashoffset: 126 - samplePercent * 1.26 }} />
            </div>
            <p>Deviation is calculated relative to your tared ground zero.</p>
            <button 
              onClick={resetAll}
              disabled={busy}
              style={{
                width: "100%",
                marginTop: "12px",
                padding: "8px",
                background: "var(--color-bg-elevated)",
                border: "1px solid var(--color-border-mid)",
                borderRadius: "6px",
                color: "var(--color-text-primary)",
                cursor: "pointer",
                fontSize: "12px"
              }}
            >
              🔄 Reset Alarms &amp; Baseline
            </button>
          </div>
        </aside>

        <section className="main-panel">
          <div className="mesh-controls">
            <div>
              <p className="eyebrow">LIVE MESH TOPOLOGY</p>
              <h2>Surface Sensor Mesh</h2>
              <small>{onlineCount} ACTIVE NODES · HEX STATUS MAP</small>
            </div>
            <div style={{ display: "flex", gap: "6px", flexWrap: "wrap" }}>
              <button disabled={busy} onClick={toggleBlast} style={{ borderColor: "#F59E0B", color: "#FBBF24" }}>
                💥 Blast Mode
              </button>
              <button disabled={busy} onClick={calibrate} style={{ borderColor: "#3B82F6", color: "#60A5FA" }}>
                🎯 Zero
              </button>
              <button disabled={busy} onClick={resetAll} style={{ borderColor: "#EF4444", color: "#F87171" }}>
                🔄 Reset
              </button>
              <button disabled={busy} onClick={() => simulate("normal")}>Normal</button>
              <button disabled={busy} onClick={() => simulate("shift")}>Shift</button>
              <button disabled={busy} className="critical-button" onClick={() => simulate("collapse")}>Cave-In</button>
            </div>
          </div>
          <HexMeshCanvas
            nodes={status?.nodes ?? []}
            selectedId={selected?.nodeId || selected?.id || null}
            onSelect={setSelectedId}
          />
        </section>

        <aside className="right-panel">
          <div style={{ display: "flex", justifyContent: "space-between", alignItems: "center" }}>
            <p className="panel-label">SYSTEM EVENTS &amp; AUDIT LOG</p>
            <button 
              onClick={exportCSV} 
              style={{
                background: "transparent",
                border: "1px solid var(--color-border)",
                borderRadius: "4px",
                color: "var(--color-accent-light)",
                fontSize: "10px",
                padding: "2px 6px",
                cursor: "pointer"
              }}
            >
              📥 CSV
            </button>
          </div>
          <div className="event-log">
            {status?.events.slice(0, 12).map((event, index) => (
              <div className={`event-row level-${event.level}`} key={`${event.time}-${index}`}>
                <time>{event.time}</time>
                <i />
                <span>{event.text}</span>
              </div>
            ))}
          </div>
          <div className="trend">
            <p className="panel-label">NODE TREND — {selected?.nodeId || selected?.id || "--"}</p>
            <div className="sparkline">
              <svg viewBox="0 0 240 74" aria-label="Selected node trend">
                <polyline points="0,52 22,48 44,51 66,38 88,46 110,36 132,41 154,25 176,33 198,19 220,28 240,14" />
              </svg>
            </div>
            <code>ΔPITCH · ΔROLL · VIBRATION RMS</code>
          </div>
        </aside>
      </section>

      <section className={`bottom-bar ${expanded ? "open" : ""}`}>
        <button onClick={() => setExpanded(value => !value)}>
          SYSTEM THRESHOLDS &amp; SOP <span>{expanded ? "▲" : "▼"}</span>
        </button>
        {expanded && (
          <div className="bottom-details">
            <div>
              <p><strong>Current Diagnostic:</strong> {status?.rationale}</p>
              <p style={{ marginTop: "8px", fontSize: "12px", color: "var(--color-text-muted)" }}>
                Multi-node consensus filters out single-point vibrations (trucks/blasting) and triggers emergency alerts only when spatial coherence confirms strata sag.
              </p>
            </div>
            <table>
              <thead>
                <tr>
                  <th>METRIC</th>
                  <th>NORMAL (GREEN)</th>
                  <th>WATCH (YELLOW)</th>
                  <th>CRITICAL (RED)</th>
                </tr>
              </thead>
              <tbody>
                <tr>
                  <td>Delta Tilt (ΔP/ΔR)</td>
                  <td>&lt; 2.0°</td>
                  <td>2.0° – 5.0°</td>
                  <td>&gt; 8.0°</td>
                </tr>
                <tr>
                  <td>Vibration RMS</td>
                  <td>&lt; 0.15g</td>
                  <td>0.15g – 0.40g</td>
                  <td>&gt; 0.40g sustained</td>
                </tr>
                <tr>
                  <td>Multi-Node Coherence</td>
                  <td>Isolated</td>
                  <td>2+ nodes creeping</td>
                  <td>Synchronous Collapse</td>
                </tr>
              </tbody>
            </table>
          </div>
        )}
      </section>
    </main>
  );
}
