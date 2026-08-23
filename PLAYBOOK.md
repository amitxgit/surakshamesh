# SurakshaMesh — complete build and pitch playbook

**PS:** SIH26025 · Coal India Limited · Ministry of Coal · Hardware · Disaster Management
**Official title (verbatim on slide 1):** Development of an AI-enabled Low Cost Real Time Mine Subsidence Monitoring, Prediction and Early Warning System for Underground Coal Mines in India
**Product:** SurakshaMesh
**Idea one-liner (form):** SurakshaMesh — Low-cost wireless surface mesh for real-time mine subsidence early warning
**Internal:** 25 Aug 2026 · FoT DU · official 6-slide template only · save as PDF
**Team:** 6 people, same institution, ≥1 woman, unique name with no college/DU/FoT/Delhi in it
**Recommended team name:** DhartiNode (backup: PanelWatch → Keryx → Garuda)

This document is the single source of truth. Assign the six roles in §7 tonight. Do not rewrite CIL’s Background / Description / Expected Solution on the PPT — quote them, then show *your* mesh.

---

## 0. What we are actually shipping

Three layers. Only Layer A has to work on the 25th. Layer B is the story on the slides. Layer C is the 90-day finale path.

| Layer | When | What is real | What is simulated |
|---|---|---|---|
| **A — Internal slice** | 23–25 Aug | 2 ESP32 + MPU6050 on a plank, ESP-NOW, live tilt/vib, LED, laptop grid goes red when you lift a corner | Ground deformation (you tilt the plank by hand). No mine, no DGPS, no SMS modem. |
| **B — PPT / form** | Same week | Architecture, BOM, uniqueness vs DGPS, 90-day plan, research refs | Field radio drawn as LoRa; GIS as a coalfield grid; SMS as a toast that says “SMS sent” |
| **C — Finale** | ~90 days if nominated | 6–8 weatherproof solar LoRa nodes, crack/stretch, Pi gateway, OSM map, GSM SMS, Isolation Forest / small LSTM on logged tilts | Still no millimetre survey claim. DGPS stays the *calibration* layer. |

**The product sentence you will repeat:**

> SurakshaMesh is a dense, cheap, always-on **first-alert surface mesh** over an underground panel. Sparse survey-grade GPS remains calibration. The mesh is the early-warning fabric CIL asked for.

**Do not claim:** millimetre DGPS accuracy from MPU6050; real CIL borehole / Jharia data; a drone or InSAR; a transformer / ChatGPT; a digital twin; that this replaces CMPDI surveys.

CIL’s own uniqueness line (use it): *wireless surface mesh for real-time subsidence detection*.

---

## 1. How the system works

```
  surface over an underground panel
  ┌────────────┐   ┌────────────┐   ┌────────────┐
  │ Node 1     │   │ Node 2     │   │ Node 3     │   (finale: 6–8)
  │ ESP32      │   │ ESP32      │   │ ESP32      │
  │ MPU6050    │   │ MPU6050    │   │ MPU6050    │
  │ tilt + vib │   │ tilt + vib │   │ + flex     │
  │ RGB LED    │   │ RGB LED    │   │ RGB LED    │
  └─────┬──────┘   └─────┬──────┘   └─────┬──────┘
        │ ESP-NOW        │ ESP-NOW        │
        │ (internal)     │                │
        │ LoRa (field / PPT)              │
        └────────────┬────────────────────┘
                     ▼
              USB hub node  (or dedicated gateway)
              JSON lines @ 115200
                     ▼
              Laptop Flask dashboard  (offline)
              grid · Watch / Warning / Critical · beep · CSV log
              toast: “SMS would be sent”   (real GSM = Layer C)
```

### Sensing (what CIL named, mapped to chips)

