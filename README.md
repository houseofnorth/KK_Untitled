# Kitty Kraus — Untitled

<table>
<tr>
<td width="66%"><img src="hardware/docs/pcb%20top.png" alt="Controller board" width="100%"></td>
<td valign="middle">
<b>Artist</b><br>Kitty Kraus<br><br>
<b>Original controller</b><br>Roman Zulek<br><br>
<b>Controller revision</b><br>Fubbi Karlsson<br>House of North, 2026
</td>
</tr>
</table>

A motor gives the pendulum a push now and then, rests, and every so often takes a long pause — never quite the same twice. This page explains how to run the piece. Technical details are in [HARDWARE.md](hardware/HARDWARE.md) and [FIRMWARE.md](firmware/FIRMWARE.md).

> [!WARNING]
> **Keep clear of the pendulum before it starts.** The arm can move suddenly and with great force, and it can hit anyone within its swing. Before you switch the mains on or hold RUN / STOP, make sure **nobody is in the swing area**. The blinking red light is your last warning — the first push follows straight after. To stop it at any time, press **RUN / STOP**.

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

Four lights up top tell you how the piece is feeling. Down below: two buttons to press and two sockets for the cables. Each light sits right above its partner.

**Up top — the lights**

| Light | Colour | It's telling you |
|---|---|---|
| RUN / STOP | red | The pendulum's heartbeat — see below |
| BRAKE RELEASE | green | On = brake off, the arm is free to swing |
| VERSI DRIVE | amber | How the motor drive is doing — see below |
| WIFI ENABLE | blue | On = Wi-Fi is up · fast blinking = new software going in |

**Down below — buttons and sockets**

| Under | What it is | What it does |
|---|---|---|
| RUN / STOP | Red button | Press once to stop · hold 2 s to start |
| BRAKE RELEASE | Silver socket | The cable to the motor brake |
| VERSI DRIVE | Black socket | The cable to the motor drive |
| WIFI ENABLE | Blue button | Wi-Fi on for 15 minutes, for the settings page on your phone |

**Red — the heartbeat**

| Red light | Meaning |
|---|---|
| Off | Stopped |
| Slow breathing | Running, chilling between pushes |
| Blinking faster and faster | A push is on its way |
| On | Pushing — here we go |

**Amber — the motor drive**

| Amber light | Meaning |
|---|---|
| On | Drive connected and ready to roll |
| Slow breathing | Drive off or not plugged in — the piece won't start |
| Fast blinking | The drive has a problem. The piece is paused and the brake holds the arm. **It starts again by itself** as soon as the drive is OK — keep clear. |

If amber keeps blinking, switch the mains off for a minute and on again.

## The settings page (phone)

1. Press **WIFI ENABLE**. The blue light comes on.
2. On your phone, join the Wi-Fi **Untitled** (password: **deltitnu**).
3. Open **http://192.168.4.1** in the browser. Turn mobile data off if the page doesn't load.

The Wi-Fi switches itself off after 15 minutes, but not while a phone is connected. **Reset to 15 min** at the bottom of the page restarts the timer.

### Main page

- **Status** at the top: what the piece is doing now and when the next push comes.
- **Start / Stop** — the same as the RUN / STOP button. **Push now** gives the next push straight away.
- **Sliders**, each with a minimum and a maximum — every time the piece picks a random value in between:

| Slider | What it sets |
|---|---|
| **Push strength** | How long, and so how hard, each push is. Longer pushes swing higher. |
| **Rest** | Time from one push to the next. |
| **Long pause every** | How often the piece takes a long pause. Switch off for no long pauses. |
| **Long pause length** | How long a long pause lasts. |

- Changes work **at once**, but are not kept until you press **Save**. **Undo** goes back to the last saved settings. Saved settings survive a power cut.
- **EN / DE** at the top switches the language.

### Advanced settings

The link **Advanced settings** at the bottom leads to the technician page. It shows the panel lights, counters, the drive signals and every setting in numbers, and it is where the software is updated. You normally don't need it — only change things there if you know what they do.

## Help

| Problem | Try |
|---|---|
| Nothing happens after mains on | Is the green light on after a few seconds? If not, press and hold RUN / STOP for 2 s. If the red light stays off, check that *Autostart* is ticked in Advanced settings. |
| Amber blinking fast | Drive fault — the piece waits. Switch mains off for a minute, then on. |
| Settings page won't load | Press WIFI ENABLE again, check you are on the **Untitled** Wi-Fi, turn mobile data off, type `http://192.168.4.1`. |
| Arm moves weakly | See the *Site Fault Check* in [Specs](Specs/). |
