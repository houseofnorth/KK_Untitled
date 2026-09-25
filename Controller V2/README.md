# Controller V2 — XIAO ESP32C3 board

![V2 board render](docs/V2%20board%20render.png)

Second revision of the Chaos Pendulum controller. Drives the VersiDrive i E2 VFD (K2, run impulse) and the motor's 24 V spring-applied brake (K1).
Safe default: brake engaged, VFD disabled until firmware commands otherwise.

- 100 × 90 mm, 2 layers, GND pour both sides, tented vias
- Assembled at JLCPCB (ordered 2026-09-25, files in `OUTPUT/CAM 02`)
- Hand-soldered: XIAO ESP32C3, MOTOR and PANEL terminal blocks
- Power: Mean Well IRM 5 V + IRM 24 V inside the enclosure. D9 (B5819W) feeds 5 V into the XIAO, so USB and external 5 V can be connected at the same time.

## XIAO pin map

| XIAO | GPIO | Net | Function |
|---|---|---|---|
| D0 | 2 | BTN_SIG | Wi-Fi / test button (R11 pull-up, strapping pin) |
| D1 | 3 | VFD_AO_SIG | VFD analog out via 68k/10k divider (V at VFD = mV × 7.8 / 1000) |
| D2 | 4 | RUN_SIG | RUN switch |
| D3 | 5 | LED_VFD_OK | VFD status LED (onboard + panel) |
| D4 | 6 | VFD_OK_SIG | VFD "drive OK" relay feedback, active low |
| D5 | 7 | LED_BRAKE | Brake release LED (onboard + panel) |
| D6 | 21 | LED_MOVE | Motor impulse LED (onboard + panel) |
| D7 | 20 | K2_GATE | Q2 → K2 motor/VFD run relay |
| D8 | 8 | LED_SYS | System status LED, active low (strapping pin) |
| D9 | 9 | PIN_9 | Test pad only (BOOT strapping pin) |
| D10 | 10 | K1_GATE | Q1 → K1 brake relay |

## Connectors (Phoenix MCV 1.5 3.81 mm)

**MOTOR** — 12-pin 1803523, plug MC 1.5/12-ST-3.81

| Pin | Net | Goes to |
|---|---|---|
| 1 | 5V | IRM 5 V + |
| 2 | GND | IRM 5 V − |
| 3 | 24V+ | IRM 24 V + |
| 4 | 24V− | IRM 24 V − |
| 5 | BRAKE+ | Brake plug pin 1 |
| 6 | BRAKE− (= 24V−) | Brake plug pin 2 |
| 7 | VFD_24V | VFD +24 V out (K2 common) |
| 8 | VFD_DI1 | VFD DI1 run input |
| 9 | VFD_OK_IN | VFD relay out ("drive OK") |
| 10 | GND | VFD relay common |
| 11 | VFD_AO_IN | VFD analog out (terminal 8) |
| 12 | GND | VFD 0 V |

**PANEL** — 10-pin 1803507, plug MC 1.5/10-ST-3.81

| Pin | Net | Goes to |
|---|---|---|
| 1 | RUN_IN | RUN switch |
| 2 | GND | RUN switch |
| 3 | BTN_IN | Wi-Fi / test button |
| 4 | GND | button |
| 5 | PLED_BRAKE | Brake LED + |
| 6 | PLED_MOVE | Move LED + |
| 7 | PLED_VFD_OK | VFD OK LED + |
| 8 | GND | Panel LED − (brake, move, VFD OK) |
| 9 | PLED_SYS (SYS+) | System LED + |
| 10 | LED_SYS (SYS−) | System LED − |

Panel LEDs have their series resistors on the board.

## Protection

- D3/D4 1N4148W flyback across the relay coils
- D10 SMAJ33CA (bidirectional) across the brake coil output
- D6 SMF5.0A on VFD_OK_IN, D8 SMF12A on VFD_AO_IN

## Ordering at JLCPCB

Upload from `OUTPUT/CAM 02`: Gerber zip + `Chaos Pendulum PCB_BOM.csv` + `Chaos Pendulum PCB_CPL.csv`.

- K1/K2: Omron G6S-2F-TR DC5 (C397130). TX2SA-5V was out of stock.
- Via covering: Tented.
- **Placement preview fix:** with these Fusion footprints JLC shows Q1, Q2 and all four LEDs turned 90° wrong. Rotate each 90° clockwise, then check:
  - Q1/Q2: pin-1 dot on the top-right pad
  - LEDs: cathode (−) on the left pad for Brake release and Motor impulse, right pad for System status and VFD status
- Diodes and relays come out correct as exported.
- Bottom silkscreen (House of North) looks mirrored in JLC's preview; that is correct.

## Files

- `V2 BOM - JLC.csv`, `V2 BOM - hand solder.csv`, `V1 BOM.xls`
- `OUTPUT/CAM 02/` — production files as ordered
- `docs/V2 enclosure panels.svg` — enclosure panel layout (PANEL + MOTOR sides)
- `docs/V2 option 1 - VFD analog feedback.svg` — VFD analog output wiring
- Firmware: `../Controller/firmware/Untitled/` (pin map above)
