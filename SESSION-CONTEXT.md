# SurakshaMesh — Session Context & Handoff

**Last Updated:** August 24, 2026  
**Problem Statement:** SIH26025 · Coal India Limited · Ministry of Coal · Disaster Management  
**Product Title:** SurakshaMesh — Low-Cost Wireless Surface Mesh for Real-Time Mine Subsidence Early Warning

---

## 1. Repositories & Local Directories

| Repository / Directory | Path | Git Branch & Status |
|---|---|---|
| **Core Firmware, CAD, PPT & Playbook** | `c:\SurakshaMesh` | `origin/main` (clean, up-to-date) |
| **Next.js Modern Hex Dashboard** | `C:\Users\amit0\Desktop\surakshamesh-dashboard` | `origin/master` (clean, up-to-date) |
| **GitHub Remote 1** | `https://github.com/amitxgit/surakshamesh` | Merged PR `#1` |
| **GitHub Remote 2** | `https://github.com/amitxgit/surakshamesh-dashboard` | Direct commits pushed |

---

## 2. Hardware Architecture & Firmware Setup

### 3-Node Topology
- **Central Gateway (`NODE-01`):** ESP32 plugged into laptop USB (or Wi-Fi hotspot mode). Ingests ESP-NOW packets and outputs JSON.
- **Mesh Node 02 (`NODE-02`):** Remote ESP32 + MPU6050 mounted on wooden plank / test rig powered via battery/power bank.
- **Mesh Node 03 (`NODE-03`):** Remote ESP32 + MPU6050 mounted on wooden plank / test rig powered via battery/power bank.

### Pinout (Common for All 3 Nodes)
- **MPU6050 IMU:** `VCC -> 3V3` | `GND -> GND` | `SDA -> GPIO 21` | `SCL -> GPIO 22` | `AD0 -> GND` (Address `0x68`)
- **RGB Status LED:** `R -> GPIO 25 (220Ω)` | `G -> GPIO 26 (220Ω)` | `B -> GPIO 27 (220Ω)` | `Cathode -> GND`
- **Firmware Location:** [`firmware/node/node.ino`](file:///c:/SurakshaMesh/firmware/node/node.ino)

---

## 3. How to Run the Dashboard & Tools

### Start Web Dashboard
```powershell
cd C:\Users\amit0\Desktop\surakshamesh-dashboard
npm.cmd run dev
```
Open **`http://localhost:3000`** in browser.

### Option A: Run Live Hardware Bridge (USB Mode)
Connect `NODE-01` to laptop via USB cable, then start the bridge:
```powershell
cd C:\Users\amit0\Desktop\surakshamesh-dashboard
npm.cmd run bridge COM3
```

### Option B: Run Software Simulator (No Hardware Needed)
```powershell
cd C:\Users\amit0\Desktop\surakshamesh-dashboard
npm.cmd run simulate           # Baseline calibration
npm.cmd run simulate:shift     # Single/dual node Warning
npm.cmd run simulate:collapse  # Multi-node critical subsidence event
```

---

## 4. 3D CAD Models & Presentation Mockups

- **Parametric KCL CAD Source:** [`cad/surakshaMesh/`](file:///c:/SurakshaMesh/cad/surakshaMesh/) (Open `main.kcl` or `service_view.kcl` in [app.zoo.dev](https://app.zoo.dev)).
- **Exploded 3D Slide Graphic (16:9 4K Render):** [`ppt/surakshamesh-exploded-cad.jpg`](file:///c:/SurakshaMesh/ppt/surakshamesh-exploded-cad.jpg)
- **Official SIH Slide Deck:** [`ppt/SIH-official-template.pptx`](file:///c:/SurakshaMesh/ppt/SIH-official-template.pptx)

---

## 5. Next Steps for Next Session / SIH Demo

1. **Physical Flashing:** Open Arduino IDE, select *ESP32 Dev Module*, and flash:
   - Node 1 with `NODE_INDEX 1`, `IS_GATEWAY 1`
   - Node 2 with `NODE_INDEX 2`, `IS_GATEWAY 0`
   - Node 3 with `NODE_INDEX 3`, `IS_GATEWAY 0`
2. **Plank Testing:** Tape nodes with identical sensor orientation to the 60×20 cm wooden plank.
3. **Record 45s Fallback Video:** Record a video of lifting the plank corner and triggering the red alert on screen.
4. **Slide Deck Finalization:** Insert `ppt/surakshamesh-exploded-cad.jpg` with technical layer callouts into the official PowerPoint template.
