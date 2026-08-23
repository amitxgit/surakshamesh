# SurakshaMesh — animator brief

**Job:** 60-second explainer for Smart India Hackathon internal (25 Aug 2026)  
**Product:** SurakshaMesh  
**PS:** SIH26025 · Coal India Limited · Hardware · Disaster Management  
**Team name on end card:** DhartiNode *(change if they lock another name)*  
**Owner from our side:** C2 (demo director) · **approver:** C1 (PPT) + speaker  
**This video is the animated explainer.** The 45-second phone clip of the real plank is a *separate* backup. Do not replace the live demo with this film. Faculty should see the plank. This film plays if USB dies, or as a 20 s loop on the laptop before we stand up.

Read this whole page before you keyframe. Honesty is part of the design. If a shot would win a VFX reel and lose a CIL engineer, cut it.

---

## 1. One-sentence film

> Underground coal mining can sag the ground above it. Surveys find cracked houses too late. SurakshaMesh is a cheap, always-on **surface sensor mesh** that paints Watch / Warning / Critical — a first alert, not a millimetre GPS survey.

---

## 2. Deliverables (what you hand us)

| File | Spec | Use |
|---|---|---|
| `SurakshaMesh_60s_16x9.mp4` | 1920×1080, 30 fps, H.264, AAC 48 kHz, **≤60.0 s**, burned-in captions | Pitch / backup |
| `SurakshaMesh_60s_16x9_SILENT.mp4` | Same picture, **no music, captions stay** | Noisy room |
| `SurakshaMesh_20s_16x9.mp4` | Shots 03–07 only, recut, captions | Laptop loop |
| `SurakshaMesh_20s_9x16.mp4` | Optional WhatsApp cut | Team share |
| Project file | AE / Blender / Premiere, linked assets | Revisions |

**Deadline for internal:** first cut **Monday 24 Aug 18:00**, lock **22:00** (same as PPT freeze). If you cannot finish 60 s, ship the 20 s loop. A finished 20 s beats a 60 s with missing shots.

Do **not** put DU, FoT, Delhi, or the college name anywhere.

---

## 3. Look lock

Treat this as a **mine-safety technical film**, not a startup launch.

| Token | Value | Use |
|---|---|---|
| Coal charcoal | `#1A1A1A` | Ground, boxes, UI chrome |
| Dust paper | `#F4F1EA` | Captions, house walls |
| Safety yellow | `#F5C400` | Radio arcs, gasket, Watch |
| Map green | `#2E7D32` | Clear / LED / cells |
| Alert red | `#C0392B` | Warning / Critical / the one bad cell |
| Muted ochre | earth, gallery rock | |

**Style:** 2.5D isometric + a few photoreal product holds. Think explainer-from-a-PSU, not After Effects template pack. Slow cameras. No glitch. No HUD scan-lines. No lens flare on the LED.

**Type (captions only — do not invent UI copy):**
- End card / captions: Inter, IBM Plex Sans, or Noto Sans. Weight 600 for captions.
- Captions: white on 70% charcoal bar, 42–48 px at 1080p, 2 lines max, ~1.8 s minimum hold.
- Never put millimetre readouts, fake lat/long, or “99%” on screen.

**Canonical node (model this once, reuse everywhere):**
- Fist-sized IP65 box, charcoal, thin yellow gasket, stubby antenna, small solar on lid, **one** round LED (green / yellow / red only).
- Internal cutaway (shot 04 only): ESP32-class board + IMU brick + battery. Label in caption, not as tiny 3D text.
- Same box in the field and on the plank. Do not switch designs mid-film.

Style stills for look (`animator/style/`):

| File | Use as | Corrective note |
|---|---|---|
| `style-01-node.jpg` | Canonical node | Lock this: solar lid, yellow gasket, one round LED, stubby antenna. Ignore Grok watermark. |
| `style-02-cutaway.jpg` | Shot 01 massing | Keep the surface dish + house. **Restyle underground as bord-and-pillar coal void**, not a furnished workshop. |
| `style-03-mesh.jpg` | Shots 03 / 05 mood | Keep dusk, yellow radio arcs, laptop on a table. **Field nodes must be the hero box**, not cubes on yellow sticks. |
| `style-04-map.jpg` | Shot 07 | 2×3 grid, one red cell, no numbers — this is the map language. |

