# ESP32-C3 EV Charger AC Power Monitor

The ESP32-C3 EV Charger is an MQTT-based AC power monitor for a 240 Vac EV charging circuit,
built on an ESP32-C3 SupeMini. It publishes voltage, current, real power, apparent power (VA),
and power factor to Home Assistant via MQTT discovery.

## Hardware

- **ESP32-C3 SuperMini** development board with USB adapter and 3.3 Vdc regulator
- **ZMPT101B** voltage transformer module (5 V supply), for line voltage sensing
- Resistor divider to center dc bias and low pass filter for the voltage channel
- **Open-jaw current transformer**, 100 A, 3000:1, wired with 4 primary turns
  for an effective ratio of 750:1 and maximum current of 25 A
- Resistor divider to center dc bias, CT burden resistor, and low pass filter fot current channel
- Both channels scale sensor outputs to the ESP32-C3's 0 to 3.3 V ADC range
- **WX-DC12003** converter module provides 5 Vdc system power from the 240 Vac line
- **Bi-Color LED** for for operating and failure mode indication
- Stripboard construction, with high-voltage components physically grouped
  and separated from the low-voltage ESP32-C3/signal-conditioning section
- **NEMA 6-20R** 205 Vac, 20 A receptacle for EVSE
- **2-Gang PVC (Non-metallic) weatherproof Outlet Box** Hubbell Model #PDB77550GY
- **6-ft 12/3 SJTW Extension cord NEMA 6-20 R/R 20 A 250 V**
  
## Documentation

- [Schematic](hardware/schematic/ac_monitor_schematic.pdf)
- [Stripboard layout](hardware/stripboard/ac_monitor_stripboard.pdf)
- [Parts list](/hardware/parts_list.md)
- [Build photos](hardware/photos/)

### Pin assignments

| Signal      | Pin        | Notes                   |
|-------------|------------|-------------------------|
| Voltage     | GPIO0 (A0) | ZMPT101B divider output |
| Current     | GPIO3 (A3) | CT burden/bias node     |
| LED (red)   | GPIO5      | Red anode               |
| LED (green) | GPIO6      | Green anode             |

Use only GPIO0, GPIO1, GPIO3 and GPIO4 for ADC. 
Avoid GPIO2. It is a boot-mode strapping pin. 
Avoid GPIO5. It is on ADC2 shared with Wi-Fi.

## Features

- MQTT discovery — auto-registers all sensors in Home Assistant on connect,
  with RSSI, BSSID, and firmware version grouped under the Diagnostic category
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

- **Voltage:** validated via Variac sweep (0-145 V) against a true-RMS DMM,
  plus a cross-check at 240 V line voltage against the DMM, EVSE display,
  and dryer/EV splitter display
- **Current:** derived from CT ratio and measured burden resistance;
  pending real-load validation against the EVSE's displayed current

See the `CALIBRATE` mode in `config.h`/`ac_monitor.ino` for the bench
procedure.

## MQTT Topics

| Topic                                           | Purpose                           |
|-------------------------------------------------|-----------------------------------|
| `home/power/ac_monitor/state`                   | JSON payload: vrms, irms, watts, va, pf, rssi, fw, bssid |
| `home/power/ac_monitor/status`                  | `online` / `offline` (LWT)        |
| `homeassistant/sensor/ac_monitor_01/.../config` | HA discovery topics               |

## LED Indications

| Aspect       | Indication                       |
|--------------|----------------------------------|
| OFF          | No power                         |
| GREEN STEADY | Normal operation (idle)          |
| GREEN SLOW   | Charging active                  |
| GREEN FAST   | Connecting to Wi-Fi & MQTT       |
| RED STEADY   | Not used                         |
| RED SLOW     | CALIBRATE mode                   |
| RED FAST     | Wi-Fi or MQTT connection failure |

## Build

Arduino IDE 2.3.10 (PlatformIO has ESP32-C3 compatibility issues at time of
writing). Requires:

- `PubSubClient` by Nick O'Leary — note: `MQTT_MAX_PACKET_SIZE` is set to 512 in
  `mqttConnection.h` (before the library include) to accommodate HA
  discovery payloads; the library default of 256 bytes silently truncates
  larger publishes
- `ArduinoJson` by Benoit Blanchon
- `TickTwo` by Stefan Staub
- ESP32 board package (native USB support for the C3)
- Schematic drawn in [ExpressSCH Plus](https://expresssch.apponic.com/}
- Stripboard design in [Lochmaster 4 - free viwer available]([https://www.electronic-software-shop.com/lng/en/electronic-software/](https://www.electronic-software-shop.com/lng/en/electronic-software/lochmaster-40.html))

## Safety

This device is powered by and senses a live 240 V circuit. The ZMPT101B and current
transformer provide galvanic isolation, but standard mains safety practice
still applies: de-energize the circuit before wiring or troubleshooting, verify low-voltage
electronics separately before re-energizing, and maintain proper
creepage/clearance between high- and low-voltage sections of the board and receptacle.

## Author

Karl Berger (W4KRL), with Claude
