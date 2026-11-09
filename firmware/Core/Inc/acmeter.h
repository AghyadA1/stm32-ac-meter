/**
 * @file    acmeter.h
 * @brief   Signal processing for the STM32 AC meter.
 *
 * Every ADC sample updates the measurements incrementally with a sliding
 * window, so the CPU never re-scans a whole buffer:
 *   - true RMS of voltage and current (DC offset removed)
 *   - frequency (from the voltage signal)
 *   - phase shift between voltage and current
 *   - apparent (S), real (P) and reactive (Q) power
 */
#ifndef ACMETER_H
#define ACMETER_H

#include <stdint.h>

/* Window sizes ------------------------------------------------------------ */
#define BUFFER_SIZE   1100U  /* samples kept per channel for the RMS windows  */
#define BUFFER_SIZE1  70U    /* signal periods averaged for frequency / phase */

/* Analog front-end scale factors (ADC volts -> real units, calibrated) ---- */
#define ADC_VREF          3.3f
#define ADC_FULL_SCALE    4095.0f
#define VOLTAGE_SCALE     30.5848f  /* divider ratio ~1/0.032, calibrated  */
#define CURRENT_SCALE     1.2876f   /* shunt + amplifier, calibrated       */

/* Zero-crossing hysteresis, as a fraction of the RMS value */
#define CROSSING_THRESHOLD 0.65f

/* Results (read by the display code) -------------------------------------- */
extern float rms_voltage_ac;   /* V    */
extern float rms_current_ac;   /* A    */
extern float freq;             /* Hz   */
extern float phase;            /* rad  */

/* Per-sample entry points: call from the ADC conversion-complete ISR ------- */
void ACM_ProcessVoltageSample(uint16_t raw_value);
void ACM_ProcessCurrentSample(uint16_t raw_value);

/* Building blocks ---------------------------------------------------------- */
void  update_rms_buffer(uint16_t new_adc_value);   /* voltage RMS window */
void  update_rms_buffer2(uint16_t new_adc_value);  /* current RMS window */
void  update_freq_buffer(void);
void  update_phase_buffer(void);

/* Power ------------------------------------------------------------------- */
float S(void);      /* apparent power  S = Vrms * Irms           (VA)  */
float power(void);  /* real power      P = Vrms * Irms * |cos φ| (W)   */
float Q(void);      /* reactive power  Q = Vrms * Irms * |sin φ| (VAR) */

#endif /* ACMETER_H */
