# Firmware — `firmware/Untitled/`

Installation firmware for the V2 controller (Seeed XIAO ESP32C3). Port of Roman Zulek's original Arduino logic (push / rest / long pause) plus panel buttons and lights, a Wi-Fi settings page and over-the-air updates. Current version: see `FW_VERSION` at the top of [`Untitled.ino`](Untitled/Untitled.ino) (1.7.8).

How to use the piece: [README](../README.md) · Board, wiring, drive settings: [Hardware.md](../hardware/Hardware.md)

## Files

| File | What |
|---|---|
| `Untitled.ino` | Logic, inputs, LEDs, settings storage, web API, OTA |
| `artist.h` | Main page `/` — status, Start/Stop, sliders, EN/DE |
| `page.h` | Technician page `/tech` — status lights, stats, inputs, settings, firmware upload |
| `overrides.h` | Overrides page `/overrides` — LED levels, LED test, VersiDrive (VFD check, motor current) |
| `versidrive.h` | VersiDrive page `/versidrive` — documents the drive and motor, with live readouts (`/setup` redirects here) |

## Build and flash

Board **XIAO_ESP32C3**, esp32 Arduino core 3.x. Open `firmware/Untitled/Untitled.ino` in the Arduino IDE (it loads both `.h` files), or:

```
arduino-cli compile --fqbn esp32:esp32:XIAO_ESP32C3 --output-dir build/Untitled firmware/Untitled
```

- Step by step for updating: [README.md](README.md) in this folder.
- **Over the air (normal):** `/tech` → Firmware update → upload `Untitled.ino.bin` (not `merged.bin`). The piece stops during the update and restarts; with Autostart on it starts again by itself.
- **USB:** Arduino IDE Upload, or `esptool.py --chip esp32c3 write_flash 0x0 Untitled.ino.merged.bin`. Plug USB in before the IRM supplies are on (see Hardware.md, *USB with the IRMs connected*).
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
| WIFI ENABLE | BTN_IN, GPIO2 | AP on (blue blinks fast 1.5 s, then 2 Hz until a phone connects), or restart its timer if already on. Only switches on; ignored during an upload. GPIO2 is a strapping pin: holding it during power-up stops the board from booting. |

### Lights (priority top to bottom)

| LED | Pin | Pattern |
|---|---|---|
| Red MOVE | D6 | solid = pushing · speeding-up blink = warning before a push · breathing (3 s) = running, resting · off = stopped |
| Green BRAKE | D5 | on = brake released |
| Amber VFD_OK | D3 | on = drive OK · breathing = drive not OK, stopped · 2 Hz = drive not OK while it should run |
| Blue SYS | D8 (active low) | 4 Hz blink = booting (first 2 s) or Wi-Fi starting after WIFI ENABLE (1.5 s) · 2 Hz blink = AP up · on = phone connected · off = Wi-Fi off |
| All four | | chase red → green → amber → blue (120 ms a step) = OTA upload in progress |

Each LED has its own level, set on `/overrides` (defaults red 50 %, green 50 %, amber 30 %, blue 50 %). Breathing runs from 10 % to 100 % of that level and always starts at full, so it joins smoothly onto a push or blink.

## Wi-Fi and pages

- Access point **`Untitled`**, password in `AP_PASS` (`Untitled.ino`). Page at `http://192.168.4.1` or `http://untitled.local`.
- **AP off at power-up** — WIFI ENABLE switches it on. Exception: right after a successful OTA update the AP starts by itself (NVS flag `wifiNext`), so the new version can be checked. Off again after *Wi-Fi turns off after* (15 min, 0 = stays on) **without use**, connected phones or not: opening a page or any POST (Save, live slider changes, Start/Stop, LED test …) restarts the timer; status/config polling does not. *Reset 15 min* or WIFI ENABLE restarts the timer.
- Main page `/`: live changes apply at once, **Save** stores them in flash, **Undo** returns to the saved state.
- `/tech`: settings apply on **Save**; *Defaults* restores factory values (including the `/overrides` ones).
- Nav bar: the main page shows *Main · Advanced*; `/tech`, `/overrides` and `/versidrive` show all four. The main page footer links to House of North (needs normal internet).
- `/overrides`: linked only from the technician pages. Changes apply at once, **Save** keeps them, **Undo** returns to the saved state. *LED test* holds all four LEDs solid at their levels; it ends 10 s after the page stops pinging (untick, or leave the page).

### Settings and defaults

`/tech`:

| Setting | Default |
|---|---|
| Kick min / max | 200 / 900 ms |
| Rest min / max | 10 / 45 s |
| Pause every min / max | 10 / 10 min |
| Pause length min / max | 60 / 60 s |
| Move warning | 3 s |
| Brake release wait / engage wait | 1250 / 1250 ms |
| Wi-Fi turns off after | 15 min without use |
| Autostart after power-up | 1 (on) — set 0 for installation / maintenance |

`/overrides`:

| Setting | Default |
|---|---|
| Red / green / amber / blue LED level | 50 / 50 / 30 / 50 % |
| Stop on VFD fault | on — untick to test without the drive, tick again before going live |
| Resume after drive OK for | 5 s |
| Motor rated current (VFD P-08) | 0 — enter it for the A readout on `/tech`; same value as P-08 on `/versidrive` |

`/versidrive` documents how the motor and drive are set — the controller has no data link to the VersiDrive and cannot change its parameters. Fields: motor nameplate (V, A, kW, rpm, Hz, cos φ, star/delta), P-01 … P-25, brake release / engage time, peak push current, drive model, notes. Stored as one JSON string (`/api/setup`, NVS key `setup`, max 2500 chars). **P-08 is not part of that record**: it is the config value *Motor rated current*, so editing it on either page changes both.

### Readouts

- **Motor current (T8):** VersiDrive analog out via 68k/10k divider. With P-25 = 9 and P-08 entered: A = V(T8) × 0.2 × P-08, live and peak per push (last / average).
- Uptime, pushes and pauses since boot, boot count, VFD faults, free memory, button states.

### Web API

`GET /api/status`, `GET /api/config` · `POST /api/config` (apply + save), `/api/live` (apply only), `/api/revert`, `/api/defaults`, `/api/start`, `/api/stop`, `/api/kick` (push after the warning), `/api/wifi` (restart AP timer), `/api/reboot`, `/api/ledtest` (on=1, LED test for 10 s), `GET/POST /api/setup` (site record, `d` = JSON), `/update` (OTA upload).

## Settings storage

Settings live in NVS (`Preferences`, namespace `kk`) as one struct with a version number (`CFG_VERSION`). On update, older versions are migrated: tuned values are kept and new fields get their defaults. Migrations so far switch on *Stop on VFD fault* (v7) and *Autostart* (v10), and set the four LED levels from the old *LED brightness* / *Amber LED level* (v11).

## Future (V3): push strength output — hidden

Code for a second dimension of "push strength" is in place but hidden on `/tech` and off: PWM on GPIO9 (1 kHz, 10-bit) → GP8101S → VFD AI1, value random between Strength min and max, chosen after each push so the module settles during the rest. Settings `useStr`, `strInv` (off for the GP8101S), `strMin`, `strMax`. To show them, remove `display:none` on `#future` in `page.h`. With the output on, the main page splits *Push length* and *Push strength*. Hardware notes in Hardware.md.
