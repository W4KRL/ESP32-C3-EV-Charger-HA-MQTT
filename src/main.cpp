/**
 * @file ESP32-C3-EV-Charger-HA-MQTT.ino
 * @brief Main control module for the ESP32‑C3 AC power monitor.
 *
 * @details
 * Coordinates all major subsystems — ADC sampling and RMS/power computation,
 * Wi‑Fi and OTA services, MQTT discovery/telemetry for Home Assistant, and the
 * bicolor LED status indicator — and defines the overall setup/loop behavior.
 *
 * The firmware supports two compile‑time modes selected via CALIBRATE in
 * config.h:
 *
 *  - **Normal mode:**
 *    Connects to Wi‑Fi and MQTT, publishes measurements at an interval that
 *    adapts to load (LOAD_THRESHOLD_A), and reflects idle/active charging
 *    states on the LED (steady green = idle, slow green = charging).
 *
 *  - **Calibration mode:**
 *    Disables Wi‑Fi and MQTT, outputs raw and scaled readings to serial every
 *    CAL_INTERVAL_MS for bench calibration, and flashes the LED red to indicate
 *    calibration activity.
 *
 * The loop() function remains fully non‑blocking. Measurement, MQTT servicing,
 * and ArduinoOTA handling run continuously. Two independent TickTwo objects
 * manage scheduled tasks without delay():
 *
 *  - **publishTicker:**
 *    Triggers publishReading() at either IDLE_INTERVAL_MS or ACTIVE_INTERVAL_MS
 *    depending on load. When the charging state changes, loop() updates the
 *    interval, restarts the ticker to realign timing with the transition, and
 *    immediately publishes a reading so the state change is reported without
 *    delay.
 *
 *  - **slowFlash/fastFlash (alertFlash.cpp):**
 *    Drive the LED’s 500 ms and 150 ms flash patterns, updated via
 *    updateBicolorLed().
 *
 * @author Karl Berger with Claude
 * @date 2026.07.26
 */

// ─── Libraries ──────────────────────────────────────────────────────────────
#include "alertFlash.h"     // Bicolor LED status and alert patterns
#include "config.h"         // Credentials, tunable parameters, and globals
#include "measurement.h"    // ADC sampling and electrical parameter calculation
#include "mqttConnection.h" // MQTT connection and Home Assistant integration
#include "wifiConnection.h" // Wi-Fi connection and OTA setup helpers
#include <Arduino.h>        // Core Arduino library
#include <ArduinoOTA.h>     // OTA update service
#include <TickTwo.h>        // Non-blocking scheduler by Stefan Staub
#include <WiFi.h>           // Wi-Fi connectivity

// Ticker controlling adaptive publishReading() timing
TickTwo publishTicker(publishReading, IDLE_INTERVAL_MS, 0, MILLIS);

// ─── Setup ──────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);                         // Initialize USB serial
  delay(1000);                                  // Allow ESP32-C3 USB enumeration
  Serial.println("\n=== AC Power Monitor ==="); // Startup banner
  initBicolorLed();                             // Prepare LED subsystem

#ifdef CALIBRATE                    // Calibration mode selected at compile time
  setBicolorLedState(LED_RED_SLOW); // Indicate calibration mode
#else                               // Normal operating mode
  wifiConnect();                            // Establish Wi-Fi connection
  initOTA();                                // Enable OTA update handling
  mqtt.setServer(MQTT_HOST, MQTT_PORT);     // Configure MQTT broker
  mqtt.setKeepAlive(MQTT_KEEPALIVE_S);      // Keep-alive interval
  mqtt.setBufferSize(MQTT_MAX_PACKET_SIZE); // Larger buffer for HA discovery payloads
  measure();                                // Prime ADC offset values before first publish
  mqttConnect();                            // Connect to Home Assistant
  publishReading();                         // Immediate publish so fw version confirms on every boot
  publishTicker.start();                    // Start ticker at idle interval
#endif
} // setup()

// ─── Loop ───────────────────────────────────────────────────────────────────
void loop() {
#ifdef CALIBRATE
  static unsigned long lastCalMs = 0;
  updateBicolorLed(); // Maintain LED_RED_SLOW flash pattern
  measure();          // Continuous sampling
  if (millis() - lastCalMs >= CAL_INTERVAL_MS) {
    lastCalMs = millis();
    Serial.printf("vrms_mv=%.2f irms_mv=%.2f vrms=%.2f irms=%.2f\n", vrms_mv,
                  irms_mv, reading.vrms, reading.irms);
  }
  return; // Skip Wi-Fi/MQTT entirely during calibration
#endif

  if (WiFi.status() != WL_CONNECTED)
    wifiConnect(); // Auto-reconnect Wi-Fi
  if (!mqtt.connected())
    mqttConnect();     // Auto-reconnect MQTT
  mqtt.loop();         // MQTT traffic and callbacks
  ArduinoOTA.handle(); // OTA service handler
  updateBicolorLed();  // LED flash pattern updates
  measure();           // Continuous ADC sampling

  bool isActive = (reading.irms >= LOAD_THRESHOLD_A); // Charging state detection
  setBicolorLedState(isActive ? LED_GREEN_SLOW : LED_GREEN_STEADY);

  static bool wasActive = false;
  if (isActive != wasActive) {
    unsigned long newInterval =
        isActive ? ACTIVE_INTERVAL_MS : IDLE_INTERVAL_MS;
    publishTicker.interval(newInterval); // Update publish interval
    publishTicker.stop();                // Reset ticker timing on state change
    publishTicker.start();               // Restart with new interval
    publishReading();                    // Immediate publish on transition
    wasActive = isActive;                // Track previous state
  }

  publishTicker.update(); // Trigger publishReading() when ticker fires

  if (millis() - lastSuccessfulPublish > MQTT_LIVENESS_TIMEOUT_MS) {
    WiFi.disconnect(); // Force reconnect sequence if MQTT stalls
    wifiConnect();
  }
} // loop()
