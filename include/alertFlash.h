/**
 * @file alertFlash.h
 * @brief Bicolor LED status indicator interface.
 *
 * @details
 * Declares the LED state enum and the three public functions used to
 * drive a bicolor (green/red) status LED:
 *   - initBicolorLed()      configure pins, call once from setup()
 *   - setBicolorLedState()  record desired state; resets flash timing
 *                           and starts/stops the appropriate TickTwo
 *                           ticker whenever the state actually changes
 *   - updateBicolorLed()    must be called every loop() pass (and during
 *                           any blocking wait windows) to service the
 *                           active ticker and actually toggle the pins
 *
 * Steady states hold the LED on continuously. Slow states flash at
 * 500 ms: fast states flash at 150 ms.
 *
 * @author Karl Berger with Claude
 * @date 2026.07.21
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

// Configure the green/red LED pins as outputs. Call once from setup().
void initBicolorLed();

/**
 * @brief Set the desired LED state.
 * @param state One of the LedState values. Out-of-range values fall
 *              back to LED_OFF.
 *
 * No-op if state matches the current state. On an actual change,
 * stops any running flash ticker, resets flash timing, drives the LED
 * off, then either sets a steady color or starts the appropriate
 * (slow/fast) ticker.
 */
void setBicolorLedState(uint8_t state);

/**
 * @brief Service the active flash ticker and update the LED pins.
 *
 * Must be called frequently and unconditionally — every loop() pass,
 * and inside any blocking wait loop — or flash states will stall.
 */
void updateBicolorLed();