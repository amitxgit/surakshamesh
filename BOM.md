# SurakshaMesh hardware BOM

Prices are Delhi student-retail ballpark (Robu / Robocraze / local, Aug 2026). Confirm before you buy. **Do not wait for LoRa or flex to start Node 1.**

---

## A. Internal kit — 25 Aug (buy today)

### Must buy / pull from the lab

| # | Item | Qty | Why | Typical ₹ | Line |
|---|---|---|---|---|---|
| 1 | ESP32-WROOM-32 DevKit (30-pin, USB-C or Micro-USB) | 2 (3 better) | MCU + Wi-Fi/ESP-NOW. 3rd board = dedicated hub | 350–450 | 700–1350 |
| 2 | MPU6050 6-axis breakout (GY-521) | 2 (3 spare) | Tilt + vibration from one chip | 80–120 | 160–360 |
| 3 | 400-point breadboard | 2 | No soldering this week | 70–90 | 140–180 |
| 4 | Male–female jumper pack | 1 | IMU and LED | 50–80 | 50–80 |
| 5 | 5 mm RGB LED, common cathode + 3× 220 Ω | 2 | On-node Watch/Warning/Critical | 20 | 40 |
| 6 | USB data cable that matches the DevKit | 2–3 | Charge-only cables fail serial | 40–80 | 80–240 |
| 7 | Power bank ≥1 A | 1 | Remote node. Test under load | already owned | 0 |
| 8 | Wooden plank ~60 × 20 cm, 12–18 mm | 1 | Fake panel. Hardware store | 100–200 | 150 |
| 9 | Electrical tape + superglue / hot glue | 1 | Strain relief, labels | 50 | 50 |
| | **Must-have total (2 nodes, USB power)** | | | | **~₹1,400–2,500** |

3rd ESP32 is worth it: hub never shares an IMU brown-out with a sensor.

### Should add if the shop has stock *today*

| # | Item | Qty | Why | Typical ₹ |
|---|---|---|---|---|
| 10 | Active buzzer | 1 | Extra to dashboard beep (gateway GPIO 14) | 20 |
| 11 | 0.96" SSD1306 OLED | 1 | Pitch numbers without a laptop | 140–180 |
| 12 | Extra MPU6050 | 1 | They die. They get wired 5 V by mistake. | 100 |
| 13 | Female–female jumpers | 1 pack | OLED / LoRa | 50 |

### Only if already in the lab (do not order for 25 Aug)

| # | Item | Qty | Slide value |
|---|---|---|---|
| 14 | SX1278 / Ra-02 LoRa 433 MHz | 2 | Field-radio prop. Firmware can stay ESP-NOW |
| 15 | Flex sensor 2.2" + 10 kΩ | 1 | Crack channel on Node 2 (GPIO 34) |
| 16 | MPU9250 | 2 | Swap for 6050 if you already have them (9-axis unused) |

**Internal all-in if LoRa + flex appear:** ~₹3,000–5,000.

---

## B. Wiring (H1)

### MPU6050 — every node

| MPU6050 | ESP32 |
|---|---|
| VCC | **3V3 only** |
| GND | GND |
| SDA | GPIO 21 |
| SCL | GPIO 22 |
| XDA / XCL | nc |
| ADO | GND → I2C `0x68` (3V3 → `0x69`, then change the sketch) |
| INT | nc |

Chip lettering the same way on both nodes. Tape an arrow “+pitch this way”.

### RGB LED — common cathode

| LED | ESP32 | Resistor |
|---|---|---|
| R | GPIO 25 | 220 Ω |
| G | GPIO 26 | 220 Ω |
| B | GPIO 27 | 220 Ω |
| cathode | GND | — |

No RGB: leave pins empty. Firmware still runs; onboard LED blinks with risk if `USE_ONBOARD_LED 1`.

### Optional flex (Node 2)

| Flex | ESP32 |
|---|---|
| one end | 3V3 |
| other end | GPIO 34 and 10 kΩ to GND (divider) |

Set `USE_FLEX 1` and `NODE_ID 2`.

### Optional LoRa SX1278 (table prop / later firmware)

| SX1278 | ESP32 |
|---|---|
| VCC | 3V3 |
| GND | GND |
| SCK | GPIO 18 |
| MISO | GPIO 19 |
| MOSI | GPIO 23 |
| NSS | GPIO 5 |
| RST | GPIO 14 |
| DIO0 | GPIO 2 |