| CIL asked | Internal (Layer A) | Finale (Layer C) |
|---|---|---|
| Tilt / inclination | MPU6050 pitch + roll | Same, better cal, optional inclinometer |
| Vibration | RMS of accel after removing 1 g, 50 Hz window | Same + peak-hold |
| Displacement / stretch | *Not on the plank* — talk it, don’t fake it | Draw-wire or ultrasonic between two posts |
| Crack initiation | Optional flex / break-wire on one node | Flex + painted break-wire |
| Optional GNSS | Off | NEO-6M / 8M for node identity, **not** millimetre coords |
| Mesh | ESP-NOW 2.4 GHz | LoRa 433/868 MHz, 1–3 km |
| AI | Tilt-rate + coincident vib; optional Isolation Forest on a 30-sample window | Isolation Forest + small LSTM on days of logs |
| GIS | 2–3 coloured cells | OSM tiles of a coalfield + risk overlay |
| SMS / email | On-screen toast | SIM800L / 4G hat from gateway |
| Offline-first | Local HTML, no cloud | SD + store-and-forward |

### Packet (one line per second, USB serial)

```json
{"id":1,"pitch":1.24,"roll":-0.41,"vib":0.031,"t":18440,"risk":0}
```

`risk`: `0` Watch-clear (green), `1` Watch (yellow), `2` Warning (red), `3` Critical (red + beep).

### Thresholds for the plank demo (tune in 5 minutes)

| State | Rule | LED |
|---|---|---|
| Green | \|pitch\| < 2° and \|roll\| < 2° and vib < 0.15 g RMS | Green |
| Watch (yellow) | \|pitch\| or \|roll\| ≥ 2° **or** vib ≥ 0.15 | Yellow |
| Warning (red) | \|pitch\| or \|roll\| ≥ 5° | Red |
| Critical | Warning **or** (Watch held ≥ 8 s **and** vib spike) | Red + dashboard beep |

Hysteresis: to go green again, stay under 1.5° for 2 s. Otherwise the cell flickers and the demo looks broken.

**“AI” line for faculty (honest):** anomaly detection on a deformation time-series. Internal = rules on tilt-rate and coincident vibration, optional Isolation Forest vs a 20 s flat-plank baseline. Finale = same plus a small LSTM. Not a language model.

---

## 2. Hardware — what to build this week

Full shopping list, pins, and prices: [BOM.md](BOM.md).

### Minimum kit that can win an internal (buy today)

2× ESP32 DevKit + 2× MPU6050 is the floor. 3× ESP32 is better (two sensors + one USB hub). If you only have two boards, the USB board is **both** a sensor and the hub (`IS_USB_HUB 1` in `node.ino`).

Each node (15 minutes of wiring):

```
MPU6050  VCC → ESP32 3V3     (not 5 V)
         GND → GND
         SDA → GPIO 21
         SCL → GPIO 22
         ADO → GND           (address 0x68)

RGB LED  R → GPIO 25 via 220 Ω
         G → GPIO 26 via 220 Ω
         B → GPIO 27 via 220 Ω
         common cathode → GND

USB      laptop (hub node) or power bank (remote node)
```

Mount both IMUs **the same way** on the plank (chip writing readable, X arrow along the plank). If one is rotated, one cell goes red when you tilt “the wrong way” and you will debug it in front of judges.

### Demo rig (H2 owns this)

- Plank or soil tray ~60 × 20 cm. Two nodes, 30–40 cm apart, taped down.
- Label the cells **N1** and **N2** on the wood so they match the dashboard.
- Laptop facing judges. Plank at the edge of the table so a hand can lift one corner.
- Spare USB cable, spare jumper pack, tape, one extra MPU6050 in a bag.
- 45 s phone-video backup already on the laptop, full-screen ready.

### Optional this week (only if already in the lab)

- 2× SX1278 LoRa — flash a *second* firmware later; put the module on the table so the LoRa slide is not a drawing.
- 1× flex sensor on Node 2 — crack channel. Nice, not required.
- Onboard LED only if you have no RGB. The box must still show risk if the laptop dies.

---

## 3. Software — how to implement

Two programs. Firmware on the ESP32s, dashboard on the laptop. No cloud.

### 3.1 Firmware (`firmware/node/node.ino`)

One sketch, every board.

1. Install Arduino IDE → Boards Manager → **esp32 by Espressif** → board **ESP32 Dev Module**, 115200.
2. No extra libraries. `Wire.h` + `esp_now.h` only.
3. Edit three lines at the top, then flash:

```c
#define NODE_ID      1    // 1 on first board, 2 on second
#define IS_USB_HUB   1    // 1 on the board plugged into the laptop; 0 on the other
#define FLIP_PITCH   0    // 1 if this IMU is mounted backwards
```

