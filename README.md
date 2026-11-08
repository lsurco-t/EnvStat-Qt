# EnvStat Qt

A desktop dashboard that monitors an ESP32 temperature and humidity sensor
over a USB/UART serial connection. Built with C++20, Qt 6 Widgets and CMake.

It reads the serial output of the companion `wifi_temperature_IOT` firmware
(ESP32 + DHT11), shows the latest readings, plots the session history and
displays the device's status and error events.

## Features

- Port and baud-rate selection, with connect and disconnect
- Large temperature and humidity cards, with a visual alert when the
  temperature falls below a configurable threshold
- History chart of the session (temperature and humidity, custom `QPainter`
  widget). Only received samples are marked, and the line is broken across
  gaps longer than three minutes
- Event console for device status and errors, colour-coded and bounded to
  1000 lines
- Status bar with connection state, last reading time and a freshness
  indicator (up to date, overdue, stale)
- Handles unplugged devices and port errors without crashing
- Bounded in-memory history (1440 samples by default)

The ESP32 enters deep sleep after each reading, so it reports roughly once a
minute. The freshness indicator exists to make that quiet period visible.

## Architecture

```text
+--------------------------------------------------+
| UI            MainWindow, ChartWidget, style.qss |
+--------------------------------------------------+
                        ^  Qt signals and slots
+--------------------------------------------------+
| App           Measurement, MeasurementStore      |
+--------------------------------------------------+
                        ^
+--------------------------------------------------+
| Device        SerialWorker, TelemetryParser      |
+--------------------------------------------------+
                        ^  QSerialPort
                     ESP32 firmware
```

```text
src/
├── main.cpp
├── app/      Measurement.h, MeasurementStore.{h,cpp}
├── device/   SerialWorker.{h,cpp}, TelemetryParser.{h,cpp}
└── ui/       MainWindow.{h,cpp}, ChartWidget.{h,cpp}, style.qss, resources.qrc
tests/        tst_TelemetryParser.cpp, tst_MeasurementStore.cpp
```

- `SerialWorker` owns the `QSerialPort`, keeps a persistent buffer, splits
  newline-delimited records and emits typed signals.
- `TelemetryParser` and `MeasurementStore` live in a static library with no
  GUI dependency, so they are unit-tested without hardware.
- The UI uses Qt Widgets in C++ with a QSS stylesheet that follows the system
  palette (light and dark). No QML.

## Serial protocol

The application expects newline-terminated records:

```text
DATA,temperature_c=23.5,humidity_pct=48.0
STATUS,wifi_connecting
STATUS,wifi_connected
STATUS,sleeping,seconds=60
ERROR,dht_read_failed
```

Recognised events: `wifi_connecting`, `wifi_connected`,
`wifi_connection_failed`, `dht_read_failed`, `telegram_alert_sent`,
`telegram_all_clear_sent`, `telegram_alert_failed` and
`telegram_all_clear_failed`. Unrecognised event names are shown as
"Unknown device event". Lines that match nothing are ignored. Readings are
timestamped on receipt, because the firmware sends no timestamp.

## Build and run

Requirements: a C++20 compiler, CMake 3.15 or newer, and Qt 6 with the
Widgets, SerialPort and Test modules.

On Ubuntu:

```bash
sudo apt install qt6-base-dev qt6-serialport-dev
```

```bash
cmake -S . -B build-linux
cmake --build build-linux -j
./build-linux/EnvStat-Qt
```

Run the tests:

```bash
cd build-linux && ctest --output-on-failure
```

### Serial port access

- **Linux:** add your user to the `dialout` group (`sudo usermod -aG dialout $USER`)
  and log in again.
- **WSL2:** attach the USB device from Windows with
  [usbipd-win](https://github.com/dorssel/usbipd-win), for example
  `usbipd attach --wsl --busid <BUSID>`. The port then appears as
  `/dev/ttyUSB0`.

## Testing status

- Unit tests cover the telemetry parser and the measurement store.
- Manually tested with an ESP32 and DHT11 sensor through a CP2102 USB-UART
  bridge on Linux (WSL2).
- Not tested on Windows or macOS.
- `SerialWorker` and the UI are not covered by automated tests.

## Known limitations and improvements

- `SerialWorker` runs on the GUI thread. Moving it to a `QThread` is planned;
  `Measurement` is already declared as a Qt meta-type for queued signals.
- The port list is refreshed manually, with no hot-plug detection.
- No automatic reconnect after the device is unplugged.
- History is kept in memory only, with no export (CSV) and no persistence.
- Only the `DATA,...` measurement format is parsed. The older human-readable
  firmware output is not supported.
- No serial-input simulator yet, so the worker cannot be tested without
  hardware.
- Windows build and packaging are not verified.
