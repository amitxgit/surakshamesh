# 🛡️ SurakshaMesh

> **Real-Time Wireless Surface Sensor Mesh for Underground Mine Subsidence Monitoring, Prediction & Early Warning**  
> *Developed for Smart India Hackathon (SIH26025) — Ministry of Coal & Coal India Limited (CIL)*

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Framework](https://img.shields.io/badge/Framework-Next.js%2015%20(App%20Router)-black.svg?style=flat-square&logo=next.js)](https://nextjs.org/)
[![UI](https://img.shields.io/badge/UI-React%2019%20%7C%20TailwindCSS-38bdf8.svg?style=flat-square&logo=react)](https://react.dev/)
[![Hardware](https://img.shields.io/badge/Hardware-ESP32%20%7C%20MPU6050-orange.svg)](#-hardware-requirements--pinout)
[![Mesh Radio](https://img.shields.io/badge/Radio-ESP--NOW%20Mesh%20(2.4GHz)-green.svg)](#-system-architecture)

---

## 📌 Overview

**SurakshaMesh** is an autonomous, low-cost surface sensor mesh deployed above underground coal mining galleries and bord-and-pillar extraction panels. It provides continuous, real-time detection of micro-tilt deformation, dynamic vibration energy, and strata collapse events—delivering proactive early warnings in under 1 second before traditional monthly walk-around surveys detect cracks.

<img width="1428" height="649" alt="SurakshaMesh Surface Deployment" src="https://github.com/user-attachments/assets/e417e271-d45f-447a-87dd-38c01487491a" />

*Figure 1: SurakshaMesh surface deployment nodes for continuous ground deformation tracking.*

For the complete geotechnical research dossier, geomechanical failure derivations, CIMFR damage criteria, and circuit schematics, see the comprehensive [Master Technical Compendium](SURAKSHAMESH_MASTER_TECHNICAL_COMPENDIUM.md).

---

## ✨ Key Features

- **High-Frequency Edge DSP (50 Hz):** ESP32 nodes continuously sample 6-DOF inertial data from MPU6050 sensors at 50 Hz, running a 98/2 Complementary Filter for pitch/roll and a DC-eliminating AC filter for dynamic vibration RMS.
- **STA/LTA Seismic Event Detector:** Implements the seismological Short-Term Average (0.5s) to Long-Term Average (30.0s) ratio detector directly in C++ on the microcontroller to capture transient strata shear waves ($\text{STA}/\text{LTA} > 4.0\text{x}$).
- **Automated Blast vs. Subsidence Classifier:** A 30-second post-tremor observation finite state machine distinguishes quarry blasting from real ground failure:
  - *Quarry Blast (Type 3):* High transient vibration with zero residual tilt ($<0.5^\circ$) automatically mutes false alarms for 30s.
  - *Strata Subsidence (Type 2):* High vibration accompanied by permanent tilt shift ($\ge 0.5^\circ$) triggers multi-tier evacuation alarms.
- **Multi-Node Spatial Coherence Consensus:** Server-side engine evaluates synchronous tilt vector signs across adjacent nodes, instantly rejecting single-point disturbances (e.g. heavy dumper trucks).
- **Online Welford Z-Score Anomaly Detector:** Continuous statistical outlier detection running online Welford variance tracking without heavy external Python dependencies.
- **Next.js 15 Command Center:** Cyber-industrial digital twin with interactive SVG hexagonal mesh topology, Web Audio emergency klaxon, 3-trace trend sparklines, and 17-column CSV audit logging.
- **Autonomous ESP-NOW Mesh:** Zero-infrastructure peer-to-peer 2.4 GHz wireless mesh with sub-50ms latency—requiring zero cellular, router, or internet connection in remote coalfields.

---

## 🏗️ System Architecture

```
                       Surface Over Underground Panel
    ┌─────────────────┐      ┌─────────────────┐      ┌─────────────────┐
    │  Sensor Node 1  │      │  Sensor Node 2  │      │  Sensor Node N  │
    │  ESP32 + MPU6050│      │  ESP32 + MPU6050│      │  ESP32 + MPU6050│
    │  Local 90dB Siren│     │  Local 90dB Siren│     │  Local 90dB Siren│
    └────────┬────────┘      └────────┬────────┘      └────────┬────────┘
             │                        │                        │
             └───────────────┬────────┴────────────────────────┘
                             │ ESP-NOW P2P Wireless Mesh (Sub-50ms)
                             ▼
                    ┌─────────────────┐
                    │ Central Gateway │ (NODE-01 Hub)
                    │ 29-Byte Binary  │
                    └────────┬────────┘
                             │ USB Serial CDC @ 115200 / Wi-Fi HTTP
                             ▼
                ┌─────────────────────────┐
                │ Next.js 15 Command Center│
                │ - Interactive Hex Mesh  │
                │ - STA/LTA Sparklines    │
                │ - Web Audio Synth Alarm │
                │ - Z-Score Anomaly Engine│
                │ - 17-Column CSV Log     │
                └─────────────────────────┘
```

---

## 📸 Hardware & Design Showcase

| Node Hardware & Sensor Rig | Internal Cutaway & Assembly |
| :---: | :---: |
| ![Hardware Node](docs/images/node_hardware.jpg) | <img width="772" height="547" alt="Assembly" src="https://github.com/user-attachments/assets/5daf1990-acf3-45a4-95ca-4e423d2cccd9" /> |
| *ESP32 + MPU6050 station with local audio/visual feedback* | *Internal battery, sensor cradle, and electronics placement* |

| Parametric CAD Exploded Assembly | Spatial Telemetry Map |
| :---: | :---: |
| ![CAD Model](docs/images/cad_exploded.jpg) | <img width="1600" height="914" alt="Dashboard" src="https://github.com/user-attachments/assets/e4094ec3-7ee2-462f-b473-c2af403e85c3" /> |
| *Parametric 3D enclosure model designed in KCL* | *Spatial risk grid and continuous monitoring map* |

---

## 📊 Telemetry Packet Format

Nodes broadcast a packed 29-byte binary struct over ESP-NOW, serialized to JSON over USB Serial:

```json
{
  "nodeId": "NODE-02",
  "role": "field",
  "seq": 142,
  "pitch": 5.82,
  "roll": -0.84,
  "vibration": 0.0820,
  "stalta": 5.40,
  "temp": 27.4,
  "t": 142050,
  "risk": 2,
  "evt": 2
}
```

- **`nodeId` / `role`**: Node identifier and mesh role (`gateway` or `field`).
- **`seq`**: Monotonic packet counter (0–65535).
- **`pitch` / `roll`**: Dual-axis tilt angles in degrees computed via complementary filter.
- **`vibration`**: Dynamic AC RMS acceleration in g ($1\text{g} = 9.81\,\text{m/s}^2$).
- **`stalta`**: Real-time STA/LTA ratio ($>4.0\text{x}$ triggers event detector).
- **`temp`**: Internal MPU6050 silicon temperature in °C.
- **`t`**: Node uptime in milliseconds.
- **`risk`**: Risk state (`0`: Normal, `1`: Watch, `2`: Warning, `3`: Critical).
- **`evt`**: Event classifier (`0`: None, `1`: Pending, `2`: Subsidence, `3`: Blast).

---

## 🛠️ Hardware Requirements & Pinout

### Bill of Materials (BOM)
- **Microcontroller:** ESP32 DevKit V1 (30-pin, dual-core 240 MHz) — ₹380
- **Sensor:** MPU6050 6-Axis Accelerometer & Gyroscope (GY-521) — ₹95
- **Battery:** 21700 / 18650 3.7V Lithium-Ion Cell ($4000\text{ mAh}$) — ₹160
- **Power Management:** TP4056 USB-C with DW01A protection — ₹25
- **Acoustic Siren:** 5V Active Piezo Buzzer (90 dB @ GPIO 14) — ₹15
- **Enclosure:** IP65 Weatherproof ABS Box ($80\times 80\times 40\text{ mm}$) — ₹65
- **Total Hardware Cost:** **₹770 per node** (< ₹15,000 for a 10-node array, 3,000× cheaper than commercial tiltmeters)

### Node Pin Mapping

| Peripheral | Component Pin | ESP32 GPIO | Electrical Notes |
| :--- | :--- | :--- | :--- |
| **MPU6050** | VCC | `3V3` | Regulated 3.3V power (Never connect 5V) |
| | GND | `GND` | Common Ground |
| | SDA | `GPIO 21` | Fast I2C Data (400 kHz) |
| | SCL | `GPIO 22` | Fast I2C Clock (400 kHz) |
| | AD0 | `GND` | Hardware I2C Address `0x68` |
| **Active Buzzer** | Positive (+) | `GPIO 14` | Digital output / PWM siren driver |
| | Negative (-) | `GND` | Ground |
| **Status LED** | Onboard LED | `GPIO 02` | High during Watch/Warning/Critical |

---

## 🚀 Quick Start

### 1. Web Command Center Setup (Next.js 15)

```powershell
# Install dependencies
npm install

# Start local Next.js development server
npm run dev
```
Open **[http://localhost:3000](http://localhost:3000)** in your browser.

### 2. Connect Hardware or Run Simulator

#### Option A: Live USB Gateway (ESP32 on COM Port)
```powershell
# Launch USB Serial Bridge (specify your COM port)
npm run bridge COM3
```

#### Option B: Interactive Multi-Scenario Simulator (No Hardware Needed)
```powershell
# Launch interactive simulator
npm run simulate:interactive
```
*Keyboard controls:*
- `[0]` Normal baseline (Green)
- `[1]` Watch (Yellow - Single Node Tilt)
- `[2]` Warning / Shift (Orange - Multi-Node Spatial Coherence)
- `[3]` Critical Collapse (Red - Severe Failure + Web Audio Siren)
- `[b]` Blast Vibration Spike (Auto-Suppressed)
- `[e]` Dynamic Event Lifecycle Demo (Tremor onset $\to$ Pending $\to$ Strata shear $\to$ Subsidence confirmed)

---

## 📂 Repository Structure

```
surakshamesh/
├── app/                                    # Next.js 15 Digital Twin Command Center
│   ├── api/                                # REST APIs (/telemetry, /status, /blast, /calibrate, /reset)
│   ├── components/HexMeshCanvas.tsx        # Interactive SVG Hexagonal Mesh Topology
│   └── page.tsx                            # Main dashboard, sparklines, synth audio siren, CSV export
├── cad/                                    # Parametric 3D CAD enclosure models (KCL / Zoo.dev)
├── docs/images/                            # Hardware photos, renders, and system diagrams
├── firmware/node/node.ino                  # Unified ESP32 firmware (ESP-NOW mesh + STA/LTA + FSM)
├── lib/telemetry.ts                        # Telemetry engine, Welford Z-score, rate calc, blast suppress
├── tools/
│   ├── serial-bridge.mjs                   # USB Serial-to-HTTP gateway bridge daemon
│   └── simulate.mjs                        # Multi-scenario interactive simulator
├── SURAKSHAMESH_MASTER_TECHNICAL_COMPENDIUM.md # Master technical & geotechnical research compendium
├── LICENSE                                 # MIT License
└── README.md                               # Project documentation
```

---

## 📜 License

This project is licensed under the [MIT License](LICENSE) - see the LICENSE file for details.

## 👤 Author

**Amit Kumar**  
- GitHub: [@amitxgit](https://github.com/amitxgit)  
- LinkedIn: [linkedin.com/in/kxamit](https://www.linkedin.com/in/kxamit/)
