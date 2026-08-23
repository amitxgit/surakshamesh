# SurakshaMesh — System Improvements Instruction

**Primary owner: S2 (dashboard)**
**S1 tasks: Items 4 and 5 (firmware additions, clearly marked)**
**Covers:** Node heartbeat, blast suppress button, inter-node spatial correlation, sequence tracking, temperature logging, CSV enhancements, Isolation Forest update
**Internal deadline:** 25 Aug 2026
**Starred items are required for the internal. All others are for the finale.**

---

## Item 1: Node Heartbeat and Dead-Node Detection ★ Required for 25 Aug

**Owner: S2**

### Problem

If a node stops transmitting, the dashboard shows the last known values indefinitely. The grid cell stays green or yellow forever. In a safety system this is a defect, not a feature. A node that goes silent in a Watch zone is more alarming than a noisy one. Judges will ask "what happens if a node battery dies or a cable gets cut?"

### Implementation in app.py

Add a `last_seen` dict at module level. Update it on every packet arrival. Add a status function that computes the current state of each node based on how long ago it was heard.

```python
import time

HEARTBEAT_TIMEOUT_S = 10    # seconds before a node is considered lost
EXPECTED_NODES = [1, 2]     # update to match deployed node IDs

last_seen = {}              # node_id -> time.time() of most recent packet

# Call this every time a packet arrives:
def on_packet(data):
    node_id = data['id']
    last_seen[node_id] = time.time()
    # ... rest of existing packet handling

def node_status(node_id):
    if node_id not in last_seen:
        return 'WAITING'
    age = time.time() - last_seen[node_id]
    if age > HEARTBEAT_TIMEOUT_S:
        return 'LOST'
    return 'OK'
```

### Risk escalation when a node goes LOST

Apply this rule when computing the effective risk for the dashboard display. Store the last known risk for each node separately from the incoming packet risk field.

```python
last_known_risk = {}    # node_id -> most recent risk value from packet

def effective_risk(node_id):
    status = node_status(node_id)
    risk = last_known_risk.get(node_id, 0)
    if status == 'LOST':
        if risk == 0:
            return 0, 'LOST'      # was green, now just shows LOST, no alert escalation
        else:
            return min(risk + 1, 3), 'LOST'   # escalate by one level
    return risk, status
```

The escalation rule: a Watch node going LOST becomes Warning. A Warning node going LOST becomes Critical. The reasoning is that in a safety context, unexplained silence in a hazard zone is treated as a possible hazard, not as all-clear.

### Display

When `node_status()` returns `'LOST'`, grey out the cell and show:

```
NODE 2 — LOST
Last contact: 23 s ago
```

In grey text on a dark background. Do not show the last-known pitch/roll values because they are stale. Show only the LOST state and the age.

When the node reconnects and a new packet arrives, restore the cell immediately with live data. Log a reconnection event in the CSV with a timestamp.

Add the `age` calculation to the Flask state endpoint so the JavaScript can display it:

```python
@app.route('/state')
def state():
    nodes = []
    for nid in EXPECTED_NODES:
        status = node_status(nid)
        age_s  = round(time.time() - last_seen[nid], 1) if nid in last_seen else None
        r, _   = effective_risk(nid)
        nodes.append({
            'id': nid,
            'status': status,
            'age_s': age_s,
            'risk': r,
            # pitch, roll, vib, stalta etc. from last_known_data[nid]
        })
    return jsonify(nodes)
```

### Demo moment

During the 25 Aug internal, practice once where H1 unplugs the USB cable to node 2 while it is displaying Watch. The dashboard should transition that cell to LOST within 10 seconds and escalate to Warning. Then plug it back in. This takes about 30 seconds and makes a strong impression on any judge who asks about fault tolerance.

---

## Item 2: Blasting Schedule Suppress Button ★ Required for 25 Aug

**Owner: S2**

### Problem