4. Open Serial Monitor on the hub at 115200. You should see JSON from id 1 and id 2, once a second.
5. Tilt the plank. `pitch` or `roll` climbs; `risk` goes 1 then 2; LED colour follows.

**What the firmware does, in order, every 20 ms (50 Hz):**

1. Read raw accel/gyro.
2. Complementary filter → pitch, roll (degrees).
3. Push accel magnitude (minus 1 g) into a 50-sample ring → vibration RMS.
4. Apply thresholds → `risk`.
5. Drive RGB.
6. Every 1 s: ESP-NOW broadcast the packet. If `IS_USB_HUB`, also `Serial.println` JSON for itself and for every packet it hears.

**If ESP-NOW fails** (only one JSON id on serial): the USB board still demos alone. One live node beats a slide. Fix channel/MAC later; do not stall the PPT for mesh.

**Day-1 debug (S1, before mesh):** `#define IS_USB_HUB 1`, watch pitch on Serial Plotter (`pitch` only). Lift an end. If the number does not move, the IMU is not talking (ADO, 3V3, SDA/SCL swap).

### 3.2 Dashboard (`dashboard/app.py`)

```
cd dashboard
pip install flask pyserial
python app.py COM6          # Windows — Device Manager → USB-SERIAL
python app.py /dev/ttyUSB0  # Linux
```

Browser: `http://127.0.0.1:5000`

- Two (or three) cells. Colour = risk. Numbers = live pitch / roll / vib.
- Beep on Warning/Critical (browser AudioContext — click the page once to allow sound).
- Toast **SMS would be sent to mine office** on Critical. Fake on purpose. Say so if asked.
- `data/log.csv` appends every packet. That file is the “historical data” for the AI slide.
- Optional Isolation Forest: `pip install scikit-learn`, press **Learn baseline** while the plank is flat for 20 s. Then tilt. Score > threshold → Watch. If sklearn is missing, rules still run.

Offline-first is literal: no internet, no Firebase, no Maps API key.

### 3.3 Software folder layout

```
firmware/node/node.ino      S1
dashboard/app.py            S2
dashboard/templates/index.html
dashboard/data/log.csv      created at runtime
```

Do not split firmware across two people. One person flashes both boards so IDs and hub flag cannot drift.

---

## 4. Creative — how we show it

### 4.1 Room choreography (60 seconds, C2 directs)

| t | Who | What judges see |
|---|---|---|
| 0:00 | Speaker | “SIH26025, Coal India. Surveys find cracked houses. We find tilt while the panel is still moving.” |
| 0:10 | H1 | Two boxes on the plank, both LEDs green, map green. |
| 0:15 | Speaker | “Each node: ESP32, IMU, mesh. This is a surface grid over an underground gallery.” |
| 0:25 | H1 | Slowly lift the **N2** corner ~5–8 cm. Do not yank. |
| 0:35 | Everyone | N2 LED yellow → red. Cell N2 goes red. Beep. Toast. N1 stays green. |
| 0:45 | Speaker | “Watch / Warning / Critical. First-alert mesh. DGPS stays calibration. Not a millimetre survey.” |
| 0:55 | H1 | Lower the plank. Cell returns green. Stop talking. |

Backup if USB dies: on-node RGB still works. Point at the LED. Then play the 45 s video.

### 4.2 Pitch around the demo (5 minutes + Q&A)

Do not read slides. Demo is the middle of the talk, not an appendix.

1. **Title (15 s)** — PS ID, SurakshaMesh, Hardware, Disaster Management.
2. **Problem (30 s)** — CIL still uses periodic surveys and post-facto damage. That is too late.
3. **Idea (30 s)** — indigenous surface mesh, ESP32-class, tilt + vib, Watch/Warning/Critical, offline-first.
4. **Live slice (60 s)** — table above.
5. **Technical (60 s)** — point at slide 3 diagram: node → ESP-NOW now / LoRa field → gateway → GIS → alert. AI = anomaly on tilt time-series.
6. **Feasibility (45 s)** — BOM ~₹1.5k/node internal, ~₹6–8k field. 90-day path. Risks: false alarm, power, dust — dual-trigger, solar sleep, IP65.
7. **Impact + close (30 s)** — houses, roads, farm land over Bord-and-pillar panels. Made-in-India, student-prototype-friendly, as CIL wrote.

