/**
 * @file wifiConnection.cpp
 * @brief Wi-Fi connection and OTA update service implementation.
 *
 * @details
 * Connects to the configured local Wi-Fi network in station mode and
 * blocks until association completes. Initializes ArduinoOTA with a
 * hostname and password, wiring up start/end/progress/error callbacks
 * for serial-visible OTA status during firmware updates.
 *
 * @author Karl Berger with Claude
 * @date 2026.07.09
 */

#include "wifiConnection.h"  // Self header
#include "config.h"          // Credentials
#include <WiFi.h>            // Wi-Fi library
#include <ArduinoOTA.h>      // OTA library
#include "alertFlash.h"      // Mode indications

/**
 * @brief Connect to WiFi with a bounded timeout.
 * @return true if connected, false if timed out.
 * @warning Blocking. Caller must decide retry/reboot behavior on failure.
 */
bool wifiConnect() {
  setBicolorLedState(LED_GREEN_FAST);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("WiFi connecting");

  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - startAttempt >= WIFI_CONNECT_TIMEOUT_MS) {
      setBicolorLedState(LED_RED_FAST);
      Serial.println("\nWiFi connect timed out");
      return false;
    }
    updateBicolorLed();  // keep flash alive during blocking wait
    delay(500);
    Serial.print(".");
  }

  Serial.printf("\nConnected — IP: %s\n", WiFi.localIP().toString().c_str());
  return true;
}  // wifiConnect()

void initOTA() {
  ArduinoOTA.setHostname(OTA_HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);

  ArduinoOTA.onStart([]() {
    Serial.println("OTA start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA complete");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("OTA %u%%\r", progress / (total / 100));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA error [%u]\n", error);
  });

  ArduinoOTA.begin();
  Serial.println("OTA ready");
}  // initOTA()