# Controller — XIAO ESP32C3 board

New PCB controller replacing the original Arduino setup. Drives the VersiDrive i VFD (K2) and the Seva motor's spring-applied brake (K1).
Safe default: brake engaged, VFD disabled until firmware commands otherwise.

Current hardware is **V2** — pin map, connectors and ordering notes in [../Controller V2/README.md](../Controller%20V2/README.md). The firmware here uses the V2 pin map (K1 = D10, K2 = D7, LED_BRAKE = D5, LED_MOVE = D6).

V1 pin map (old board, for reference): K1 = D8, K2 = D7, LED red = D5, LED green = D6.

## Wi-Fi app — tuning page

- Wi-Fi `KK-Untitled`, password `untitled2026` → http://192.168.4.1 (or http://untitled.local)
- AP turns off 15 min after boot (setting; 0 = always on), stays up while a phone is connected
- Stays stopped after power-up (Autostart on the technician page `/tech` changes this, off by default)

## Panel buttons (momentary to GND)

- **RUN** (PANEL 1–2): stopped → hold 2 s, the Move LED warning blink starts (you can let go), then the first push. Short press does nothing. Starting or running → any press stops and brakes at once. Web Start/Stop works the same way.
- **Wi-Fi** (PANEL 3–4): press turns the AP on, or restarts its 15 min timer. The System LED shows Wi-Fi status. Don't hold it during power-up (GPIO2 strapping pin — the board won't boot).

## Panel LEDs (1.5.1)

| LED | Shows |
|---|---|
| Red (MOVE) | solid = push · speeding-up blink = warning before a push · slow breathing = running, resting · off = stopped |
| Amber (VFD_OK) | on = drive OK · breathing = drive not OK / not connected (stopped) · 2 Hz blink = drive not OK while the piece should run |
| Green (BRAKE) | on = brake released |
| Blue (SYS) | on = Wi-Fi up · fast blink = firmware update |

Stop on VFD fault is on by default from 1.5.1 (also switched on when upgrading from older settings).

## Motor current and strength output (1.5.0, `/tech`)

- **Motor current:** set VFD P-25 = 9 (AO = output current, 0–10 V = 0–200 % of P-08) and enter P-08 on `/tech`. The page then shows A = V(T8) × 0.2 × P-08 live and per push.
- **Strength output (optional, off by default):** PWM on the GPIO9 test pad drives a "3.3V PWM to 0-10V" D/A module into VFD AI1 (T6). Push strength is random between Strength min and max (% of 10 V), chosen after each push. The artist page then shows *Push length* and *Push strength* separately; with the output off, the push length slider is labelled *Push strength*.
  - Wiring (active low, boot-safe): DIN+ → XIAO 3.3 V, DIN− → GPIO9 pad, module supply from VFD T1 (+24 V) / T9 (0 V), AO → T6. Free a line in the 6-pole VFD cable by jumpering T11 to T9 at the drive (brown becomes spare).
  - The drive's own speed pot must come off T6, and P-03 must be short.
- OTA: upload `Untitled.ino.bin` (not merged.bin) on the page
- Defaults = original: kick 200–900 ms, rest 10–45 s, pause 60 s every 10 min, brake waits 1250 ms

## Files

- `firmware/Untitled/` — **installation firmware**. Port of the original logic (kick / rest / pause) + Wi-Fi tuning page and OTA.
- `firmware/ControllerTest/` — relay/LED test cycle. Board: XIAO_ESP32C3 (esp32 core 3.x).
- `hardware/fusion/` — Fusion Electronics exports
- `hardware/gerbers/` — Gerber zip for JLCPCB
- `hardware/jlcpcb/` — BOM + CPL
- `build/` — local compiled binaries (not committed)
- `Schematic.png`, `IMG_9721.png` — schematic and board photo