### 4.3 PPT (C1) — official template only

File: `ppt/SIH-official-template.pptx`. **Do not add a 7th content slide. Do not change the pointer headings.** Fill, export **PDF**.

Paste-ready copy is §8.

Visual language (keep it boring and mine-safety, not startup):

- Coal charcoal `#1A1A1A`, safety yellow `#F5C400`, alert red `#C0392B`, map green `#2E7D32`.
- One diagram on slide 3 (the architecture ASCII, redrawn). One 2×3 BOM/cost table on slide 4. No screenshots of ChatGPT. One photo of the plank on slide 2 if it fits.
- Product name **SurakshaMesh**. Team name in the footer as the template already has.

### 4.4 Form blurb (already locked — C2 pastes this)

> SurakshaMesh — Low-cost wireless surface mesh for real-time mine subsidence early warning
>
> An indigenous, low-cost surface sensor mesh deployed above underground coal panels. Each node is an ESP32-class station with tilt/IMU, vibration, and optional crack/stretch sensing. Nodes form a local wireless mesh (ESP-NOW for the prototype; LoRa for the field version), report continuously to a gateway, and a laptop/phone dashboard shows a live deformation map, risk colour, and alerts.
>
> When the ground starts to sag, stations see tilt trend, vibration spikes, and (if fitted) crack/opening before a walk-around survey finds cracked houses. The control room gets Watch / Warning / Critical, with offline logging if the wide-area network is down.
>
> CIL still depends on periodic surveys and post-facto damage assessment. That is too late. SurakshaMesh is dense, cheap, and always on: a first-alert layer, not a replacement for DGPS. Sparse survey-grade GPS remains the calibration layer; the mesh is the early-warning fabric Coal India asked for (Arduino/ESP32/Pi, tilt, vibration, crack, mesh, AI, GIS, SMS, offline-first).

### 4.5 Backup video (C2, night of 24 Aug)

Phone, landscape, 45 s, no music, no zoom-and-enhance.

1. Wide shot: laptop + plank.
2. Close: both LEDs green.
3. Hand lifts corner; LED + map go red.
4. Cut to Serial Monitor JSON scrolling.
5. Last frame: freeze on red cell, voice-over one sentence.

File name: `SurakshaMesh-demo.mp4` on the presentation laptop **and** a USB stick.

---

## 5. What is required (checklist)

### Must have by 25 Aug morning

- [ ] 2 working nodes (IMU + LED). One USB hub printing JSON.
- [ ] Dashboard colours + beep on a laptop that is not on exam login.
- [ ] Plank labelled N1 / N2.
- [ ] Official 6-slide PDF (template, not a custom deck).
- [ ] 45 s backup video.
- [ ] Team of 6 registered; ≥1 woman; name without college.
- [ ] Idea description pasted (block above).
- [ ] Everyone can run the plank (not just S1).

### Should have

- [ ] ESP-NOW from node 2 actually arriving (two IDs on serial).
- [ ] Isolation Forest baseline button working.
- [ ] One SX1278 on the table (even if firmware still ESP-NOW).
- [ ] Printed BOM page as a leave-behind.

### Must not have

- [ ] Last year’s IDs (SIH25020, SIH25036) anywhere.
- [ ] Drone, millimetre, Jharia dataset, “digital twin”, transformer.
- [ ] Phone IMU as the product.
- [ ] Cloud dashboard as a single point of failure.
- [ ] A 12-slide unofficial PPT.

### Faculty / FoT

- Form: events.du.ac.in — Hackathon Internal SIH 2026.
- Contact if the PG dropdown rejects: **Dr. Juhi Jain — sih@fot.du.ac.in**.
- Course = actual master’s, Year = 1 or 2, Branch = ECE or Other.
- Members cannot change after the form. One student, one team.

---

## 6. Clock — 23 / 24 / 25 Aug

Today is **23 Aug**. Internal is **25 Aug**. This is a 48-hour build, not a product.

### Sunday 23 Aug (today)