In a real mine, blasting is scheduled. Crews know in advance when it will happen. Without a way to tell the system "we are about to blast, suppress vibration alerts," the system will cry wolf on every scheduled blast and mine staff will stop trusting it within a week. Judges with any domain knowledge will ask this. The firmware-level blast classification from the ALGO instruction handles autonomous suppression after the fact. This button provides manual suppression before the fact, for planned events.

### Backend in app.py

```python
manual_suppress_until = 0.0     # Unix timestamp, 0 = not suppressed

@app.route('/suppress', methods=['POST'])
def suppress():
    global manual_suppress_until
    try:
        payload  = request.get_json(force=True)
        duration = int(payload.get('duration', 300))
        duration = max(60, min(duration, 1800))          # clamp 1 min to 30 min
        manual_suppress_until = time.time() + duration
        return jsonify({'status': 'ok', 'suppress_until': manual_suppress_until,
                        'remaining_s': duration})
    except Exception as e:
        return jsonify({'status': 'error', 'msg': str(e)}), 400

@app.route('/suppress_cancel', methods=['POST'])
def suppress_cancel():
    global manual_suppress_until
    manual_suppress_until = 0.0
    return jsonify({'status': 'cancelled'})

@app.route('/suppress_status')
def suppress_status():
    remaining = max(0.0, manual_suppress_until - time.time())
    return jsonify({'active': remaining > 0, 'remaining_s': int(remaining)})

def is_manually_suppressed():
    return time.time() < manual_suppress_until
```

### Frontend in index.html

Add a control row below the grid:

```html
<div id="blast-controls" style="margin-top:16px; padding:12px; border:1px solid #555;">
    <strong style="color:#F5C400;">BLAST WINDOW CONTROL</strong>
    <div style="margin-top:8px;">
        <button id="btn-suppress-5m"  onclick="suppressBlast(300)">Suppress 5 min</button>
        <button id="btn-suppress-10m" onclick="suppressBlast(600)">Suppress 10 min</button>
        <button id="btn-suppress-15m" onclick="suppressBlast(900)">Suppress 15 min</button>
        <button id="btn-cancel"       onclick="cancelSuppress()" style="display:none;">Cancel</button>
        <span id="suppress-status" style="margin-left:16px; color:#F5C400;"></span>
    </div>
</div>

<script>
function suppressBlast(duration) {
    fetch('/suppress', {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify({duration: duration})
    })
    .then(r => r.json())
    .then(d => { if (d.status === 'ok') updateSuppressStatus(); });
}

function cancelSuppress() {
    fetch('/suppress_cancel', {method: 'POST'}).then(() => updateSuppressStatus());
}

function updateSuppressStatus() {
    fetch('/suppress_status').then(r => r.json()).then(d => {
        const statusEl = document.getElementById('suppress-status');
        const cancelBtn = document.getElementById('btn-cancel');
        if (d.active) {
            statusEl.textContent = 'SUPPRESS ACTIVE: ' + d.remaining_s + 's remaining';
            cancelBtn.style.display = 'inline';
        } else {
            statusEl.textContent = '';
            cancelBtn.style.display = 'none';
        }
    });
}

setInterval(updateSuppressStatus, 1000);
</script>
```

### In the dashboard risk computation

When `is_manually_suppressed()` returns True, set `vib = 0.0` for all nodes before computing the displayed risk colour. Tilt still counts. This means a planned blast cannot push any node into Warning or Critical, but a real tilt event coincident with the blast window will still show up.

Add a column `manual_suppressed` (1 or 0) to the CSV log row for every packet during suppression. This creates a defensible audit trail: if you review the log later and see a spike in vib during a time when `manual_suppressed: 1`, you know to disregard it.

Also add a visual indicator at the top of the dashboard during suppression. A yellow bar across the top of the page reading "BLAST SUPPRESS ACTIVE" is enough.

---

