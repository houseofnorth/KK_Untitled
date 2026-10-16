# Hardware — controller V2 (XIAO ESP32C3)

| Top | Bottom |
|---|---|
| ![PCB top](docs/pcb%20top.png) | ![PCB bottom](docs/pcb%20bottom.png) |

How to use the piece: [README](../README.md) · Software: [Firmware.md](../firmware/Firmware.md)

Second revision of the Chaos Pendulum controller. Drives the VersiDrive i E2 VFD (K2, run impulse) and the motor's 24 V spring-applied brake (K1).
Safe default: brake engaged, VFD disabled until firmware commands otherwise.

- 100 × 90 mm, 2 layers, GND pour both sides, tented vias
- Assembled at JLCPCB (ordered 2026-09-25, files in [`hardware/production/`](production/))
- Hand-soldered: XIAO ESP32C3, MOTOR and PANEL terminal blocks
- Power: Mean Well IRM 5 V + IRM 24 V inside the enclosure. D9 (B5819W) feeds 5 V into the XIAO, so USB and external 5 V can be connected at the same time.

## XIAO pin map

| XIAO | GPIO | Net | Function |
|---|---|---|---|
| D0 | 2 | BTN_SIG | Wi-Fi button (R11 pull-up, strapping pin) |
| D1 | 3 | VFD_AO_SIG | VFD analog out via 68k/10k divider (V at VFD = mV × 7.8 / 1000) |
| D2 | 4 | RUN_SIG | RUN button |
| D3 | 5 | LED_VFD_OK | VFD status LED (onboard + panel) |
| D4 | 6 | VFD_OK_SIG | VFD "drive OK" relay feedback, active low |
| D5 | 7 | LED_BRAKE | Brake release LED (onboard + panel) |
| D6 | 21 | LED_MOVE | Motor impulse LED (onboard + panel) |
| D7 | 20 | K2_GATE | Q2 → K2 motor/VFD run relay |
| D8 | 8 | LED_SYS | Wi-Fi (system) LED, active low (strapping pin) |
| D9 | 9 | PIN_9 | Test pad (BOOT strapping pin). Future (V3): strength PWM → GP8101S → VFD AI1 |
| D10 | 10 | K1_GATE | Q1 → K1 brake relay |

## Connectors (Phoenix MCV 1.5 3.81 mm)

**MOTOR** — 12-pin 1803523, plug MC 1.5/12-ST-3.81

| Pin | Net | Goes to |
|---|---|---|
| 1 | 5V | IRM 5 V + |
| 2 | GND | IRM 5 V − |
| 3 | 24V+ | IRM 24 V + |
| 4 | 24V− | IRM 24 V − |
| 5 | BRAKE+ | Brake M12 socket pin 1 (brown) |
| 6 | BRAKE− (= 24V−) | Brake M12 socket pin 3 (blue); pins 2, 4 not connected |
| 7 | VFD_24V | VFD +24 V out (K2 common) |
| 8 | VFD_DI1 | VFD DI1 run input |
| 9 | VFD_OK_IN | VFD relay out ("drive OK") |
| 10 | GND | VFD relay common |
| 11 | VFD_AO_IN | VFD analog out (terminal 8) |
| 12 | GND | VFD 0 V |

**PANEL** — 10-pin 1803507, plug MC 1.5/10-ST-3.81

Board silkscreen: RUN_IN, GND, BTN_IN, GND, BRAKE, MOVE, VFD_OK, GND, SYS+, SYS-.

| Pin | Net | Goes to |
|---|---|---|
| 1 | RUN_IN | RUN button |
| 2 | GND | RUN button |
| 3 | BTN_IN | Wi-Fi button |
| 4 | GND | button |
| 5 | PLED_BRAKE (BRAKE) | Green brake LED + |
| 6 | PLED_MOVE (MOVE) | Red run/stop LED + |
| 7 | PLED_VFD_OK (VFD_OK) | Amber VersiDrive LED + |
| 8 | GND | Panel LED − (brake, move, VFD OK) |
| 9 | PLED_SYS (SYS+) | Blue Wi-Fi LED + |
| 10 | LED_SYS (SYS−) | Blue Wi-Fi LED − |

Panel LEDs have their series resistors on the board.

## Protection

- D3/D4 1N4148W flyback across the relay coils
- D10 SMAJ33CA (bidirectional) across the brake coil output
- D6 SMF5.0A on VFD_OK_IN, D8 SMF12A on VFD_AO_IN

## Ordering at JLCPCB

Upload from `hardware/production/`: Gerber zip + `Chaos Pendulum PCB_BOM.csv` + `Chaos Pendulum PCB_CPL.csv`.

- K1/K2: Omron G6S-2F-TR DC5 (C397130). TX2SA-5V was out of stock.
- Via covering: Tented.
- **Placement preview fix:** with these Fusion footprints JLC shows Q1, Q2 and all four LEDs turned 90° wrong. Rotate each 90° clockwise, then check:
  - Q1/Q2: pin-1 dot on the top-right pad
  - LEDs: cathode (−) on the left pad for Brake release and Motor impulse, right pad for System status and VFD status
- Diodes and relays come out correct as exported.
- Bottom silkscreen (House of North) looks mirrored in JLC's preview; that is correct.

## Files (`hardware/`)

