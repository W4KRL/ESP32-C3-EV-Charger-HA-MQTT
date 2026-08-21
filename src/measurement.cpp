/**
 * @file measurement.cpp
 * @brief ADC sampling and RMS/power calculation implementation.
 *
 * @details
 * ## Burst sampling
 * The core of this module is the **burst**: a fixed-duration acquisition
 * window, BURST_US = SAMPLE_CYCLES line cycles (computed from
 * LINE_FREQUENCY), during which measure() repeatedly samples both the
 * voltage and current ADC channels back-to-back and accumulates their
 * sums, sums of squares, and cross-product. Each call to measure()
 * performs exactly one burst — it blocks for the burst's duration
 * (typically tens of milliseconds) and returns with the PowerReading
 * struct updated from that burst's data. It must be called repeatedly
 * (once per loop() pass in normal mode) to keep readings current.
 *
 * ## Non-burst (per-call) processing
 * After the burst loop finishes, measure() performs work that does not
 * touch the ADC:
 *  - Updates the running DC-bias offsets (offsetV/offsetI) from this
 *    burst's mean, seeding the *next* burst so RMS is computed as
 *    deviation from the true bias point rather than an assumed midpoint.
 *  - Derives RMS voltage/current from the burst sums, in raw millivolts
 *    (exposed via vrms_mv/irms_mv for CALIBRATE mode).
 *  - Applies VOLTAGE_SCALE/CURRENT_SCALE to compute real-world vrms,
 *    irms, apparent power, real power, and power factor into the global
 *    PowerReading struct. Real power is derived from the V*I cross-term
 *    and takes the absolute value, so it reports correct magnitude
 *    regardless of CT or ZMPT wiring polarity.
 *  - Zeroes irms/apparent/realPower/pf when irms falls below
 *    CURRENT_DEADBAND_A, to suppress residual ADC noise-floor current
 *    when idle, without affecting the reported voltage.
 *
 * initADC() is unrelated to any burst: it configures ADC attenuation
 * once at startup, before the first call to measure().
 *
 * @author Karl Berger with Claude
 * @date 2026.08.21
 */

// measurement.cpp
#include "measurement.h" // Self header
#include "config.h"      // Pin assignments, SAMPLE_CYCLES, LINE_FREQUENCY,
#include <Arduino.h>
// SAMPLE_INTERVAL_US, VOLTAGE_SCALE, CURRENT_SCALE,
// CURRENT_DEADBAND_A, reading (inline global)

float vrms_mv = 0.0f; // Raw burst RMS voltage in mV (CALIBRATE mode only)
float irms_mv = 0.0f; // Raw burst RMS current in mV (CALIBRATE mode only)

void initADC() {
  analogSetAttenuation(ADC_11db); // Widen ADC input range to ~0-3.3 V (non-burst, one-time setup)
} // initADC()

// ─── Measurement ──────────────────────────────────────────────────────────────
void measure() {
  static float offsetV = 0.0f; // DC-bias offset for V, seeded from the previous burst's mean; persists between calls
  static float offsetI = 0.0f; // DC-bias offset for I, seeded from the previous burst's mean; persists between calls

  double sumV2 = 0.0, sumI2 = 0.0, sumP = 0.0; // Burst accumulators for squared/cross terms; double limits rounding error over many samples
  float sumV = 0.0f, sumI = 0.0f;              // Burst accumulators for the means; float precision is sufficient here

  const unsigned long BURST_US = (unsigned long)(SAMPLE_CYCLES * (1000000.0f / LINE_FREQUENCY)); // Burst duration in microseconds

  // ─── Burst: acquire samples for BURST_US ─────────────────────────────────
  long count = 0;
  unsigned long startTime = micros();
  while ((micros() - startTime) < BURST_US) {
    float v_mv = analogReadMilliVolts(PIN_VOLTAGE);
    float i_mv = analogReadMilliVolts(PIN_CURRENT);

    sumV += v_mv;
    sumI += i_mv;
    sumV2 += (v_mv - offsetV) * (v_mv - offsetV);
    sumI2 += (i_mv - offsetI) * (i_mv - offsetI);
    sumP += (v_mv - offsetV) * (i_mv - offsetI);
    count++;

    delayMicroseconds(SAMPLE_INTERVAL_US); // Paces the samples; this is what makes the burst block for BURST_US
  }

  // ─── Post-burst: guard against an empty burst ────────────────────────────
  if (count == 0) { // No samples collected — avoid divide-by-zero in the calculations below
    reading.vrms = 0;
    reading.irms = 0;
    reading.va = 0;        // Volt-amps
    reading.realPower = 0; // Watts
    reading.pf = 1.0f;     // Report unity PF at no load, matching the idle deadband branch below
    return;
  }

  // ─── Post-burst: seed next burst's DC-bias offsets ───────────────────────
  offsetV = sumV / count;
  offsetI = sumI / count;

  // ─── Post-burst: RMS in raw millivolts (CALIBRATE mode output) ───────────
  vrms_mv = sqrtf((float)(sumV2 / count));
  irms_mv = sqrtf((float)(sumI2 / count));

  // ─── Post-burst: scale to real-world units and update PowerReading ───────
  reading.vrms = vrms_mv * VOLTAGE_SCALE;
  reading.irms = irms_mv * CURRENT_SCALE;
  reading.va = reading.vrms * reading.irms;                                // Apparent power (VA)
  reading.realPower = fabsf(sumP / count * VOLTAGE_SCALE * CURRENT_SCALE); // Real power (W); abs() makes sign independent of CT/ZMPT wiring polarity
  reading.pf = (reading.va > 0.1f)
                   ? constrain(reading.realPower / reading.va, 0.0f, 1.0f) // Normal case: PF = real / apparent
                   : 0.0f;                                                 // Apparent power too small to divide safely

  // ─── Post-burst: idle deadband suppression ───────────────────────────────
  if (reading.irms < CURRENT_DEADBAND_A) {
    // Suppress ADC noise-floor current when idle; PF reported as 1.0
    // (rather than 0.0) so idle periods don't distort HA history graphs
    reading.irms = 0.0f;
    reading.va = 0.0f;
    reading.realPower = 0.0f;
    reading.pf = 1.0f; // Keep HA graphs clean at idle
  }
} // measure()