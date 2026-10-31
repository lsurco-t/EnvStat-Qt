# EnvStat Qt

## Project overview

EnvStat Qt is a cross-platform desktop application for monitoring an ESP32
temperature and humidity sensor over a USB/UART serial connection.

The application will be built with modern C++17, Qt 6, and CMake. It will
provide a visual dashboard for the readings produced by the
[`wifi_temperature_IOT`](../wifi_temperature_IOT/) firmware, while also
displaying device and connection logs.

The project is intended to demonstrate practical skills in:

- Embedded hardware and desktop software integration
- Asynchronous I/O and thread-safe GUI updates
- Serial protocol design and incremental parsing
- Error handling and connection recovery
- Separation of device, application, and presentation concerns
- Cross-platform C++ application development

## Current firmware behavior

The ESP32 firmware currently:

1. Starts at 115200 baud.
2. Connects to Wi-Fi.
3. Reads temperature and humidity from a DHT11 sensor on GPIO 4.
4. Prints status messages and a measurement to the serial port.
5. Sends Telegram alerts when the temperature falls below 20 °C or returns
   to normal.
6. Disconnects Wi-Fi and enters deep sleep for approximately 60 seconds.

The current measurement line is human-readable and has the following shape:

```text
Temperature: 23.5 °C Humidity: 48.0 %
```

The firmware also emits status messages such as:

```text
Waking up... Connecting to wi-fi...
Wi-fi connected!
Failed to read from DHT sensor!
Going into deep sleep for 60 second
```

Because the device enters deep sleep after each reading, this is a periodic
telemetry source rather than a continuous real-time stream. The desktop
application should therefore expect approximately one reading per minute,
plus connection and boot messages.

## Proposed architecture

The application should use three layers with explicit responsibilities:

```text
+------------------------------------------------------------+
| Presentation                                               |
| MainWindow, charts, current values, status, event console  |
+------------------------------------------------------------+
                           ^
                           | Qt signals and slots
+------------------------------------------------------------+
| Application / device service                               |
| SerialWorker, framing, parsing, validation, reconnection  |
+------------------------------------------------------------+
                           ^
                           | QSerialPort, 115200 baud
+------------------------------------------------------------+
| ESP32 firmware                                             |
| DHT11 measurement, Wi-Fi/Telegram status, deep sleep      |
+------------------------------------------------------------+
```

### Device and serial layer

`SerialWorker` should own the `QSerialPort` instance and run in a dedicated
`QThread` or equivalent asynchronous execution context. It should:

- Open and close the selected serial port.
- Read arbitrary-sized byte chunks without assuming one read equals one line.
- Keep a persistent buffer and process complete newline-delimited records.
- Preserve incomplete records until more bytes arrive.
- Emit parsed measurements, status messages, and errors through typed signals.
- Detect disconnections and report them explicitly.
- Avoid blocking the GUI thread.

`QSerialPort`'s ready-read signals are preferable to repeatedly polling with
`waitForReadyRead()`. If blocking calls are used, they must remain inside the
worker thread and have bounded timeouts.

### Application layer

The application layer should convert device events into a stable internal
model, for example:

```cpp
struct Measurement
{
    QDateTime receivedAt;
    double temperatureCelsius;
    double relativeHumidity;
};
```

It should validate ranges and reject malformed or incomplete measurements
without crashing. It should also track connection state separately from device
state, since a connected serial port does not guarantee that the ESP32 is
awake or that a sensor reading succeeded.

### Presentation layer

`MainWindow` should remain responsible for presentation only. It should
display:

- The latest temperature and humidity values
- A history chart of received measurements
- The time of the last valid reading
- Connection state and selected port
- Visual threshold warnings
- A timestamped device/event console
- Explicit error messages and reconnect controls

The chart should account for the roughly one-minute sampling interval and
should not imply that measurements exist between received samples.

## Initial user experience

The first usable version should provide:

1. A port selector populated from available serial devices.
2. Connect and disconnect controls.
3. A baud-rate setting defaulting to `115200`.
4. Large current-value cards for temperature and humidity.
5. A line chart of measurements received during the session.
6. A last-reading timestamp and stale-data indicator.
7. A scrolling console for device status and parsing errors.
8. A clear warning when temperature is below the configured threshold.

The stale-data indicator is important: after a device enters deep sleep, the
absence of serial traffic is expected for a period. The UI should distinguish
that expected interval from an unexpected disconnect or a failed sensor read.

## Serial protocol recommendation

