# SurakshaMesh — Telemetry Gateway & Command Center

[![Framework](https://img.shields.io/badge/Framework-Next.js%2015%20(App%20Router)-black.svg?style=flat-square&logo=next.js)](https://nextjs.org/)
[![UI](https://img.shields.io/badge/UI-React%2019%20%7C%20TailwindCSS-38bdf8.svg?style=flat-square&logo=react)](https://react.dev/)
[![Language](https://img.shields.io/badge/Language-TypeScript%205-blue.svg?style=flat-square&logo=typescript)](https://www.typescriptlang.org/)
[![License](https://img.shields.io/badge/License-MIT-green.svg?style=flat-square)](LICENSE)

> **Real-Time 3D Hexagonal Mesh Command Center & Mine Subsidence Early Warning System**  
> *Developed for Smart India Hackathon (SIH26025) — Ministry of Coal & Coal India*

Embedded C++ Firmware & Hardware Repository: **[surakshamesh-firmware](https://github.com/amitxgit/surakshamesh-firmware)**

---

## 📌 System Overview

The **SurakshaMesh Gateway** is a mission-critical telemetry visualizer and command center. It interfaces with the ESP32 wireless sensor mesh via USB-CDC Serial stream or direct Wi-Fi HTTP, processing multi-node 6-DOF inertial metrics, vibration RMS, and subsidence risk states at millisecond latency.

```
 +-------------------------------------------------------------------------------+
 |                       SURAKSHAMESH MONITORING ECOSYSTEM                       |
 |                                                                               |
 |  +-------------------------+             +---------------------------------+  |
 |  |  surakshamesh-firmware  |             |      surakshamesh-gateway       |  |
 |  |  (ESP32 C++, DSP, CAD)  | -- Serial / |  (Next.js 15, React 19, TS)     |  |
 |  |  - 50 Hz IMU Sampling   |    HTTP     |  - 3D Hexagonal Topology View   |  |
 |  |  - ESP-NOW 2.4GHz Mesh  | ----------> |  - Multi-Node Subsidence Alert  |  |
 |  |  - MPU6050 & Actuators  |             |  - Telemetry Ingestion Engine   |  |
 |  +-------------------------+             +---------------------------------+  |
 +-------------------------------------------------------------------------------+
```

---

## ⚡ Key Dashboard Capabilities

- **3D Hexagonal Ground Topology:** Real-time spatial deformation mesh with interactive risk level shading.
- **Micro-Seismic Vibration & Tilt Graphs:** Live streaming charts displaying pitch, roll, and dynamic RMS vibration energy per node.
- **Multi-Node Telemetry Bridge:** Low-latency Node.js Serial-CDC parser (`serial-bridge.mjs`) routing USB UART packets directly into Next.js Route Handlers.
- **Hardware-Free Multi-Scenario Simulator:** Built-in physics-based generator simulating baseline drift, coherent structural shift, and critical ground failure scenarios.

---

## 🚀 Quick Start

### 1. Install Dependencies & Start Web Server
```powershell
npm install
npm run dev
```
Open **[http://localhost:3000](http://localhost:3000)** in your browser.

### 2. Connect Live Hardware or Simulator

#### A. Multi-Node Hardware Simulator (No Hardware Required):
```powershell
npm run simulate              # Normal baseline learning
npm run simulate:shift        # Coherent subsidence warning
npm run simulate:critical     # Severe ground failure alert
npm run simulate:interactive  # Interactive keyboard scenario switcher (0-3, b)
```

#### B. Live USB Hardware Gateway (ESP32 on COM Port):
```powershell
npm run bridge COM3
```

---

## 📂 Repository Contents

| Directory / File | Description |
|---|---|
| `app/` | Next.js 15 App Router pages, layout, and REST API telemetry endpoints (`/api/telemetry`, `/api/status`, `/api/reset`) |
| `components/` | Real-time sensor gauge widgets, 3D hexagonal mesh renderer, and alarm audio triggers |
| `tools/serial-bridge.mjs` | High-throughput USB Serial-to-HTTP bridge daemon |
| `tools/simulate.mjs` | Multi-node sensor simulator engine |
| `animator/` | 60s 3D animation brief, voiceover script, and storyboard assets |
| `ppt/` | Official SIH presentation slide decks and master pitch copy |

---

## 🔗 Embedded Subsystem

For ESP32 C++ firmware, I2C drivers, Complementary filter algorithms, ESP-NOW wireless mesh protocols, and parametric 3D CAD models, see:  
👉 **[amitxgit/surakshamesh-firmware](https://github.com/amitxgit/surakshamesh-firmware)**