Do not wire LoRa onto the *same* GPIO 14 as a buzzer.

### Optional OLED SSD1306

| OLED | ESP32 |
|---|---|
| VCC | 3V3 |
| GND | GND |
| SDA | GPIO 21 (shared with MPU) |
| SCL | GPIO 22 |

### USB hub node vs remote node

- Hub: USB to laptop, `IS_USB_HUB 1`, `NODE_ID 1`.
- Remote: power bank USB, `IS_USB_HUB 0`, `NODE_ID 2`.

---

## C. What each rupee is *not*

| Part | Not for |
|---|---|
| MPU6050 | Millimetre heave. ~0.1–0.5° tilt after baseline. |
| ESP-NOW | Field range. Tens of metres, 2.4 GHz, internal only. |
| Breadboard | Dusty mine. Finale is a soldered node + IP65. |
| Phone as IMU | The product. Judges have seen that lose. |

---

## D. Finale node (Layer C) — one field station

Budget this on slide 4. Do not buy it this week.

| # | Item | Qty / node | Typical ₹ | Notes |
|---|---|---|---|---|
| 1 | ESP32 DevKit or ESP32-WROOM module on a carrier | 1 | 400 | Same firmware family |
| 2 | MPU6050 or industrial MEMS inclinometer later | 1 | 100–800 | 6050 is enough to start the 90 days |
| 3 | SX1278 / SX1262 LoRa | 1 | 300–600 | 433 or 868 MHz, one band for the whole mesh |
| 4 | Flex or break-wire | 1 | 50–250 | Crack |
| 5 | Draw-wire or HC-SR04 pair (stretch, 2 nodes share) | 0.5 | 200 | Between posts |
| 6 | NEO-6M/8M GPS | optional | 250–400 | Identity / coarse location, **not** millimetre |
| 7 | IP65 box ~115 × 90 × 55 | 1 | 150–300 | Glands for antenna |
| 8 | 18650 + holder + TP4056 + DW01 | 1 | 150–250 | Protected cell only |
| 9 | 5 V boost (if DevKit needs 5 V USB-style) | 1 | 40 | Or feed 3V3-only custom board |
| 10 | 1–2 W 6 V solar + diode | 1 | 150–300 | Roof of the box |
| 11 | SMA antenna for LoRa | 1 | 80–150 | Do not leave the helical inside metal |
| 12 | Silica gel + conformal coat | 1 | 50 | Dust + dew |
| | **Per field node** | | **~₹6,000–8,000** | Slide 4 number |

### Finale site kit (once per panel demo)

| Item | Qty | Typical ₹ |
|---|---|---|
| Raspberry Pi 4/Zero 2 W + 32 GB (gateway) | 1 | 3,000–6,000 |
| USB LoRa hat **or** a hub ESP32 + SX1278 | 1 | 400–1,500 |
| SIM800L / 4G HAT + SIM | 1 | 400–1,500 |
| Spare 18650 pack, tape, stakes, 20 m cable | 1 | 1,000 |
| **Gateway + spares** | | **~₹5,000–10,000** |

**Panel-scale demo total (8 nodes + gateway): ~₹50,000–70,000.** That is the feasibility number. One CMPDI survey crew-day is in the same order of magnitude; say that qualitatively, not as a fake invoice.

---

## E. Tools (not on the slide, needed tonight)

| Tool | Who |
|---|---|
| Laptop with Arduino IDE + ESP32 board support + Chrome | S1, S2 |
| USB-serial actually enumerating (CH340 / CP2102 driver) | S1 |
| Multimeter | H1 (3V3 vs 5V, continuity) |
| Phone for 45 s video | C2 |
| Tape, marker, scissors | H2 |

---

## F. Shop plan for 23 Aug

1. Pull every ESP32 and MPU6050 already in the lab **before** going to Lajpat / online.
2. If you must buy: 2 ESP32, 2 MPU6050, jumpers, 2 RGB, cables. Everything else is optional.
3. Do not start a PCB. Do not wait for JLCPCB. Breadboard wins internals.
4. Keep GST invoices — C1 can crop a line onto slide 4 if they want a “we priced this” artefact.