Match palette and massing. You do not have to match camera 1:1. Print `storyboard.html` and put it next to the monitor.

---

## 4. Story math (60 seconds = 8 shots)

| Shot | Time | Picture | Caption (burned in) | VO (if we have sound) |
|---|---|---|---|---|
| **01** | 0:00–0:07 | Isometric cutaway: underground gallery + surface house. Ground above the void dishes a few degrees. **Slow.** | Underground coal panels can sag the ground above them. | “Underground coal mining leaves a void. On the surface, the ground can sag — quietly.” |
| **02** | 0:07–0:14 | Same house, a hairline crack in a wall. A surveyor’s staff / total station arrives *after*. Calendar ticks are fine; a collapsing town is not. | Today CIL finds this with periodic surveys — often after houses crack. | “Coal India still finds this with periodic surveys and cracked houses. That is too late.” |
| **03** | 0:14–0:22 | Six identical nodes drop/stake onto the surface in a grid. LEDs green. No drone deploying them — a hand or a simple drop-in. | SurakshaMesh: a low-cost wireless **surface mesh** above the panel. | “SurakshaMesh is a low-cost wireless surface mesh. Small stations sit above the panel, always on.” |
| **04** | 0:22–0:30 | Cut into one node: IMU (tilt + vibration), radio, LED. Simple arrows, not a BOM explosion. | Each node: tilt, vibration, optional crack. LED = local risk. | “Each node feels tilt and vibration. The box still talks if the laptop dies.” |
| **05** | 0:30–0:38 | Packets hop box → box → gateway → laptop. Arcs in safety yellow. On the laptop: six green cells. Label the radio honestly: **ESP-NOW (prototype) / LoRa (field)**. | Mesh to gateway to an offline map. ESP-NOW now. LoRa in the field. | “They report to a gateway. A live map. Offline first.” |
| **06** | 0:38–0:46 | **The money shot.** Ground under node N2 tilts a few degrees (match our plank: a corner lift, not an earthquake). N2 LED green → yellow → red. N1 stays green. | When the ground starts to move, that cell goes red. | “When the ground starts to sag, that station sees it — Watch, then Warning.” |
| **07** | 0:46–0:53 | Laptop map: N2 cell red. Banner: **WARNING**. Toast: **SMS would be sent to mine office**. Hold. Optional tiny line: *field build: real SMS*. | Watch / Warning / Critical. First alert to the mine office. | “The control room gets Watch, Warning, or Critical — even if the wide-area network is down, the log stays.” |
| **08** | 0:53–1:00 | Split: left = mesh grid, right = a single survey peg labelled **DGPS — calibration**. They are not fighting; they stack. End card. | First-alert mesh. Survey-grade GPS stays calibration. | “Not a replacement for survey-grade GPS. The early-warning fabric Coal India asked for.” |

**End card (hold 4 s, then cut to black):**

```
SurakshaMesh
Low-cost wireless surface mesh
for real-time mine subsidence early warning
SIH26025  ·  Coal India  ·  Hardware
Team DhartiNode
```

No college. No “AI-powered” as the headline. If you need a third line of tech: `ESP32 · IMU · mesh · GIS · offline`.

---

## 5. Voice, music, captions

- **VO language:** Indian English, calm, male or female, ~140 wpm. We will record if you cannot. Script is in `VO-SCRIPT.txt` — **do not rewrite claims**.
- **Music:** low industrial pulse, no drop, no cinematic trailer horns. Duck −12 dB under VO. Provide the silent master anyway; internals are loud.
- **SFX (sparse):** soft soil creak on sag, two-note beep on red (same family as our dashboard 880 Hz), radio tick on packet hop. No explosions, no earthquake rumble.
- **Captions are mandatory.** Assume the VO is off. Every shot must still make the point.

---

## 6. Do / do not (non-negotiable)

