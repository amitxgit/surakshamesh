# SurakshaMesh — SIH26025

Coal India / Ministry of Coal · Hardware · Disaster Management

**Idea:** Low-cost wireless surface mesh for real-time mine subsidence early warning.

| Component / File | What it is |
|---|---|
| [PLAYBOOK.md](PLAYBOOK.md) | Full design, build instructions, live plank demo guide, 6-person split, 6-slide PPT copy, Q&A |
| [BOM.md](BOM.md) | Internal + finale hardware list with pin assignments and estimated costs |
| [firmware/node/node.ino](firmware/node/node.ino) | Unified sketch for Central Gateway (`NODE-01`) and Mesh Nodes (`NODE-02`, `NODE-03`) with ESP-NOW + Serial/Wi-Fi telemetry |
| [cad/](cad/) | Parametric 3D CAD enclosure model designed in KCL (Zoo AI / Zoo.dev) with assembly notes and export guides |
| [ppt/SIH-official-template.pptx](ppt/SIH-official-template.pptx) | Official SIH 6-slide template — paste copy from PLAYBOOK §8, export PDF |
| [animator/](animator/) | 60s explainer brief, VO, storyboard, look frames — send this folder to the animator |
| [surakshamesh-dashboard](https://github.com/amitxgit/surakshamesh-dashboard) | Modern Next.js 16 + React 19 real-time hexagonal mesh control dashboard |

---

## Quick Start — Running the System

### 1. Start the Live Dashboard
```powershell
cd C:\Users\amit0\Desktop\surakshamesh-dashboard
npm.cmd run dev
```
Open `http://localhost:3000` in your browser.

### 2. Connect the Hardware or Simulator

- **Hardware-free Simulator:**
  ```powershell
  cd C:\Users\amit0\Desktop\surakshamesh-dashboard
  npm.cmd run simulate           # Normal baseline learning
  npm.cmd run simulate:shift     # Single/dual node tilt alert
  npm.cmd run simulate:collapse  # Multi-node critical subsidence event
  ```

- **Live USB Gateway Bridge (Central ESP32 on COM3):**
  ```powershell
  cd C:\Users\amit0\Desktop\surakshamesh-dashboard
  npm.cmd run bridge COM3
  ```

- **Wireless Wi-Fi Hotspot Mode:**
  Set `#define USE_WIFI_HTTP 1` in `firmware/node/node.ino` on `NODE-01` and connect the ESP32 directly to your laptop hotspot.
