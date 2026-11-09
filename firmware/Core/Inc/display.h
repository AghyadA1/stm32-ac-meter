/**
 * @file    display.h
 * @brief   3-digit multiplexed 7-segment display (5361AS, common cathode).
 *
 * Segments are driven by an SN7448N BCD decoder (inputs BCD_A..BCD_D) and the
 * digits are selected through a ULN2003A (DIGIT1..DIGIT3). TIM2 interrupts at
 * 180 Hz light one digit per interrupt, so the whole number is refreshed
 * every ~16.7 ms, fast enough to look steady.
 */
#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

void DisplayNumber(int number);  /* split 0..999 into three digits    */
void SetBCD(uint8_t value);      /* drive BCD_A..BCD_D for one digit   */
void Display_Refresh(void);      /* light the next digit (TIM2 ISR)    */

#endif /* DISPLAY_H */
