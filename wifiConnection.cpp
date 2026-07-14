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
 * @date 2026.07.14
 */

#include "wifiConnection.h"  // Self header
#include "config.h"          // Credentials
#include <WiFi.h>            // Wi-Fi library
#include <esp_wifi.h>        // For esp_wifi_get_config/set_config (AP scan/sort settings)
#include <ArduinoOTA.h>      // OTA library
#include "alertFlash.h"      // Mode indications

/**
 * @brief Connect to WiFi with a bounded timeout.
 * @return true if connected, false if timed out.
 * @warning Blocking. Caller must decide retry/reboot behavior on failure.
 */
bool wifiConnect() {
  setBicolorLedState(LED_GREEN_FAST);

  WiFi.mode(WIFI_STA);   // set station mode
  WiFi.setSleep(false);  // keep the radio active to maintain a stable network connection

  // Set SSID/password without connecting yet, so our scan/sort override
  // below isn't clobbered by WiFi.begin()'s own internal config write
  WiFi.begin(WIFI_SSID, WIFI_PASS, 0, nullptr, /*connect=*/false);

  // Force a full-channel scan and RSSI-based AP selection so OneMesh
  // picks the strongest AP sharing the SSID, not just the first to answer
  wifi_config_t conf;
  esp_wifi_get_config(WIFI_IF_STA, &conf);
  conf.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
  conf.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;
  esp_wifi_set_config(WIFI_IF_STA, &conf);

  esp_wifi_connect();  // now actually connect, with our scan/sort settings intact
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