| Who | Done when |
|---|---|
| H1 + H2 | Parts in hand. Node 1 wired. IMU on Serial Plotter moves when the board tilts. |
| S1 | `node.ino` flashing. `NODE_ID 1`, `IS_USB_HUB 1`. JSON on COM port. |
| S2 | Flask opens, fake JSON (typed) colours a cell. Then switch to real serial. |
| C1 | Template copied, slide 1 filled, slide 3 diagram sketched on paper. |
| C2 | Team name locked. Form draft. Demo table booked in their head. |
| All | 20:00 WhatsApp: photo of Node 1 LED + a screenshot of JSON. |

### Monday 24 Aug

| When | Who | Done when |
|---|---|---|
| Morning | H1, S1 | Node 2 alive. Both IDs on serial. LEDs agree with risk. |
| Morning | H2 | Plank, labels, tape, spare cable. |
| Afternoon | S2, S1 | Live dashboard. Beep. CSV logging. |
| Afternoon | C1 | Slides 2–6 filled from §8. One plank photo on slide 2. |
| Evening | C2 + all | Three full 60 s demos. Record the best as backup video. |
| 22:00 | C1 | PDF freeze. No more slide edits unless a fact is wrong. |

### Tuesday 25 Aug

| When | Who |
|---|---|
| −90 min | H1/S1 dry-run on the *presentation* laptop (drivers). |
| −30 min | Video file tested full-screen. PDF on USB + laptop. |
| Pitch | Speaker (C2 or whoever is calmer) + H1 on the plank + S2 on the browser. |
| After | Do not pack the kit until the last judge has left. |

**Kill order if time dies**

1. Drop Isolation Forest.
2. Drop LoRa on the table.
3. Drop node 2 — one USB node + video of a second node from last night.
4. Never drop: one live IMU, LED, PDF, uniqueness sentence.

---

## 7. Six people — hardware / software / creative

Same six own both the kit and the PDF. Do not split into “PPT team” and “build team”. Everyone must be able to lift the plank and explain a cell.

| ID | Track | Role | Owns | Does not own |
|---|---|---|---|---|
| **H1** | Hardware | Node electronics | Wire both IMUs, RGB, USB power, calibration, spare MPU | Firmware edits, slides |
| **H2** | Hardware | Rig + BOM | Plank, mounts, labels, optional flex/LoRa on the table, shopping, spare bag | Dashboard |
| **S1** | Software | Firmware / mesh | `node.ino`, IDs, hub flag, ESP-NOW, thresholds, serial JSON | PPT wording |
| **S2** | Software | Dashboard + AI + alerts | Flask, grid, beep, CSV, Isolation Forest button, SMS toast | Soldering |
| **C1** | Creative | PPT + uniqueness + refs | Official 6-slide PDF, architecture drawing, research slide, cost table | Flashing boards |
| **C2** | Creative | Demo + form + Q&A | 60 s script, 45 s video, idea-description paste, Q&A cards, timekeeper, speaker | Rewriting CIL’s PS |

If a person is missing, merge **H2 into H1** and **C2 into C1**. Do not merge S1 into anyone — firmware is the critical path.

### H1 — Node electronics (hardware)

**Build**
- Solder-less: MPU6050 on 3V3 / GND / 21 / 22. Confirm 0x68 with a 2-line I2C scanner if pitch stays zero.
- RGB on 25/26/27. Common cathode. If you only have the onboard LED, say so; S1 already falls back.
- Same IMU orientation on both nodes. Mark “FRONT” with tape.
- Power: hub node from laptop USB. Remote node from a power bank that can keep 5 V under Wi-Fi load (cheap banks brown out — test).

**Show**
- You operate the plank. Slow lift. You do not talk over the speaker.

**Required from you**
- Two live IMUs by Monday noon.
- A known-good spare MPU6050 in the bag.

### H2 — Rig, sensors, BOM (hardware)

**Build**
- Plank/tray, anti-slip, labels N1/N2 matching dashboard.
- Cable strain relief so a lift does not rip SDA.
- Optional flex on Node 2 analog GPIO 34 (S1 will pick it up if `USE_FLEX 1`).
- Optional SX1278 sitting in an anti-static bag *on the table* with a printed “field radio: LoRa 433 MHz” card.
- Print [BOM.md](BOM.md) internal table as a one-pager.

**Show**
- You reset the plank between Q&A if asked “do it again”.

