"use client";

import { useEffect, useMemo, useState } from "react";
import { HexMeshCanvas, type MeshNode } from "./components/HexMeshCanvas";

type Event = { time: string; level: number; text: string };
type Status = {
  overallLevel: number;
  rationale: string;
  nodes: MeshNode[];
  onlineCount?: number;
  events: Event[];
  baseline: { samples: number; ready: boolean };
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

  const calibrate = async () => {
    setBusy(true);
    await fetch("/api/calibrate", { method: "POST" });
    await refresh();
    setBusy(false);
  };

  const simulate = async (kind: string) => {
    setBusy(true);
    await fetch(`/api/demo/${kind}`, { method: "POST" });
    await refresh();
    setBusy(false);
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

  return (
    <main className={`level-${level}`}>
      <header className="topbar">
        <div className="brand">
          <span className="brand-mark">S</span>
          <div>
            <h1>SurakshaMesh</h1>
            <p>SUBSIDENCE EARLY-WARNING NETWORK</p>
          </div>
        </div>
        <div className="top-meta">
          <span>SECTOR / DEMO-01</span>
          <button 
            className="calibrate-header-btn"
            onClick={calibrate}
            disabled={busy}
            title="Set current physical sensor position as 0.0° level ground"
            style={{
              padding: "5px 12px",
              background: "rgba(59, 130, 246, 0.15)",
              border: "1px solid #3B82F6",
              borderRadius: "6px",
              color: "#60A5FA",
              cursor: "pointer",
              fontSize: "12px",
              fontWeight: 600
            }}
          >
            🎯 TARE / ZERO BASELINE
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
              onClick={calibrate}
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
              Zero Ground Baseline
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
            <div>
              <button disabled={busy} onClick={calibrate} style={{ borderColor: "#3B82F6", color: "#60A5FA" }}>
                🎯 Zero Baseline
              </button>
              <button disabled={busy} onClick={() => simulate("normal")}>Normal</button>
              <button disabled={busy} onClick={() => simulate("shift")}>Simulate Shift</button>
              <button disabled={busy} className="critical-button" onClick={() => simulate("collapse")}>Simulate Cave-In</button>
            </div>
          </div>
          <HexMeshCanvas
            nodes={status?.nodes ?? []}
            selectedId={selected?.nodeId || selected?.id || null}
            onSelect={setSelectedId}
          />
        </section>

        <aside className="right-panel">
          <p className="panel-label">SYSTEM EVENTS & AUDIT LOG</p>
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
