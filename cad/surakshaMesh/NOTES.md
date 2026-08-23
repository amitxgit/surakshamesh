# SurakshaMesh enclosure notes

- Nominal assembled envelope: 120 × 80 × 40 mm; 3 mm walls and 3.4 mm reinforced floor.
- Four M4 lid screws use 4.5 mm clearance holes, 9 mm recessed counterbores, and 5.6 mm blind heat-set-insert pockets in 11 mm bosses.
- Seal geometry uses a continuous 2 mm-wide, 1.2 mm-deep body groove, a compressed gasket model, and a matching lid compression bead. Final gasket material and compression should be validated with the chosen supplier profile.
- SMA panel opening is 6.5 mm with an external flat pad and two impact guard rails; verify the production connector washer, anti-rotation flat, and panel-thickness requirements before release.
- The electronics tray places the IMU near enclosure center on a rigid platform with orthogonal X/Y reference bars. ESP32 USB and LoRa antenna keep-out envelopes are included.
- The battery cradle provides 19.8 mm diametral clearance for a 19 mm cell, end stops, side rails, and two strap paths. Add compliant foam or elastomer pads during assembly.
- KCL currently has no dedicated modeled-text generator here, so the lid includes recessed fields sized for `SURAKSHAMESH`, `FIELD SENSOR`, and `NODE-02`; add engraving after STEP import or with a downstream CAD text tool.
- Due current engine limitations on several valid overlapping unions, the lower enclosure, tray, and battery retainer are exported as aligned overlapping multi-body manufactured-part aggregates. They remain separate from one another in the assembly and are suitable for later union/repair in Fusion 360 or slicing as coincident printable geometry.
