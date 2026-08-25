# SurakshaMesh — Session Context & Handoff

**Last Updated:** August 24, 2026  
**Problem Statement:** SIH26025 · Coal India Limited · Ministry of Coal · Disaster Management  
**Product Title:** SurakshaMesh — Low-Cost Wireless Surface Mesh for Real-Time Mine Subsidence Early Warning

---

## 1. Unified Repository Architecture

| Component | Path | Description |
|---|---|---|
| **Unified Workspace Root** | `c:\SurakshaMesh` | Official Monorepo (`origin/main`) |
| **Next.js Command Center** | `app/`, `lib/`, `package.json` | Next.js 15 + React 19 real-time digital twin & telemetry UI |
| **ESP32 Firmware** | `firmware/node/node.ino` | Central Gateway & Mesh Node firmware with ESP-NOW & Active Buzzer |
| **CAD Enclosure** | `cad/surakshaMesh/` | KittyCAD (KCL) parametric assembly models |
| **PPT Automation** | `ppt/`, `tools/build_ppt.py` | Official SIH presentation generator |
| **Telemetry Tools** | `tools/` | Serial bridge, multi-node simulator, backend tests |
| **GitHub Remote** | `https://github.com/amitxgit/surakshamesh` | Merged and up-to-date |

---

## 2. Hardware Architecture & Firmware Setup

### 3-Node Topology
- **Central Gateway (`NODE-01`):** ESP32 plugged into laptop USB (or Wi-Fi hotspot mode). Ingests ESP-NOW packets and outputs JSON.
- **Mesh Node 02 (`NODE-02`):** Remote ESP32 + MPU6050 mounted on wooden plank / test rig powered via battery/power bank.
- **Mesh Node 03 (`NODE-03`):** Remote ESP32 + MPU6050 mounted on wooden plank / test rig powered via battery/power bank.

### Pinout (Common for All 3 Nodes)
- **MPU6050 IMU:** `VCC -> 3V3` | `GND -> GND` | `SDA -> GPIO 21` | `SCL -> GPIO 22` | `AD0 -> GND` (Address `0x68`)
- **Active Buzzer:** `Pos(+) -> GPIO 14` | `Neg(-) -> GND`
- **Status LED:** Onboard LED (GPIO 2)
- **Firmware Location:** [`firmware/node/node.ino`](file:///c:/SurakshaMesh/firmware/node/node.ino)

---

## 3. How to Run the Dashboard & Tools

### Start Web Command Center
```powershell
npm run dev
```
Open **`http://localhost:3000`** in browser.

### Option A: Run Live Hardware Bridge (USB Mode)
Connect `NODE-01` to laptop via USB cable, then start the bridge:
```powershell
npm run bridge COM3
```

### Option B: Run Software Simulator (No Hardware Needed)
```powershell
npm run simulate              # Normal baseline learning
npm run simulate:shift        # Coherent subsidence Warning
npm run simulate:critical     # Multi-node critical subsidence event
npm run simulate:interactive  # Interactive live scenario switcher (0-3, b)
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
