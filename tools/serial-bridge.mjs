import { spawn } from "node:child_process";
import readline from "node:readline";

/**
 * SurakshaMesh Serial-to-HTTP Gateway Bridge
 * 
 * Bridges USB Serial JSON data from the central ESP32 to the Next.js dashboard.
 * Usage: node tools/serial-bridge.mjs [PORT] [BASE_URL] [BAUD_RATE]
 * Example: node tools/serial-bridge.mjs COM3 http://localhost:3000 115200
 */

const targetPort = process.argv[2] ?? process.env.SERIAL_PORT ?? "COM3";
const baseUrl = process.argv[3] ?? process.env.BASE_URL ?? "http://localhost:3000";
const baudRate = Number.parseInt(process.argv[4] ?? process.env.BAUD_RATE ?? "115200", 10);

console.log("====================================================");
console.log("  SurakshaMesh USB Serial -> HTTP Gateway Bridge   ");
console.log("====================================================");
console.log(`Target Port: ${targetPort}`);
console.log(`Baud Rate:   ${baudRate}`);
console.log(`Endpoint:    ${baseUrl}/api/telemetry`);
console.log("Connecting...\n");

function normalizePacket(raw) {
  const nodeId = raw.nodeId ?? (typeof raw.id === "number" ? `NODE-0${raw.id}` : String(raw.id ?? "NODE-01"));
  const role = raw.role ?? (nodeId === "NODE-01" ? "gateway" : "field");
  const pitch = Number(raw.pitch ?? 0);
  const roll = Number(raw.roll ?? 0);
  const vibration = Number(raw.vibration ?? raw.vib ?? 0);
  const timestamp = raw.timestamp ?? new Date().toISOString();

  return {
    nodeId,
    role,
    pitch,
    roll,
    vibration,
    timestamp
  };
}

async function forwardTelemetry(packets) {
  try {
    const res = await fetch(`${baseUrl}/api/telemetry`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ packets })
    });
    if (!res.ok) {
      console.warn(`[API WARN] ${res.status}: ${res.statusText}`);
    } else {
      const summary = packets.map(p => `${p.nodeId}(P:${p.pitch}° R:${p.roll}° V:${p.vibration.toFixed(4)}g)`).join(", ");
      console.log(`[✓] Forwarded: ${summary}`);
    }
  } catch (err) {
    console.error(`[API ERROR] Failed to forward to ${baseUrl}:`, err.message);
  }
}

// Launch PowerShell serial reader script on Windows
const psScript = `
$port = New-Object System.IO.Ports.SerialPort '${targetPort}', ${baudRate}, None, 8, One
$port.ReadTimeout = 5000
try {
    $port.Open()
    Write-Output "CONNECTED"
    while ($port.IsOpen) {
        try {
            $line = $port.ReadLine()
            if ($line) { Write-Output $line }
        } catch [TimeoutException] {
            # ignore timeout on quiet bus
        } catch {
            Write-Error $_.Exception.Message
            break
        }
    }
} catch {
    Write-Error $_.Exception.Message
} finally {
    if ($port.IsOpen) { $port.Close() }
}
`;

const proc = spawn("powershell", ["-NoProfile", "-Command", psScript]);

const rl = readline.createInterface({
  input: proc.stdout,
  terminal: false
});

rl.on("line", line => {
  const trimmed = line.trim();
  if (!trimmed) return;
  if (trimmed === "CONNECTED") {
    console.log(`⚡ Serial connection established on ${targetPort}. Listening for packets...`);
    return;
  }

  // Parse JSON line from ESP32
  try {
    const parsed = JSON.parse(trimmed);
    if (parsed.err) {
      console.warn("[NODE ERROR]", parsed.err);
      return;
    }
    if (parsed.boot) {
      console.log(`[NODE BOOT] Node ID: ${parsed.nodeId}, Gateway: ${parsed.isGateway}`);
      return;
    }

    if (Array.isArray(parsed.packets)) {
      const normalized = parsed.packets.map(normalizePacket);
      forwardTelemetry(normalized);
    } else {
      const normalized = [normalizePacket(parsed)];
      forwardTelemetry(normalized);
    }
  } catch {
    // Non-JSON debug message from ESP32
    console.log(`[ESP32 RAW] ${trimmed}`);
  }
});

proc.stderr.on("data", data => {
  const msg = data.toString().trim();
  if (msg) console.error(`[SERIAL ERROR] ${msg}`);
});

proc.on("close", code => {
  console.log(`Serial bridge process exited with code ${code}.`);
});

process.on("SIGINT", () => {
  proc.kill();
  process.exit();
});
