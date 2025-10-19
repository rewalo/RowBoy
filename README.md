<h1 align="center">
  <br>
  <img src="https://raw.githubusercontent.com/rewalo/RowBoy/main/assets/rowboy_logo.png" alt="RowBoy" width="150">
  <br>
  <b>RowBoy Firmware Prototype v1.0 (ESP32-S3)</b>
  <br>
</h1>

<p align="center">
  The open firmware powering the <b>RowBoy Handheld</b>, a hybrid game console, media player, and portable development platform built on the <b>ESP32-S3 WROOM-1</b>.  
  Designed for homebrew gaming, emulation, and multimedia playback, with developer-first modularity baked in from the start.  
  <br/><br/>
  <i><i>Part  devkit. Part console. Entirely open source.</i></i>
  <br/>
  Licensed under the <a href="https://www.gnu.org/licenses/gpl-3.0.html" target="_blank">GNU General Public License v3.0 (GPLv3)</a>.
</p>

---

## Overview

**RowBoy** is an experimental handheld firmware environment designed to transform the ESP32-S3 into a self-contained console and media device.  
It merges the structure of a modern embedded devkit with the polish of a retro-inspired gaming OS.

Planned and existing capabilities:

- **Dynamic Menu UI:** Carousel or list-based interface with themes, animation, and autosave, all built by moi
- **Gamepad & Touch Input:** Supports Bluetooth controllers via Bluepad32, as well as direct mechanical and touch inputs.
- **App System (WIP):** Modular launchers for games, emulators, and utilities (e.g. music player, settings, file browser).
- **Audio Pipeline (WIP):** PCM5102 DAC + PAM8403 amplifier with planned support for tracker-style playback.
- **SD Card Integration:** JSON-backed settings, storage for games, and music library management.
- **Developer Toolkit:** Built-in debug layers, configurable pins, and custom firmware hooks for your own projects.

---

## System Architecture

```text
+-------------------------------------------------------------------------+
|                         RowBoy Firmware Core                            |
+-------------------------------------------------------------------------+
|  MenuUI.cpp / MenuUI.h     → UI rendering, transitions, autosave        |
|  controls.cpp / .h         → Unified input abstraction (pad/touch/mech) |
|  gamepad.cpp / .h          → Bluepad32 controller integration           |
|  audio.cpp / .h (planned)  → PCM / I2S playback, music layer            |
|  sdcard.cpp / .h           → SD mount, file I/O, JSON persistence       |
|  config.h                  → Central build configuration & theming      |
+-------------------------------------------------------------------------+
|  Input:  Gamepad | Touch | Buttons                                      |
|  Output: TFT_eSPI Sprite Renderer | I2S DAC | SD Filesystem             |
+-------------------------------------------------------------------------+
```

---

## Hardware Reference

| Component | Example Part | Notes |
|------------|--------------|-------|
| **MCU** | ESP32-S3 WROOM-1 | Dual-core, PSRAM recommended |
| **Display** | 480×320 ILI9488 / ST7796 | Driven via TFT_eSPI |
| **Storage** | MicroSD (HSPI) | For ROMs, settings, and music |
| **Audio** | PCM5102 / PAM8403 | Stereo output with volume control |
| **Input** | Bluepad32 gamepad, tactile buttons, or touch | Unified in InputMapper |
| **Battery** | 1S Li-ion via charger module | Optional |
| **Backlight / LED** | PWM-controlled | Uses ledcWrite() |

### Default SPI & IO Pins

| Signal | Pin | Purpose |
|:-------|:----|:--------|
| **TFT_CS** | 9 | TFT Chip Select |
| **SD_CS**  | 10 | SD Card CS |
| **TFT_MOSI** | 42 | Shared MOSI |
| **TFT_MISO** | 38 | Shared MISO |
| **TFT_SCLK** | 2  | Shared Clock |
| **LED_PIN** | 4  | Status / Power LED |
| **TFT_BL**  | 1  | Backlight PWM |
| **BTN_PIN** | 5  | Pairing / Menu Button |

