#ifndef _BACKEND_RGBLED_H_
#define _BACKEND_RGBLED_H_

#include "stm32h7xx.h"

#define RG_PWM_TIM             		        TIM3
#define RG_PWM_TIM_CLK_ENABLE()       		__TIM3_CLK_ENABLE()

#define B_PWM_TIM             		        TIM2
#define B_PWM_TIM_CLK_ENABLE()       		__TIM2_CLK_ENABLE()

//Òý½Åºê¶¨Òå
#define PWM_LEDR_GPIO_CLK_ENABLE()        __GPIOB_CLK_ENABLE()
#define PWM_LEDR_GPIO_PORT                GPIOB
#define PWM_LEDR_GPIO_PIN                 GPIO_PIN_0
#define PWM_LEDR_AF                       GPIO_AF2_TIM3
#define R_PWM_TIM_CH                      TIM_CHANNEL_3

#define PWM_LEDG_GPIO_CLK_ENABLE()        __GPIOB_CLK_ENABLE()
#define PWM_LEDG_GPIO_PORT                GPIOB 
#define PWM_LEDG_GPIO_PIN                 GPIO_PIN_1
#define PWM_LEDG_AF                       GPIO_AF2_TIM3
#define G_PWM_TIM_CH                      TIM_CHANNEL_4

#define PWM_LEDB_GPIO_CLK_ENABLE()        __GPIOA_CLK_ENABLE()
#define PWM_LEDB_GPIO_PORT                GPIOA 
#define PWM_LEDB_GPIO_PIN                 GPIO_PIN_3
#define PWM_LEDB_AF                       GPIO_AF1_TIM2
#define B_PWM_TIM_CH                      TIM_CHANNEL_4




void TIM_GPIO_Config(void);
void TIM_Mode_Config(void);
void SetRGBColor(uint32_t rgb);
void TIM_RGBLED_Close(void);
void SetColorValue(uint8_t r,uint8_t g,uint8_t b);
#endif 


