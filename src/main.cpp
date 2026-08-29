/**
 * @file ESP32-C3-EV-Charger-HA-MQTT.ino
 * @brief Main control module for the ESP32-C3 AC power monitor.
 *
 * @details
 * Coordinates all major subsystems: ADC sampling and RMS/power computation,
 * Wi-Fi and OTA services, MQTT discovery/telemetry for Home Assistant, and
 * the bicolor LED status indicator. Defines the overall setup/loop behavior.
 *
 * The firmware supports two compile-time modes, selected by defining
 * CALIBRATE in config.h:
 *
 *  - **Normal mode:**
 *    Connects to Wi-Fi and MQTT, publishes measurements at an interval that
 *    adapts to load (LOAD_THRESHOLD_A), and reflects idle/charging state on
 *    the LED (steady green = idle, slow green = charging).
 *
 *  - **Calibration mode:**
 *    Disables Wi-Fi and MQTT, streams raw and scaled readings to serial
 *    every CAL_INTERVAL_MS for bench calibration, and flashes the LED red
 *    to indicate calibration activity.
 *
 * loop() is fully non-blocking: measurement, MQTT servicing, and
 * ArduinoOTA handling all run every pass. Two independent TickTwo objects
 * schedule periodic work without delay():
 *
 *  - **publishTicker:**
 *    Fires publishReading() at IDLE_INTERVAL_MS or ACTIVE_INTERVAL_MS,
 *    depending on load. On a charging-state transition, loop() updates the
 *    interval, restarts the ticker so its timing realigns with the
 *    transition, and publishes a reading immediately so the state change
 *    is reported without waiting for the next tick.
 *
 *  - **slowFlash / fastFlash (alertFlash.cpp):**
 *    Drive the LED's 500 ms and 150 ms flash patterns; advanced each pass
 *    via updateBicolorLed().
 *
 * @author Karl Berger with Claude
 * @date 2026.08.29
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

// Ticker that drives adaptive-interval publishReading() calls
TickTwo publishTicker(publishReading, IDLE_INTERVAL_MS, 0, MILLIS);

// ─── Setup ──────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);                         // Start USB serial
  delay(1000);                                  // Allow ESP32-C3 USB enumeration
  Serial.println("\n=== AC Power Monitor ==="); // Startup banner
  initBicolorLed();                             // Prepare LED subsystem

#ifdef CALIBRATE                    // Calibration mode selected at compile time
  setBicolorLedState(LED_RED_SLOW); // Indicate calibration mode
#else                               // Normal operating mode
  wifiConnect();                            // Establish Wi-Fi connection
  initOTA();                                // Enable OTA update handling
  mqtt.setServer(MQTT_HOST, MQTT_PORT);     // Configure MQTT broker
  mqtt.setKeepAlive(MQTT_KEEPALIVE_S);      // Set keep-alive interval
  mqtt.setBufferSize(MQTT_MAX_PACKET_SIZE); // Enlarge buffer for HA discovery payloads
  delay(300);                               // Let the 5 V rail settle before ADC sampling
  measure();                                // First pass: seeds offsetV/offsetI (reading is invalid)
  measure();                                // Second pass: uses the real offsets, so reading is now valid AC-only
  mqttConnect();                            // Connect to Home Assistant
  publishReading();                         // Publish immediately so fw version is confirmed on every boot
  publishTicker.start();                    // Start ticker at the idle interval
#endif
} // setup()

// ─── Loop ───────────────────────────────────────────────────────────────────
void loop() {
#ifdef CALIBRATE
  static unsigned long lastCalMs = 0;
  updateBicolorLed(); // Advance the LED_RED_SLOW flash pattern
  measure();          // Sample continuously
  if (millis() - lastCalMs >= CAL_INTERVAL_MS) {
    lastCalMs = millis();
    Serial.printf("vrms_mv=%.2f vrms=%.2f irms_mv=%.2f irms=%.2f\n", vrms_mv, reading.vrms,
                  irms_mv, reading.irms);
  }
  return; // Skip Wi-Fi/MQTT entirely during calibration
#endif

  if (WiFi.status() != WL_CONNECTED)
    wifiConnect(); // Reconnect Wi-Fi if dropped
  if (!mqtt.connected())
    mqttConnect();     // Reconnect MQTT if dropped
  mqtt.loop();         // Service MQTT traffic and callbacks
  ArduinoOTA.handle(); // Service pending OTA requests
  updateBicolorLed();  // Advance the LED flash pattern
  measure();           // Sample continuously

  bool isActive = (reading.irms >= LOAD_THRESHOLD_A); // True when load current indicates charging
  setBicolorLedState(isActive ? LED_GREEN_SLOW : LED_GREEN_STEADY);

  static unsigned long lastActiveMillis = 0;
  static bool hadActivity = false;
  if (isActive) {
    lastActiveMillis = millis(); // refresh the hold on every sighting of real current
    hadActivity = true;
  }
  // Stay in the active reporting state through a finish-charge taper: hold for at
  // least one full idle interval since the last time current actually crossed threshold.
  bool reportActive =
      isActive || (hadActivity && (millis() - lastActiveMillis < ACTIVE_HOLD_MS));

  static bool wasActive = false;
  if (reportActive != wasActive) {
    unsigned long newInterval =
        reportActive ? ACTIVE_INTERVAL_MS : IDLE_INTERVAL_MS;
    publishTicker.interval(newInterval); // Switch to the interval for the new state
    publishTicker.stop();                // Reset ticker timing on state change
    publishTicker.start();               // Restart with the new interval
    publishReading();                    // Publish immediately on the transition
    wasActive = reportActive;            // Remember state for next comparison
  }

  publishTicker.update(); // Call publishReading() when the ticker fires

  // Liveness watchdog: bound tracks whichever interval is currently in effect,
  // so a stalled connection is caught in ~2 min while charging instead of up
  // to 11 min if the fixed idle-sized bound were used year-round.
  unsigned long livenessTimeout =
      (reportActive ? ACTIVE_INTERVAL_MS : IDLE_INTERVAL_MS) + LIVENESS_MARGIN_MS;
  if (millis() - lastSuccessfulPublish > livenessTimeout) {
    WiFi.disconnect(); // Force a reconnect sequence if MQTT has stalled
    wifiConnect();
  }
} // loop()