---

## Getting Started

### 1. Clone or Download

```bash
git clone https://github.com/rewalo/RowBoy.git
```

Or download the ZIP and extract to your Arduino `Documents/Arduino/` folder.

---

### 2. Install Requirements

- **Arduino IDE**
- **ESP32 Board Support** (via Boards Manager)
  - Add this URL under *File → Preferences → Additional boards manager URLs:*
    ```
    https://espressif.github.io/arduino-esp32/package_esp32_index.json
    ```
  - Recommended: `esp32 v3.2.1`  
- **Libraries**
  - [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI)
  - [Bluepad32](https://github.com/ricardoquesada/bluepad32)
  - [ArduinoJson](https://arduinojson.org/)
  - [SD / FS (built-in for ESP32)**]

---

### 3. Configure TFT_eSPI

RowBoy uses a custom display setup for **ST7796** drivers.

1. Go to your TFT_eSPI folder:
   ```
   Documents/Arduino/libraries/TFT_eSPI/
   ```
2. Open `User_Setup_Select.h` and include your driver header:
   ```cpp
   // #include <User_Setup.h>
   #include <User_Setups/RowBoy_ST7796.h>
   ```
3. Copy the provided setup header (`RowBoy_ST7796.h`)
   into `User_Setups/` and adjust your pins if needed.

---

### 4. Upload the Firmware

1. Select **Board → ESP32-S3 Dev Module**  
2. Set appropriate COM port  
3. Click **Upload** (or press `Ctrl + U`)  
4. The TFT should display the **RowBoy Menu UI** within seconds

---

## Menu System

| Action | Behavior |
|---------|-----------|
| **Left / Right** | Navigate between items (horizontal layout) |
| **Up / Down** | Scroll (vertical layout) |
| **A (Confirm)** | Activate / edit item |
| **B (Back)** | Return to previous menu |
| **Start** | Enter submenu / special action |
| **Select** | Alternate mode or toggle |

> When autosave is enabled, your settings are written to `/settings.json` automatically on SD.  
> Brightness, theme, and transition style persist across reboots.

---

## Developer Mode

Edit `config.h` to tweak:
- Pin mappings  
- Default orientation  
- Font IDs  
- Color scheme  
- Animation styles  
- Input repeat delays  
- Debug toggles (`MENU_LOGS`, `GAMEPAD_LOGS`, etc.)

All subsystems dynamically read from this config.

---

## Planned Features

- Emulator frontend for NES / GB / SMS
- Tracker-style music playback system
- File manager with basic copy/delete
- USB serial “dev console” for debugging
- Customizable themes and UI skins
- Homebrew SDK: Write and package your own games, utilities, or apps directly for RowBoy using a C++ API
- Dynamic Launcher System: Auto-detects and lists user-installed apps stored on SD (/apps/ directory)

---

## Folder Structure

```
RowBoy/
├─ GameConsolePrototype1.ino     # Main firmware entry
├─ MenuUI.h / MenuUI.cpp         # Core UI framework
├─ controls.h / controls.cpp     # Unified input layer
├─ gamepad.h / gamepad.cpp       # Bluepad32 integration
├─ sdcard.h / sdcard.cpp         # SD mount & logging
├─ config.h                      # Build-time configuration
├─ audio.h / audio.cpp (planned) # Audio playback interface
└─ assets/                       # (Optional/Planned) Icons / themes / ROMs
```

---

## Contributing

Pull requests and forks are welcome.  
If you build your own RowBoy variant, feel free to open a PR showcasing it.

---

## License

This project is licensed under the  
**GNU General Public License v3.0 (GPLv3)**  
See the [LICENSE](LICENSE) file for details.

---

<p align="center">
  <sub>Firmware design by <b>Will (rewalo)</b> • RowBoy is a passion project, and my first one, so please go easy on me!</sub>
</p>
