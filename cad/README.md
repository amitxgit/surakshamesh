# SurakshaMesh — 3D CAD Enclosure Model

This folder contains the complete parametric CAD assembly for the SurakshaMesh rugged field enclosure, designed in **KCL** (KittyCAD / Zoo Design Language) using **Zoo AI (Text-to-CAD / Zoo.dev)**.

---

## 1. Quick View in Browser (Instant & Free)

1. Open **[https://app.zoo.dev](https://app.zoo.dev)** in Google Chrome or Edge.
2. Sign in or create a free Zoo account.
3. Click **Open Project** or drag this `cad/surakshaMesh` folder directly into the Zoo Web App.
4. Select `main.kcl` (for the fully assembled enclosure) or `service_view.kcl` (for the internal service layout).
5. Zoo will compile the KCL code and render a high-performance WebGL 3D interactive viewport:
   - **Left Click + Drag:** Rotate model
   - **Right Click + Drag:** Pan
   - **Scroll Wheel:** Zoom
   - **Section Tool:** Inspect internal electronics tray and gasket compression

---

## 2. Viewing in Visual Studio Code

1. In VS Code, install the official extension: **Zoo Modeling App (KCL)**.
2. Open the `cad/surakshaMesh` directory in VS Code.
3. Open `main.kcl`.
4. An interactive 3D model viewport will render side-by-side with the code. Editing any parametric values (e.g. wall thickness, screw spacing in `parameters.kcl`) will update the 3D model in real time.

---

## 3. Exporting for 3D Printing & Standard CAD

In the Zoo Modeling App ([app.zoo.dev](https://app.zoo.dev)):
- Click **Export** in the top navigation bar.
- Choose:
  - **STEP (`.step` / `.stp`):** For Autodesk Fusion 360, FreeCAD, SolidWorks, Onshape.
  - **STL (`.stl`):** For slicing in Bambu Studio, PrusaSlicer, Creality Print, or UltiMaker Cura.
  - **GLTF / OBJ:** For rendering or embedding in web presentations.

---

## 4. Mechanical & Assembly Specifications

- **Nominal Envelope:** 120 × 80 × 40 mm with 3.0 mm reinforced outer walls and 3.4 mm floor.
- **Lid Fasteners:** 4× M4 socket-head cap screws with 4.5 mm clearance holes, 9.0 mm recessed counterbores, and 5.6 mm blind heat-set insert pockets in 11.0 mm corner bosses.
- **Weather Sealing:** Continuous 2.0 mm wide, 1.2 mm deep body groove with matching lid compression bead for silicone/EPDM O-ring or molded gasket.
- **RF / Antenna Port:** 6.5 mm SMA panel opening with external anti-rotation flat pad and dual protective guard rails.
- **Internal Tray:** Centered shock-isolated IMU platform, dedicated battery cradle for a 19 mm cylindrical lithium cell (18650), and keep-outs for ESP32 DevKit and LoRa transceiver modules.

---

## 5. File Inventory

| File | Description |
|---|---|
| `main.kcl` | Top-level assembly containing enclosure, lid, tray, gasket, hardware, and sensors |
| `service_view.kcl` | Exploded / service view for maintenance and assembly drawings |
| `parameters.kcl` | Master parametric variables (dimensions, wall thickness, clearances, screw coordinates) |
| `lower_enclosure.kcl` | Rugged bottom housing with mounting bosses, gasket groove, and SMA port |
| `lid.kcl` | Top cover with compression bead, M4 counterbores, and recessed labeling fields |
| `electronics_tray.kcl` | Internal mounting tray with IMU alignment bars and PCB standoffs |
| `battery_retainer.kcl` | 18650 battery cradle with retention clip |
| `gasket.kcl` | Perimeter sealing gasket |
| `esp32_envelope.kcl` | ESP32 DevKit keep-out volume |
| `lora_envelope.kcl` | SX1278 LoRa transceiver keep-out volume |
| `imu_envelope.kcl` | MPU6050 breakout board keep-out volume |
| `m4_screw.kcl` | Standard M4 fastener model |
| `sma_bulkhead.kcl` | Panel-mount SMA connector model |
| `battery_cell.kcl` | 18650 cylindrical cell reference volume |
| `NOTES.md` | Mechanical design notes from Zoo AI generation |
