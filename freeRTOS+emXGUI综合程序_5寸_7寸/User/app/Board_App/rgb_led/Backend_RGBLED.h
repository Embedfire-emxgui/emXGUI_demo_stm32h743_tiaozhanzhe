#ifndef _BACKEND_RGBLED_H_
#define _BACKEND_RGBLED_H_

#include "stm32h7xx.h"

#define RGB_PWM_TIM             		        TIM5
#define RGB_PWM_TIM_CLK_ENABLE()       			__TIM5_CLK_ENABLE()

//Òý½Åºê¶¨Òå
#define PWM_LEDR_GPIO_CLK_ENABLE()        __GPIOH_CLK_ENABLE()
#define PWM_LEDR_GPIO_PORT                GPIOH
#define PWM_LEDR_GPIO_PIN                 GPIO_PIN_10
#define PWM_LEDR_AF                       GPIO_AF2_TIM5
#define R_PWM_TIM_CH                      TIM_CHANNEL_1

#define PWM_LEDG_GPIO_CLK_ENABLE()        __GPIOH_CLK_ENABLE()
#define PWM_LEDG_GPIO_PORT                GPIOH
#define PWM_LEDG_GPIO_PIN                 GPIO_PIN_11
#define PWM_LEDG_AF                       GPIO_AF2_TIM5
#define G_PWM_TIM_CH                      TIM_CHANNEL_2

#define PWM_LEDB_GPIO_CLK_ENABLE()        __GPIOH_CLK_ENABLE()
#define PWM_LEDB_GPIO_PORT                GPIOH
#define PWM_LEDB_GPIO_PIN                 GPIO_PIN_12
#define PWM_LEDB_AF                       GPIO_AF2_TIM5
#define B_PWM_TIM_CH                      TIM_CHANNEL_3




void TIM_GPIO_Config(void);
void TIM_Mode_Config(void);
void SetRGBColor(uint32_t rgb);
void TIM_RGBLED_Close(void);
void SetColorValue(uint8_t r,uint8_t g,uint8_t b);
#endif 


