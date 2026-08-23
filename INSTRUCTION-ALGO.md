# SurakshaMesh — Algorithm Implementation Instruction

**Owners:** S1 (firmware) and S2 (dashboard)
**Covers:** STA/LTA event detector, tilt rate computation, event classification
**Internal deadline:** 25 Aug 2026
**Status:** To implement — start after node.ino is flashing and basic JSON is confirmed

---

## Why this exists

The current firmware detects vibration by comparing a 1-second RMS of acceleration magnitude against a fixed 0.15g threshold. That will false-alarm constantly in a real mine. Blasting, truck traffic, conveyor belts, and heavy drills all exceed 0.15g. The system has no way to tell the difference between a vehicle driving overhead and the onset of a cave-in. Judges will ask this question directly.

This instruction defines three components that give an honest technical answer.

- **STA/LTA:** detects that an impulsive event just happened
- **Tilt rate:** detects whether the ground is actively moving and how fast
- **Event persistence classification:** distinguishes blast (vibration with no lasting tilt shift) from subsidence (vibration followed by permanent tilt change)

These three do not replace the existing Watch/Warning/Critical threshold logic. They run alongside it and add an `event_type` field to every packet that the dashboard uses to suppress false alarms and label real ones.

---

## Component 1: STA/LTA Event Detector

**Owner: S1 (firmware)**

### What it does

STA/LTA is the standard event detector from seismology. STA is the mean signal energy in a short window. LTA is the mean signal energy in a long window. When the STA spikes above the LTA by a factor of 4 or more, an impulsive event just happened. The ratio is dimensionless and adapts to the ambient noise floor, which is why it works where a fixed threshold fails.

```
STA window:  0.5 s  (25 samples at 50 Hz)
LTA window:  30 s   (1500 samples at 50 Hz)
Trigger:     STA / LTA > 4.0
```

The LTA ring buffer is 1500 floats times 4 bytes, which is 6 KB. ESP32 has 520 KB SRAM. This is fine.

### Key implementation rule

Do not recompute the sum from scratch each sample. Maintain a running sum for each buffer. Subtract the outgoing element and add the incoming element. Cost is O(1) per sample.

### New globals

Add these alongside the existing `vib_buf` declarations near the top of `node.ino`:

```c
// STA/LTA event detector
#define STA_SAMPLES     25        // 0.5 s at 50 Hz
#define LTA_SAMPLES     1500      // 30 s at 50 Hz
#define STALTA_THRESH   4.0f      // trigger ratio

static float   sta_buf[STA_SAMPLES];
static float   lta_buf[LTA_SAMPLES];
static int     sta_i   = 0;
static int     lta_i   = 0;
static float   sta_sum = 0.0f;
static float   lta_sum = 0.0f;
static float   stalta_ratio = 0.0f;
```

### Event state machine globals

Also add these near the top:

```c
// Event state machine
#define EVT_TILT_DELTA_DEG  0.5f      // minimum tilt shift to classify as subsidence
#define EVT_CHECK_MS        30000UL   // ms to wait before classifying the event
#define EVT_RESET_MS        90000UL   // ms after which the machine resets

typedef enum { EVT_NONE = 0, EVT_PENDING = 1, EVT_SUBSIDENCE = 2, EVT_BLAST = 3 } evt_t;

static evt_t    evt_state           = EVT_NONE;
static uint32_t evt_time            = 0;
static float    evt_pitch_snapshot  = 0.0f;
static float    evt_roll_snapshot   = 0.0f;
static uint8_t  event_type          = 0;
static uint16_t seq_counter         = 0;
```

### Loop insertion point

Inside `loop()`, after computing `vib_rms` and before the existing `classify()` call, insert the STA/LTA update block.

The variable `mag` is already computed in the existing loop for vib_rms. Reuse it here. If your loop computes `mag` as `sqrtf(ax*ax + ay*ay + az*az)`, then `ac_abs` below is just `fabsf(mag - 1.0f)`.

