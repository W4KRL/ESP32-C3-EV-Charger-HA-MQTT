/**
 * @file mqttConnection.cpp
 * @brief MQTT client implementation: broker connection, HA discovery, and telemetry publishing.
 *
 * @details
 * Defines the shared PubSubClient/WiFiClient instances declared in
 * mqttConnection.h, and implements connection management (with LWT-based
 * availability reporting), periodic power-reading publication, and
 * Home Assistant MQTT discovery registration for all sensor entities.
 *
 * @author Karl Berger with Claude
 * @date 2026.07.08
 */

#include "mqttConnection.h"  // self-header
#include "config.h"          // credentials and parameters
#include <WiFi.h>            // Wi-Fi library
#include <ArduinoJson.h>     // json library

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

void mqttConnect() {
  while (!mqtt.connected()) {
    Serial.print("MQTT connecting...");
    if (mqtt.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS,
                     MQTT_TOPIC_STATUS, /*willQoS=*/1, /*willRetain=*/true, "offline")) {
      Serial.println(" connected");
      mqtt.publish(MQTT_TOPIC_STATUS, "online", /*retain=*/true);
      publishDiscovery();  // register sensors with HA on each connection
    } else {
      Serial.printf(" failed rc=%d, retry 5s\n", mqtt.state());
      delay(5000);
    }
  }
}  // mqttConnect()

void publishReading() {
  JsonDocument doc;
  doc["vrms"] = serialized(String(reading.vrms, 1));
  doc["irms"] = serialized(String(reading.irms, 3));
  doc["watts"] = serialized(String(reading.realPower, 1));
  doc["va"] = serialized(String(reading.apparent, 1));
  doc["pf"] = serialized(String(reading.pf, 3));
  doc["rssi"] = WiFi.RSSI();
  doc["fw"] = FW_VERSION;

  char buf[256];
  serializeJson(doc, buf, sizeof(buf));
  mqtt.publish(MQTT_TOPIC_POWER, buf);
  Serial.printf("TX: %s\n", buf);
}  // publishReading()

void publishDiscovery() {
struct SensorDef {
  const char* name;
  const char* id;
  const char* unit;
  const char* deviceClass;
  const char* icon;
  const char* category;
  const char* stateClass;
};

static const SensorDef sensors[] = {
  { "AC Voltage",       "vrms",  "V",   "voltage",         nullptr,         nullptr,      "measurement" },
  { "AC Current",       "irms",  "A",   "current",         nullptr,         nullptr,      "measurement" },
  { "Real Power",       "watts", "W",   "power",           nullptr,         nullptr,      "measurement" },
  { "Apparent Power",   "va",    "VA",  nullptr,           "mdi:sine-wave", nullptr,      "measurement" },
  { "Power Factor",     "pf",    "",    "power_factor",    nullptr,         nullptr,      "measurement" },
  { "RSSI",             "rssi",  "dBm", "signal_strength", nullptr,         "diagnostic", "measurement" },
  { "Firmware Version", "fw",    "",    nullptr,           "mdi:chip",      "diagnostic", nullptr },
};

  Serial.printf("Buffer size: %d, Max packet: %d\n", mqtt.getBufferSize(), MQTT_MAX_PACKET_SIZE);  // ← add this line here, before the loop

  for (const auto& s : sensors) {
    char topic[128];
    snprintf(topic, sizeof(topic),
             "homeassistant/sensor/%s/%s/config",
             MQTT_CLIENT_ID, s.id);

    JsonDocument doc;
    doc["name"] = s.name;
    doc["unique_id"] = String(MQTT_CLIENT_ID) + "_" + s.id;
    doc["state_topic"] = MQTT_TOPIC_POWER;
    doc["value_template"] = String("{{ value_json.") + s.id + " }}";
    doc["unit_of_measurement"] = s.unit;
    if (s.deviceClass) doc["device_class"] = s.deviceClass;
    if (s.icon) doc["icon"] = s.icon;
    if (s.category) doc["entity_category"] = s.category;
    if (s.stateClass) doc["state_class"] = s.stateClass;
    doc["availability_topic"] = MQTT_TOPIC_STATUS;
    doc["payload_available"] = "online";
    doc["payload_not_available"] = "offline";

    JsonObject device = doc["device"].to<JsonObject>();
    device["identifiers"][0] = MQTT_CLIENT_ID;
    device["name"] = "AC Power Monitor";
    device["model"] = "ESP32-C3";
    device["manufacturer"] = "W4KRL";

    char buf[512];
    size_t len = serializeJson(doc, buf, sizeof(buf));
    bool ok = mqtt.publish(topic, buf, /*retain=*/true);
    Serial.printf("Discovery: %s len=%d %s\n", topic, len, ok ? "OK" : "FAILED");  // ← replaces the old Serial.printf("Discovery: %s\n", topic);
  }
}  // publishDiscovery()
