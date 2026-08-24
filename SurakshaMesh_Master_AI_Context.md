# 🛡️ SURAKSHAMESH — MASTER AI PRESENTATION CONTEXT & SYSTEM DOSSIER
> **Target Use:** Feed this entire document directly into AI Slide / Presentation tools (e.g. Gamma.app, Beautiful.ai, ChatGPT / Claude, Canva AI) to generate a high-impact, diagram-rich, chart-dense 6-slide Smart India Hackathon (SIH) presentation deck.

---

## 📌 1. PROJECT METADATA & REPOSITORY
* **Project Name:** SurakshaMesh (Team DhartiNode)
* **Tagline:** *AI-Enabled Low-Cost Wireless Surface Mesh for Real-Time Mine Subsidence Monitoring, Prediction & Early Warning*
* **Hackathon:** Smart India Hackathon (SIH) 2026
* **Problem Statement ID:** `SIH26025`
* **Problem Statement Title:** *Development of an AI-enabled Low Cost Real Time Mine Subsidence Monitoring, Prediction and Early Warning System for Underground Coal Mines in India*
* **Organization / Ministry:** Coal India Limited (CIL) / Ministry of Coal
* **Theme & Category:** Disaster Management | Hardware Category
* **Official GitHub Repository:** [https://github.com/amitxgit/surakshamesh](https://github.com/amitxgit/surakshamesh)
* **Status:** Verified Live 3-Node Physical Hardware Working Prototype + ESP-NOW Mesh + AI Engine + Next.js Digital Twin Dashboard.

---

## 🌍 2. REAL-WORLD PROBLEM & INDUSTRY CONTEXT
* **The Underground Coal Mining Crisis:**
  * When underground coal seams are extracted using **Bord-and-Pillar** or **Longwall caving** methods (common in Jharia, Raniganj, Korba, Singrauli), the overlying rock layers (strata) lose support, causing gradual or catastrophic **surface subsidence**.
  * **Consequences:** Cracking of surface residential houses, collapse of haul roads, derailment of railway lines, disruption of paddy fields, ingress of monsoon water into underground workings, and opening of oxygen paths triggering spontaneous coal seam fires.
* **The Fatal Flaw of Current CIL Practices:**
  * Mines currently rely on **manual, periodic DGPS (Differential GPS) & Total Station surveying campaigns** (conducted once every 15 to 30 days).
  * **The Problem:** Subsidence is dynamic. By the time a manual surveying crew walks the lease, cracks and sinkholes have already formed. Warning arrives **post-facto**.
  * **InSAR Satellite Imagery:** High revisit time (6–12 days), zero penetration through dense monsoon cloud cover, zero real-time alerting.
* **The SurakshaMesh Paradigm:**
  * SurakshaMesh is a dense, always-on **24/7 first-alert surface mesh** that continuously monitors micro-tilt rates and vibration spikes.
  * **Sparse DGPS remains periodic calibration survey; SurakshaMesh acts as the continuous real-time early warning trigger.**

---

## 📐 3. HARDWARE & PERIPHERAL ARCHITECTURE

```
                                  [ SURAKSHAMESH NODE ENCLOSURE ]
┌─────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                                                                                 │
│   ┌───────────────────────────┐         ┌───────────────────────────────────────────────────┐   │
│   │   FB21700 Li-Ion Cell     │         │              ESP32 Dual-Core (240MHz)             │   │
│   │     4000mAh, 3.7V         │         │               Xtensa LX6 Microcontroller          │   │
│   └─────────────┬─────────────┘         │                                                   │   │
│                 │ (B+ / B-)             │  [Core 0] ESP-NOW P2P Mesh Radio (2.4GHz)         │   │
│   ┌─────────────▼─────────────┐         │  [Core 1] 50Hz Sensor Fusion & FreeRTOS           │   │
│   │   TP4056 USB-C Charger    │         │                                                   │   │
│   │   & Protection Module     │         │   GPIO 21 (SDA) ──────► MPU6050 6-Axis IMU        │   │
│   └─────────────┬─────────────┘         │   GPIO 22 (SCL) ──────► 50Hz Accel/Gyro Data      │   │
│                 │ (OUT+ / OUT-)         │                                                   │   │
│   ┌─────────────▼─────────────┐         │   GPIO 14 (PWM) ──────► Active Buzzer (Alarm)     │   │
│   │    Power ON/OFF Switch    ├────────►│   GPIO 02 (LED) ──────► Status Indication LED     │   │
│   └───────────────────────────┘ (VIN)   └─────────────────────────┬─────────────────────────┘   │
│                                                                   │                             │
└───────────────────────────────────────────────────────────────────┼─────────────────────────────┘
                                                                    │ ESP-NOW (Sub-50ms)
                                                                    ▼
                                                    [ CENTRAL GATEWAY NODE-01 ]
                                                    (USB-Serial / Wi-Fi Hotspot)
                                                                    │
                                                                    ▼
                                                    [ NEXT.JS 16 DIGITAL TWIN ]
```

### Complete Hardware Specifications:
1. **Core Processor:** ESP32-WROOM-32D / ESP32S (Dual-Core 240 MHz, 520 KB SRAM, 4 MB Flash).
2. **Inertial Measurement Unit (IMU):** MPU6050 (6-DOF: 3-Axis Gyroscope $\pm 250^\circ/\text{s}$, 3-Axis Accelerometer $\pm 2g$) over $400\text{ kHz}$ Fast I2C.
3. **Power Subsystem:**
   * 1× FB21700 Industrial Lithium-Ion Cell ($4000\text{ mAh}$, $3.7\text{V}$, $14.8\text{ Wh}$).
   * 1× TP4056 USB-C Li-Ion Charging + DW01A Battery Protection Module.
   * Power Draw: $80\text{ mA}$ active $\rightarrow$ **~50 hours active runtime** / $10\text{ µA}$ deep-sleep $\rightarrow$ **~65–110 days runtime**.
4. **Local Alerting System:**
   * **Active Buzzer** on `GPIO 14`: Silent on Normal/Watch; $200\text{ms}$ pulse on Warning ($>5^\circ$ persistent); $80\text{ms}$ rapid klaxon on Critical ($>8^\circ$).
   * **Status LED** on `GPIO 02`: Real-time visual traffic light.
5. **Physical Enclosure:**
   * Compact square black IP65 weatherproof ABS box ($80\text{mm} \times 80\text{mm} \times 40\text{mm}$) with silicone rubber gasket, external power toggle switch, and top mounting bracket.

---

## ⚡ 4. FIRMWARE & SENSOR FUSION ALGORITHMS (`node.ino`)

### A. Complementary Attitude Filter (50 Hz / 20ms Loop)
Combines low-frequency gravity vector from Accelerometer with high-frequency dynamic response from Gyroscope:
$$\theta_{\text{acc}} = \text{atan2}(-a_x, \sqrt{a_y^2 + a_z^2}) \times 57.2958^\circ$$
$$\phi_{\text{acc}} = \text{atan2}(a_y, a_z) \times 57.2958^\circ$$
$$\text{Pitch}_t = \alpha \cdot (\text{Pitch}_{t-1} + g_y \cdot \Delta t) + (1 - \alpha) \cdot \theta_{\text{acc}}$$
$$\text{Roll}_t = \alpha \cdot (\text{Roll}_{t-1} + g_x \cdot \Delta t) + (1 - \alpha) \cdot \phi_{\text{acc}}$$
*(Where $\alpha = 0.98$, $\Delta t = 0.02\text{s}$, eliminates gyro drift and accelerometer motor vibration noise).*

### B. Vibration RMS Acceleration
$$\text{Vibration RMS} = \sqrt{\frac{1}{N} \sum_{i=1}^{N} (|\vec{a}_i| - 1.0g)^2} \quad (N = 50\text{ samples})$$

### C. Firmware Ground-Zero Auto-Tare on Boot
During the first $1.5\text{s}$ (50 samples), resting mounting angles are averaged to establish baseline offsets:
$$\Delta\text{Pitch} = \text{Pitch}_{\text{raw}} - \text{Pitch}_{\text{zero}}, \quad \Delta\text{Roll} = \text{Roll}_{\text{raw}} - \text{Roll}_{\text{zero}}$$
Ensures that regardless of mounting surface slope, a resting node begins at $\Delta 0.00^\circ$ (Level Ground / Silent Buzzer).

### D. ESP-NOW Binary Telemetry Packet (21 Bytes)
```cpp
typedef struct __attribute__((packed)) {
  uint8_t  id;      // 1: NODE-01, 2: NODE-02, 3: NODE-03
  float    pitch;   // Relative Delta Pitch (degrees, float32)
  float    roll;    // Relative Delta Roll (degrees, float32)
  float    vib;     // Rolling Vibration RMS (g, float32)
  uint32_t t_ms;    // Microcontroller Uptime (ms, uint32)
  uint8_t  risk;    // 0: Normal, 1: Watch, 2: Warning, 3: Critical
} packet_t;
```

---

## 🧠 5. AI DECISION ENGINE & GEOTECHNICAL LOGIC (`lib/telemetry.ts`)

```
                                  [ RAW SENSOR STREAM (50Hz) ]
                                                │
                                                ▼
                          [ AUTO-TARE RELATIVE DEVIATION (ΔP, ΔR) ]
                                                │
                                                ▼
                           [ WELFORD'S BASELINE NOISE LEARNING ]
                                (60-Sample Statistical Window)
                                                │
                                                ▼
                       ┌────────────────────────┴────────────────────────┐
                       ▼                                                 ▼
          [ ISOLATION FOREST ANOMALY ]                     [ SPATIAL COHERENCE CONSENSUS ]
          Evaluates multi-variate vector:                  Validates synchronous tilt across
          [ΔP, ΔR, dP/dt, dR/dt, Vib, STA/LTA]             ≥2 adjacent nodes in same direction
                       │                                                 │
                       └────────────────────────┬────────────────────────┘
                                                │
                                                ▼
                                [ SCHEDULED BLAST MODE FILTER ]
                                (Suppresses transient 60s spikes)
                                                │
                                                ▼
                                [ SEVERITY LEVEL CLASSIFICATION ]
                                🟢 Level 0: Normal    (<2.0°)
                                🟡 Level 1: Watch     (2.0° - 5.0°)
                                🟠 Level 2: Warning   (≥5.0° held ≥3s)
                                🔴 Level 3: Critical  (≥8.0° / Multi-Node)
                                                │
                                                ▼
                           [ ALARM BROADCAST & FORENSIC AUDIT ]
                           • Web Audio 1200Hz Klaxon Siren
                           • Hardware Node Active Buzzer Pulse
                           • Live 3D Hexagonal Canvas Animation
                           • Timestamped CSV Telemetry Logger
```

### Core Algorithms:
1. **Welford's Algorithm (Baseline Noise Learning):**
   * Computes streaming variance $\sigma^2$ and mean $\mu$ over a 60-sample window without storing arrays:
   $$M_k = M_{k-1} + \frac{x_k - M_{k-1}}{k}, \quad S_k = S_{k-1} + (x_k - M_{k-1})(x_k - M_k)$$
   $$\sigma^2 = \frac{S_k}{k - 1}$$
2. **Unsupervised Anomaly Detection (*Isolation Forest*):**
   * Multi-dimensional feature vector: $\vec{x} = [\Delta\text{Pitch}, \Delta\text{Roll}, \dot{\theta}_p, \dot{\theta}_r, \text{Vib}_{\text{RMS}}, \text{STA/LTA}]$.
   * Isolates anomalous ground shift clusters without requiring pre-labeled training datasets.
3. **Spatial Coherence Multi-Node Correlation:**
   * Genuine coal mining subsidence forms a **subsidence trough (bowl)** affecting multiple adjacent points.
   * If `NODE-02` and `NODE-03` tilt simultaneously in congruent vectors ($\vec{v}_2 \cdot \vec{v}_3 > 0.7$), spatial confidence jumps from 0.40 to **0.95**, eliminating false alarms from stray animals or passing dumper trucks.
4. **Scheduled Blast Suppression Mode:**
   * Controlled dynamite blasting in open-cast/underground benches generates transient high-frequency vibration spikes.
   * Operators click **"Scheduled Blast Mode (60s)"** $\rightarrow$ mutes transient vibration alarms while keeping tilt rate detection active.

---

## 💻 6. FULL-STACK DIGITAL TWIN & DASHBOARD (`app/page.tsx`)
* **Tech Stack:** Next.js 16 (App Router), React 19, TypeScript, Tailwind CSS, Web Audio API, HTML5 2.5D Canvas.
* **Interactive Features:**
  * **Hexagonal Mesh Visualizer:** Real-time geometric rendering of surface nodes showing delta vectors ($\Delta P, \Delta R$), status glow, and dead-node dimming if packet gap $>5\text{s}$.
  * **Multi-Node Telemetry Cards:** Live pitch/roll gauges, vibration bars, uptime indicators, and risk tags.
  * **Geotechnical Controls:** Auto-Tare Baseline Zero, Scheduled Blast Suppression (60s countdown), and Master System Reset (`/api/reset`).
  * **Audio Siren:** Synthesizes emergency warning sirens in the browser on Level 3 Critical events.
  * **Forensic CSV Export:** 1-click download of timestamped sensor data logs.

---

## 📊 7. COMPARATIVE ANALYSIS & COST ADVANTAGE TABLE

| Metric / Feature | Conventional DGPS / Total Station | Satellite InSAR (Radar) | Borehole Extensometers | **SurakshaMesh (Proposed)** |
|---|---|---|---|---|
| **Monitoring Frequency** | Manual (Every 15–30 Days) | Periodic (Every 6–12 Days) | Continuous (Point only) | **Continuous (50 Hz / Real-Time)** |
| **Alert Latency** | Days (Post-Facto) | Days to Weeks | Seconds | **Sub-Second (< 500 ms)** |
| **Weather Dependency** | Good Weather Required | Blocked by Monsoons/Clouds | Weather Immune | **All-Weather IP65 Sealed** |
| **Spatial Coverage** | Sparse Discrete Pegs | Wide Area (100+ km²) | Single Point Drilling | **Dense Mesh (Panel-Local)** |
| **Blasting Filter** | None | None | None | **Built-in AI & Scheduled Filter** |
| **Capital Cost** | ₹15–25 Lakhs / kit | ₹5–10 Lakhs / survey | ₹8–12 Lakhs / borehole | **₹1.5k (Proto) / ₹6–8k (Field Node)** |
| **Operating Cost** | High (Survey Crew Payroll) | High (Data License Fee) | High (Borehole Rigging) | **Zero (Autonomous Solar Powered)** |

---

## 🚀 8. 90-DAY GRAND FINALE ROADMAP (FUTURE WORK)

```
┌───────────────────────────┬───────────────────────────┬───────────────────────────┐
│     PHASE 1 (DAYS 1-30)   │     PHASE 2 (DAYS 31-60)  │     PHASE 3 (DAYS 61-90)  │
│   LONG-RANGE RF & POWER   │   AI & EXTENSOMETER FUSION│  FIELD PILOT & COMPLIANCE │
├───────────────────────────┼───────────────────────────┼───────────────────────────┤
│ • Upgrade to SX1278 LoRa  │ • Integrate Draw-Wire     │ • 8-Node Field Deployment │
│   Sub-GHz (433/868 MHz)   │   Crack Extensometers     │   at Jharia/Raniganj CIL  │
│ • 2–5 km Mountain Range   │ • Knothe Time-Factor      │ • DGMS Intrinsic Safety   │
│ • Slotted TDMA Scheduling │   Subsidence Bowl Fitting │   Hazard Testing          │
│ • 1.5W Solar Lid + 10µA   │ • LSTM 24-48h Advance     │ • Offline Pit-Office GIS  │
│   Deep-Sleep (3+ yrs life)│   Ground Sag Forecasting  │   & 4G LTE SMS Gateway    │
└───────────────────────────┴───────────────────────────┴───────────────────────────┘
```

1. **Sub-GHz LoRa Mesh Networking:** Replacing ESP-NOW with SX1278/SX1262 LoRa modules utilizing Slotted TDMA scheduling to support 100+ nodes across a $5\text{ km}$ open-cast / underground mining lease.
2. **Solar Harvesting & Extreme Low-Power:** Implementing ESP32 deep-sleep duty cycling ($10\text{ µA}$) with a top-mounted $1.5\text{W}$ monocrystalline solar panel, yielding $3\text{ to }5\text{ years}$ maintenance-free continuous operation.
3. **Surface Crack Extensometer Integration:** Adding spring-loaded draw-wire sensors across known geological fissures for sub-millimeter displacement tracking.
4. **Predictive AI Modeling (Knothe Time-Factor + LSTM):** Fitting ground displacement curves to the classical Knothe subsidence theory and training a lightweight temporal LSTM model to predict subsidence depth and fissure formation $24\text{ to }48\text{ hours}$ in advance.
5. **Industrial Edge Gateway & DGMS Certification:** Deploying a Raspberry Pi 5 ruggedized industrial gateway with a 4G LTE HAT for automated SMS/Email sirens and pursuing DGMS Intrinsic Safety certification for fiery mines.

---

## 🎨 9. SLIDE-BY-SLIDE VISUAL & DIAGRAM INSTRUCTIONS FOR AI

### Slide 1: Title Page
* **Visual Elements:** Bold typography, Ministry of Coal & Coal India logos (placeholders), sleek dark mining gradient background with subtle hexagonal network overlay.
* **Key Text:** Smart India Hackathon 2026, Problem Statement SIH26025, Team SurakshaMesh (DhartiNode).

### Slide 2: Idea Title & Proposed Solution
* **Visual Elements:** 3-Part visual layout:
  1. *The Challenge:* Icon of underground coal extraction causing surface sag and cracked houses.
  2. *The Solution:* Illustration of autonomous black box nodes forming a mesh above the mining panel.
  3. *Key Highlights:* 3 Cards highlighting "24/7 First Alert", "Spatial Coherence", and "₹6-8k Low Cost".

### Slide 3: Technical Approach & Architecture
* **Visual Elements:**
  * **System Flow Diagram:** `[50Hz MEMS Node] ──(ESP-NOW)──► [Gateway] ──(Serial/API)──► [AI Anomaly Engine] ──► [3D Hex Digital Twin + Klaxon Siren]`.
  * **Hardware Exploded CAD View:** Callout pointing to ESP32, MPU6050, 21700 Battery, TP4056, and Active Buzzer.
  * **AI Box:** Highlighting Dynamic Auto-Tare, Welford's Learning, and Isolation Forest Anomaly Detection.

### Slide 4: Feasibility, Viability & Risk Mitigation
* **Visual Elements:** 2-Column Risk vs Mitigation Table:
  * *Risk 1 (Thermal Drift)* $\rightarrow$ *Mitigation (MPU6050 temp compensation).*
  * *Risk 2 (Blasting Shock)* $\rightarrow$ *Mitigation (3s persistence + 60s Scheduled Blast Mode).*
  * *Risk 3 (Range in Rocky Mine)* $\rightarrow$ *Mitigation (Sub-GHz SX1278 LoRa).*
  * *Risk 4 (Coal Dust / Rain)* $\rightarrow$ *Mitigation (IP65 sealed enclosure).*
* **Callout Stat:** "Live Working 3-Node Prototype Verified; <₹60,000 for Full 8-Node Panel Demo."

### Slide 5: Impact & Real-World Benefits
* **Visual Elements:** 4 Grid Metric Cards with colorful icons:
  1. 🛡️ **Social:** Hours-to-days early evacuation warning before sinkhole formation.
  2. 💰 **Economic:** Prevents multi-crore haul road blockages and equipment burial.
  3. 🌿 **Environmental:** Detects surface fissures before water floods mines or oxygen fuels coal fires.
  4. 🇮🇳 **Strategic:** 100% indigenous Aatmanirbhar Bharat hardware tailored for Indian coalfields.

### Slide 6: Research, References & 90-Day Finale Roadmap
* **Visual Elements:**
  * **Horizontal Timeline:** 3-Phase Roadmap (Phase 1: LoRa & Solar $\rightarrow$ Phase 2: AI & Crack Extensometers $\rightarrow$ Phase 3: Field Pilot at CIL & DGMS Certification).
  * **Reference Badges:** CIL Guidelines, DGMS Ground Control Circulars, CMPDI Subsidence Manual, Isolation Forest (IEEE).