**Do**
- Surface mesh over an **underground panel**. Nodes live on the *ground*, not in the gallery.
- Slow tilt. A few degrees. A dish, a corner lift. This matches the live plank demo.
- Two (or six) nodes; one goes red, neighbours stay green. That is the product.
- Offline laptop. Dark room. Honest toast: “SMS *would be* sent”.
- DGPS / total station as a **separate calibration peg**, still in the picture at the end.

**Do not**
- Drones, copters, photogrammetry flyovers, InSAR satellites slamming into the map.
- City-destroying sinkholes, screaming extras, fire, flooding — wrong PS.
- Neural-net brains, ChatGPT bubbles, transformer architecture, “AI” as a glowing head.
- Millimetre readouts (`0.12 mm`), fake Jharia / Raniganj coordinates, “trained on CIL borehole data”.
- Phone-as-the-sensor. The product is the box.
- Replacing DGPS. Do not animate the GPS peg fading out.
- Real Coal India / Government of India lockups unless C1 confirms we may. Use **wording**, not the emblem.
- College / DU / FoT / Delhi in the end card.
- Last year’s IDs (SIH25020, SIH25036).
- Fast MTV cutting. 8 shots, not 40.

If you are unsure whether a flourish is honest, cut it. C2 would rather a still diagram than a lie.

---

## 7. How this film must match the live demo

Judges will see the plank 30 seconds after this video (or instead of it). Continuity is a trust issue.

| In the film | On the table |
|---|---|
| LED green / yellow / red | Same RGB on the ESP32 |
| Cell N2 goes red, N1 stays green | H1 lifts the **N2** corner |
| Toast “SMS would be sent” | Dashboard toast, same words |
| Radio: ESP-NOW now, LoRa later | If a LoRa module is on the table, it is a prop |
| Node is a charcoal box | Ours is a breadboard this week — **stylise the field node in the film**, but keep LED logic identical. Do not pretend the breadboard *is* IP65 in a live-action insert. |

No live-action insert of the breadboard inside the 3D field. Two worlds: film = field story; table = prototype. Shot 06’s *motion* (corner lift) is the bridge.

---

## 8. 20-second loop (if 60 s slips)

Keep shots **03, 06, 07** and a 3 s end card.

Caption-only version:

1. Surface mesh above the panel.  
2. One node tilts; that cell goes red.  
3. Watch / Warning / Critical. First alert — not DGPS.

Looping: last frame should match first (green grid) so the laptop can play it all morning.

---

## 9. Production notes for a 48-hour student animator

- **Tooling:** Blender isometric (preferred) or Illustrator/AE 2.5D. Do not start a photoreal mine sim.
- **Reuse:** 1 node mesh, 1 house, 1 gallery boolean, 1 laptop shader, 1 LED material (emission green/yellow/red).
- **Easing:** 80% of motion is the ground tilt and the LED. Cameras: slow push-ins only (≤10% over 6 s).
- **Colour grade:** slightly dusty, not teal-and-orange.
- **Reviews:** send shot 06 stills first (the red cell). If that still is wrong, the film is wrong.
- **File names:** `S01_cutaway.mp4` … `S08_endcard.mp4` plus a concat master. Keep shot masters; we may drop 02 or 04 live if time is tight.

---

## 10. Credit / legal

- Problem statement language is CIL/SIH. Do not paraphrase into “we invented subsidence monitoring”.
- Uniqueness line you may put on screen once (shot 03 or 08): **Wireless surface mesh for real-time subsidence detection.**
- Music: only tracks we can play in a government hall (original, CC0, or our own). No unlicensed trailer music.

---

## 11. Sign-off checklist (C2 ticks before we play it)

- [ ] 60 s or a finished 20 s — not 47 s of 60 s promised  
- [ ] Captions on the silent master  
- [ ] LED logic matches the plank  
- [ ] No drone, no millimetre, no college name, no fake CIL dataset  
- [ ] End card: SurakshaMesh · SIH26025 · Coal India · Hardware · team name  
- [ ] File on the presentation laptop **and** a USB stick  

First review package: **shot 01 still, shot 06 still, end card still, 20 s animatic with scratch VO.**
