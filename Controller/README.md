# Controller — XIAO ESP32C3 board

New PCB controller replacing the original Arduino setup. Drives the VersiDrive i VFD (K2) and the Seva motor's spring-applied brake (K1).
Safe default: brake engaged, VFD disabled until firmware commands otherwise.

| Signal | GPIO |
|---|---|
| K1 brake relay (via Q1) | 8 |
| K2 motor/VFD relay (via Q2) | 7 |
| LED red (K1 status) | 5 |
| LED blue (K2 status) | 6 |

- `firmware/ControllerTest/` — relay/LED test cycle. Board: XIAO_ESP32C3 (esp32 core 3.x).
- `hardware/fusion/` — Fusion Electronics exports
- `hardware/gerbers/` — Gerber zip for JLCPCB
- `hardware/jlcpcb/` — BOM + CPL
- `build/` — local compiled binaries (not committed)
- `Schematic.png`, `IMG_9721.png` — schematic and board photo
