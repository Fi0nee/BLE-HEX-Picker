# BLE HEX Picker

This is a small helper firmware for ESP32 — an add-on companion to the main project https://github.com/Fi0nee/LS-Buttplug.

Purpose
- **What:** A firmware for experimenting with and testing manufacturer data HEX values used in BLE to control devices.
- **Why:** By discovering working values you can determine which byte sequences correspond to vibration levels and other device modes.

Where to put discovered values
- **File:** src/LS.h — the discovered values belong in the `manufacturerDataList` array used by the main project.

Example vibration profiles array
```cpp
// ---------------- HEX ----------------
static uint8_t manufacturerDataList[][MANUFACTURER_DATA_LENGTH] = {
    {MANUFACTURER_DATA_PREFIX, 0xE5, 0x00, 0x00}, // Stop
    {MANUFACTURER_DATA_PREFIX, 0xF4, 0x00, 0x00}, // L1
    {MANUFACTURER_DATA_PREFIX, 0xF7, 0x00, 0x00}, // L2
    {MANUFACTURER_DATA_PREFIX, 0xF6, 0x00, 0x00}, // L3
    {MANUFACTURER_DATA_PREFIX, 0xF1, 0x00, 0x00}, // L4
    {MANUFACTURER_DATA_PREFIX, 0xF3, 0x00, 0x00}, // L5
    {MANUFACTURER_DATA_PREFIX, 0xE7, 0x00, 0x00}, // L6
    {MANUFACTURER_DATA_PREFIX, 0xE6, 0x00, 0x00}, // L7
};
```

Field notes
- `MANUFACTURER_DATA_PREFIX` — the manufacturer prefix (fixed bytes before the command byte).
- The second byte (for example `0xE5`, `0xF4`, etc.) is the actual command / mode code for the device.
- The last two bytes are commonly zero (`0x00, 0x00`) and may be reserved for additional parameters.

How to use
1. Flash the ESP32 with this firmware.
2. Use the web UI served by the device to tweak/edit manufacturer data profiles and broadcast them to test device behavior.
3. When you find working codes, copy the corresponding bytes into the `manufacturerDataList` array in src/LS.h of the main project (https://github.com/Fi0nee/LS-Buttplug).

Notes
- This repository is a helper firmware only — changes you want applied to the main firmware should be made in the `LS-Buttplug` project.
- Test carefully: incorrect commands may cause unwanted device behavior.

Flashing (How to flash)
-----------------------

1. Connect the ESP32 to your computer via USB.
2. Install the PlatformIO extension in VS Code or install the PlatformIO CLI.
3. Open this folder in VS Code (the root contains `platformio.ini`).
4. Build and upload the firmware from the project folder:

```bash
pio run --target upload
```

Or use the PlatformIO UI in VS Code: **PlatformIO: Upload**.

What to change in code before flashing
-------------------------------------

- WiFi: set your WiFi SSID and password in `src/main.cpp`. Edit these lines:

```cpp
const char* WIFI_SSID = "YOUR_SSID";
const char* WIFI_PASS = "YOUR_PASSWORD";
```

- Manufacturer data: if you want default profiles baked into the firmware, edit `manufacturerDataList` in `src/LS.h` with the values you want the device to advertise by default.

- Additional parameters: if your data format differs, adjust `MANUFACTURER_DATA_LENGTH` and `MANUFACTURER_DATA_PREFIX` in `src/LS.h` accordingly.
