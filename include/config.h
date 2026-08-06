/**
 * @file config.h
 * @brief Tunable parameters and shared data types for the AC power monitor.
 *
 * @details
 * Centralizes non-sensitive credentials, specific build configuration:
 * MQTT topic/client naming, OTA hostname, ADC sampling and adaptive-reporting
 * parameters, voltage/current calibration constants (see the Calibration
 * section below for derivation and bench procedure), pin assignments, and the
 * PowerReading struct shared across ac_monitor.ino and mqttConnection.cpp.
 *
 * @warning Wi-Fi, MQTT, and OTA credentials live in secrets.h, which is
 * gitignored and must be created locally from secrets.h.example below.
 *
 * @warning VOLTAGE_SCALE and CURRENT_SCALE must be calibrated against a
 * known reference (DMM/clamp meter) before trusting reported values --
 * see the Calibration section for the bench procedure.
 *
 * @author Karl Berger with Claude
 * @date 2026.08.06
 */

#pragma once

#include "secrets.h" // WIFI_SSID, WIFI_PASS, MQTT_HOST, MQTT_USER, MQTT_PASS, OTA_PASSWORD

static const char *FW_VERSION = "1.3.3"; // bump on each release
// 1.0.0 2026.07.06 initial commit
// 1.0.1 2026.07.06 minor cleanup
// 1.0.2 2026.07.07 rescaled CURRENT_SCALE due to error in CT turns
// 1.1.0 2026.07.08 added state_class to SensorDef for rssi
// 1.1.1 2026.07.08 returned CURRENT_SCALE=0.03456f for CT 4-turns
// 1.2.0 2026.07.09 add LED mode indications
// 1.2.1 2026.07.10 moved measure() to measurement module
// 1.2.2 2026.07.14 improve WiFi connection loss response
// 1.3.0 2026.07.22 use TickTwo to drive publishReading and LED flashes, change
// 1.3.1 2026.07.24 fix micros() rollover in measure()
// 1.3.2 2026.08.01 reduce CURRENT_DEADBAND_A from 0.3f to 0.1f
// 1.3.3 2026.08.02 add double call to measure() in setup() to prime offsetV/offsetI before first valid reading

// ─── Wi-Fi Timeout ──────────────────────────────────────────────────────────
static constexpr unsigned long WIFI_CONNECT_TIMEOUT_MS = 15000; // 15 s WiFi connect timeout

// ─── MQTT ───────────────────────────────────────────────────────────────────
#define MQTT_CLIENT_ID "ac_monitor_01"
#define MQTT_TOPIC_POWER "home/power/ac_monitor/state"
#define MQTT_TOPIC_STATUS "home/power/ac_monitor/status"
static constexpr int MQTT_PORT = 1883;
static constexpr unsigned long MQTT_CONNECT_TIMEOUT_MS = 15000; // 15 s MQTT connect timeout
static constexpr int MQTT_KEEPALIVE_S = 60;                     // seconds between activity pings
inline unsigned long lastSuccessfulPublish = 0;                 // defined in mqttConnection.cpp. make it global here

// ─── OTA ────────────────────────────────────────────────────────────────────
#define OTA_HOSTNAME "ac-monitor-01"

// ─── Electrical Parameters ──────────────────────────────────────────────────
static constexpr float LINE_FREQUENCY = 60.0f; // Hz – use local utility frequency
static constexpr int SAMPLE_CYCLES = 50;       // number of cycles per measurement burst (~833ms @ 60 Hz)
static constexpr int SAMPLE_INTERVAL_US = 400; // microseconds between ADC samples (~2500 samples/sec, 41/cycle)

// ─── Adaptive reporting ─────────────────────────────────────────────────────
static constexpr float LOAD_THRESHOLD_A = 1.0f;                      // amps — switches report interval
static constexpr unsigned long IDLE_INTERVAL_MS = 10UL * 60 * 1000;  // 10 min when I < threshold
static constexpr unsigned long ACTIVE_INTERVAL_MS = 1UL * 60 * 1000; // 60 sec when I >= threshold
// MQTT_LIVENESS_TIMEOUT_MS must exceed the longest gap between publishes (idle
// interval), plus a safety margin
static constexpr unsigned long MQTT_LIVENESS_TIMEOUT_MS = IDLE_INTERVAL_MS + 60UL * 1000;
static constexpr float CURRENT_DEADBAND_A = 0.1f; // treat current & power as zero to mask noise