```c
// ---- STA/LTA update ----
float ac_abs = fabsf(mag - 1.0f);

// STA running sum
sta_sum -= sta_buf[sta_i];
sta_buf[sta_i] = ac_abs;
sta_sum += ac_abs;
sta_i = (sta_i + 1) % STA_SAMPLES;

// LTA running sum
lta_sum -= lta_buf[lta_i];
lta_buf[lta_i] = ac_abs;
lta_sum += ac_abs;
lta_i = (lta_i + 1) % LTA_SAMPLES;

float sta_mean = sta_sum / (float)STA_SAMPLES;
float lta_mean = lta_sum / (float)LTA_SAMPLES;
stalta_ratio   = (lta_mean > 0.001f) ? (sta_mean / lta_mean) : 0.0f;
bool event_now = (stalta_ratio > STALTA_THRESH);

// ---- Event state machine ----
if (event_now && evt_state == EVT_NONE) {
    evt_state          = EVT_PENDING;
    evt_time           = now;
    evt_pitch_snapshot = pitch_deg;
    evt_roll_snapshot  = roll_deg;
    event_type         = 1;
}

if (evt_state == EVT_PENDING && (now - evt_time) >= EVT_CHECK_MS) {
    float dp = fabsf(pitch_deg - evt_pitch_snapshot);
    float dr = fabsf(roll_deg  - evt_roll_snapshot);
    if (dp > EVT_TILT_DELTA_DEG || dr > EVT_TILT_DELTA_DEG) {
        evt_state  = EVT_SUBSIDENCE;
        event_type = 2;
    } else {
        evt_state  = EVT_BLAST;
        event_type = 3;
    }
}

// Reset 90 s after classification so the machine can catch the next event
if ((evt_state == EVT_SUBSIDENCE || evt_state == EVT_BLAST)
    && (now - evt_time) >= EVT_RESET_MS) {
    evt_state  = EVT_NONE;
    event_type = 0;
}
```

The `now` variable is `millis()`. It is already computed earlier in the loop for the existing classify() call. Do not call `millis()` a second time. Reuse the same value.

### Packet struct update

Replace the existing `packet_t` definition with this:

```c
typedef struct __attribute__((packed)) {
    uint8_t  id;
    uint16_t seq;         // packet counter, wraps at 65535
    float    pitch;
    float    roll;
    float    vib;
    float    stalta;      // current STA/LTA ratio
    uint32_t t_ms;
    uint8_t  risk;
    uint8_t  event_type;  // 0 none, 1 pending, 2 subsidence, 3 blast
} packet_t;
```

Total size: 1 + 2 + 4 + 4 + 4 + 4 + 4 + 1 + 1 = 25 bytes. ESP-NOW max payload is 250 bytes. This is fine.

### Fill the new fields when building the packet

Add these lines where the packet is populated before `send_now()`:

```c
p.seq        = seq_counter++;
p.stalta     = stalta_ratio;
p.event_type = event_type;
```

### Update emit_json()

Add the new fields to the JSON output so S2 can read them on the dashboard:

```c
static void emit_json(const packet_t *p) {
    Serial.print("{\"id\":");
    Serial.print(p->id);
    Serial.print(",\"seq\":");
    Serial.print(p->seq);
    Serial.print(",\"pitch\":");
    Serial.print(p->pitch, 2);
    Serial.print(",\"roll\":");
    Serial.print(p->roll, 2);
    Serial.print(",\"vib\":");
    Serial.print(p->vib, 3);
    Serial.print(",\"stalta\":");
    Serial.print(p->stalta, 2);
    Serial.print(",\"t\":");
    Serial.print(p->t_ms);
    Serial.print(",\"risk\":");
    Serial.print(p->risk);
    Serial.print(",\"evt\":");
    Serial.print(p->event_type);
    Serial.println("}");
}
```

### Tuning procedure

Flash the firmware. Open Serial Monitor at 115200. Let the plank sit completely flat and still for 45 seconds so the LTA buffer fills with the ambient noise floor. You should see `stalta` values between 0.8 and 1.5 during this period. If it is already above 3.0 while the plank is untouched, the environment is too noisy. Raise `STALTA_THRESH` to 6.0 and retest.

Then give the plank a single sharp tap with one finger. `stalta` should spike above 4.0 and decay back within 1 to 2 seconds. If it spikes but never decays, the LTA is accumulating the event energy and needs a longer window. Increase `LTA_SAMPLES` to 2500 (50 seconds). If it does not spike at all, lower `STALTA_THRESH` to 2.5 temporarily to confirm the signal chain is working, then raise it back.