## Item 3: Inter-Node Spatial Correlation — Mention for 25 Aug, Implement for Finale

**Owner: S2**

### Why this matters

This is the most important unrealized capability in the current design. A single sensor tells you that something happened near it. A mesh tells you that something happened across a region and in which direction it is propagating. That distinction is the entire engineering argument for why a mesh is worth deploying instead of one good sensor.

Random noise, a blast, and a heavy truck affect each node independently with no directional pattern. Subsidence is a spatially coherent phenomenon: it tilts adjacent nodes in the same direction because it is a large-scale deformation of the ground surface, not a point disturbance. Detecting that coherence is what no single-node system can do.

### For the 25 Aug internal

You have two nodes on a plank. When you lift one end, both nodes tilt in the same direction (the lifted end goes up on both). Coherence will always fire. That is fine. Use the two-node demo to establish the concept and tell judges: "In the field with 6 to 8 nodes arranged over a panel, coherence becomes a reliable discriminator because a blast or a truck hits the nodes at random angles while ground subsidence tilts them all in the same direction."

Put one line on slide 3: "Multi-node spatial coherence detection distinguishes ground-level deformation from point disturbances. A single sensor cannot do this."

### Implementation for finale (include in app.py now so it runs in the background)

```python
def check_spatial_coherence(node_states):
    """
    node_states: dict of node_id -> dict with keys:
        'risk': int (0-3)
        'pitch_rate': float (deg/s)
        'roll_rate': float (deg/s)
        'status': str ('OK', 'LOST', 'WAITING')

    Returns (coherent, direction_str, message_str)
    """
    DEAD_BAND = 0.005   # deg/s - ignore rates below this

    active = {
        nid: s for nid, s in node_states.items()
        if s['status'] == 'OK' and s['risk'] >= 1
    }

    if len(active) < 2:
        return False, None, None

    pitch_rates = [s['pitch_rate'] for s in active.values()]
    roll_rates  = [s['roll_rate']  for s in active.values()]

    pitch_pos = all(r >  DEAD_BAND for r in pitch_rates)
    pitch_neg = all(r < -DEAD_BAND for r in pitch_rates)
    roll_pos  = all(r >  DEAD_BAND for r in roll_rates)
    roll_neg  = all(r < -DEAD_BAND for r in roll_rates)

    pitch_coherent = pitch_pos or pitch_neg
    roll_coherent  = roll_pos  or roll_neg

    if not (pitch_coherent or roll_coherent):
        return False, None, None

    parts = []
    if pitch_coherent:
        parts.append('pitch ' + ('positive' if pitch_pos else 'negative'))
    if roll_coherent:
        parts.append('roll ' + ('positive' if roll_pos else 'negative'))

    direction = ', '.join(parts)
    n = len(active)
    msg = f'COHERENT DEFORMATION: {n} nodes moving in the same direction ({direction})'
    return True, direction, msg
```

Call this once per second in the main dashboard update loop. Pass in the current node states dictionary.

### Display

When coherence is detected, show a banner across the top of the grid:

```
SPATIAL COHERENCE DETECTED — 2 nodes tilting in same direction — likely subsidence
```

Yellow for Watch, red for Warning or Critical. Clear the banner when coherence breaks.

Log a coherence event to the CSV with a start timestamp and, when it clears, an end timestamp and duration. This is what makes the CSV log forensically useful.

### What to say to judges

"Each node is independent. But subsidence is not a node-level event. It is a ground-level event that propagates across the panel. When two or more adjacent nodes tilt in the same direction simultaneously, that is a signature that is physically impossible from a blast or a passing vehicle. Our system detects and labels that coherence. A single sensor cannot do this."

---

## Item 4: Temperature Read and Log ★ Required for 25 Aug

**Owner: S1 (firmware addition)**

### Problem