// ─── Device Connections ─────────────────────────────────────────────────────
static constexpr int PIN_VOLTAGE = 0;   // ADC1_CH0 from ZMPT101B voltage sense module
static constexpr int PIN_CURRENT = 3;   // ADC1_CH3 from Current Transformer
static constexpr int PIN_LED_RED = 5;   // GPIO5 bicolor LED
static constexpr int PIN_LED_GREEN = 6; // GPIO6 bicolor LED

// ─── Calibration ────────────────────────────────────────────────────────────
//  #define CALIBRATE  // uncomment for calibration
//
// NOTE: Use CALIBRATE only on a bench top with USB connection to the PC.
//
const unsigned long CAL_INTERVAL_MS = 10 * 1000; // interval between calibration output
static constexpr float VOLTAGE_SCALE = 1.0f;     // ← MUST calibrate before use
static constexpr float CURRENT_SCALE = 0.03456f; // ← MUST calibrate before use
// analogReadMilliVolts() returns calibrated mV (0–3100 mV at ADC_11db).
// These scales convert mV-rms at the ADC pin to real-world units.
//
// VOLTAGE_SCALE  =  V_mains_rms  /  V_adc_rms_mV
//   Example: ZMPT101B trimmed so 120 Vrms → 120.0 mV rms at ADC
//            VOLTAGE_SCALE = 120.0 / 120.0 = 1.000
//
// CURRENT_SCALE  =  I_load_A_rms  /  V_adc_rms_mV
//   Example: CT 750:1 21.7 Ohm burden (measured)
//            20 Arms / 750 = 0.0267 Arms
//            Vburden = 0.0267 Arms * 21.7 Ohms = 0.579 Vrms_mv
//            CURRENT_SCALE = 20.0A / 0.579 mV / 1000 = 0.03456
//   Simplified: CURRENT_SCALE = CTratio ÷ Rburden ÷ 1000
//
// Bench procedure:
//   1. Uncomment #define CALIBRATE above
//   2. Connect a known resistive load (space heater, incandescent bulb).
//   3. Measure true Vrms and Irms with a DMM / clamp meter.
//   4. Observe measured values on Serial Monitor
//   4. VOLTAGE_SCALE = DMM_Vrms / vrms_mv
//   5. CURRENT_SCALE = DMM_Irms / irms_mv

// ─── Data Types ─────────────────────────────────────────────────────────────
// AC power measurement – updated by measure(), published by publishReading()
struct PowerReading {
  float vrms;      // RMS voltage (V)
  float irms;      // RMS current (A)
  float va;        // Apparent power (VA)
  float realPower; // Real power (W)
  float pf;        // Power factor (0.0 - 1.0)
};

inline PowerReading reading; // single global instance — defined here, used
                             // everywhere config.h is included

// ─── secrets.h template ─────────────────────────────────────────────────────
/**
 * @brief Template for secrets.h — create this file locally; do not commit it.
 *
 * @details
 * This project keeps Wi-Fi, MQTT, and OTA credentials in a separate
 * secrets.h file so they are never committed to source control. Add
 * secrets.h to .gitignore, then create secrets.h alongside config.h with
 * the following contents (substitute your own values):
 *
 * @code
 * #pragma once
 *
 * // ─── WiFi ───────────────────────────────────────────────
 * #define WIFI_SSID "your_ssid_here"
 * #define WIFI_PASS "your_wifi_password_here"
 *
 * // ─── MQTT ───────────────────────────────────────────────
 * #define MQTT_HOST "192.168.x.x"
 * #define MQTT_USER ""  // leave empty if broker has no auth
 * #define MQTT_PASS ""
 *
 * // ─── OTA ────────────────────────────────────────────────
 * #define OTA_PASSWORD "choose_a_password_here"
 * @endcode
 */
