export type MeshNode = {
  id?: string;
  nodeId?: string;
  role: string;
  online: boolean;
  pitch: number;
  roll: number;
  vibration: number;
  level: number;
  deltaPitch?: number;
  deltaRoll?: number;
  deltaTilt?: number;
};

const label = ["NORMAL", "WATCH", "WARNING", "CRITICAL"];
const color = ["#34D399", "#FBBF24", "#FB923C", "#F43F5E"];
const offlineColor = "#64748B";
const centralColor = "#3D5AFF";

const points = (cx: number, cy: number, radius: number) =>
  Array.from({ length: 6 }, (_, index) => {
    const angle = (Math.PI / 180) * (index * 60);
    return `${cx + radius * Math.cos(angle)},${cy + radius * Math.sin(angle)}`;
  }).join(" ");

function position(index: number) {
  const primary = [
    [450, 235],
    [210, 315],
    [690, 315]
  ];
  if (index < primary.length) return primary[index];
  const angle = (index - 3) * 1.35 - 0.45;
  return [450 + Math.cos(angle) * 260, 235 + Math.sin(angle) * 150];
}

export function HexMeshCanvas({
  nodes,
  selectedId,
  onSelect
}: {
  nodes: MeshNode[];
  selectedId: string | null;
  onSelect: (id: string) => void;
}) {
  const gateway = nodes.find(node => node.role === "gateway") ?? nodes[0];
  const gatewayPosition = gateway ? position(nodes.indexOf(gateway)) : [450, 235];

  return (
    <svg
      className="mesh-canvas"
      viewBox="0 0 900 500"
      role="img"
      aria-label={`${nodes.length} connected sensor nodes. Select a node to inspect its readings.`}
    >
      {/* Mesh link lines */}
      {nodes
        .filter(node => node !== gateway)
        .map(node => {
          const nodeId = node.nodeId || node.id || "NODE";
          const [x, y] = position(nodes.indexOf(node));
          const isOnline = node.online;
          const active = isOnline && node.level >= 2;
          const linkColor = !isOnline ? offlineColor : color[node.level];
          return (
            <g key={`link-${nodeId}`}>
              <line x1={gatewayPosition[0]} y1={gatewayPosition[1]} x2={x} y2={y} className="mesh-link" />
              {isOnline && (
                <line
                  x1={gatewayPosition[0]}
                  y1={gatewayPosition[1]}
                  x2={x}
                  y2={y}
                  className={`mesh-flow ${active ? "mesh-flow-alert" : ""}`}
                  stroke={linkColor}
                />
              )}
            </g>
          );
        })}

      {/* Hexagonal Nodes */}
      {nodes.map((node, index) => {
        const nodeId = node.nodeId || node.id || `NODE-${String(index + 1).padStart(2, "0")}`;
        const [x, y] = position(index);
        const isOnline = node.online;
        const active = isOnline && node.level >= 2;
        const selected = selectedId === nodeId;
        const central = node === gateway || node.role === "gateway";
        const nodeColor = !isOnline ? offlineColor : central ? centralColor : color[node.level];
        const radius = central ? 90 : 82;
        const type = central ? "CENTRAL GATEWAY" : `MESH NODE ${String(index + 1).padStart(2, "0")}`;
        const statusText = !isOnline ? "OFFLINE" : central ? "ACTIVE" : label[node.level];

        const dP = node.deltaPitch ?? node.pitch;
        const dR = node.deltaRoll ?? node.roll;

        return (
          <g
            className={`mesh-node ${!isOnline ? "node-offline" : ""}`}
            key={nodeId}
            onClick={() => onSelect(nodeId)}
            role="button"
            tabIndex={0}
            aria-label={`Select ${nodeId}`}
            onKeyDown={event => event.key === "Enter" && onSelect(nodeId)}
            style={{ opacity: isOnline ? 1 : 0.45 }}
          >
            {/* Pulse glow for active alerts */}
            {active && <circle className="mesh-pulse" cx={x} cy={y} r={radius} stroke={nodeColor} />}

            {/* Outer Hexagon border */}
            <polygon points={points(x, y, radius + 10)} className="hex-outer" stroke={nodeColor} />

            {/* Inner Hexagon body */}
            <polygon
              points={points(x, y, radius)}
              className={`hex-face ${selected ? "hex-selected" : ""} ${central ? "central-face" : ""}`}
              stroke={nodeColor}
            />

            {/* Top Status Dot */}
            <circle
              cx={x}
              cy={y - (central ? 50 : 45)}
              r={central ? 4.5 : 4}
              fill={nodeColor}
              className={active ? "alert-dot" : ""}
            />

            {/* Header: Node Type */}
            <text
              x={x}
              y={y - (central ? 30 : 26)}
              className="mesh-type"
              textAnchor="middle"
              dominantBaseline="middle"
            >
              {type}
            </text>

            {/* Title: Node ID */}
            <text
              x={x}
              y={y - (central ? 10 : 8)}
              className="mesh-name"
              textAnchor="middle"
              dominantBaseline="middle"
            >
              {nodeId}
            </text>

            {/* Status Badge */}
            <text
              x={x}
              y={y + (central ? 12 : 11)}
              className="mesh-status"
              fill={nodeColor}
              textAnchor="middle"
              dominantBaseline="middle"
            >
              {statusText}
            </text>

            {/* Pitch & Roll Relative Delta */}
            <text
              x={x}
              y={y + (central ? 34 : 31)}
              className="mesh-data"
              textAnchor="middle"
              dominantBaseline="middle"
            >
              {isOnline ? `ΔP: ${dP > 0 ? "+" : ""}${dP.toFixed(1)}° · ΔR: ${dR > 0 ? "+" : ""}${dR.toFixed(1)}°` : "NO SIGNAL"}
            </text>

            {/* Vibration */}
            <text
              x={x}
              y={y + (central ? 49 : 45)}
              className="mesh-data mesh-data-vib"
              textAnchor="middle"
              dominantBaseline="middle"
            >
              {isOnline ? `VIB: ${node.vibration.toFixed(3)}g` : "DISCONNECTED"}
            </text>
          </g>
        );
      })}
    </svg>
  );
}
