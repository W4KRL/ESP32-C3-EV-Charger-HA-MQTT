/**
 * @file measurement.cpp
 * @brief ADC sampling and RMS/power calculation implementation.
 *
 * @details
 * initADC() configures ADC attenuation once at startup. measure() samples
 * both the voltage and current ADC channels for a burst of SAMPLE_CYCLES
 * line cycles, tracking each channel's DC bias as a running offset
 * (seeded from the previous burst's mean) so that RMS is computed as
 * deviation from the true bias point rather than an assumed midpoint.
 *
 * From the accumulated sums it derives RMS voltage/current (in raw
 * millivolts, exposed via vrms_mv/irms_mv for CALIBRATE mode), then
 * applies VOLTAGE_SCALE/CURRENT_SCALE to compute real-world vrms, irms,
 * apparent power, real power, and power factor into the global
 * PowerReading struct. Real power is derived from the V*I cross-term and
 * takes the absolute value, so it reports correct magnitude regardless of
 * CT or ZMPT wiring polarity.
 *
 * Readings below CURRENT_DEADBAND_A are zeroed (irms, apparent, realPower,
 * pf) to suppress residual ADC noise-floor current when idle, without
 * affecting the reported voltage.
 *
 * @author Karl Berger with Claude
 * @date 2026.07.24
 */

// measurement.cpp
#include <Arduino.h>
#include "measurement.h"  // self header
#include "config.h"       // PIN_VOLTAGE, PIN_CURRENT, SAMPLE_CYCLES, LINE_FREQUENCY,
                          // SAMPLE_INTERVAL_US, VOLTAGE_SCALE, CURRENT_SCALE,
                          // CURRENT_DEADBAND_A, reading (inline global)

float vrms_mv = 0.0f;
float irms_mv = 0.0f;

void initADC() {
  analogSetAttenuation(ADC_11db);  // Set ADC to default
}  // initADC()

// ─── Measurement ──────────────────────────────────────────────────────────────
void measure() {
  static float offsetV = 0.0f;                  // static: initialized once, retains value between calls
  static float offsetI = 0.0f;                  // updated each burst to track ADC DC bias
  double sumV2 = 0.0, sumI2 = 0.0, sumP = 0.0;  // need double for squared values
  float sumV = 0.0f, sumI = 0.0f;
  const unsigned long BURST_US = (unsigned long)(SAMPLE_CYCLES * (1000000.0f / LINE_FREQUENCY));

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

    delayMicroseconds(SAMPLE_INTERVAL_US);
  }

  if (count == 0) {  // guard: burst produced no samples — zero output and bail out
    reading.vrms = 0;
    reading.irms = 0;
    reading.apparent = 0;
    reading.realPower = 0;
    reading.pf = 1.0f;  // to match the CURRENT_DEADBAND_A test
    return;
  }

  offsetV = sumV / count;
  offsetI = sumI / count;

  vrms_mv = sqrtf((float)(sumV2 / count));
  irms_mv = sqrtf((float)(sumI2 / count));

  // update values in PowerReading struct
  reading.vrms = vrms_mv * VOLTAGE_SCALE;
  reading.irms = irms_mv * CURRENT_SCALE;
  reading.apparent = reading.vrms * reading.irms;
  reading.realPower = fabsf(sumP / count * VOLTAGE_SCALE * CURRENT_SCALE);
  reading.pf = (reading.apparent > 0.1f)
                 ? constrain(reading.realPower / reading.apparent, 0.0f, 1.0f)
                 : 0.0f;
  if (reading.irms < CURRENT_DEADBAND_A) {
    // suppress ADC noise-floor current when idle; PF reported as 1.0
    // (rather than 0.0) so idle periods don't distort HA history graphs
    reading.irms = 0.0f;
    reading.apparent = 0.0f;
    reading.realPower = 0.0f;
    reading.pf = 1.0f;  // to keep HA graphs clean
  }
}  // measure()