/*
 * @file ESP32-C3-EV-Charger-HA-MQTT.ino
 * @author Karl Berger with Claude
 * @date 2026.07.06
 *  
*/
#include <WiFi.h>            // built-in
#include <ArduinoOTA.h>      // built-in
#include "config.h"          // credentials, adjustable parameters and globals
#include "wifiConnection.h"  // wi-fi connection and OTA handler
#include "mqttConnection.h"  // mqtt service

// ─── Globals ──────────────────────────────────────────────────────────────────
float vrms_mv = 0.0f;  // raw pre-scale voltage RMS, exposed for CALIBRATE mode
float irms_mv = 0.0f;  // raw pre-scale current RMS, exposed for CALIBRATE mode

// ─── Measurement ──────────────────────────────────────────────────────────────
void measure() {
  static float offsetV = 0.0f;  // static: initialized once, retains value between calls
  static float offsetI = 0.0f;  // updated each burst to track ADC DC bias

  double sumV2 = 0.0, sumI2 = 0.0, sumP = 0.0;  // need double for squared values

  float sumV = 0.0f, sumI = 0.0f;

  const long BURST_US = (long)(SAMPLE_CYCLES * (1000000.0f / LINE_FREQUENCY));
  long count = 0;

  unsigned long endTime = micros() + BURST_US;
  while (micros() < endTime) {
    float v_mv = analogReadMilliVolts(PIN_VOLTAGE);
    float i_mv = analogReadMilliVolts(PIN_CURRENT);

    sumV += v_mv;
    sumI += i_mv;
    sumV2 += (v_mv - offsetV) * (v_mv - offsetV);
    sumI2 += (i_mv - offsetI) * (i_mv - offsetI);
    sumP += (v_mv - offsetV) * (i_mv - offsetI);
    count++;

    delayMicroseconds(SAMPLE_INTERVAL_US);
  }

  if (count == 0) {  // guard: burst produced no samples — zero output and bail out
    reading.vrms = 0;
    reading.irms = 0;
    reading.apparent = 0;
    reading.realPower = 0;
    reading.pf = 0;
    return;
  }

  offsetV = sumV / count;
  offsetI = sumI / count;

  vrms_mv = sqrtf((float)(sumV2 / count));
  irms_mv = sqrtf((float)(sumI2 / count));

  // update values in PowerReading struct
  reading.vrms = vrms_mv * VOLTAGE_SCALE;
  reading.irms = irms_mv * CURRENT_SCALE;
  reading.apparent = reading.vrms * reading.irms;
  reading.realPower = fabsf(sumP / count * VOLTAGE_SCALE * CURRENT_SCALE);
  reading.pf = (reading.apparent > 0.1f)
                 ? constrain(reading.realPower / reading.apparent, 0.0f, 1.0f)
                 : 0.0f;
  if (reading.irms < CURRENT_DEADBAND_A) {
    // zero out readings due to non-linear ADC
    reading.irms = 0.0f;
    reading.apparent = 0.0f;
    reading.realPower = 0.0f;
    reading.pf = 0.0f;
  }

#ifdef CALIBRATE
  // Serial.printf("vrms_mv=%.2f irms_mv=%.2f\n", vrms_mv, irms_mv);
  // Serial.printf("count=%ld offsetV=%.1f offsetI=%.1f\n", count, offsetV, offsetI);
#endif
}  // measure()

// ─── Setup ────────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(1000);                                   // Allow ESP32-C3 USB to enumerate
  Serial.println("\n=== AC Power Monitor ===");  // Welcome
  analogSetAttenuation(ADC_11db);                // Set ADC to default

#ifndef CALIBRATE
  wifiConnect();                         // Connect to local Wi-Fi
  initOTA();                             // Initialize OTA
  mqtt.setServer(MQTT_HOST, MQTT_PORT);  // Initiize HA MQTT host
  mqtt.setKeepAlive(60);                 // Set activity ping
  mqtt.setBufferSize(512);               // Discovery payloads ~420 bytes, default 256 too small
  measure();                             // warm-up: seed offsetV/offsetI before first real publish
  mqttConnect();                         // CONNECT TO HA
#endif
}  // setup()

// ─── Loop ─────────────────────────────────────────────────────────────────────
void loop() {
#ifdef CALIBRATE
  static unsigned long lastCalMs = 0;
  measure();
  if (millis() - lastCalMs >= CAL_INTERVAL_MS) {
    lastCalMs = millis();
    Serial.printf("vrms_mv=%.2f irms_mv=%.2f vrms=%.2f irms=%.2f\n",
                  vrms_mv, irms_mv, reading.vrms, reading.irms);
  }
  return;  // skip WiFi/MQTT entirely while calibrating
#endif

  if (WiFi.status() != WL_CONNECTED) wifiConnect();  // Reconnect to Wi-Fi if needed
  if (!mqtt.connected()) mqttConnect();              // Reconnect to MQTT if needed
  mqtt.loop();                                       // Handle MQTT
  ArduinoOTA.handle();                               // Handle OTA
  measure();
  publishReading();  // Publish measurements to MQTT

  long intervalMs = (reading.irms < LOAD_THRESHOLD_A) ? IDLE_INTERVAL_MS : ACTIVE_INTERVAL_MS;
  unsigned long wakeAt = millis() + intervalMs;
  while (millis() < wakeAt) {
    ArduinoOTA.handle();
    mqtt.loop();
    delay(100);
  }
}  // loop()