MPU6050 tilt readings drift with temperature due to the temperature coefficient of the MEMS accelerometer. In a mining surface environment in India, morning-to-afternoon temperature swings of 15 to 25 degrees Celsius cause approximately 0.3 to 0.8 degrees of apparent tilt change. Without logging temperature, you cannot distinguish real ground movement from thermal drift when reviewing the CSV. Judges who understand MEMS sensors will ask about this.

You do not need to apply a correction for the internal. Just read and log the temperature. The log becomes your evidence that you know this issue exists and have the data to address it.

### What to change in node.ino

The MPU6050 stores temperature in registers 0x41 and 0x42, a 16-bit signed integer. The scaling formula is: temperature in Celsius equals raw value divided by 340 plus 36.53.

You are already reading these two bytes in `mpu_read()` and discarding them with two `Wire.read()` calls. Change the function to capture this value instead.

**Current code in mpu_read():**

```c
Wire.read(); Wire.read(); // temp
```

**Replace with:**

```c
int16_t raw_temp = ((int16_t)Wire.read() << 8) | Wire.read();
float temp_c = raw_temp / 340.0f + 36.53f;
```

Either add `temp_c` as a pointer parameter to `mpu_read()` or store it in a file-scope static. A static is simpler:

```c
static float node_temp_c = 25.0f;   // default until first read

// Inside mpu_read, after the line above:
node_temp_c = raw_temp / 340.0f + 36.53f;
```

**Add to the JSON output in emit_json():**

```c
Serial.print(",\"temp\":");
Serial.print(node_temp_c, 1);
```

Add `"temp"` to the packet struct only if you have byte budget to spare. It is not strictly needed in the ESP-NOW packet. Emitting it in JSON on the hub is enough for logging.

### What S2 does with the temperature value

Parse the `temp` field from JSON. Add it as a column to the CSV log. That is all. Do not display it on the main grid cells (it is diagnostic information, not a risk indicator). If you want to show it, a small text line at the very bottom of the dashboard reading "Node 1: 38.2°C | Node 2: 37.9°C" is sufficient.

### What this enables

After the demo, open the CSV and look at two columns: `pitch` and `temp` over a 2-hour window. If pitch drifts 0.4 degrees while temperature rises 12 degrees, you can compute the drift coefficient for that specific board and environment. That becomes a real result you can cite in the finale feasibility slide: "We measured 0.03 deg/°C thermal drift on our MPU6050 nodes and plan to apply a linear correction in the field firmware."

---

## Item 5: Sequence Number Tracking

**Owner: S1 (firmware) and S2 (dashboard)**

The sequence number is defined in the ALGO instruction and should already be added to the packet struct as part of that work. This item defines what S2 does with it.

### In the dashboard

Track the most recently received sequence number per node:

```python
last_seq = {}   # node_id -> last seen seq value

def check_dropped_packets(node_id, seq):
    if node_id not in last_seq:
        last_seq[node_id] = seq
        return 0
    expected = (last_seq[node_id] + 1) % 65536
    if seq == expected:
        last_seq[node_id] = seq
        return 0
    # Compute how many were dropped (handles wrap-around)
    dropped = (seq - expected) % 65536
    last_seq[node_id] = seq
    return dropped

# Call this each time a packet arrives:
dropped = check_dropped_packets(data['id'], data['seq'])
if dropped > 0:
    log_event(f"NODE {data['id']}: {dropped} packet(s) dropped")
```

Add a `dropped_packets` column to the CSV log. In the demo environment with two nodes side by side this will always be zero. In a field deployment with obstacles, terrain, and interference this will not be zero, and the sequence counter becomes a mesh health metric that tells you which nodes have poor radio connectivity.

---

## Item 6: CSV Log Enhancement ★ Required for 25 Aug

**Owner: S2**

### Current state

The CSV log captures pitch, roll, vib, t, risk. The PLAYBOOK calls it "historical data for the AI slide." In its current form it does not earn that description.

### Required columns

Update the CSV writer to include all of these columns in this order:

