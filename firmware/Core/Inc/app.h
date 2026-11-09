/**
 * @file    app.h
 * @brief   Application glue: starts the peripherals, handles the button,
 *          and chooses what the display shows.
 *
 * Hook it into a CubeMX-generated main.c:
 *
 *     / * USER CODE BEGIN 2 * /
 *     App_Start();
 *     / * USER CODE END 2 * /
 *
 *     while (1)
 *     {
 *       App_Loop();
 *     }
 *
 * The HAL callbacks (ADC, TIM2, EXTI) are defined in app.c.
 */
#ifndef APP_H
#define APP_H

void App_Start(void);
void App_Loop(void);

#endif /* APP_H */
