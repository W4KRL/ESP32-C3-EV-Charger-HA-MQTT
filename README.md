# ESP32-C3 EV Charger AC Power Monitor

An MQTT-based AC power monitor for a 240 V EV charging circuit, built on an ESP32-C3.
Publishes voltage, current, real power, apparent power, and power factor to
Home Assistant via MQTT discovery.

## Hardware

- **ESP32-C3** (native USB, ADC1 only — see Pin Assignments below)
- **ZMPT101B** voltage transformer module (5 V supply), for line voltage sensing
- **Open-jaw current transformer**, 100 A : 3000:1, wired with 4 primary turns
  for an effective ratio of 750:1
- Resistor divider (voltage channel) and burden resistor + bias network
  (current channel) scale both sensor outputs into the ESP32-C3's 0–3.3 V
  ADC range
- Stripboard construction, with high-voltage components physically grouped
  and separated from the low-voltage ESP32-C3/signal-conditioning section
  
## Documentation

- [Schematic](hardware/schematic/ac_monitor_schematic.pdf)
- [Stripboard layout](hardware/stripboard/ac_monitor_stripboard.pdf)
- [Parts list](parts_list.md)
- [Build photos](hardware/photos/)

### Pin assignments

| Signal  | Pin  | Notes                                  |
|---------|------|-----------------------------------------|
| Voltage | GPIO0 (A0) | ZMPT101B divider output           |
| Current | GPIO3 (A3) | CT burden/bias node               |

GPIO2 and GPIO4 are avoided — GPIO2 is a boot-mode strapping pin, and GPIO4
is part of the C3's default JTAG mapping.

## Features

- MQTT discovery — auto-registers all sensors in Home Assistant on connect,
  with RSSI and firmware version grouped under the Diagnostic category
- Last Will and Testament (LWT) for accurate availability reporting
- Adaptive publish interval — shorter interval while current is above a
  configurable load threshold, longer interval when idle
- Current deadband to suppress noise-floor readings as zero when idle
- Over-the-air (OTA) firmware updates
- Optional `CALIBRATE` build mode for bench calibration (serial-only output,
  no WiFi/MQTT)

## Configuration

Credentials are kept out of version control:

1. Copy `secrets.h.example` to `secrets.h`
2. Fill in your WiFi SSID/password, MQTT broker/credentials, and OTA password
3. `secrets.h` is gitignored and will not be committed

Non-sensitive parameters (MQTT topics, sampling, calibration constants, pin
assignments) live in `config.h`. See the comments there for calibration
constant derivation and bench procedure.

## Calibration

`VOLTAGE_SCALE` and `CURRENT_SCALE` in `config.h` must be calibrated against
a known reference before trusting reported values:

- **Voltage:** validated via Variac sweep (0–145 V) against a true-RMS DMM,
  plus a cross-check at 240 V line voltage against the DMM, EVSE display,
  and dryer/EV splitter display
- **Current:** derived from CT ratio and measured burden resistance;
  pending real-load validation against the EVSE's displayed current

See the `CALIBRATE` mode in `config.h`/`ac_monitor.ino` for the bench
procedure.

## MQTT Topics

| Topic                              | Purpose                          |
|-------------------------------------|-----------------------------------|
| `home/power/ac_monitor/state`       | JSON payload: vrms, irms, watts, va, pf, rssi, fw |
| `home/power/ac_monitor/status`      | `online` / `offline` (LWT)        |
| `homeassistant/sensor/ac_monitor_01/.../config` | HA discovery topics    |

## Build

Arduino IDE 2.3.10 (PlatformIO has ESP32-C3 compatibility issues at time of
writing). Requires:

- `PubSubClient` — note: `MQTT_MAX_PACKET_SIZE` is set to 512 in
  `mqttConnection.h` (before the library include) to accommodate HA
  discovery payloads; the library default of 256 bytes silently truncates
  larger publishes
- `ArduinoJson`
- ESP32 board package (native USB support for the C3)

## Safety

This device senses a live 240 V circuit. The ZMPT101B and current
transformer provide galvanic isolation, but standard mains safety practice
still applies: de-energize the circuit before wiring, verify low-voltage
electronics separately before re-energizing, and maintain proper
creepage/clearance between high- and low-voltage sections of the board.

## Author

Karl Berger (W4KRL), with Claude