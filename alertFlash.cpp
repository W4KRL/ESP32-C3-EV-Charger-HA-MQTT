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
 * @date 2026.07.21
 */

#include "alertFlash.h"  // Self header
#include "config.h"      // LED GPIO connections
#include <TickTwo.h>     // Timer object for LED flash

static uint8_t currentState = LED_OFF;
static bool flashOn = false;

// clang-format off
static void writeGreen() { digitalWrite(PIN_LED_GREEN, HIGH); digitalWrite(PIN_LED_RED, LOW); }
static void writeRed()   { digitalWrite(PIN_LED_GREEN, LOW); digitalWrite(PIN_LED_RED, HIGH); }
static void writeOff()   { digitalWrite(PIN_LED_GREEN, LOW); digitalWrite(PIN_LED_RED, LOW); }

static void toggleFlash() {
  flashOn = !flashOn;
  if (!flashOn) {
    writeOff();
  } else if (currentState == LED_GREEN_SLOW || currentState == LED_GREEN_FAST) {
    writeGreen();
  } else if (currentState == LED_RED_SLOW || currentState == LED_RED_FAST) {
    writeRed();
  }
}

// Instantiate tickers
TickTwo slowFlash(toggleFlash, 500, 0, MILLIS);
TickTwo fastFlash(toggleFlash, 150, 0, MILLIS);

// set LED pin connections
void initBicolorLed() {
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);
}

void setBicolorLedState(uint8_t state) {
  if (state > LED_RED_FAST) state = LED_OFF;
  if (state == currentState) return;

  currentState = state;
  slowFlash.stop();
  fastFlash.stop();
  flashOn = false;
  writeOff();  // matches original: starts dark, first toggle (after one interval) lights it

  switch (currentState) {
case LED_GREEN_STEADY:
  writeGreen();
  break;
    case LED_RED_STEADY:   writeRed();   break;
    case LED_GREEN_SLOW:
    case LED_RED_SLOW:     slowFlash.start(); break;
    case LED_GREEN_FAST:
    case LED_RED_FAST:     fastFlash.start(); break;
    case LED_OFF:
    default: break;        // already off
  }
}
// clang-format on

// Call in loop()
void updateBicolorLed() {
  slowFlash.update();
  fastFlash.update();
}