**Required from you**
- Demo table kit packed Monday night: plank, 3 cables, tape, power bank, spare jumpers, video on a stick.

### S1 — Firmware / mesh (software)

**Build**
- Flash both boards. Never leave `NODE_ID` at 1 on both.
- Tune `WATCH_DEG`, `WARN_DEG`, `VIB_G` on *this* plank so a 5 cm lift is yellow and an 8–10 cm lift is red.
- Confirm two JSON ids. If not: same Wi-Fi channel, both in range, hub flag only on the USB board.
- Keep a USB-only fallback: even with mesh dead, hub node still prints itself.

**Show**
- Serial Monitor is a backup visual if the browser dies. You are ready to Alt-Tab.

**Required from you**
- `node.ino` compiles on the presentation laptop (install ESP32 board support there too).

### S2 — Dashboard + AI + alerts (software)

**Build**
- `app.py` reads the hub COM port. Large cells. Colour-blind safe: green / yellow / red **and** the word Watch/Warning/Critical.
- Beep only on rising edge to Warning/Critical, not every packet.
- `Learn baseline` button: 20 s flat → Isolation Forest or a mean/std gate if sklearn is missing.
- Log CSV. One chart is enough (pitch vs time for N2) if you have 30 extra minutes.

**Show**
- Browser already open, zoom 125%, no bookmarks bar. You click once before the talk so audio is allowed.

**Required from you**
- Works with the laptop **offline** (airplane mode test Monday night).

### C1 — PPT (creative)

**Build**
- Fill the official template. Pointer headings stay. Points, not paragraphs.
- Slide 3 is a diagram, not a tech-stack dump. Slide 6 is real papers / CIL / DGMS, not Wikipedia.
- Photo of the actual plank on slide 2 (uniqueness is “we built the mesh CIL named”).
- Export PDF. Filename: `SIH26025_SurakshaMesh_<TeamName>.pdf`.

**Show**
- You advance slides. Pause on slide 3 during the demo (architecture still visible) *or* go black and let the laptop dashboard be the visual — agree with C2 Monday night.

**Required from you**
- PDF freeze 24 Aug 22:00. Printed 6-slide handout optional.

### C2 — Demo director (creative)

**Build**
- Memorise the 60 s table. Cut anyone who overtalks.
- Record video. Form paste. Q&A cards from §9 in everyone’s pocket.
- Confirm team name uniqueness (no Bharat/Hack/Coders/Innovators, no 2024–25 clones, no DU).

**Show**
- You are the voice unless the team has a stronger speaker. Introduce H1 by pointing, not by naming six people.

**Required from you**
- Three timed rehearsals. A written 5-minute rundown. Video file in two places.

### Cross-training (30 minutes Monday)

Each person, once: flash is already done → lift plank → point at the red cell → say the product sentence. If they cannot, they cannot face a judge who asks them.

---

## 8. Six-slide copy (paste into the official template)

Change only the bullets, not the headings the template already prints.

### Slide 1 — TITLE PAGE

- **SMART INDIA HACKATHON 2026**
- **Problem Statement ID:** SIH26025
- **Problem Statement Title:** Development of an AI-enabled Low Cost Real Time Mine Subsidence Monitoring, Prediction and Early Warning System for Underground Coal Mines in India
- **Theme:** Disaster Management
- **PS Category:** Hardware
- **Team ID:** *(portal / internal id)*
- **Team Name:** DhartiNode *(or locked name)*
- Idea / product line under the title if there is a free text box: **SurakshaMesh — surface mesh, first alert**

### Slide 2 — IDEA TITLE

**Title text:** SurakshaMesh — low-cost wireless surface mesh for real-time mine subsidence early warning

**Proposed Solution (Describe your Idea/Solution/Prototype)**
- Distributed ESP32-class nodes on the *surface* above an underground panel (the localisation CIL asked for).
- Each node: tilt/IMU + vibration RMS + optional crack/stretch. RGB = Watch / Warning / Critical even if the radio dies.
- Mesh: ESP-NOW on the prototype, LoRa on the field node. Gateway → offline laptop/phone map → SMS/email in the field build.
- Live prototype: 2 nodes on a plank. Lift a corner → that cell goes red. Pipeline is real; deformation is a controlled tilt.