- `production/` — Gerber zip, BOM and CPL exactly as ordered at JLCPCB
- `BOM - JLC.csv`, `BOM - hand solder.csv` — assembled and hand-soldered parts
- `docs/enclosure panels.svg` — enclosure panel layout (PANEL + MOTOR sides), September concept
- `docs/pcb top.png`, `docs/pcb bottom.png` — board, both sides
- `docs/VFD cable labels.png`, `docs/VFD cable solder layout.png` — strand colours, labels, Quick-Lock solder side
- `docs/option 1 - VFD analog feedback.svg` — VFD analog output wiring
- `docs/kk_untitled_qr.svg` — QR code for the panel
- `labels/* Niimbot.xlsx` — label sheets for the Niimbot app (VFD cable, brake cable, panel LED wires)
- Manuals and the site fault check: [`Specs/`](../Specs/)

## Installation / wiring

### VFD cable — Quick-Lock 6-pole chassis socket → MOTOR 7–12

Internal wiring, socket pin order 1–6 = MOTOR 7–12:

| Socket pin | Colour | MOTOR pin | Net | VersiDrive terminal |
|---|---|---|---|---|
| 1 | Red | 7 | VFD_24V | T1 (+24 V out) |
| 2 | Orange | 8 | VFD_DI1 (run) | T2 |
| 3 | Green | 9 | VFD_OK_IN | T10 (relay) |
| 4 | Brown | 10 | GND (relay common) | T11 (relay) |
| 5 | Yellow | 11 | VFD_AO_IN | T8 (analog out) |
| 6 | Black | 12 | GND (0 V) | T9 (0 V) |

Keep yellow (analog) apart from red/orange (24 V) where the wires are bundled.

### Cable shield (VFD signal cable)

Ground the shield at **one end only — the controller end**. VersiDrive manual, *Schutzabschluss (Kabelabschirmung)*:
"Wenn ein abgeschirmtes Kabel für die Steuer- und Signalverdrahtung benutzt wird, muss die Abschirmung nur am Quellenende geerdet werden, nicht umrichterseitig."

- Controller end: solder the shield onto **pin 6** of the cable plug together with the 0 V core → reaches MOTOR 12 (GND) via the black internal wire.
- VFD end: cut the shield back and insulate it (heatshrink). Do not connect it to the drive.
- Grounding both ends creates a ground loop through the drive. The shield mainly protects the analog output (AO).

### Panel → PCB (PANEL header, inside the enclosure)

- Unshielded stranded wire, 0.14–0.25 mm² (AWG 24–26), with ferrules in the Phoenix plug.
- Twist each input with its own GND: RUN_IN with pin 2, BTN_IN with pin 4.
- BRAKE / MOVE / VFD_OK LED anodes on pins 5–7, all cathodes to GND pin 8.
- SYS LED: anode to pin 9 (SYS+), cathode to pin 10 (SYS−) — not to GND.
- Panel LEDs fitted: red (MOVE), amber (VFD_OK), green (BRAKE), blue (SYS). Blue and true green (Vf ≈ 3 V) run dimmer on 3.3 V through 220 Ω; raise LED brightness on `/tech`, or drop that resistor to ~56 Ω.
- Route away from the IRM mains input and the 24 V brake wires; cross at right angles. Leave slack to unplug, label wires, cable-tie near the plug.

### USB with the IRMs connected

The XIAO 5V pin is the USB-C VBUS line, and D9 back-feeds it from the IRM. A Mac's USB-C port then refuses to connect.
Plug USB in **before** switching the IRMs on, use a USB-A port/hub with an A-to-C cable, or update over the air (Advanced settings → Firmware update).

## VersiDrive i E2 settings

Photograph every parameter before changing anything. P-15 and above need **P-14 = 101**.

| Param | Set to | Why |
|---|---|---|
| P-12 | 0 | Terminal control, DI1 (T2) = run |
| P-18 | 1 | Relay T10/T11 = drive ready (fail-safe, read as VFD_OK) |
| P-25 | 9 | T8 = output current, 0–10 V = 0–200 % of P-08 |
| P-03 | short (~0.3 s) | A push lasts 0.2–0.9 s; a 5 s ramp hides it |
| P-07/08/09 | motor nameplate | P-08 also goes into `/tech` for the A readout |

## Bench test without the drive

The drive-OK input is a relay contact from MOTOR 9 (VFD_OK_IN) to MOTOR 10 (GND). A switch across 9–10 fakes the drive: closed = drive OK, open = fault. Never put supply voltage on pin 9 (3.3 V pull-up, 5 V TVS).

## Future (V3): push strength output — not fitted

PWM on the GPIO9 test pad → DFRobot Gravity GP8101S (DFR1036, range switch 0–10 V) → VFD AI1 (T6), so each push can vary in strength as well as length. The firmware support exists but is hidden and off (see [Firmware.md](../firmware/Firmware.md)).

- Module + → XIAO 3V3, − → GND, PWM in → GPIO9 pad (bodge wire), VOUT → brown core → T6.
- Free the brown core by jumpering T11 to T9 at the drive (relay common to 0 V); drive OK still works over green.
- Take the drive's own speed-pot wiper off T6; keep P-03 short.
- GPIO9 is a boot strapping pin: the module input must read above ~100 kΩ to GND, then confirm the board still boots.
- Not usable with the LC-LM358-PWM2V (needs ≥ 4.5 V PWM).