Do not tune `STALTA_THRESH` below 2.5 or it will trigger on footsteps and table vibration.

### What STA/LTA does NOT do

It tells you an impulsive event happened. It does not classify the frequency content of the event. Frequency-domain classification (FFT) is defined in the Finale Path section at the end of this document.

---

## Component 2: Tilt Rate Computation

**Owner: S2 (dashboard)**

No firmware change needed for this component beyond what is already logged. Pitch and roll are already in every packet. The dashboard computes the rate from the history.

### What it does

Absolute tilt angle tells you the current state of the ground. Tilt rate tells you whether it is actively moving and how fast. A 3 degree tilt that developed over 5 minutes is a different situation from the same tilt that drifted over 3 days. Tilt rate is the signal that confirms active movement. It is also a better input to the Isolation Forest than raw angle.

### Implementation in app.py

Add a deque per node at module level:

```python
from collections import deque

RATE_WINDOW_S = 30
tilt_history = {}   # node_id -> deque of (t_ms, pitch, roll)

def update_tilt_history(node_id, t_ms, pitch, roll):
    if node_id not in tilt_history:
        tilt_history[node_id] = deque()
    buf = tilt_history[node_id]
    buf.append((t_ms, pitch, roll))
    while buf and (t_ms - buf[0][0]) > RATE_WINDOW_S * 1000:
        buf.popleft()

def compute_tilt_rate(node_id):
    """Returns (pitch_rate, roll_rate) in deg/s. Returns (0.0, 0.0) if insufficient history."""
    if node_id not in tilt_history:
        return 0.0, 0.0
    buf = tilt_history[node_id]
    if len(buf) < 2:
        return 0.0, 0.0
    t0, p0, r0 = buf[0]
    t1, p1, r1 = buf[-1]
    dt_s = (t1 - t0) / 1000.0
    if dt_s < 1.0:
        return 0.0, 0.0
    return (p1 - p0) / dt_s, (r1 - r0) / dt_s
```

Call `update_tilt_history()` each time a packet arrives from serial. Call `compute_tilt_rate()` when building the dashboard state object for each node before rendering.

### Thresholds

These are provisional. Tune against the actual plank and how quickly it tilts in a realistic demo lift.

| Rate (deg/s) | Meaning | Dashboard action |
|---|---|---|
| 0.0 to 0.01 | Static or drift only | No action |
| 0.01 to 0.1 | Slow movement | Show rate number in cell |
| 0.1 to 0.5 | Active movement | Add MOVING label to cell |
| Above 0.5, sustained 10 s | Rapid movement | Escalate cell risk by one level |

The escalation for sustained high rate is a dashboard-side escalation only. Do not change the firmware risk field to implement this. The firmware risk field reflects tilt angle alone. The dashboard may show a higher effective risk due to rate.

### Display

In each grid cell, below the pitch/roll numbers, show one line:

```
RATE: +0.24 deg/s
```

A positive value means the node is tilting in the positive pitch direction. A negative value means it is recovering or moving the other way. This line helps judges see that the system is tracking movement velocity, not just position.

---

## Component 3: Event Display on the Dashboard

**Owner: S2 (dashboard)**

### What to show per node based on evt field

Read the `evt` field from the incoming JSON. Map it to a visual state on the relevant grid cell.

| evt value | Meaning | Display |
|---|---|---|
| 0 | No event | Normal cell, no banner |
| 1 | Event detected, classifying | Yellow banner: EVENT DETECTED - classifying (30 s) |
| 2 | Subsidence confirmed | Red banner: SUBSIDENCE EVENT - tilt shift confirmed |
| 3 | Blast or machinery | Grey banner: EVENT CLASSIFIED: BLAST / MACHINERY |

```javascript
function getEventBanner(evt) {
    if (evt === 1) return {text: 'EVENT DETECTED — classifying...', color: '#F5C400'};
    if (evt === 2) return {text: 'SUBSIDENCE EVENT — tilt shift confirmed', color: '#C0392B'};
    if (evt === 3) return {text: 'BLAST / MACHINERY — alert suppressed', color: '#555'};
    return null;
}
```

