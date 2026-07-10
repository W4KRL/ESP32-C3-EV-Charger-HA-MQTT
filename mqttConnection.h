/**
 * @file mqttConnection.h
 * @brief MQTT client interface for Home Assistant discovery and telemetry publishing.
 *
 * @details
 * Declares the shared PubSubClient instance and the functions that connect to
 * the broker, publish live power readings, and publish HA MQTT discovery
 * config topics.
 *
 * @warning MQTT_MAX_PACKET_SIZE must be defined here, before <PubSubClient.h>
 * is included. The library's default (256 bytes) truncates discovery
 * payloads (~400-450 bytes) silently -- mqtt.publish() still returns true,
 * but most messages never reach the broker. Do not reorder these two lines
 * or move the define elsewhere without preserving the include order.
 *
 * @author Karl Berger with Claude
 * @date 2026.06.26
 */

#pragma once

#include "config.h"               // credentials and parameters
#define MQTT_MAX_PACKET_SIZE 512  // define before including PubSubClient
#include <PubSubClient.h>         // MQTT library

extern PubSubClient mqtt;     // MQTT client instance defined in mqttConnection.cpp

bool mqttConnect();       // connect to MQTT broker
void publishReading();    // publishes global reading to MQTT broker
void publishDiscovery();  // publishes HA MQTT auto-discovery config topics