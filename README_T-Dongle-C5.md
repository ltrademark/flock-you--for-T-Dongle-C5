# Flock You — LilyGO T-Dongle C5

A port of [`flock-you`](https://github.com/spuder/flock-you) to the **LilyGO T-Dongle C5**
(Espressif **ESP32-C5**: single-core RISC-V, dual-band 2.4/5 GHz Wi-Fi 6, BLE 5). It scans
for Flock Safety cameras / "Penguin" / "Pigvision" devices and Raven (ShotSpotter/SoundThinking)
gunshot detectors over Wi-Fi (promiscuous sniffing) and BLE, then alerts on the on-board TFT
screen and RGB LED and prints JSON detection records over USB serial.

This document is specific to the **T-Dongle C5**. For the other supported boards
(Xiao ESP32-S3/C3, T-Dongle S3, ESP32-S3 SuperMini) see the main [`README.md`](README.md).

---

## Is it plug-and-play?

**Yes.** Once flashed, the firmware lives in the dongle's on-board 16 MB flash — it is
permanent and survives power loss. Plug the dongle into **any USB power source** (another
computer, a USB wall charger, a battery bank) and it boots in ~1 second and starts scanning
on its own. It does **not** need this computer, WSL, PlatformIO, or a serial connection to
run — those are only needed to *flash* it or to *read* the JSON output. The on-screen alerts
and LED strobe work standalone.

---

## Hardware summary

| | |
|---|---|
| MCU | ESP32-C5 (RISC-V, 240 MHz), 16 MB flash, 8 MB PSRAM |
| Radios | Dual-band Wi-Fi 6 (2.4 + 5 GHz), Bluetooth LE 5 |
| Display | 0.96" ST7735 80×160 IPS TFT |
| RGB LED | 1× APA102 (data = GPIO5, clock = GPIO4) |
| Button | BOOT (GPIO28) |
| USB | Native USB Serial/JTAG (no UART bridge chip); Type-A dongle |

Pin map (from LilyGO's `pin_config.h`): LCD CS=10, DC=3, RST=1, MOSI=2, SCLK=6, MISO=7,
BL=0 (active-LOW). These are set in `src/display.cpp` / `src/rgb_led.cpp`.

---

## Flashing

You only need to do this once. There are three routes — pick the one that fits your setup.
A pre-built image is already included at **`firmware/t_dongle_c5-factory.bin`**, so routes A
and C don't require building anything.

### Route A — Browser flasher (easiest, no installs)

Works from **Windows/macOS/Linux** in Chrome or Edge; no toolchain needed.

1. Open <https://espressif.github.io/esptool-js/>
2. Plug in the T-Dongle C5. Click **Connect** and pick its port
   (it appears as *"USB JTAG/serial debug unit"*).
3. Set **Flash Address** to `0x0` and choose the file
   `firmware/t_dongle_c5-factory.bin` from this repo.
4. Click **Program**. When it finishes, unplug and replug.
5. If Connect fails: hold the **BOOT** button while plugging in, then retry.

> Note: some versions of the fancier [ESP Web Tools](https://esptool.github.io/) installer
> don't yet list `ESP32-C5` as a chip family. `esptool-js` (the link above) always works —
> it just writes the raw image at `0x0`.

### Route B — PlatformIO (build + flash from source)

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

### Route C — esptool directly

```bash
pip install esptool
esptool --chip esp32c5 --baud 460800 write-flash 0x0 firmware/t_dongle_c5-factory.bin
```

### Flashing from WSL (Linux on Windows)

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

---

## Monitoring (reading detections)

The firmware prints boot logs, channel-hop status, and one JSON record per detection at
**115200 baud** over the native USB serial port.

**PlatformIO monitor** (from a real terminal):
```bash
pio device monitor -e t_dongle_c5
```

**Any serial terminal** works too — point it at the dongle's port at 115200 8N1
(`screen /dev/ttyACM0 115200`, PuTTY, the Arduino IDE monitor, the `esptool-js` console, etc.).

> **The ESP32-C5's hardware USB serial only transmits once the host opens the port with DTR
> asserted.** Proper terminals (PlatformIO, screen, PuTTY, Arduino IDE) do this automatically.
> A plain `cat /dev/ttyACM0` will show *nothing* because it doesn't assert DTR — that's not a
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

---

## Web dashboard (localhost:5000)

The repo ships an optional Flask + WebSocket dashboard in [`api/`](api/) that reads the
device's serial output and shows detections in a browser in real time — a live serial
terminal, a detections table with vendor lookup and per-MAC counts, stats, and CSV/KML
export. It runs entirely on your own machine; the dongle only talks to the PC over USB.

### 1. Install (one time)

Requires Python 3.8+. From the repo root:

```bash
cd api
python3 -m venv venv
./venv/bin/pip install -r requirements.txt
```

> On Debian/Ubuntu/WSL, `python3 -m venv` may fail with *"ensurepip is not available"*.
> If so, install the `python3-venv` package (`sudo apt install python3-venv`), or use
> virtualenv instead: `pip install --user virtualenv && python3 -m virtualenv venv`.

### 2. Run

```bash
./venv/bin/python flockyou.py
```

(The `api/README.md` says `python app.py`, but the actual entrypoint is `flockyou.py`.)
The server listens on `0.0.0.0:5000`.

### 3. Open and connect

1. Browse to **http://localhost:5000**. (On WSL, the Windows browser reaches it fine —
   WSL2 forwards `localhost`.)
2. In the UI, pick the dongle's serial port from the port dropdown — it shows up as
   **"USB JTAG/serial debug unit"** (`/dev/ttyACM0` on Linux/WSL, `COMx` on Windows) — and
   click **Connect**.
3. The **Serial Terminal** tab immediately shows the live feed (`[WiFi] Hopped to channel N`,
   `[BLE] scan...`). The **Detections** view fills in as devices are found.

### Seeing content without a real camera

Detections only appear when a matching device is actually in range. To see the UI populate
in the meantime, use the built-in **test detection** — a button in the UI, or via curl:

```bash
curl -X POST http://localhost:5000/api/test/detection \
  -H "Content-Type: application/json" \
  -d '{"protocol":"wifi","detection_method":"beacon","ssid":"FlockSafety-TEST","mac_address":"58:8e:81:12:34:56","rssi":-48,"channel":6,"threat_score":100}'
```

### Notes

- **One reader at a time.** While the dashboard is connected it owns the serial port, so a
  separate `pio device monitor` / terminal can't read it simultaneously (and vice-versa).
- The dashboard asserts DTR when it opens the port, so the C5's USB serial streams correctly
  (a plain `cat` would show nothing — see the Monitoring section).
- **WSL:** if you replug the dongle, re-run `sudo chmod 666 /dev/ttyACM0` and reconnect in
  the UI. If flashing from Windows instead, detach it from WSL first (`usbipd detach`).
- GPS is optional and separate — the dongle has no GPS, so leave the GPS port unset.

## SD card logging (standalone)

Insert a FAT32-formatted microSD card and the dongle logs every device it detects
to **`/flock_log.csv`** on the card — no computer needed. This makes it a portable
wardriving-style logger: carry it around on USB power, then pull the card and read the
file later. **No card is required** — without one, the device just runs normally and the
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
- The `latitude,longitude,altitude_m,accuracy_m` columns are intentionally **left blank** —
  the C5 has no GPS. They're already in the header so the file is map-ready: you can fill
  them in later by time-correlating with a phone GPS track, or convert the CSV to GPX/GeoJSON
  or the WiGLE upload format once you have coordinates.

> Formatting: use a plain **FAT32** card (cards up to 32 GB work best). If a card isn't
> detected, reformat it FAT32 and reinsert.

## On-device behavior (no computer needed)

- **Boot:** screen shows a "FLOCK YOU" splash, then a cyan **SCANNING** screen with the
  current Wi-Fi channel and BLE status. The LED does a quick red→blue→cyan self-test.
- **Detection:** red alert screen + red/blue LED strobe (~3 s), then a slow orange
  "in range" heartbeat pulse while the device stays nearby.
- **Out of range:** after 30 s with no re-detection it returns to the scanning screen.

---

## Options

### 5 GHz scanning (experimental)

The C5 is dual-band, but 5 GHz promiscuous capture on the current core is still maturing, so
the firmware hops **2.4 GHz channels (1–13) only** by default. To also sweep the common 5 GHz
channels, uncomment this line in the `[env:t_dongle_c5]` section of `platformio.ini` and
re-flash:

```ini
    -DENABLE_5GHZ_SCAN=1
```

If 5 GHz isn't supported on your core build, those channel sets simply fail to tune (harmless)
and 2.4 GHz scanning continues normally.

---

## Troubleshooting

| Symptom | Fix |
|---|---|
| Board won't enter flashing / `Connect` fails | Hold **BOOT** (GPIO28) while plugging in, then flash again. |
| No serial output | Use a real terminal (PlatformIO/screen/PuTTY), not `cat` — DTR must be asserted. Confirm 115200 baud. |
| Screen text mirrored / offset / wrong colors | Adjust `setRotation()` or the `initR(INITR_MINI160x80_PLUGIN)` variant in `src/display.cpp`. The panel is identical to the T-Dongle S3. |
| Nothing on screen at all | Backlight is active-LOW on GPIO0; verify `TFT_BL` handling in `src/display.cpp`. |
| WSL: `/dev/ttyACM0` missing | `sudo modprobe cdc_acm` after `usbipd attach`. |
| WSL: permission denied on port | `sudo chmod 666 /dev/ttyACM0` (repeat after each replug). |
| PlatformIO can't find the C5 platform | The `[env:t_dongle_c5]` env uses the pioarduino fork; upstream `espressif32` does **not** support the C5. Don't change the `platform =` URL. |
| SD card not logging | Check the serial console at boot for `[SD] card mounted` vs `[SD] no card / mount failed`. Reformat the card as **FAT32** (≤32 GB) and reseat it. The card is optional — the scanner works without it. |

---

## Build/toolchain notes

- **Platform:** [pioarduino](https://github.com/pioarduino/platform-espressif32) fork of
  `platform-espressif32` (upstream has no ESP32-C5 support). Pinned via the `platform =` URL
  in `platformio.ini`.
- **Board definition:** vendored at `boards/Lilygo-T-Dongle-C5.json`.
- **BLE:** NimBLE-Arduino **2.x** (required by the Arduino 3.x / IDF 5.x core; 2.x renamed the
  scan-callback API and switched scan durations to milliseconds — handled behind `USE_NIMBLE_V2`).
- **RGB LED:** Pololu **APA102** library, vendored at `lib/APA102/` (FastLED has no C5 pin map yet).
- **Display:** Adafruit ST7735 + GFX over the C5's FSPI bus.
