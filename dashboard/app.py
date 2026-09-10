"""
SurakshaMesh Telemetry Dashboard
Usage:  python app.py COM6
        python app.py /dev/ttyUSB0
        python app.py --demo          # Simulation mode without hardware
Browser: http://127.0.0.1:5000
"""
from __future__ import annotations

import csv
import json
import os
import sys
import threading
import time
from collections import defaultdict
from pathlib import Path

from flask import Flask, jsonify, render_template, request

PORT = None
DEMO = False
BAUD = 115200
LOG = Path(__file__).parent / "data" / "log.csv"

nodes: dict[int, dict] = {}
lock = threading.Lock()
baseline_buf: dict[int, list] = defaultdict(list)
baseline_on = False
iforest = None
sklearn_ok = False

try:
    from sklearn.ensemble import IsolationForest  # type: ignore
    sklearn_ok = True
except Exception:
    sklearn_ok = False


def upsert(pkt: dict) -> None:
    nid = int(pkt.get("id", 0))
    if nid <= 0:
        return
    rec = {
        "id": nid,
        "pitch": float(pkt.get("pitch", 0)),
        "roll": float(pkt.get("roll", 0)),
        "vib": float(pkt.get("vib", 0)),
        "t": int(pkt.get("t", 0)),
        "risk": int(pkt.get("risk", 0)),
        "ai": 0,
        "ts": time.time(),
    }
    with lock:
        if baseline_on:
            baseline_buf[nid].append([rec["pitch"], rec["roll"], rec["vib"]])
        if iforest is not None:
            try:
                s = iforest.predict([[rec["pitch"], rec["roll"], rec["vib"]]])[0]
                rec["ai"] = 1 if s == -1 else 0
                if rec["ai"] and rec["risk"] == 0:
                    rec["risk"] = 1
            except Exception:
                pass
        prev = nodes.get(nid, {}).get("risk", 0)
        rec["rising"] = rec["risk"] >= 2 and prev < 2
        nodes[nid] = rec
    LOG.parent.mkdir(parents=True, exist_ok=True)
    new = not LOG.exists()
    with LOG.open("a", newline="") as f:
        w = csv.writer(f)
        if new:
            w.writerow(["ts", "id", "pitch", "roll", "vib", "risk"])
        w.writerow([rec["ts"], nid, rec["pitch"], rec["roll"], rec["vib"], rec["risk"]])


def serial_loop() -> None:
    import serial  # pyserial

    while True:
        try:
            ser = serial.Serial(PORT, BAUD, timeout=1)
        except Exception as e:
            print("serial open failed:", e)
            time.sleep(2)
            continue
        print("opened", PORT)
        buf = ""
        try:
            while True:
                chunk = ser.read(256).decode("utf-8", errors="ignore")
                if not chunk:
                    continue
                buf += chunk
                while "\n" in buf:
                    line, buf = buf.split("\n", 1)
                    line = line.strip()
                    if not line.startswith("{"):
                        continue
                    try:
                        pkt = json.loads(line)
                    except json.JSONDecodeError:
                        continue
                    if "pitch" in pkt:
                        upsert(pkt)
        except Exception as e:
            print("serial dropped:", e)
            time.sleep(1)


def demo_loop() -> None:
    """Keyboard-less demo: nodes sit green; HTTP /tilt/<id> lifts them."""
    upsert({"id": 1, "pitch": 0.1, "roll": 0.0, "vib": 0.01, "t": 0, "risk": 0})
    upsert({"id": 2, "pitch": 0.2, "roll": -0.1, "vib": 0.02, "t": 0, "risk": 0})
    while True:
        time.sleep(1)


app = Flask(__name__)


@app.get("/")
def index():
    return render_template("index.html")


@app.get("/api/nodes")
def api_nodes():
    with lock:
        body = {
            "nodes": list(nodes.values()),
            "sklearn": sklearn_ok,
            "baseline": baseline_on,
            "fitted": iforest is not None,
        }
    return jsonify(body)


@app.post("/api/baseline")
def api_baseline():
    global baseline_on, iforest, baseline_buf
    action = (request.json or {}).get("action", "start")
    with lock:
        if action == "start":
            baseline_on = True
            baseline_buf = defaultdict(list)
            iforest = None
            return jsonify({"ok": True, "msg": "recording baseline — keep plank flat 20 s"})
        baseline_on = False
        rows = []
        for v in baseline_buf.values():
            rows.extend(v)
        if sklearn_ok and len(rows) >= 20:
            m = IsolationForest(contamination=0.05, random_state=0)
            m.fit(rows)
            iforest = m
            return jsonify({"ok": True, "msg": f"Isolation Forest fitted on {len(rows)} samples"})
        return jsonify(
            {
                "ok": True,
                "msg": f"baseline stored ({len(rows)} samples). sklearn missing or too few — rules only.",
            }
        )


@app.post("/api/tilt/<int:nid>")
def api_tilt(nid: int):
    """Demo helper: pretend this node just saw a 6° tilt."""
    upsert(
        {
            "id": nid,
            "pitch": 6.2,
            "roll": 0.4,
            "vib": 0.18,
            "t": int(time.time() * 1000),
            "risk": 2,
        }
    )
    return jsonify({"ok": True})


@app.post("/api/flat/<int:nid>")
def api_flat(nid: int):
    upsert(
        {
            "id": nid,
            "pitch": 0.1,
            "roll": 0.0,
            "vib": 0.02,
            "t": int(time.time() * 1000),
            "risk": 0,
        }
    )
    return jsonify({"ok": True})


def main() -> None:
    global PORT, DEMO
    args = sys.argv[1:]
    if not args or args[0] == "--demo":
        DEMO = True
        print("demo mode — no serial. Open / and POST /api/tilt/2")
        threading.Thread(target=demo_loop, daemon=True).start()
    else:
        PORT = args[0]
        threading.Thread(target=serial_loop, daemon=True).start()
    print("http://127.0.0.1:5000")
    app.run(host="127.0.0.1", port=5000, debug=False, threaded=True)


if __name__ == "__main__":
    main()