**How it addresses the problem**
- CIL today: periodic surveys + post-facto damage. Warning arrives after houses crack.
- SurakshaMesh: always-on micro-movement (tilt trend, vib spike, crack/opening) as a **first-alert** layer.
- Sparse DGPS / total station remains the **calibration** survey. The mesh does not replace it.

**Innovation and uniqueness**
- Uniqueness is CIL’s own hook: *wireless surface mesh for real-time subsidence* — not a drone, not InSAR, not a single GPS peg.
- Dense and cheap (₹1.5k internal node / ₹6–8k field node) vs sparse millimetre campaigns.
- Offline-first: SD/CSV log if the wide-area link is down. Student-prototype-friendly hardware named in the PS (Arduino/ESP32/Pi).

### Slide 3 — TECHNICAL APPROACH

**Technologies**
- Hardware: ESP32, MPU6050/9250, optional flex / SX1278 LoRa, RGB, USB or 18650.
- Firmware: C/Arduino, complementary filter @ 50 Hz, ESP-NOW (internal) / LoRa (field), JSON `{id,pitch,roll,vib,t,risk}`.
- Dashboard: local Flask + HTML canvas, no cloud. GIS grid now; OSM coalfield tiles in finale.
- AI/ML: tilt-rate + coincident vibration; Isolation Forest on a baseline window; finale small LSTM on logged series.
- Alerts: on-node LED, dashboard colour, beep; field SMS/email from the gateway.

**Methodology / process (draw this, don’t paragraph it)**

```
IMU 50 Hz → pitch/roll + vib RMS → on-node risk
        ↓ ESP-NOW / LoRa
   Gateway JSON → anomaly check → GIS cell colour
        ↓
   Watch / Warning / Critical → LED + log + (field) SMS
```

Working prototype in the room: two-node plank, USB JSON, live grid.

### Slide 4 — FEASIBILITY AND VIABILITY

**Feasibility**
- PS names Arduino/ESP32/Pi, LoRa/Zigbee/Wi-Fi, tilt/vib/crack, GIS, SMS, offline — this kit is that list, cut to a 3-day slice.
- Internal BOM ~₹2–3k for two nodes (USB power). Field node ~₹6–8k including box, solar, LoRa.
- 90 days to 6–8 weatherproof nodes + Pi gateway + GSM is a lab schedule, not a factory.

**Challenges and risks**
- MPU6050 is not a millimetre inclinometer → drift, temperature.
- False alarms from trucks / blasting vibration.
- Dust, rain, 45 °C, theft; 2.4 GHz ESP-NOW will not cross a dump.
- No real CIL panel this week.

**Strategies**
- Claim **early trend + density**, not survey grade. DGPS as monthly calibration of the same pegs.
- Dual trigger: tilt trend **and** vibration / crack. Hysteresis. Isolation Forest vs a quiet baseline.
- Field radio is LoRa, not ESP-NOW. IP65 + solar sleep (sample 15 min, radio burst).
- Deformation in internals is **induced and labelled**. Field trial only with CIL/CMPDI after nomination.

### Slide 5 — IMPACT AND BENEFITS

**Target audience**
- CIL colliery manager / safety officer, CMPDI, DGMS, state disaster cell, people in houses and fields over Bord-and-pillar panels (Jharia, Raniganj, Korba-type leases).

**Benefits**
- Social: hours-to-days of warning before a walk-around finds a cracked wall; fewer surprise sinkholes at the surface.
- Economic: one failed house or blocked haul road dwarfs a ₹6–8k node; fewer emergency stoppages from undetected sag.
- Environmental: earlier isolation of cracked paddy / forest patches; less unplanned pumping and fire risk in subsided ground.
- Strategic: Made-in-India, scalable panel-by-panel, student-prototype-friendly — matches the PS close.

Do **not** print a fake “20% fewer accidents” number. You do not have that data.

### Slide 6 — RESEARCH AND REFERENCES

