/**
 * @file measurement.h
 * @brief ADC sampling and RMS/power calculation interface.
 *
 * @details
 * Declares the functions that configure ADC attenuation and perform the
 * per-burst voltage/current sampling that updates the global PowerReading
 * struct (see config.h). Each call to measure() samples both channels for
 * SAMPLE_CYCLES line cycles, tracks the DC bias offset per channel, and
 * computes RMS voltage, RMS current, real power, apparent power (va), and
 * power factor, applying VOLTAGE_SCALE/CURRENT_SCALE and
 * CURRENT_DEADBAND_A (all in config.h).
 *
 * vrms_mv and irms_mv expose the raw, pre-scale RMS values in millivolts
 * at the ADC pin, used for bench calibration in CALIBRATE mode.
 *
 * @author Karl Berger with Claude
 * @date 2026.07.10
 */

#pragma once

void initADC();  // sets ADC gain
void measure();  // samples ADC, updates the global `reading` struct

extern float vrms_mv;  // raw pre-scale voltage RMS, exposed for CALIBRATE mode
extern float irms_mv;  // raw pre-scale current RMS, exposed for CALIBRATE mode