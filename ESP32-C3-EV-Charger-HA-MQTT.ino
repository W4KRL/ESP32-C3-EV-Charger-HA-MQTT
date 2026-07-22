/**
 * @file ESP32-C3-EV-Charger-HA-MQTT.ino
 * @brief Top-level orchestration for the ESP32-C3 AC power monitor.
 *
 * @details
 * Wires together the project's modules — measurement (ADC sampling and
 * RMS/power calculation), MQTT (Home Assistant discovery and telemetry),
 * Wi-Fi/OTA, and the bicolor LED status indicator — and drives the main
 * setup/loop sequence.
 *
 * Two run modes, selected at compile time via CALIBRATE in config.h:
 *  - Normal mode: connects to Wi-Fi and MQTT, publishes readings at an
 *    interval that adapts to load (LOAD_THRESHOLD_A), and reflects
 *    idle/active state on the LED (steady green = idle, slow green =
 *    charging).
 *  - CALIBRATE mode: skips Wi-Fi/MQTT entirely, dumps raw and scaled
 *    readings to serial every CAL_INTERVAL_MS for bench calibration, and
 *    flashes the LED red to indicate calibration mode is active.
 *
 * loop() is non-blocking throughout: measure(), mqtt.loop(), and
 * ArduinoOTA.handle() run unthrottled on every pass. Two independent
 * TickTwo (Stefan Staub) objects handle everything that runs on a
 * schedule without ever calling delay():
 *  - publishTicker drives publishReading(), switching between
 *    IDLE_INTERVAL_MS and ACTIVE_INTERVAL_MS as load crosses
 *    LOAD_THRESHOLD_A. On every idle/active transition, loop() sets the
 *    new interval, stops and restarts the ticker (re-anchoring its
 *    internal timer to the moment of transition rather than a stale
 *    baseline), and calls publishReading() directly so the transition
 *    itself is reported immediately rather than waiting out the new
 *    interval.
 *  - slowFlash/fastFlash (in alertFlash.cpp) drive the bicolor LED's
 *    500 ms and 150 ms flash patterns, serviced via updateBicolorLed().
 *
 * @author Karl Berger with Claude
 * @date 2026.07.22
 */

// ─── Libraries ────────────────────────────────────────────────────────────────
#include <WiFi.h>            // Built-in Wi-Fi connection
#include <ArduinoOTA.h>      // Built-in Over-the-Air update service
#include <TickTwo.h>         // Ticker library by Stefan Staub https://github.com/sstaub/TickTwo
#include "config.h"          // Credentials, adjustable parameters and globals
#include "wifiConnection.h"  // Wi-fi connection and OTA handler
#include "mqttConnection.h"  // MQTT service
#include "alertFlash.h"      // Bicolor LED indicator for mode indications
#include "measurement.h"     // Read ADC and calculate electrical parameters

// instantiate Ticker for publishReading()
TickTwo publishTicker(publishReading, IDLE_INTERVAL_MS, 0, MILLIS);

// ─── Setup ────────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);                          // Serial Monitor for USB connection
  delay(1000);                                   // Allow ESP32-C3 USB to enumerate
  Serial.println("\n=== AC Power Monitor ===");  // Welcome
  initBicolorLed();                              // Prepare LED for mode alerts

#ifdef CALIBRATE                     // Run calibration mode if defined
  setBicolorLedState(LED_RED_SLOW);  // Indicate calibration mode
#else                                // else continue with normal start up
  wifiConnect();                             // Connect to local Wi-Fi
  initOTA();                                 // Initialize OTA
  mqtt.setServer(MQTT_HOST, MQTT_PORT);      // Initiaize HA MQTT host
  mqtt.setKeepAlive(MQTT_KEEPALIVE_S);       // Set activity ping
  mqtt.setBufferSize(MQTT_MAX_PACKET_SIZE);  // Discovery payloads ~420 bytes, default 256 too small
  measure();                                 // Warm-up: seed offsetV/offsetI before first real publish
  mqttConnect();                             // Connect to Home Assistant
  publishTicker.start();                     // Start ticker at idle interval
#endif
}  // setup()

// ─── Loop ─────────────────────────────────────────────────────────────────────
void loop() {
#ifdef CALIBRATE
  static unsigned long lastCalMs = 0;
  updateBicolorLed();  // sustain LED_RED_SLOW flash
  measure();
  if (millis() - lastCalMs >= CAL_INTERVAL_MS) {
    lastCalMs = millis();
    Serial.printf("vrms_mv=%.2f irms_mv=%.2f vrms=%.2f irms=%.2f\n",
                  vrms_mv, irms_mv, reading.vrms, reading.irms);
  }
  return;  // skip WiFi/MQTT entirely while calibrating
#endif

  if (WiFi.status() != WL_CONNECTED) wifiConnect();  // Reconnect to WiFi if needed
  if (!mqtt.connected()) mqttConnect();              // Reconnect to MQTT if needed
  mqtt.loop();                                       // Handle MQTT traffic, callbacks, and keep-alives
  ArduinoOTA.handle();                               // Handle Over the Air updates
  updateBicolorLed();                                // Driven internally by slowFlash/fastFlash TickTwo objects
  measure();                                         // Free-running, every pass

  bool isActive = (reading.irms >= LOAD_THRESHOLD_A);  // Detect charging
  setBicolorLedState(isActive ? LED_GREEN_SLOW : LED_GREEN_STEADY);

  static bool wasActive = false;
  if (isActive != wasActive) {
    unsigned long newInterval = isActive ? ACTIVE_INTERVAL_MS : IDLE_INTERVAL_MS;
    publishTicker.interval(newInterval);
    publishTicker.stop();   // End current interval immediately upon active/idle transition
    publishTicker.start();  // Guarantees a clean restart with the new interval
    publishReading();       // Force an immediate publish on an active/idle transition
    wasActive = isActive;   // Remember mode for next loop() pass
  }
  publishTicker.update(); // Call publishReading when fired by ticker

  if (millis() - lastSuccessfulPublish > MQTT_LIVENESS_TIMEOUT_MS) {
    WiFi.disconnect();
    wifiConnect();
  }
}  // loop()
