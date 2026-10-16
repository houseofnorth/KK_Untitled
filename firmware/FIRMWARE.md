# Firmware — `firmware/Untitled/`

Installation firmware for the V2 controller (Seeed XIAO ESP32C3). Port of Roman Zulek's original Arduino logic (push / rest / long pause) plus panel buttons and lights, a Wi-Fi settings page and over-the-air updates. Current version: see `FW_VERSION` at the top of [`Untitled.ino`](Untitled/Untitled.ino) (1.6.1).

How to use the piece: [README](../README.md) · Board, wiring, drive settings: [HARDWARE.md](../hardware/HARDWARE.md)

## Files

| File | What |
|---|---|
| `Untitled.ino` | Logic, inputs, LEDs, settings storage, web API, OTA |
| `artist.h` | Main page `/` — status, Start/Stop, sliders, EN/DE |
| `page.h` | Technician page `/tech` — status lights, stats, inputs, all settings, firmware upload |

## Build and flash

Board **XIAO_ESP32C3**, esp32 Arduino core 3.x. Open `firmware/Untitled/Untitled.ino` in the Arduino IDE (it loads both `.h` files), or:

```
arduino-cli compile --fqbn esp32:esp32:XIAO_ESP32C3 --output-dir build/Untitled firmware/Untitled
```

- **Over the air (normal):** `/tech` → Firmware update → upload `Untitled.ino.bin` (not `merged.bin`). The piece stops during the update and restarts; with Autostart on it starts again by itself.
- **USB:** Arduino IDE Upload, or `esptool.py --chip esp32c3 write_flash 0x0 Untitled.ino.merged.bin`. Plug USB in before the IRM supplies are on (see HARDWARE.md, *USB with the IRMs connected*).
- The current build is kept here as [`Untitled.ino.bin`](Untitled.ino.bin) — that's the file to upload. After compiling, copy `build/Untitled/Untitled.ino.bin` over it. `build/` itself is local only (git-ignored).

## Behaviour

- **Power-up:** brake engaged, VFD run off. With **Autostart** (default on) the start sequence follows; with Autostart off the piece stays stopped until RUN is held 2 s or Start is pressed.
- **Start sequence:** brake released, red warning blink (speeds up over *Move warning*, 3 s), first push.
- **Push:** VersiDrive run input (K2) on for a random time between Kick min and max.
- **Rest:** next push a random time between Rest min and max after the start of the last one.
- **Long pause:** every *Pause every min–max* (random), the next push waits *Pause length min–max* (random).
- **Stop:** run input off, brake engaged, then *Brake engage wait*.
- **Drive fault** (*Stop on VFD fault*, default on): drive-OK relay open → move off, brake engaged; resumes once drive OK has been stable for 5 s. At power-up the piece waits for drive ready.

### Buttons (momentary to GND, always active)

| Button | Input | Action |
|---|---|---|
| RUN / STOP | RUN_IN, GPIO4 | Stopped: hold 2 s = start (may let go once red blinks); short press ignored. Starting or running: any press = stop + brake. Web Start/Stop act the same. |
| WIFI ENABLE | BTN_IN, GPIO2 | AP on, or restart its timer. GPIO2 is a strapping pin: holding it during power-up stops the board from booting. |

### Lights (priority top to bottom)

| LED | Pin | Pattern |
|---|---|---|
| Red MOVE | D6 | solid = pushing · speeding-up blink = warning before a push · breathing (3 s) = running, resting · off = stopped |
| Green BRAKE | D5 | on = brake released |
| Amber VFD_OK | D3 | on = drive OK · breathing = drive not OK, stopped · 2 Hz = drive not OK while it should run |
| Blue SYS | D8 (active low) | on = AP up · fast blink = OTA update |

All LEDs scale with *LED brightness* (default 50 %). Amber also runs at *Amber LED level* (temporary tuning setting, default 60 %; to be hard-coded). Breathing always starts at full brightness so it joins smoothly onto a push or blink.

## Wi-Fi and pages

- Access point **`Untitled`**, password in `AP_PASS` (`Untitled.ino`). Page at `http://192.168.4.1` or `http://untitled.local`.
- AP on at power-up, off after *Wi-Fi on after boot* (15 min, 0 = always on), never while a phone is connected. WIFI ENABLE or *Reset 15 min* restarts it.
- Main page `/`: live changes apply at once, **Save** stores them in flash, **Undo** returns to the saved state.
- `/tech`: settings apply on **Save**; *Defaults* restores factory values.

### Settings (`/tech`) and defaults

| Setting | Default |
|---|---|
| Kick min / max | 200 / 900 ms |
| Rest min / max | 10 / 45 s |
| Pause every min / max | 10 / 10 min |
| Pause length min / max | 60 / 60 s |
| Move warning | 3 s |
| Brake release wait / engage wait | 1250 / 1250 ms |
| LED brightness | 50 % |
| Wi-Fi on after boot | 15 min |
| Stop on VFD fault | on |
| Resume after drive OK for | 5 s |
| Autostart after power-up | on — untick for installation / maintenance |
| Soft start kicks | 0 (off) — first N pushes ramp 50 → 100 % |
| Motor rated current (VFD P-08) | 0 — enter it for the A readout |
| Amber LED level (temporary) | 60 % |

### Readouts

- **Motor current (T8):** VersiDrive analog out via 68k/10k divider. With P-25 = 9 and P-08 entered: A = V(T8) × 0.2 × P-08, live and peak per push (last / average).
- Uptime, pushes and pauses since boot, boot count, VFD faults, free memory, button states.

### Web API

`GET /api/status`, `GET /api/config` · `POST /api/config` (apply + save), `/api/live` (apply only), `/api/revert`, `/api/defaults`, `/api/start`, `/api/stop`, `/api/kick` (push after the warning), `/api/wifi` (restart AP timer), `/api/reboot`, `/update` (OTA upload).

## Settings storage

Settings live in NVS (`Preferences`, namespace `kk`) as one struct with a version number (`CFG_VERSION`). On update, older versions are migrated: tuned values are kept and new fields get their defaults. Migrations so far switch on *Stop on VFD fault* (v7) and *Autostart* (v10).

## Future (V3): push strength output — hidden

Code for a second dimension of "push strength" is in place but hidden on `/tech` and off: PWM on GPIO9 (1 kHz, 10-bit) → GP8101S → VFD AI1, value random between Strength min and max, chosen after each push so the module settles during the rest. Settings `useStr`, `strInv` (off for the GP8101S), `strMin`, `strMax`. To show them, remove `display:none` on `#future` in `page.h`. With the output on, the main page splits *Push length* and *Push strength*. Hardware notes in HARDWARE.md.
