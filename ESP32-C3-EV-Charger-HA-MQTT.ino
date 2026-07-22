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
 * @author Karl Berger with Claude
 * @date 2026.07.10
 */

// ─── Libraries ────────────────────────────────────────────────────────────────
#include <WiFi.h>            // Built-in
#include <ArduinoOTA.h>      // Built-in
#include <TickTwo.h>         // Ticker library by Stefan Staub https://github.com/sstaub/TickTwo
#include "config.h"          // Credentials, adjustable parameters and globals
#include "wifiConnection.h"  // Wi-fi connection and OTA handler
#include "mqttConnection.h"  // MQTT service
#include "alertFlash.h"      // Bicolor LED indicator for mode indications
#include "measurement.h"     // Read ADC and calculate electrical parameters

void publishActive();
void publishIdle();

TickTwo activeTicker(publishActive, ACTIVE_INTERVAL_MS, 0, MILLIS);
TickTwo idleTicker(publishIdle, IDLE_INTERVAL_MS, 0, MILLIS);

// ─── Setup ────────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);                          // Serial Monitor for USB connection
  delay(1000);                                   // Allow ESP32-C3 USB to enumerate
  Serial.println("\n=== AC Power Monitor ===");  // Welcome
  initBicolorLed();                              // prepare LED for mode alerts

#ifdef CALIBRATE                     // run calibration mode if defined
  setBicolorLedState(LED_RED_SLOW);  // Indicate calibration mode
#else                                // else continue with normal start up
  wifiConnect();                             // Connect to local Wi-Fi
  initOTA();                                 // Initialize OTA
  mqtt.setServer(MQTT_HOST, MQTT_PORT);      // Initiaize HA MQTT host
  mqtt.setKeepAlive(MQTT_KEEPALIVE_S);       // Set activity ping
  mqtt.setBufferSize(MQTT_MAX_PACKET_SIZE);  // Discovery payloads ~420 bytes, default 256 too small
  measure();                                 // warm-up: seed offsetV/offsetI before first real publish
  mqttConnect();                             // CONNECT TO HA
  idleTicker.start();                        // begin in idle (not charging) state
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

  if (WiFi.status() != WL_CONNECTED) wifiConnect();
  if (!mqtt.connected()) mqttConnect();
  mqtt.loop();
  ArduinoOTA.handle();
  updateBicolorLed();  // now driven internally by slowFlash/fastFlash TickTwo objects
  measure();           // free-running, every pass

  bool isActive = (reading.irms >= LOAD_THRESHOLD_A);
  setBicolorLedState(isActive ? LED_GREEN_SLOW : LED_GREEN_STEADY);

  static bool wasActive = false;
  if (isActive != wasActive) {
    if (isActive) {
      idleTicker.stop();
      activeTicker.start();
    } else {
      activeTicker.stop();
      idleTicker.start();
    }
    publishReading();  // force immediate publish on transition
    wasActive = isActive;
  }

  activeTicker.update();
  idleTicker.update();

  if (millis() - lastSuccessfulPublish > MQTT_LIVENESS_TIMEOUT_MS) {
    WiFi.disconnect();
    wifiConnect();
  }
}  // loop()

void publishActive() {
  publishReading();
}
void publishIdle() {
  publishReading();
}