```
timestamp_unix, node_id, seq, pitch, roll, vib, stalta, evt, risk,
pitch_rate, roll_rate, temp, coherent, manual_suppressed, node_status, dropped_packets
```

Definitions:

- `timestamp_unix`: Python time.time() at the moment the packet was processed on the dashboard, not the firmware's `t_ms`. This gives you a wall-clock timestamp for the log.
- `seq`: from the packet, for cross-referencing
- `stalta`: from the packet
- `evt`: from the packet (0/1/2/3)
- `pitch_rate`, `roll_rate`: computed by the dashboard from tilt history
- `temp`: from the packet if present, empty string if node firmware not yet updated
- `coherent`: 1 if spatial coherence was active this second across the mesh, 0 otherwise
- `manual_suppressed`: 1 if the blast suppress button is active, 0 otherwise
- `node_status`: OK / LOST / WAITING
- `dropped_packets`: number of seq gaps detected this packet (0 in normal operation)

### Implementation

```python
import csv
import time

LOG_PATH = 'data/log.csv'
LOG_HEADERS = [
    'timestamp_unix', 'node_id', 'seq', 'pitch', 'roll', 'vib', 'stalta', 'evt', 'risk',
    'pitch_rate', 'roll_rate', 'temp', 'coherent', 'manual_suppressed', 'node_status',
    'dropped_packets'
]

def ensure_log_headers():
    import os
    if not os.path.exists(LOG_PATH) or os.path.getsize(LOG_PATH) == 0:
        with open(LOG_PATH, 'w', newline='') as f:
            csv.writer(f).writerow(LOG_HEADERS)

def log_packet(data, pitch_rate, roll_rate, coherent, node_status, dropped):
    row = {
        'timestamp_unix':  round(time.time(), 3),
        'node_id':         data.get('id'),
        'seq':             data.get('seq', ''),
        'pitch':           round(data.get('pitch', 0), 3),
        'roll':            round(data.get('roll', 0), 3),
        'vib':             round(data.get('vib', 0), 4),
        'stalta':          round(data.get('stalta', 0), 2),
        'evt':             data.get('evt', 0),
        'risk':            data.get('risk', 0),
        'pitch_rate':      round(pitch_rate, 4),
        'roll_rate':       round(roll_rate, 4),
        'temp':            data.get('temp', ''),
        'coherent':        1 if coherent else 0,
        'manual_suppressed': 1 if is_manually_suppressed() else 0,
        'node_status':     node_status,
        'dropped_packets': dropped
    }
    with open(LOG_PATH, 'a', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=LOG_HEADERS)
        writer.writerow(row)
```

Call `ensure_log_headers()` at startup before the Flask server begins. Call `log_packet()` every time a packet arrives.

---

## Item 7: Isolation Forest Feature Update — for 25 Aug if sklearn is already installed

**Owner: S2**

The PLAYBOOK already defines the Isolation Forest baseline button. This item updates the feature vector to include the new signals.

### Current feature vector (assumed)

```python
features = [abs(pitch), abs(roll), vib]
```

### Updated feature vector

```python
features = [
    abs(pitch),
    abs(roll),
    vib,
    abs(pitch_rate),
    abs(roll_rate),
    stalta
]
```

A node that is sitting quietly has near-zero pitch, near-zero roll, low vib, near-zero pitch rate, near-zero roll rate, and STA/LTA close to 1.0. Its anomaly score will be low. A node with elevated pitch, rising rate, and a recent STA/LTA spike will have a high score even if no single threshold has been crossed yet. This is a better detector than any single threshold and it is an honest description of what Isolation Forest is doing.

