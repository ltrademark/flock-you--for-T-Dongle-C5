# Flock You: Flock Safety Detection System

<img src="flock.png" alt="Flock You" width="300px">

**Professional surveillance camera detection for the Oui-Spy device available at [colonelpanic.tech](https://colonelpanic.tech)**

## Overview

Flock You is an advanced detection system designed to identify Flock Safety surveillance cameras, Raven gunshot detectors, and similar surveillance devices using multiple detection methodologies. Built for the Xiao ESP32 S3 microcontroller, it provides real-time monitoring with audio alerts and comprehensive JSON output. The system now includes specialized BLE service UUID fingerprinting for detecting SoundThinking/ShotSpotter Raven acoustic surveillance devices.

## Features

### Multi-Method Detection
- **WiFi Promiscuous Mode**: Captures probe requests and beacon frames
- **Bluetooth Low Energy (BLE) Scanning**: Monitors BLE advertisements
- **MAC Address Filtering**: Detects devices by known MAC prefixes
- **SSID Pattern Matching**: Identifies networks by specific names
- **Device Name Pattern Matching**: Detects BLE devices by advertised names
- **BLE Service UUID Detection**: Identifies Raven gunshot detectors by service UUIDs (NEW)

### Audio Alert System
- **Boot Sequence**: 2 beeps (low pitch to high pitch) on startup
- **Detection Alert**: 3 fast high-pitch beeps when device detected
- **Heartbeat Pulse**: 2 beeps every 10 seconds while device remains in range
- **Range Monitoring**: Automatic detection of device leaving range

### Comprehensive Output
- **JSON Detection Data**: Structured output with timestamps, RSSI, MAC addresses
- **Real-time Web Dashboard**: Live monitoring at `http://localhost:5000`
- **Serial Terminal**: Real-time device output in the web interface
- **Detection History**: Persistent storage and export capabilities (CSV, KML)
- **Device Information**: Full device details including signal strength and threat assessment
- **Detection Method Tracking**: Identifies which detection method triggered the alert

## Hardware Requirements

### Option 1: Oui-Spy Device (Available at colonelpanic.tech)
- **Microcontroller**: Xiao ESP32 S3
- **Wireless**: Dual WiFi/BLE scanning capabilities
- **Audio**: Built-in buzzer system
- **Connectivity**: USB-C for programming and power

### Option 2: Standard Xiao ESP32 S3 Setup
- **Microcontroller**: Xiao ESP32 S3 board
- **Buzzer**: 3V buzzer connected to GPIO3 (D2)
- **Power**: USB-C cable for programming and power

### Wiring for Standard Setup
```
Xiao ESP32 S3    Buzzer
GPIO3 (D2)  ---> Positive (+)
GND         ---> Negative (-)
```

### Option 3: LilyGO T-Dongle S3
- **Microcontroller**: ESP32-S3 (USB-A dongle form factor)
- **Display**: 0.96" ST7735 80x160 TFT (on-screen scan status and detection alerts)
- **Alert**: On-board APA102 RGB LED (police-style red/blue strobe that replaces the buzzer)
- No external wiring required.

### Option 4: [LilyGO T-Dongle C5](https://lilygo.cc/en-us/products/t-dongle-c5)
- **Microcontroller**: ESP32-C5, a single-core RISC-V part with **dual-band 2.4 GHz + 5 GHz Wi-Fi 6** and BLE 5
- **Display**: 0.96" ST7735 80x160 TFT (same panel as the S3 dongle)
- **Alert**: On-board APA102 RGB LED (red/blue strobe)
- **USB**: Native USB Serial/JTAG (no UART bridge chip)
- No external wiring required.

> The [T-Dongle C5](https://lilygo.cc/en-us/products/t-dongle-c5) is the newest target. Because
> the ESP32-C5 is not yet supported by the upstream PlatformIO Espressif platform, its build
> environment uses the community [pioarduino](https://github.com/pioarduino/platform-espressif32)
> fork (Arduino-ESP32 3.x / ESP-IDF 5.x) and a NimBLE 2.x-compatible BLE path. The board
> definition and the APA102 LED driver are vendored in this repo
> (`boards/Lilygo-T-Dongle-C5.json`, `lib/APA102/`), so no extra setup is needed. Optional
> 5 GHz channel hopping is available behind the `ENABLE_5GHZ_SCAN` build flag (see
> `platformio.ini`); it is off by default because 5 GHz promiscuous capture on the C5 core is
> still experimental. A full C5 setup guide is in the [LilyGO T-Dongle C5 Guide](#lilygo-t-dongle-c5-guide)
> section below.

## Installation

### Prerequisites
- PlatformIO IDE or PlatformIO Core
- Python 3.8+ (for web interface)
- USB-C cable for programming
- Oui-Spy device from [colonelpanic.tech](https://colonelpanic.tech)

### Setup Instructions
1. **Clone the repository**:
   ```bash
   git clone <repository-url>
   cd flock-you
   ```

2. **Connect your Oui-Spy device** via USB-C

3. **Flash the firmware**:
   ```bash
   pio run --target upload
   ```

   To flash a specific board, pass its environment with `-e`:
   ```bash
   pio run -e t_dongle_c5 --target upload   # LilyGO T-Dongle C5
   pio run -e t_dongle_s3 --target upload   # LilyGO T-Dongle S3
   pio run -e xiao_esp32s3 --target upload  # Xiao ESP32-S3
   ```

   **T-Dongle C5 note:** the C5 flashes over its native USB Serial/JTAG interface via
   `esptool --chip esp32c5`. The first build downloads the pioarduino toolchain, which takes
   a few minutes. If the board is not auto-detected into download mode, hold the **BOOT**
   button (GPIO 28) while plugging it in, then run the upload again. See the
   [LilyGO T-Dongle C5 Guide](#lilygo-t-dongle-c5-guide) for full detail.

4. **Set up the web interface**:
   ```bash
   cd api
   python3 -m venv venv
   source venv/bin/activate  # On Windows: venv\Scripts\activate
   pip install -r requirements.txt
   ```

5. **Start the web server**:
   ```bash
   python flockyou.py
   ```

6. **Access the dashboard**:
   - Open your browser to `http://localhost:5000`
   - The web interface provides real-time detection monitoring
   - Serial terminal for device output
   - Detection history and export capabilities

7. **Monitor device output** (optional):
   ```bash
   pio device monitor
   ```

## LilyGO T-Dongle C5 Guide

This section is specific to the [**LilyGO T-Dongle C5**](https://lilygo.cc/en-us/products/t-dongle-c5)
(Espressif **ESP32-C5**: single-core RISC-V, dual-band 2.4/5 GHz Wi-Fi 6, BLE 5). It scans for
Flock Safety cameras, "Penguin" and "Pigvision" devices, and Raven (ShotSpotter/SoundThinking)
gunshot detectors over Wi-Fi (promiscuous sniffing) and BLE, then alerts on the on-board TFT
screen and RGB LED and prints JSON detection records over USB serial. For the other supported
boards (Xiao ESP32-S3/C3, T-Dongle S3, ESP32-S3 SuperMini) follow the general
[Installation](#installation) steps above.

### Is it plug-and-play?

**Yes.** Once flashed, the firmware lives in the dongle's on-board 16 MB flash. It is permanent
and survives power loss. Plug the dongle into **any USB power source** (another computer, a USB
wall charger, a battery bank) and it boots in about 1 second and starts scanning on its own. It
does **not** need this computer, WSL, PlatformIO, or a serial connection to run. Those are only
needed to *flash* it or to *read* the JSON output. The on-screen alerts and LED strobe work
standalone.

### Hardware summary

| | |
|---|---|
| MCU | ESP32-C5 (RISC-V, 240 MHz), 16 MB flash, 8 MB PSRAM |
| Radios | Dual-band Wi-Fi 6 (2.4 + 5 GHz), Bluetooth LE 5 |
| Display | 0.96" ST7735 80x160 IPS TFT |
| RGB LED | 1x APA102 (data = GPIO5, clock = GPIO4) |
| Button | BOOT (GPIO28) |
| USB | Native USB Serial/JTAG (no UART bridge chip); Type-A dongle |

Pin map (from LilyGO's `pin_config.h`): LCD CS=10, DC=3, RST=1, MOSI=2, SCLK=6, MISO=7,
BL=0 (active-LOW). These are set in `src/display.cpp` and `src/rgb_led.cpp`.

### Flashing

You only need to do this once. There are three routes; pick the one that fits your setup.
A pre-built image is already included at **`firmware/t_dongle_c5-factory.bin`**, so routes A
and C don't require building anything.

#### Route A: Browser flasher (easiest, no installs)

Works from **Windows/macOS/Linux** in Chrome or Edge; no toolchain needed.

1. Open <https://espressif.github.io/esptool-js/>
2. Plug in the T-Dongle C5. Click **Connect** and pick its port
   (it appears as *"USB JTAG/serial debug unit"*).
3. Set **Flash Address** to `0x0` and choose the file
   `firmware/t_dongle_c5-factory.bin` from this repo.
4. Click **Program**. When it finishes, unplug and replug.
5. If Connect fails: hold the **BOOT** button while plugging in, then retry.

> Note: some versions of the fancier [ESP Web Tools](https://esptool.github.io/) installer
> don't yet list `ESP32-C5` as a chip family. `esptool-js` (the link above) always works. It
> just writes the raw image at `0x0`.

#### Route B: PlatformIO (build and flash from source)

Requires [PlatformIO Core](https://platformio.org/install). From the repo root:

```bash
# Build only
pio run -e t_dongle_c5

# Build and flash (auto-detects the port)
pio run -e t_dongle_c5 --target upload
```

The **first build** downloads the pioarduino toolchain (Arduino-ESP32 3.3.x / ESP-IDF 5.x)
and takes several minutes; later builds are fast. Flashing goes over the C5's native USB via
`esptool --chip esp32c5`.

#### Route C: esptool directly

```bash
pip install esptool
esptool --chip esp32c5 --baud 460800 write-flash 0x0 firmware/t_dongle_c5-factory.bin
```

#### Flashing from WSL (Linux on Windows)

WSL2 does not see USB devices by default. To flash from inside WSL, forward the dongle with
[usbipd-win](https://github.com/dorssel/usbipd-win). In an **admin PowerShell** (Windows):

```powershell
winget install usbipd            # one-time
usbipd list                      # find the dongle's BUSID (e.g. 2-4)
usbipd bind   --busid <BUSID>    # one-time, admin
usbipd attach --wsl --busid <BUSID>
```

Then inside WSL, the C5's serial port needs the CDC-ACM driver and open permissions:

```bash
sudo modprobe cdc_acm            # creates /dev/ttyACM0
sudo chmod 666 /dev/ttyACM0      # allow non-root access (redo after each replug)
pio run -e t_dongle_c5 --target upload --upload-port /dev/ttyACM0
```

> `chmod 666` must be repeated every time you unplug/replug, because WSL recreates the node
> as root-only (there is no udev in WSL to relax it automatically).

### Monitoring (reading detections)

The firmware prints boot logs, channel-hop status, and one JSON record per detection at
**115200 baud** over the native USB serial port.

**PlatformIO monitor** (from a real terminal):
```bash
pio device monitor -e t_dongle_c5
```

**Any serial terminal** works too. Point it at the dongle's port at 115200 8N1
(`screen /dev/ttyACM0 115200`, PuTTY, the Arduino IDE monitor, the `esptool-js` console, etc.).

> **The ESP32-C5's hardware USB serial only transmits once the host opens the port with DTR
> asserted.** Proper terminals (PlatformIO, screen, PuTTY, Arduino IDE) do this automatically.
> A plain `cat /dev/ttyACM0` will show *nothing* because it doesn't assert DTR. That is not a
> firmware problem.

What you'll see while idle:
```
[WiFi] Hopped to channel 1
[WiFi] Hopped to channel 2
...
[BLE] scan...
```
On a hit, a full JSON record is printed (SSID/MAC/RSSI/threat score, or BLE name/UUID/Raven
firmware estimate), the screen flips to a red **!! DETECTED !!** alert, and the LED strobes
red/blue.

### Web dashboard on the C5 (localhost:5000)

The repo ships an optional Flask and WebSocket dashboard in [`api/`](api/) that reads the
device's serial output and shows detections in a browser in real time: a live serial terminal,
a detections table with vendor lookup and per-MAC counts, stats, and CSV/KML export. It runs
entirely on your own machine; the dongle only talks to the PC over USB.

#### 1. Install (one time)

Requires Python 3.8+. From the repo root:

```bash
cd api
python3 -m venv venv
./venv/bin/pip install -r requirements.txt
```

> On Debian/Ubuntu/WSL, `python3 -m venv` may fail with *"ensurepip is not available"*.
> If so, install the `python3-venv` package (`sudo apt install python3-venv`), or use
> virtualenv instead: `pip install --user virtualenv && python3 -m virtualenv venv`.

#### 2. Run

```bash
./venv/bin/python flockyou.py
```

(The `api/README.md` says `python app.py`, but the actual entrypoint is `flockyou.py`.)
The server listens on `0.0.0.0:5000`.

#### 3. Open and connect

1. Browse to **http://localhost:5000**. (On WSL, the Windows browser reaches it fine;
   WSL2 forwards `localhost`.)
2. In the UI, pick the dongle's serial port from the port dropdown. It shows up as
   **"USB JTAG/serial debug unit"** (`/dev/ttyACM0` on Linux/WSL, `COMx` on Windows), then
   click **Connect**.
3. The **Serial Terminal** tab immediately shows the live feed (`[WiFi] Hopped to channel N`,
   `[BLE] scan...`). The **Detections** view fills in as devices are found.

#### Seeing content without a real camera

Detections only appear when a matching device is actually in range. To see the UI populate
in the meantime, use the built-in **test detection**, a button in the UI, or via curl:

```bash
curl -X POST http://localhost:5000/api/test/detection \
  -H "Content-Type: application/json" \
  -d '{"protocol":"wifi","detection_method":"beacon","ssid":"FlockSafety-TEST","mac_address":"58:8e:81:12:34:56","rssi":-48,"channel":6,"threat_score":100}'
```

#### Notes

- **One reader at a time.** While the dashboard is connected it owns the serial port, so a
  separate `pio device monitor` or terminal can't read it simultaneously (and vice-versa).
- The dashboard asserts DTR when it opens the port, so the C5's USB serial streams correctly
  (a plain `cat` would show nothing; see the Monitoring section).
- **WSL:** if you replug the dongle, re-run `sudo chmod 666 /dev/ttyACM0` and reconnect in
  the UI. If flashing from Windows instead, detach it from WSL first (`usbipd detach`).
- GPS is optional and separate. The dongle has no GPS, so leave the GPS port unset.

### SD card logging (standalone)

Insert a FAT32-formatted microSD card and the dongle logs every device it detects
to **`/flock_log.csv`** on the card, with no computer needed. This makes it a portable
wardriving-style logger: carry it around on USB power, then pull the card and read the
file later. **No card is required.** Without one, the device just runs normally and the
log functions are silently skipped (you'll see `[SD] no card / mount failed` on the serial
console at boot).

Each **unique device** (by MAC) is logged once per power-on. The file is CSV:

```
timestamp_ms,protocol,detection_method,identifier,mac,rssi,channel,threat_score,latitude,longitude,altitude_m,accuracy_m
12345,wifi,beacon,"FS Ext Battery",58:8e:81:xx:xx:xx,-62,6,100,,,,
20871,ble,device_name,"Penguin",cc:cc:cc:xx:xx:xx,-70,-1,85,,,,
```

- `timestamp_ms` is **milliseconds since power-on** (the C5 has no real-time clock or GPS,
  so there's no wall-clock time).
- `channel` is `-1` for BLE detections.
- The `latitude,longitude,altitude_m,accuracy_m` columns are intentionally **left blank**
  because the C5 has no GPS. They're already in the header so the file is map-ready: you can
  fill them in later by time-correlating with a phone GPS track, or convert the CSV to
  GPX/GeoJSON or the WiGLE upload format once you have coordinates.

> Formatting: use a plain **FAT32** card (cards up to 32 GB work best). If a card isn't
> detected, reformat it FAT32 and reinsert.

### On-device behavior (no computer needed)

- **Boot:** screen shows a "FLOCK YOU" splash, then a cyan **SCANNING** screen with the
  current Wi-Fi channel and BLE status. The LED does a quick red, blue, cyan self-test.
- **Detection:** red alert screen plus red/blue LED strobe (about 3 seconds), then a slow
  orange "in range" heartbeat pulse while the device stays nearby.
- **Out of range:** after 30 seconds with no re-detection it returns to the scanning screen.

### 5 GHz scanning (experimental)

The C5 is dual-band, but 5 GHz promiscuous capture on the current core is still maturing, so
the firmware hops **2.4 GHz channels (1-13) only** by default. To also sweep the common 5 GHz
channels, uncomment this line in the `[env:t_dongle_c5]` section of `platformio.ini` and
re-flash:

```ini
    -DENABLE_5GHZ_SCAN=1
```

If 5 GHz isn't supported on your core build, those channel sets simply fail to tune (harmless)
and 2.4 GHz scanning continues normally.

### T-Dongle C5 troubleshooting

| Symptom | Fix |
|---|---|
| Board won't enter flashing / `Connect` fails | Hold **BOOT** (GPIO28) while plugging in, then flash again. |
| No serial output | Use a real terminal (PlatformIO/screen/PuTTY), not `cat`. DTR must be asserted. Confirm 115200 baud. |
| Screen text mirrored / offset / wrong colors | Adjust `setRotation()` or the `initR(INITR_MINI160x80_PLUGIN)` variant in `src/display.cpp`. The panel is identical to the T-Dongle S3. |
| Nothing on screen at all | Backlight is active-LOW on GPIO0; verify `TFT_BL` handling in `src/display.cpp`. |
| WSL: `/dev/ttyACM0` missing | `sudo modprobe cdc_acm` after `usbipd attach`. |
| WSL: permission denied on port | `sudo chmod 666 /dev/ttyACM0` (repeat after each replug). |
| PlatformIO can't find the C5 platform | The `[env:t_dongle_c5]` env uses the pioarduino fork; upstream `espressif32` does **not** support the C5. Don't change the `platform =` URL. |
| SD card not logging | Check the serial console at boot for `[SD] card mounted` vs `[SD] no card / mount failed`. Reformat the card as **FAT32** (32 GB or smaller) and reseat it. The card is optional; the scanner works without it. |

### T-Dongle C5 build and toolchain notes

- **Platform:** [pioarduino](https://github.com/pioarduino/platform-espressif32) fork of
  `platform-espressif32` (upstream has no ESP32-C5 support). Pinned via the `platform =` URL
  in `platformio.ini`.
- **Board definition:** vendored at `boards/Lilygo-T-Dongle-C5.json`.
- **BLE:** NimBLE-Arduino **2.x** (required by the Arduino 3.x / IDF 5.x core; 2.x renamed the
  scan-callback API and switched scan durations to milliseconds, handled behind `USE_NIMBLE_V2`).
- **RGB LED:** Pololu **APA102** library, vendored at `lib/APA102/` (FastLED has no C5 pin map yet).
- **Display:** Adafruit ST7735 and GFX over the C5's FSPI bus.

## Detection Coverage

### WiFi Detection Methods
- **Probe Requests**: Captures devices actively searching for networks
- **Beacon Frames**: Monitors network advertisements
- **Channel Hopping**: Cycles through all 13 WiFi channels (2.4GHz)
- **SSID Patterns**: Detects networks with "flock", "Penguin", "Pigvision" patterns
- **MAC Prefixes**: Identifies devices by manufacturer MAC addresses

### BLE Detection Methods
- **Advertisement Scanning**: Monitors BLE device broadcasts
- **Device Names**: Matches against known surveillance device names
- **MAC Address Filtering**: Detects devices by BLE MAC prefixes
- **Service UUID Detection**: Identifies Raven devices by advertised service UUIDs
- **Firmware Version Estimation**: Automatically determines Raven firmware version (1.1.x, 1.2.x, 1.3.x)
- **Active Scanning**: Continuous monitoring with 100ms intervals

### Real-World Database Integration
Detection patterns are derived from actual field data including:
- Flock Safety camera signatures
- Penguin surveillance device patterns
- Pigvision system identifiers
- Raven acoustic gunshot detection devices (SoundThinking/ShotSpotter)
- Extended battery and external antenna configurations

**Datasets from deflock.me are included in the `datasets/` folder of this repository**, providing comprehensive device signatures and detection patterns for enhanced accuracy.

### Raven Gunshot Detection System
Flock You now includes specialized detection for **Raven acoustic gunshot detection devices** (by SoundThinking/ShotSpotter) using BLE service UUID fingerprinting:

#### Detected Raven Services
- **Device Information Service** (`0000180a-...`): Serial number, model, firmware version
- **GPS Location Service** (`00003100-...`): Real-time device coordinates
- **Power Management Service** (`00003200-...`): Battery and solar panel status
- **Network Status Service** (`00003300-...`): LTE and WiFi connectivity information
- **Upload Statistics Service** (`00003400-...`): Data transmission metrics
- **Error/Failure Service** (`00003500-...`): System diagnostics and error logs
- **Legacy Services** (`00001809-...`, `00001819-...`): Older firmware versions (1.1.x)

#### Firmware Version Detection
The system automatically identifies Raven firmware versions based on advertised services:
- **1.1.x (Legacy)**: Uses Health Thermometer and Location/Navigation services
- **1.2.x**: Introduces GPS, Power, and Network services
- **1.3.x (Latest)**: Full suite of diagnostic and monitoring services

#### Raven Detection Output
When a Raven device is detected, the system provides:
- Device type identification: `RAVEN_GUNSHOT_DETECTOR`
- Manufacturer: `SoundThinking/ShotSpotter`
- Complete list of advertised service UUIDs
- Service descriptions (GPS, Battery, Network status, etc.)
- Estimated firmware version
- Threat level: `CRITICAL` with score of 100

**Configuration data sourced from `raven_configurations.json`** (provided by [GainSec](https://github.com/GainSec)) in the datasets folder, containing verified service UUIDs from firmware versions 1.1.7, 1.2.0, and 1.3.1.

## Technical Specifications

### WiFi Capabilities
- **Frequency**: 2.4GHz only (13 channels)
- **Mode**: Promiscuous monitoring
- **Channel Hopping**: Automatic cycling every 2 seconds
- **Packet Types**: Probe requests (0x04) and beacons (0x08)

### BLE Capabilities
- **Framework**: NimBLE-Arduino
- **Scan Mode**: Active scanning
- **Interval**: 100ms scan intervals
- **Window**: 99ms scan windows

### Audio System
- **Boot Sequence**: 200Hz to 800Hz (300ms each)
- **Detection Alert**: 1000Hz x 3 beeps (150ms each)
- **Heartbeat**: 600Hz x 2 beeps (100ms each, 100ms gap)
- **Frequency**: Every 10 seconds while device in range

### JSON Output Format

#### WiFi Detection Example
```json
{
  "timestamp": 12345,
  "detection_time": "12.345s",
  "protocol": "wifi",
  "detection_method": "probe_request",
  "alert_level": "HIGH",
  "device_category": "FLOCK_SAFETY",
  "ssid": "Flock_Camera_001",
  "rssi": -65,
  "signal_strength": "MEDIUM",
  "channel": 6,
  "mac_address": "aa:bb:cc:dd:ee:ff",
  "threat_score": 95,
  "matched_patterns": ["ssid_pattern", "mac_prefix"],
  "device_info": {
    "manufacturer": "Flock Safety",
    "model": "Surveillance Camera",
    "capabilities": ["video", "audio", "gps"]
  }
}
```

#### Raven BLE Detection Example (NEW)
```json
{
  "protocol": "bluetooth_le",
  "detection_method": "raven_service_uuid",
  "device_type": "RAVEN_GUNSHOT_DETECTOR",
  "manufacturer": "SoundThinking/ShotSpotter",
  "mac_address": "12:34:56:78:9a:bc",
  "rssi": -72,
  "signal_strength": "MEDIUM",
  "device_name": "Raven-Device-001",
  "raven_service_uuid": "00003100-0000-1000-8000-00805f9b34fb",
  "raven_service_description": "GPS Location Service (Lat/Lon/Alt)",
  "raven_firmware_version": "1.3.x (Latest)",
  "threat_level": "CRITICAL",
  "threat_score": 100,
  "service_uuids": [
    "0000180a-0000-1000-8000-00805f9b34fb",
    "00003100-0000-1000-8000-00805f9b34fb",
    "00003200-0000-1000-8000-00805f9b34fb",
    "00003300-0000-1000-8000-00805f9b34fb",
    "00003400-0000-1000-8000-00805f9b34fb",
    "00003500-0000-1000-8000-00805f9b34fb"
  ]
}
```

## Usage

### Startup Sequence
1. **Power on** the Oui-Spy device
2. **Listen for boot beeps** (low to high pitch)
3. **Start the web server**: `python flockyou.py` (from the `api` directory)
4. **Open the dashboard**: Navigate to `http://localhost:5000`
5. **Connect devices**: Use the web interface to connect your Flock You device and GPS
6. **System ready** when "hunting for Flock Safety devices" appears in the serial terminal

### Detection Monitoring
- **Web Dashboard**: Real-time detection display at `http://localhost:5000`
- **Serial Terminal**: Live device output in the web interface
- **Audio Alerts**: Immediate notification of detections (device-side)
- **Heartbeat**: Continuous monitoring while devices in range
- **Range Tracking**: Automatic detection of device departure
- **Export Options**: Download detections as CSV or KML files

### Channel Information
- **WiFi**: Automatically hops through channels 1-13
- **BLE**: Continuous scanning across all BLE channels
- **Status Updates**: Channel changes logged to serial terminal

## Detection Patterns

### SSID Patterns
- `flock*`: Flock Safety cameras
- `Penguin*`: Penguin surveillance devices
- `Pigvision*`: Pigvision systems
- `FS_*`: Flock Safety variants

### MAC Address Prefixes
- `AA:BB:CC`: Flock Safety manufacturer codes
- `DD:EE:FF`: Penguin device identifiers
- `11:22:33`: Pigvision system codes

### BLE Device Names
- `Flock*`: Flock Safety BLE devices
- `Penguin*`: Penguin BLE identifiers
- `Pigvision*`: Pigvision BLE devices

### Raven Service UUIDs (NEW)
- `0000180a-0000-1000-8000-00805f9b34fb`: Device Information Service
- `00003100-0000-1000-8000-00805f9b34fb`: GPS Location Service
- `00003200-0000-1000-8000-00805f9b34fb`: Power Management Service
- `00003300-0000-1000-8000-00805f9b34fb`: Network Status Service
- `00003400-0000-1000-8000-00805f9b34fb`: Upload Statistics Service
- `00003500-0000-1000-8000-00805f9b34fb`: Error/Failure Service
- `00001809-0000-1000-8000-00805f9b34fb`: Health Service (Legacy 1.1.x)
- `00001819-0000-1000-8000-00805f9b34fb`: Location Service (Legacy 1.1.x)

## Limitations

### Technical Constraints
- **WiFi Range**: Limited to 2.4GHz spectrum
- **Detection Range**: Approximately 50-100 meters depending on environment
- **False Positives**: Possible with similar device signatures
- **Battery Life**: Continuous scanning reduces battery runtime

### Environmental Factors
- **Interference**: Other WiFi networks may affect detection
- **Obstacles**: Walls and structures reduce detection range
- **Weather**: Outdoor conditions may impact performance

## Troubleshooting

### Common Issues
1. **Web Server Won't Start**: Check Python version (3.8+) and virtual environment setup
2. **No Serial Output**: Check USB connection and device port selection in web interface
3. **No Audio**: Verify buzzer connection to GPIO3
4. **No Detections**: Ensure device is in range and scanning is active
5. **False Alerts**: Review detection patterns and adjust if needed
6. **Connection Issues**: Verify device is connected via the web interface controls

For T-Dongle C5-specific issues, see the
[T-Dongle C5 troubleshooting](#t-dongle-c5-troubleshooting) table.

### Debug Information
- **Web Dashboard**: Real-time status and connection monitoring at `http://localhost:5000`
- **Serial Terminal**: Live device output in the web interface
- **Channel Hopping**: Logs channel changes for debugging
- **Detection Logs**: Full JSON output for analysis

## Legal and Ethical Considerations

### Intended Use
- **Research and Education**: Understanding surveillance technology
- **Security Assessment**: Evaluating privacy implications
- **Technical Analysis**: Studying wireless communication patterns

### Compliance
- **Local Laws**: Ensure compliance with local regulations
- **Privacy Rights**: Respect individual privacy and property rights
- **Authorized Use**: Only use in authorized locations and situations

## Credits and Research

### Research Foundation
This project is based on extensive research and public datasets from the surveillance detection community:

- **[DeFlock](https://deflock.me)**: Crowdsourced ALPR location and reporting tool
  - GitHub: [FoggedLens/deflock](https://github.com/FoggedLens/deflock)
  - Provides comprehensive datasets and methodologies for surveillance device detection
  - **Datasets included**: Real-world device signatures from deflock.me are included in the `datasets/` folder

- **[GainSec](https://github.com/GainSec)**: OSINT and privacy research
  - Specialized in surveillance technology analysis and detection methodologies
  - **Research referenced**: Some methodologies are based on their published research on surveillance technology
  - **Raven UUID Dataset Provider**: Contributed the `raven_configurations.json` dataset containing verified BLE service UUIDs from SoundThinking/ShotSpotter Raven devices across firmware versions 1.1.7, 1.2.0, and 1.3.1
  - Enables precise detection of Raven acoustic gunshot detection devices through BLE service UUID fingerprinting

### Methodology Integration
Flock You unifies multiple known detection methodologies into a comprehensive scanner/wardriver specifically designed for Flock Safety cameras and similar surveillance devices. The system combines:

- **WiFi Promiscuous Monitoring**: Based on DeFlock's network analysis techniques
- **BLE Device Detection**: Leveraging GainSec's Bluetooth surveillance research
- **MAC Address Filtering**: Using crowdsourced device databases from deflock.me
- **BLE Service UUID Fingerprinting**: Identifying Raven devices through advertised service characteristics
- **Firmware Version Detection**: Analyzing service combinations to determine device capabilities
- **Pattern Recognition**: Implementing research-based detection algorithms

### Acknowledgments
Special thanks to the researchers and contributors who have made this work possible through their open-source contributions and public datasets:

- **GainSec** for providing the comprehensive Raven BLE service UUID dataset, enabling detection of SoundThinking/ShotSpotter acoustic surveillance devices
- **DeFlock** for crowdsourced surveillance camera location data and detection methodologies
- The broader surveillance detection community for their continued research and privacy protection efforts

This project builds upon their foundational work in surveillance detection and privacy protection.

### Purchase Information
**Oui-Spy devices are available exclusively at [colonelpanic.tech](https://colonelpanic.tech)**

## License

This project is provided for educational and research purposes. Please ensure compliance with all applicable laws and regulations in your jurisdiction.

**Flock You: Professional surveillance detection for the privacy-conscious**
