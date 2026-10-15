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

## Panel LEDs (firmware 1.5.7)

| LED | Shows |
|---|---|
| Red (MOVE) | solid = push · speeding-up blink = warning before a push · slow breathing = running, resting · off = stopped |
| Amber (VFD_OK, 60 % brightness) | on = drive OK · breathing = drive not OK / not connected (stopped) · 2 Hz blink = drive not OK while the piece should run |
| Green (BRAKE) | on = brake released |
| Blue (SYS) | on = Wi-Fi up · fast blink = firmware update |

Blue and true-green LEDs need about 3 V; on 3.3 V through 220 Ω they are dimmer than red/amber. Raise **LED brightness** on `/tech` if needed.

Stop on VFD fault is on by default from 1.5.1 (also switched on when upgrading from older settings).

## Bench test without the drive

The drive-OK input is a relay contact from MOTOR 9 (VFD_OK_IN) to MOTOR 10 (GND). A switch across 9–10 fakes the drive: closed = drive OK, open = fault. Never put supply voltage on pin 9 (3.3 V pull-up, 5 V TVS).

## Motor current and strength output (`/tech`)

- **Motor current:** set VFD P-25 = 9 (AO = output current, 0–10 V = 0–200 % of P-08) and enter P-08 on `/tech`. The page then shows A = V(T8) × 0.2 × P-08 live and per push.
- **Strength output (optional, off by default, planned for V3):** PWM on the GPIO9 test pad drives a DFRobot Gravity GP8101S (DFR1036, range switch on 0–10 V) into VFD AI1 (T6). Push strength is random between Strength min and max (% of 10 V), chosen after each push. The artist page then shows *Push length* and *Push strength* separately; with the output off, the push length slider is labelled *Push strength*.
  - Wiring: module + → XIAO 3V3, − → GND, PWM in → GPIO9 pad (bodge wire), VOUT → brown core → T6. *Strength output inverted* stays **off**. Free the brown core by jumpering T11 to T9 at the drive.
  - GPIO9 is a boot strapping pin: check the module input is above ~100 kΩ to GND before connecting, then confirm the board still boots.
  - Not usable with the LC-LM358-PWM2V (needs ≥ 4.5 V PWM).
  - The drive's own speed pot must come off T6, and P-03 must be short.
- OTA: upload `Untitled.ino.bin` (not merged.bin) on the page
- Defaults = original: kick 200–900 ms, rest 10–45 s, pause 60 s every 10 min, brake waits 1250 ms

## Files

- `firmware/Untitled/` — **installation firmware**. Port of the original logic (kick / rest / pause) + Wi-Fi tuning page and OTA.
- `firmware/ControllerTest/` — V1 relay/LED test cycle. Board: XIAO_ESP32C3 (esp32 core 3.x).
- `../Controller V2/firmware/BoardTest/` — V2 bench test sketch (LEDs, relays, inputs), no Wi-Fi/OTA.
- V2 production files (Gerber, BOM, CPL) are in `../Controller V2/OUTPUT/CAM 02/`; `hardware/` here is unused.
- `build/` — local compiled binaries (not committed)
- `Schematic.png`, `IMG_9721.png` — schematic and board photo