```python
from sklearn.ensemble import IsolationForest

baseline_data = {1: [], 2: []}     # node_id -> list of feature vectors
models        = {}                  # node_id -> trained IsolationForest

def add_to_baseline(node_id, pitch, roll, vib, pitch_rate, roll_rate, stalta):
    vec = [abs(pitch), abs(roll), vib, abs(pitch_rate), abs(roll_rate), stalta]
    baseline_data[node_id].append(vec)

def train_models():
    for node_id, data in baseline_data.items():
        if len(data) < 10:
            continue
        clf = IsolationForest(n_estimators=50, max_samples=min(64, len(data)),
                               contamination=0.05, random_state=42)
        clf.fit(data)
        models[node_id] = clf
    baseline_data.clear()

def score_node(node_id, pitch, roll, vib, pitch_rate, roll_rate, stalta):
    if node_id not in models:
        return None
    vec = [[abs(pitch), abs(roll), vib, abs(pitch_rate), abs(roll_rate), stalta]]
    score = models[node_id].decision_function(vec)[0]
    # Negative score means anomaly. More negative = more anomalous.
    return round(float(score), 4)
```

The Learn Baseline button should collect data for 20 seconds (20 samples at 1 Hz) per node before calling `train_models()`. After training, display the anomaly score alongside the risk number in each cell.

---

## Implementation Timeline

Work through this list in order. Items marked with ★ must be done before 25 Aug 08:00. Others go to the finale.

| Priority | Item | Owner | Time estimate |
|---|---|---|---|
| ★ 1 | Temperature read in firmware (Item 4) | S1 | 20 min |
| ★ 2 | Node heartbeat detection (Item 1) | S2 | 30 min |
| ★ 3 | Blast suppress button backend and frontend (Item 2) | S2 | 45 min |
| ★ 4 | Tilt rate computation (from ALGO instruction) | S2 | 60 min |
| ★ 5 | Event type display on grid (from ALGO instruction) | S2 | 30 min |
| ★ 6 | CSV column update (Item 6) | S2 | 30 min |
| ★ 7 | Sequence gap tracking (Item 5) | S2 | 20 min |
| Should | Isolation Forest feature update (Item 7) | S2 | 30 min |
| Finale | Spatial coherence detection (Item 3) | S2 | 90 min |

### Drop order if time is short

Drop in this order. Never drop the items above the line.

1. Drop spatial coherence (Item 3) — mention it verbally and on the slide only
2. Drop Isolation Forest update (Item 7) — use the original 3-feature vector
3. Drop sequence gap tracking (Item 5) — keep the seq field in the log but skip gap detection
4. Never drop: heartbeat, blast suppress, tilt rate display, CSV update, temperature read

---

## Integration Test Before 25 Aug Demo

Run this sequence once on the actual presentation laptop the night before the internal:

1. Start `app.py` with laptop in airplane mode. Confirm the browser opens and the grid loads.
2. Plug in the hub node. Confirm JSON appears in Serial Monitor and the cell updates.
3. Plug in the second node. Confirm both IDs appear on the grid.
4. Let it sit flat for 60 seconds. Confirm the CSV is growing, coherence shows 0, tilt rate shows near 0.0.
5. Press the Suppress Blast button for 5 minutes. Confirm the yellow banner appears and the button is disabled.
6. Shake the plank hard. Confirm that the vib channel does not push the cell into Warning during suppression. Cancel suppression. Confirm the button re-enables.
7. Tilt the plank slowly. Confirm pitch rate climbs, cell goes Watch then Warning. Confirm coherent shows 1 in the CSV.
8. Unplug node 2. Wait 15 seconds. Confirm the cell goes grey and shows LOST. Confirm the risk escalates from whatever node 2 was showing. Plug it back in. Confirm it recovers.
9. Let an STA/LTA event fire (sharp tap). Confirm the evt banner appears and reads "classifying." Wait 30 seconds. Confirm it resolves to BLAST (3) since no tilt was held.
10. Open the CSV. Confirm all 16 columns are present and populated.

If steps 1 through 8 pass, you are ready for the internal. Steps 9 and 10 are the should-have confirmation.
