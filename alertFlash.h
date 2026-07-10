/**
 * @file alertFlash.h
 * @brief Bicolor LED status indicator interface.
 *
 * @details
 * Declares the LED state enum and the functions that configure the LED
 * pins, set the desired state, and drive the actual flash timing. The
 * LED communicates device status at a glance without needing serial or
 * MQTT access:
 *  - Off: no power
 *  - Green: normal operation (steady idle, slow flash while charging,
 *    fast flash while connecting to Wi-Fi)
 *  - Red: fault or special mode (steady is currently unused, slow flash
 *    for CALIBRATE mode, fast flash for Wi-Fi or MQTT connection failure)
 *
 * setBicolorLedState() only records the desired state; updateBicolorLed()
 * must be called regularly (in loop(), including during any blocking
 * wait windows) to actually drive the flash timing.
 *
 * @author Karl Berger with Claude
 * @date 2026.07.10
 */

#pragma once

#include <Arduino.h>
// clang-format off
enum LedState : uint8_t {
  LED_OFF =          0,  // No power
  LED_GREEN_STEADY = 1,  // Normal operation (idle)
  LED_GREEN_SLOW =   2,  // Charging active
  LED_GREEN_FAST =   3,  // Connecting to Wi-Fi
  LED_RED_STEADY =   4,  // Not used
  LED_RED_SLOW =     5,  // CALIBRATE mode
  LED_RED_FAST =     6   // Wi-Fi or MQTT connection failure
};
// clang-format on

void initBicolorLed();                   // Set pinModes
void setBicolorLedState(uint8_t state);  // Pass alert state
void updateBicolorLed();                 // Call in loop()