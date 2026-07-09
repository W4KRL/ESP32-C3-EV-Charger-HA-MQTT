/**
 * @file config.h
 * @brief Tunable parameters and shared data types for the AC power monitor.
 *
 * @details
 * Centralizes non-sensitive, per-deployment configuration: MQTT topic/client
 * naming, OTA hostname, ADC sampling and adaptive-reporting parameters,
 * voltage/current calibration constants (see the Calibration section below
 * for derivation and bench procedure), pin assignments, and the PowerReading
 * struct shared across ac_monitor.ino and mqttConnection.cpp.
 *
 * @warning Wi-Fi, MQTT, and OTA credentials live in secrets.h, which is
 * gitignored and must be created locally from secrets.h.example.
 *
 * @warning VOLTAGE_SCALE and CURRENT_SCALE must be calibrated against a
 * known reference (DMM/clamp meter) before trusting reported values --
 * see the Calibration section for the bench procedure.
 *
 * @author Karl Berger with Claude
 * @date 2026.07.08
 */

#pragma once

#include "secrets.h"  // WIFI_SSID, WIFI_PASS, MQTT_HOST, MQTT_USER, MQTT_PASS, OTA_PASSWORD

static const char* FW_VERSION = "1.0.4";  // bump on each release
// 1.0.0 2026.07.06 initial commit
// 1.0.1 2026.07.06
// 1.0.2 2026.07.07 rescaled CURRENT_SCALE due to error in windings
// 1.0.3 2026.07.08 added state_class to SensorDef for rssi
// 1.0.4 2026.07.08 returnd CURRENT_SCALE=0.03456f for 4-turns

// ─── MQTT ─────────────────────────────────────────────────────────────────────
#define MQTT_CLIENT_ID "ac_monitor_01"
#define MQTT_TOPIC_POWER "home/power/ac_monitor/state"
#define MQTT_TOPIC_STATUS "home/power/ac_monitor/status"
static const int MQTT_PORT = 1883;

// ─── OTA ──────────────────────────────────────────────────────────────────────
#define OTA_HOSTNAME "ac-monitor-01"

// ─── Power Line ───────────────────────────────────────────────────────────────
static const float LINE_FREQUENCY = 60.0f;  // Hz – use local utility frequency
static const int SAMPLE_CYCLES = 50;        // number of cycles per measurement burst (~833ms @ 60 Hz)
static const int SAMPLE_INTERVAL_US = 400;  // microseconds between ADC samples (~2500 samples/sec, 41/cycle)

// ─── Adaptive reporting ───────────────────────────────────────────────────────
static const float LOAD_THRESHOLD_A = 1.0f;                       // amps — switches report interval
static const unsigned long IDLE_INTERVAL_MS = 2UL * 60 * 1000;    // 2 min when I < threshold
static const unsigned long ACTIVE_INTERVAL_MS = 1UL * 30 * 1000;  // 30 sec when I >= threshold
static const float CURRENT_DEADBAND_A = 0.2f;                     // below this, treat current/power as zero

// ─── Device Connections ───────────────────────────────────────────────────────
static const int PIN_VOLTAGE = 0;  // ADC1_CH3
static const int PIN_CURRENT = 3;  // ADC1_CH0

// ─── Calibration ──────────────────────────────────────────────────────────────
//#define CALIBRATE  // uncomment for calibration
const unsigned long CAL_INTERVAL_MS = 10000;
static const float VOLTAGE_SCALE = 1.0f;      // ← MUST calibrate before use
static const float CURRENT_SCALE = 0.03456f;  // ← MUST calibrate before use
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

// ─── Data Types ───────────────────────────────────────────────────────────────
// AC power measurement – updated by measure(), published by publishReading()
struct PowerReading {
  float vrms;       // RMS voltage (V)
  float irms;       // RMS current (A)
  float apparent;   // Apparent power (VA)
  float realPower;  // Real power (W)
  float pf;         // Power factor (0.0 - 1.0)
};

inline PowerReading reading;  // single global instance — defined here, used everywhere config.h is included

// ─── secrets.h template ───────────────────────────────────────────────────────
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
