# Untitled

<table>
<tr>
<td width="66%"><img src="hardware/docs/pcb%20top.png" alt="Controller board" width="100%"></td>
<td valign="middle">
<b>Artist</b><br>Kitty Kraus<br><br>
<b>Original controller</b><br>Roman Zulek, 2011<br><br>
<b>Controller revision</b><br>Fubbi Karlsson<br>House of North, 2026
</td>
</tr>
</table>

The ESP32-C3 controls the electromagnetic actuation of the pendulum system according to timed intervals. A motor gives the pendulum a push now and then, rests, and every so often takes a long pause — never quite the same twice. This page explains how to run the piece. Technical details are in [Hardware.md](hardware/Hardware.md) and [Firmware.md](firmware/Firmware.md).

> [!WARNING]
> **Keep clear of the pendulum before it starts.** The arm can move suddenly and with great force, and it can hit anyone within its swing. Before you switch the mains on or hold RUN / STOP, make sure **nobody is in the swing area**. The blinking red light is your last warning — the first push follows straight after. To stop it at any time, press **RUN / STOP**.

**Contents**

1. [Starting and stopping](#starting-and-stopping)
2. [The panel](#the-panel)
3. [The settings page (phone)](#the-settings-page-phone) — [Main page](#main-page) · [Sliders](#sliders) · [Advanced settings](#advanced-settings)
4. [Help](#help)
5. Technical: [Hardware](hardware/Hardware.md) · [Firmware](firmware/Firmware.md) · [Updating the firmware](firmware/README.md)

---

## Starting and stopping

| To | Do |
|---|---|
| **Start the piece** | Switch the mains on. After a few seconds the red light blinks faster and faster, then the first push follows. |
| **Stop it** | Press **RUN / STOP** once. The motor stops and the brake holds the arm at once. |
| **Start it again** | Hold **RUN / STOP** for 2 seconds. Let go when the red light starts blinking. |
| **End of the day** | Switch the mains off. Nothing else is needed. |

A short press on RUN / STOP while the piece is stopped does nothing — this is on purpose, so it can't be started by accident.

**Keep clear of the arm** whenever the piece is running or starting.

## The panel

![Front panel](hardware/docs/front%20panel.png)

Top row: four status lights. Bottom row: two buttons and two cable sockets. Each light sits above the button or socket it belongs to.

**Lights**

| Light | Colour | Meaning |
|---|---|---|
| RUN / STOP | red | What the piece is doing — see below |
| BRAKE RELEASE | green | The brake — see below |
| VERSI DRIVE | amber | How the motor drive is doing — see below |
| WIFI ENABLE | blue | Wi-Fi — see below |

**Buttons and sockets**

| Under | What it is | What it does |
|---|---|---|
| RUN / STOP | Red button | Press once to stop · hold 2 s to start |
| BRAKE RELEASE | Silver socket | The cable to the motor brake |
| VERSI DRIVE | Black socket | The cable to the motor drive |
| WIFI ENABLE | Blue button | Wi-Fi on for 15 minutes, for the settings page on your phone |

**Red — the piece**

| Red light | Meaning |
|---|---|
| Off | Stopped |
| Slow breathing | Running, resting between pushes |
| Blinking faster and faster | A push is about to come |
| On | Pushing |

**Green — the brake**

| Green light | Meaning |
|---|---|
| On | Brake released — the arm is free and can move |
| Off | Brake on — the arm is held |

**Amber — the motor drive**

| Amber light | Meaning |
|---|---|
| On | Drive connected and ready |
| Slow breathing | Drive off or not plugged in — the piece won't start |
| Fast blinking | The drive has a problem. The piece is paused and the brake holds the arm. **It starts again by itself** as soon as the drive is OK — keep clear. |

If amber keeps blinking, switch the mains off for a minute and on again.

**All four lights chasing** left to right (red → green → amber → blue): new software is being installed. Don't switch off.

**Blue — Wi-Fi**

| Blue light | Meaning |
|---|---|
| Blinking (first 2 s after mains on) | Starting up |
| On | Wi-Fi is on — you can open the settings page |
| Slow breathing | A phone is connected |
| Off | Wi-Fi is off. Press **WIFI ENABLE** to switch it on |

## The settings page (phone)

1. If the blue light is off, press **WIFI ENABLE** — it comes on. If it's already on, Wi-Fi is up and you can go straight to step 2.
2. On your phone, join the Wi-Fi **Untitled** (password: **deltitnu**).
3. Open **http://192.168.4.1** in the browser. Turn mobile data off if the page doesn't load.

The Wi-Fi switches itself off after 15 minutes, but not while a phone is connected. **Reset to 15 min** at the bottom of the page restarts the timer.

### Main page

| Control | What it does |
|---|---|
| **Status** (top) | What the piece is doing now and when the next push comes |
| **Start / Stop** | Same as the RUN / STOP button |
| **Push now** | Gives the next push straight away — keep clear |
| **Save** | Keeps your changes, also after a power cut |
| **Undo** | Goes back to the last saved settings |
| **EN / DE** | Switches the language |
| **Reset to 15 min** | Restarts the Wi-Fi timer |

### Sliders

Each slider has two handles, a minimum and a maximum. Every time, the piece picks a random value between them — that's what keeps it from repeating. Put both handles together for a fixed value. Changes work **at once**; press **Save** to keep them.

| Slider | What it sets | Range | Factory setting |
|---|---|---|---|
| **Push strength** | How long the motor pushes. Longer = harder push = higher swing. | 0.05 – 5 s | 0.2 – 0.9 s |
| **Rest** | Time from one push to the next | 2 s – 10 min | 10 – 45 s |
| **Long pause every** | How often the piece takes a long pause. Switch it off for no long pauses. | 1 min – 3 h | 10 min |
| **Long pause length** | How long a long pause lasts | 10 s – 60 min | 60 s |

Tip: change one slider at a time and watch a few pushes before you save.

### Advanced settings

**Advanced** at the top of the page leads to the technician page. It shows the panel lights, counters, the drive signals and every setting in numbers, and it is where the software is updated. You normally don't need it — only change things there if you know what they do.

## Help

| Problem | Try |
|---|---|
| Nothing happens after mains on | Is the green light on after a few seconds? If not, press and hold RUN / STOP for 2 s — Autostart may have been switched off for maintenance. |
| Amber blinking fast | Drive fault — the piece waits. Switch mains off for a minute, then on. |
| Settings page won't load | Press WIFI ENABLE again, check you are on the **Untitled** Wi-Fi, turn mobile data off, type `http://192.168.4.1`. |
| Arm moves weakly | See the *Site Fault Check* in [Specs](Specs/). |
