# Controller — XIAO ESP32C3 board

New PCB controller replacing the original Arduino setup. Drives the VersiDrive i VFD (K2) and the Seva motor's spring-applied brake (K1).
Safe default: brake engaged, VFD disabled until firmware commands otherwise.

| Signal | XIAO pin |
|---|---|
| K1 brake relay (via Q1) | D8 (GPIO8) |
| K2 motor/VFD relay (via Q2) | D7 (GPIO20) |
| LED red (K1 status) | D5 (GPIO7) |
| LED green (K2 status) | D6 (GPIO21) |

- `firmware/Untitled/` — **installation firmware**. Port of the original logic (kick / rest / pause) + Wi-Fi tuning page and OTA.
  - Wi-Fi `KK-Untitled`, password `untitled2026` → http://192.168.4.1 (or http://untitled.local)
  - AP turns off 15 min after boot (setting; 0 = always on), stays up while a phone is connected
  - Starts running on power-up; Start/Stop on the page is not saved
  - OTA: upload `Untitled.ino.bin` (not merged.bin) on the page
  - Defaults = original: kick 200–900 ms, rest 10–45 s, pause 60 s every 10 min, brake waits 1250 ms
- `firmware/ControllerTest/` — relay/LED test cycle. Board: XIAO_ESP32C3 (esp32 core 3.x).
- `hardware/fusion/` — Fusion Electronics exports
- `hardware/gerbers/` — Gerber zip for JLCPCB
- `hardware/jlcpcb/` — BOM + CPL
- `build/` — local compiled binaries (not committed)
- `Schematic.png`, `IMG_9721.png` — schematic and board photo
