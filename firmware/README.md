# Updating the firmware

The current firmware is **[`Untitled.ino.bin`](Untitled.ino.bin)** in this folder. You upload it to the controller over its own Wi-Fi from a phone or laptop. No cable, no software to install.

> [!WARNING]
> The piece stops during the update and **starts again by itself** afterwards (Autostart). Make sure nobody is in the swing area.

## 1. Download the file

1. Open [`Untitled.ino.bin`](Untitled.ino.bin) on GitHub.
2. Press the **download** button (↓, *Download raw file*) at the top right of the file view.
3. Keep the name `Untitled.ino.bin`. Don't use a file called `…merged.bin` — that one is for USB only.

## 2. Connect to the controller

1. If the blue light is off, press **WIFI ENABLE** — it comes on. If it's already on, Wi-Fi is up and you can go straight to step 2.
2. Join the Wi-Fi **Untitled** (password **deltitnu**).
3. Open **http://192.168.4.1/tech** in the browser. Turn mobile data off if the page doesn't load.

## 3. Upload

1. Scroll to **Firmware update** at the bottom.
2. Tap **Choose file** and pick `Untitled.ino.bin`.
3. Tap **Upload**. The bar fills; the four panel lights chase left to right while it installs. **Don't switch the mains off.**
4. After "Updated – rebooting" the controller restarts (about 10 s). The blue light blinks, then comes on again.

## 4. Check

1. Join **Untitled** again if your phone dropped it, and reload the page.
2. The version at the top of the page (next to *Advanced settings*) should match `FW_VERSION` in [`Untitled/Untitled.ino`](Untitled/Untitled.ino).
3. Your settings are kept across updates.

## If it goes wrong

| Problem | Try |
|---|---|
| "Use Untitled.ino.bin, not merged.bin" | You picked the USB file. Download `Untitled.ino.bin`. |
| Upload error / page stops | Move closer, reload `/tech`, upload again. The old firmware keeps running until an upload finishes. |
| Controller doesn't come back | Switch the mains off and on. If it still doesn't start, flash over USB — see [Firmware.md](Firmware.md). |

## For developers

Building from source, USB flashing and how the code works: [Firmware.md](Firmware.md). After compiling, copy `build/Untitled/Untitled.ino.bin` over `firmware/Untitled.ino.bin` so this page always points at the current build.