- Coal India Limited / SIH26025 problem statement (use official Background + Expected Solution; uniqueness line: wireless surface mesh).
- DGMS / Ministry of Coal guidance on subsidence and surface protection above underground workings.
- CMPDI practice: periodic levelling, pillar stability, subsidence prediction curves (this mesh is *monitoring*, not a replacement for those curves).
- IMU tilt + vibration for slope / landslide early warning (IEEE / IGS papers on MEMS inclinometers + LoRa). Complementary filter: Madgwick / Mahony attitude estimation on MPU6050-class IMUs.
- LoRa/LoRaWAN mine-IoT range studies vs 2.4 GHz mesh in dusty, obstructed ground.
- Isolation Forest (Liu et al.) for unsupervised anomaly windows; keep LSTM/temporal models as finale, not the internal claim.
- Survey-grade contrast: DGPS / total station / InSAR — accurate, sparse, not always-on. Positioned as **calibration**, which is the honest architecture.

---

## 9. Q&A cards (C2 prints these)

**How accurate is MPU6050?**
About 0.1–0.5° after a flat baseline, not millimetres of heave. We detect *onset of tilt trend* across a dense grid. DGPS remains the millimetre layer.

**Why not only DGPS / InSAR?**
Cost, revisit, and no LED on the ground. CIL asked for a *localized wireless surface mesh*. We are that fabric. Satellites and pegs calibrate us.

**What is the AI, really?**
Anomaly detection on tilt/vib time-series. Rules + Isolation Forest this week. LSTM on logs if we go to the finale. Not a chatbot.

**Power?**
USB / power bank today. Field: 18650 + 1–2 W solar, 15-minute sample, LoRa burst. Sleep current is the design problem of month two.

**Range?**
ESP-NOW: tens of metres, internal only. Field: LoRa 1–3 km to a pit-office gateway. We will not pretend 2.4 GHz crosses a spoil heap.

**False alarms from blasting?**
Vibration-only never raises Critical. Critical needs tilt held with vibration, or a large tilt. Blast windows can be masked from the shift plan later.

**Have you been to a CIL mine?**
No. The *pipeline* is real; the *deformation* is a labelled plank tilt. Field trial is the post-nomination ask.

**Cost to instrument a panel?**
~₹6–8k per field node × 6–8 nodes + one gateway ≈ ₹50–70k per panel-scale demo. Surveys for the same panel cost more than that in crew time.

**Why ESP32 not a “real” industrial PLC?**
Because the PS said Arduino/ESP32/Pi and student-prototype-friendly. Industrial enclosure and LoRa come in Layer C without changing the architecture.

**Why no drone?**
Wrong product. Photogrammetry is another PS. Uniqueness here is a standing mesh that runs at 2 a.m. in the rain.

---

## 10. Finale path (90 days) — one table for slide 4

| Month | Hardware | Software | Proof |
|---|---|---|---|
| 1 | 6 nodes, IP65, LoRa, 18650, flex on 2 nodes | Sleep + LoRa packets, Pi gateway, OSM grid | 24 h garden soak, CSV |
| 2 | 1–2 W solar, GSM SMS, draw-wire stretch between two posts | Isolation Forest on real logs, SMS on Critical | Night run without Wi-Fi |
| 3 | 8th node, better cal, optional NEO-6M as *name* not as millimetre | Small LSTM on accumulated tilts; offline sync | Mock panel + video + BOM v2 |

Still no millimetre claim. Still no fake CIL dataset.

---

## 11. Claim vs lie (pin this in the group chat)

| Say | Do not say |
|---|---|
| First-alert dense mesh | Replaces DGPS / CMPDI |
| Pipeline real, deformation simulated | Trained on Jharia borehole data |
| ESP-NOW now, LoRa in field | “Our LoRa network is live across 2 km” (unless it is) |
| Anomaly detection / Isolation Forest | Transformer, generative AI, “predicts collapse to the hour” |
| ~₹6–8k field node | ₹500 industrial node |
| Watch / Warning / Critical | 99% precision |
| Optional GNSS for identity | Millimetre real-time positioning from MPU6050 |

---

## 12. Where the files live

```
Documents/SurakshaMesh/
  PLAYBOOK.md                 ← you are here
  BOM.md                      ← buy list + pins + finale
  README.md
  firmware/node/node.ino      ← flash this
  dashboard/app.py
  dashboard/templates/index.html
  ppt/SIH-official-template.pptx
```

Start order: **H1 wires Node 1 → S1 flashes → JSON on serial → S2 colours a cell → H2 builds the plank → C1 fills slides → C2 times the minute.**