Parsing the current human-readable output is acceptable for a prototype, but
it is fragile. Labels, spacing, Unicode degree symbols, and localization
changes can all break a regular-expression parser.

For a reliable application, add a machine-readable record to the firmware.
For example:

```text
DATA,temperature_c=23.5,humidity_pct=48.0
```

or, preferably, a compact JSON line:

```json
{"type":"measurement","temperature_c":23.5,"humidity_pct":48.0}
```

Every record should end with `\n`. If the protocol is changed, retain
human-readable status lines for diagnostics and use an explicit record type
for measurements. A future protocol could also include:

- Device uptime or boot identifier
- Firmware version
- Measurement sequence number
- Sensor-read success or error code
- Device-side timestamp, if time synchronization is added

The desktop application should continue to handle the current output while
the protocol is being improved, but the parser should be isolated so that this
transition does not affect the UI.

## Recommended project structure

```text
EnvStat-Qt/
├── CMakeLists.txt
├── src/
│   ├── main.cpp
│   ├── app/
│   │   ├── Measurement.h
│   │   └── MeasurementStore.cpp
│   ├── device/
│   │   ├── SerialWorker.cpp
│   │   ├── SerialWorker.h
│   │   ├── TelemetryParser.cpp
│   │   └── TelemetryParser.h
│   └── ui/
│       ├── MainWindow.cpp
│       └── MainWindow.h
└── tests/
    ├── tst_TelemetryParser.cpp
    └── tst_MeasurementStore.cpp
```

The exact structure can change, but serial access, parsing, storage, and UI
code should remain independently testable.

## Implementation roadmap

### Phase 1: Minimal working connection

- Create the Qt 6/CMake application.
- Enumerate serial ports.
- Connect to the selected port at 115200 baud.
- Show raw incoming lines in the event console.
- Implement clean disconnect and error reporting.

### Phase 2: Parsing and live values

- Add a line-buffered parser for the current firmware output.
- Emit validated `Measurement` values.
- Display the latest temperature, humidity, and reading time.
- Add parser unit tests for complete, fragmented, malformed, and unrelated
  lines.

### Phase 3: History and visual status

- Store measurements for the current session.
- Add a time-based temperature and humidity chart.
- Add threshold and stale-data indicators.
- Keep the UI responsive while the device is disconnected or sleeping.

### Phase 4: Protocol and reliability improvements

- Add machine-readable measurement records to the ESP32 firmware.
- Add automatic reconnect or a user-controlled reconnect action.
- Handle device resets, partial lines, invalid UTF-8, and unexpected port
  removal.
- Add bounded history retention and optional export.

### Phase 5: Packaging and verification

- Build on Linux and Windows, with macOS as an optional target.
- Document Qt, CMake, and compiler prerequisites.
- Add a testable serial-input simulation so most tests do not require hardware.
- Package a release build and document device permissions on Linux.

## Assessment

### What is already on the right track

- The hardware and desktop application have a clear integration boundary.
- Separating serial I/O from the GUI is the correct Qt architecture.
- Line buffering is necessary and is correctly identified as a requirement.
- Treating device logs separately from measurements is useful for diagnostics.
- CMake and Qt 6 are appropriate choices for a maintainable cross-platform
  application.
- The project has a credible connection to instrumentation and laboratory
  software concerns.

### Important corrections to the original plan

- The ESP32 does not provide continuous telemetry. It produces one reading
  approximately every 60 seconds because of deep sleep.
- A desktop application cannot assume that a measurement is immediately
  available after opening the port.
- The current text format is suitable for an initial prototype but should not
  be the long-term protocol.
- `QSerialPort` should normally be event-driven rather than polled from the
  GUI thread.
- A chart requires session history and timestamps; the current firmware
  provides neither a device timestamp nor a sequence number, so the first
  version should timestamp data on receipt.
- The GUI must report failures explicitly: unavailable ports, permission
  errors, device removal, failed sensor reads, malformed lines, and stale data.

## Overall conclusion

This is a good and realistic direction for a Qt/C++ project. The proposed
layering and technical goals are strong, and the existing ESP32 firmware gives
the desktop application a concrete data source.

The main risk is not the Qt GUI; it is the boundary between a sleeping
embedded device and a desktop application that expects live data. Start with a
small serial console and parser, make the periodic nature of the device
visible in the UI, and then add the chart and visual alerts. Introducing an
explicit machine-readable protocol before adding advanced features will make
the project substantially more reliable and easier to demonstrate.
