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
| [app/](app/) | Modern Next.js 15 + React 19 real-time hexagonal mesh control center dashboard |

---

## Quick Start — Running the System

### 1. Start the Live Command Center
```powershell
npm run dev
```
Open `http://localhost:3000` in your browser.

### 2. Connect the Hardware or Simulator

- **Hardware-free Multi-Node Simulator:**
  ```powershell
  npm run simulate              # Normal baseline learning
  npm run simulate:shift        # Coherent subsidence warning
  npm run simulate:critical     # Severe ground failure alert
  npm run simulate:interactive  # Interactive keyboard scenario switcher (0-3, b)
  ```

- **Live USB Gateway Bridge (Central ESP32 on COM3):**
  ```powershell
  npm run bridge COM3
  ```

- **Wireless Wi-Fi Hotspot Mode:**
  Set `#define USE_WIFI_HTTP 1` in `firmware/node/node.ino` on `NODE-01` and connect the ESP32 directly to your laptop hotspot.