### Blast suppression on the dashboard side

When a node reports `evt: 3`, do not count its `vib` toward the risk level for the next 60 seconds. Only tilt matters during that window. This prevents blasting from pushing a node into Warning or Critical on the grid.

```python
blast_suppress = {}   # node_id -> suppress_until as time.time()

def effective_vib(node_id, vib, evt):
    if evt == 3:
        blast_suppress[node_id] = time.time() + 60
    suppressed = node_id in blast_suppress and time.time() < blast_suppress[node_id]
    return 0.0 if suppressed else vib
```

Apply this before passing `vib` to the risk recomputation that the dashboard uses for its cell colour. The firmware `risk` field is still used as the primary risk indicator. The dashboard's local computation using `effective_vib` is an override layer only for the suppression window.

---

## Finale Path: Frequency-Domain Classification

This is not needed for the 25 Aug internal. Define it for the finale and put one sentence about it on slide 3.

At the Pi gateway, collect raw accel data from each node. After an STA/LTA trigger, capture 5 seconds of raw accel samples and run a 256-point FFT on the magnitude signal.

Compute two features from the spectrum:
- Spectral centroid: weighted mean frequency across the spectrum
- Event duration: time above the STA/LTA threshold

Classification rules:

| Event type | Duration | Spectral centroid |
|---|---|---|
| Blast | Under 2 s | Above 20 Hz |
| Machinery | Above 10 s, periodic | 10 to 50 Hz |
| Subsidence onset | Above 3 s | Below 10 Hz, with tilt change |

For the finale this replaces the 30-second tilt persistence check with a real-time frequency classifier. The tilt persistence check defined in this document is the interim approach for the internal.

Use numpy or scipy.fft at the gateway. The Pi can handle this trivially. The ESP32 cannot, which is why FFT stays at the gateway.

---

## Testing Checklist

Complete this before the 25 Aug demo. Both S1 and S2 should run through it together.

**S1 firmware tests:**

- Confirm that with the plank flat and still for 45 seconds, `stalta` in JSON stays below 2.0
- Confirm that a sharp finger-tap on the plank produces `stalta` above 4.0 in the JSON within 1 second
- Confirm that `evt` goes to 1 immediately after the STA/LTA trigger
- Confirm that `evt` goes to 3 approximately 30 seconds after the tap if the plank was not tilted
- Confirm that `evt` goes to 2 approximately 30 seconds after the tap if the plank was held tilted by 2 or more degrees
- Confirm that `seq` increments by 1 with every packet from each node
- Confirm that both node 1 and node 2 emit all new JSON fields
- Confirm that the new packet struct compiles without errors and that ESP-NOW still delivers both IDs to Serial Monitor

**S2 dashboard tests:**

- Confirm that `stalta` appears in the CSV log for every packet row
- Confirm that tilt rate shows 0.0 while the plank is flat and a nonzero value when you slowly tilt it
- Confirm that the event banner appears on the correct cell when `evt` is 1, 2, or 3
- Confirm that the blast suppression clears the vib contribution for 60 seconds when `evt` 3 arrives
- Confirm that all new fields appear in `data/log.csv`
- Confirm the dashboard works completely offline with the laptop in airplane mode

---

## What to say to judges about the algorithm

Do not use "neural network" or "AI model" to describe this. Use these exact phrases:

On the overall approach: "We use an STA/LTA event detector, which is the standard seismological method for identifying impulsive events in a continuous signal. The ratio adapts to the ambient noise floor, so it works even when background vibration changes through the day."

On the blast vs subsidence question: "After an event fires, we wait 30 seconds and check whether the tilt baseline has shifted. Blasting creates a short vibration and leaves the ground where it was. Subsidence creates a vibration and the tilt stays elevated. That persistence check is the classification. In the finale we replace it with a frequency-domain classifier at the gateway."

On what the AI is: "Anomaly detection on a multivariate time series of tilt angle, tilt rate, and STA/LTA ratio. For the internal we use Isolation Forest on a 20-second flat baseline. The STA/LTA event state adds a discrete event stream on top of the continuous anomaly score."
