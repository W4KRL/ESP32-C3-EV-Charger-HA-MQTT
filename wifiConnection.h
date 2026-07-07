/**
 * @file wifiConnection.h
 * @brief Wi-Fi connection and OTA update service interface.
 *
 * @details
 * Declares the functions that establish and maintain the local Wi-Fi
 * connection and initialize ArduinoOTA for over-the-air firmware updates.
 *
 * @author Karl Berger with Claude
 * @date 2026.06.26
 */

#pragma once

void wifiConnect();  // connect to local Wi-Fi
void initOTA();      // establish Over The Air update service