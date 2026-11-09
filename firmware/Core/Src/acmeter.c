/**
 * @file    acmeter.c
 * @brief   Sliding-window measurement of RMS, frequency, phase and power.
 *
 * How the period is split
 * -----------------------
 * Each signal period is treated as two half-ranges with hysteresis:
 *   range 1 starts when the signal rises above  (mean + 0.65 * RMS)
 *   range 2 starts when the signal falls below  (mean - 0.65 * RMS)
 * The frequency function is called on every range-2 -> range-1 transition of
 * the voltage, and the phase function on the same transition of the current.
 * Because the threshold follows the signal's own RMS, noise around the
 * midpoint can never trigger more than one crossing per period. This is what
 * let the meter work from 25 Hz up to 999 Hz.
 */
#include "acmeter.h"

#include <math.h>

/* ---- Voltage RMS window ---- */
static uint32_t voltage_buffer[BUFFER_SIZE];
static int      buffer_index   = 0;
static int      buffer_full    = 0;
static uint64_t sum            = 0;
static uint64_t sum_of_squares = 0;
static float    mean           = 0.0f;
float           rms_voltage_ac = 0.0f;

/* ---- Current RMS window ---- */
static uint32_t current_buffer[BUFFER_SIZE];
static int      buffer2_index   = 0;
static int      buffer2_full    = 0;
static uint64_t sum2            = 0;
static uint64_t sum_of_squares2 = 0;
static float    mean2           = 0.0f;
float           rms_current_ac  = 0.0f;

/* ---- Frequency (samples per period, averaged over BUFFER_SIZE1 periods) ---- */
static uint32_t freq_buffer[BUFFER_SIZE1];
static int      freq_index       = 0;
static int      last_zero_index  = 0;
static int      freq_buffer_full = 0;
static uint32_t sum_freq         = 0;
static int      x1               = 0;
static float    fre              = 0.0f;  /* mean samples per period */
float           freq             = 0.0f;

/* ---- Phase (sample delay current vs voltage, averaged) ---- */
static uint32_t phase_buffer[BUFFER_SIZE1];
static int      phase_index       = 0;
static int      phase_buffer_full = 0;
static uint32_t sum_phase         = 0;
static int      x                 = 0;
float           phase             = 0.0f;

/* ---- Crossing detection state ---- */
static uint16_t d  = 0;  /* voltage threshold in ADC counts */
static uint16_t d2 = 0;  /* current threshold in ADC counts */
static int      b  = 0;  /* voltage: 1 = in range 2 (waiting to rise) */
static int      b2 = 0;  /* current: 1 = in range 2 (waiting to rise) */

/* ------------------------------------------------------------------------- */
/* RMS                                                                        */
/* ------------------------------------------------------------------------- */

/* rms = sqrt( mean(x^2) - mean(x)^2 )
 * Subtracting mean^2 removes the 1.65 V offset added by the front-end. */
void update_rms_buffer(uint16_t new_adc_value)
{
    if (buffer_full) {
        uint16_t old = voltage_buffer[buffer_index];
        sum -= old;
        sum_of_squares -= (uint64_t)old * old;
    }

    voltage_buffer[buffer_index] = new_adc_value;
    sum += new_adc_value;
    sum_of_squares += (uint64_t)new_adc_value * new_adc_value;

    buffer_index = (buffer_index + 1) % BUFFER_SIZE;
    if (!buffer_full && buffer_index == 0)
        buffer_full = 1;

    if (buffer_full) {
        mean = (float)sum / BUFFER_SIZE;
        float mean_sq = (float)sum_of_squares / BUFFER_SIZE;

        // True RMS = sqrt(mean_of_squares - mean^2)
        float rms_adc_ac = sqrtf(mean_sq - (mean * mean));

        // Convert ADC units to volts at the meter input
        rms_voltage_ac = rms_adc_ac * (ADC_VREF / ADC_FULL_SCALE) * VOLTAGE_SCALE;
    }
}

