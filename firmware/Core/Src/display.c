/**
 * @file    display.c
 * @brief   Multiplexed 3-digit 7-segment display driver.
 */
#include "display.h"
#include "main.h"   /* CubeMX pin names: BCD_x_Pin, DIGITx_Pin, ... */

static uint8_t current_digit   = 0;
static uint8_t digit_values[3] = {0, 0, 0};

static GPIO_TypeDef *const BCD_PORT[] = {
    BCD_A_GPIO_Port, BCD_B_GPIO_Port, BCD_C_GPIO_Port, BCD_D_GPIO_Port
};
static const uint16_t BCD_PIN[] = {
    BCD_A_Pin, BCD_B_Pin, BCD_C_Pin, BCD_D_Pin
};

void DisplayNumber(int number)
{
    if (number < 0)   number = 0;
    if (number > 999) number = 999;

    digit_values[0] = number / 100;
    digit_values[1] = (number / 10) % 10;
    digit_values[2] = number % 10;
}

void SetBCD(uint8_t value)
{
    for (int i = 0; i < 4; i++) {
        HAL_GPIO_WritePin(BCD_PORT[i], BCD_PIN[i],
                          (value >> i) & 0x01 ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
}

void Display_Refresh(void)
{
    // Disable all digits
    HAL_GPIO_WritePin(DIGIT1_GPIO_Port, DIGIT1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(DIGIT2_GPIO_Port, DIGIT2_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(DIGIT3_GPIO_Port, DIGIT3_Pin, GPIO_PIN_RESET);

    // Set BCD value for current digit
    SetBCD(digit_values[current_digit]);

    // Enable current digit
    switch (current_digit) {
    case 0: HAL_GPIO_WritePin(DIGIT1_GPIO_Port, DIGIT1_Pin, GPIO_PIN_SET); break;
    case 1: HAL_GPIO_WritePin(DIGIT2_GPIO_Port, DIGIT2_Pin, GPIO_PIN_SET); break;
    case 2: HAL_GPIO_WritePin(DIGIT3_GPIO_Port, DIGIT3_Pin, GPIO_PIN_SET); break;
    }

    // Next digit
    current_digit = (current_digit + 1) % 3;
}
