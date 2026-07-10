/**
 * @file alertFlash.cpp
 * @brief Bicolor LED status indicator implementation.
 *
 * @details
 * initBicolorLed() configures the green/red LED pins as outputs.
 * setBicolorLedState() records the desired LedState (see alertFlash.h)
 * and resets flash timing whenever the state changes; it does not drive
 * the pins directly. updateBicolorLed() must be called regularly — in
 * loop() and during any blocking wait windows — to actually toggle the
 * pins: steady states hold the LED on continuously, slow states flash
 * at 500 ms, and fast states flash at 150 ms.
 *
 * @author Karl Berger with Claude
 * @date 2026.07.10
 */

#include "alertFlash.h"
#include "config.h"

void initBicolorLed() {
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);
}

static uint8_t currentState = LED_OFF;
static bool flashOn = false;
static unsigned long lastToggle = 0;

void setBicolorLedState(uint8_t state) {
  if (state > LED_RED_FAST) state = LED_OFF;

  if (state != currentState) {
    currentState = state;
    flashOn = false;
    lastToggle = millis();
  }
}

void updateBicolorLed() {
  unsigned long now = millis();

  auto writeGreen = []() {
    digitalWrite(PIN_LED_GREEN, HIGH);
    digitalWrite(PIN_LED_RED, LOW);
  };

  auto writeRed = []() {
    digitalWrite(PIN_LED_GREEN, LOW);
    digitalWrite(PIN_LED_RED, HIGH);
  };

  auto writeOff = []() {
    digitalWrite(PIN_LED_GREEN, LOW);
    digitalWrite(PIN_LED_RED, LOW);
  };

  switch (currentState) {
    case LED_OFF:
      writeOff();
      break;

    case LED_GREEN_STEADY:
      writeGreen();
      break;

    case LED_GREEN_SLOW:
      if (now - lastToggle >= 500) {
        lastToggle = now;
        flashOn = !flashOn;
      }
      if (flashOn) writeGreen();
      else writeOff();
      break;

    case LED_GREEN_FAST:
      if (now - lastToggle >= 150) {
        lastToggle = now;
        flashOn = !flashOn;
      }
      if (flashOn) writeGreen();
      else writeOff();
      break;

    case LED_RED_STEADY:
      writeRed();
      break;

    case LED_RED_SLOW:
      if (now - lastToggle >= 500) {
        lastToggle = now;
        flashOn = !flashOn;
      }
      if (flashOn) writeRed();
      else writeOff();
      break;

    case LED_RED_FAST:
      if (now - lastToggle >= 150) {
        lastToggle = now;
        flashOn = !flashOn;
      }
      if (flashOn) writeRed();
      else writeOff();
      break;

    default:
      writeOff();
      break;
  }
}