void update_rms_buffer2(uint16_t new_adc_value)
{
    if (buffer2_full) {
        uint16_t old = current_buffer[buffer2_index];
        sum2 -= old;
        sum_of_squares2 -= (uint64_t)old * old;
    }

    current_buffer[buffer2_index] = new_adc_value;
    sum2 += new_adc_value;
    sum_of_squares2 += (uint64_t)new_adc_value * new_adc_value;

    buffer2_index = (buffer2_index + 1) % BUFFER_SIZE;
    if (!buffer2_full && buffer2_index == 0)
        buffer2_full = 1;

    if (buffer2_full) {
        mean2 = (float)sum2 / BUFFER_SIZE;
        float mean_sq = (float)sum_of_squares2 / BUFFER_SIZE;

        // True RMS = sqrt(mean_of_squares - mean^2)
        float rms_adc_ac = sqrtf(mean_sq - (mean2 * mean2));

        // Convert ADC units to amperes
        rms_current_ac = rms_adc_ac * (ADC_VREF / ADC_FULL_SCALE) * CURRENT_SCALE;
    }
}

/* ------------------------------------------------------------------------- */
/* Frequency and phase                                                        */
/* ------------------------------------------------------------------------- */

/* Called once per voltage period. Stores the number of samples since the
 * previous call, and averages it over the last BUFFER_SIZE1 periods. */
void update_freq_buffer(void)
{
    x1 = buffer_index - last_zero_index;
    if (x1 < 0) x1 += BUFFER_SIZE;

    if (freq_buffer_full) {
        sum_freq -= freq_buffer[freq_index];
    }

    last_zero_index = buffer_index;
    freq_buffer[freq_index] = x1;
    sum_freq += x1;
    freq_index = (freq_index + 1) % BUFFER_SIZE1;

    b = 1 - (b + 1) / 2;  /* back to range 1 */

    if (freq_buffer_full) {
        freq = (float)sum_freq / BUFFER_SIZE1;  /* samples per period    */
        fre  = freq;                            /* kept for phase calc   */
        freq = 10000 / (freq * 3);              /* 10000/3 Hz sample rate */
    }

    if (!freq_buffer_full && freq_index == 0)
        freq_buffer_full = 1;
}

/* Called once per current period. Stores how many samples the current
 * crossing lags the last voltage crossing, averages it, and converts it to
 * radians using the mean period length. */
void update_phase_buffer(void)
{
    x = buffer2_index - last_zero_index;
    if (x < 0) x += BUFFER_SIZE;

    if (phase_buffer_full) {
        sum_phase -= phase_buffer[phase_index];
    }

    phase_buffer[phase_index] = x;
    sum_phase += x;
    phase_index = (phase_index + 1) % BUFFER_SIZE1;

    b2 = 1 - (b2 + 1) / 2;  /* back to range 1 */

    if (phase_buffer_full) {
        phase = (float)sum_phase / BUFFER_SIZE1;
        phase = phase / fre * 2 * 3.141592f;
    }

    if (!phase_buffer_full && phase_index == 0)
        phase_buffer_full = 1;
}

/* ------------------------------------------------------------------------- */
/* Per-sample processing (body of the ADC interrupt)                          */
/* ------------------------------------------------------------------------- */

void ACM_ProcessVoltageSample(uint16_t raw_value)
{
    update_rms_buffer(raw_value);

    d = rms_voltage_ac * ADC_FULL_SCALE / ADC_VREF;

    if (b == 1) {
        if (raw_value > sum / BUFFER_SIZE + CROSSING_THRESHOLD * d) {
            update_freq_buffer();
        }
    } else if (b == 0) {
        if (raw_value < sum / BUFFER_SIZE - CROSSING_THRESHOLD * d) { b = 1; }
    }
}

void ACM_ProcessCurrentSample(uint16_t raw_value)
{
    update_rms_buffer2(raw_value);

    d2 = rms_current_ac * ADC_FULL_SCALE / ADC_VREF;

    if (b2 == 1) {
        if (raw_value > sum2 / BUFFER_SIZE + CROSSING_THRESHOLD * d2) {
            update_phase_buffer();
        }
    } else if (b2 == 0) {
        if (raw_value < sum2 / BUFFER_SIZE - CROSSING_THRESHOLD * d2) { b2 = 1; }
    }
}

/* ------------------------------------------------------------------------- */
/* Power                                                                      */
/* ------------------------------------------------------------------------- */

float S(void)
{
    float s = rms_voltage_ac * rms_current_ac;
    return s;
}

float power(void)
{
    float p = rms_voltage_ac * rms_current_ac * cos(phase);
    if (p < 0) p = p * (-1);
    return p;
}

float Q(void)
{
    float q = rms_voltage_ac * rms_current_ac * sin(phase);
    if (q < 0) q = q * (-1);
    return q;
}
