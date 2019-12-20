#include "emXGUI.h"
#include "x_libc.h"
#include "./led/bsp_led.h"  
#include "Backend_RGBLED.h" 


 /**
  * @brief  配置TIM3复用输出PWM时用到的I/O
  * @param  无
  * @retval 无
  */
void TIM_GPIO_Config(void) 
{
  GPIO_InitTypeDef GPIO_InitStruct;
  
  PWM_LEDR_GPIO_CLK_ENABLE();
  PWM_LEDG_GPIO_CLK_ENABLE();
  PWM_LEDB_GPIO_CLK_ENABLE();

  GPIO_InitStruct.Pin = PWM_LEDR_GPIO_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP; 
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH ;
  GPIO_InitStruct.Alternate = PWM_LEDR_AF;
  HAL_GPIO_Init(PWM_LEDR_GPIO_PORT, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = PWM_LEDG_GPIO_PIN;
  GPIO_InitStruct.Alternate = PWM_LEDG_AF;
  HAL_GPIO_Init(PWM_LEDG_GPIO_PORT, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = PWM_LEDB_GPIO_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP; 
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH ;
  GPIO_InitStruct.Alternate = PWM_LEDB_AF;

  HAL_GPIO_Init(PWM_LEDB_GPIO_PORT, &GPIO_InitStruct);  
   
}


//使用全局变量，避免出错
TIM_HandleTypeDef TIM_Handle;
TIM_OC_InitTypeDef sConfig;
/**
  * @brief  配置TIM5输出的PWM信号的模式，如周期、极性
  * @param  无
  * @retval 无
  */
/*
 * TIMxCLK/CK_PSC --> TIMxCNT --> TIMx_ARR --> 中断 & TIMxCNT 重新计数
 *                    TIMx_CCR(电平发生变化)
 *
 * 信号周期=(TIMx_ARR +1 ) * 时钟周期
 * 
 */
/*    _______    ______     _____      ____       ___        __         _
 * |_|       |__|      |___|     |____|    |_____|   |______|  |_______| |________|
 */
void TIM_Mode_Config(void)
{

  //使能TIM5的时钟
  RGB_PWM_TIM_CLK_ENABLE();    
  
  TIM_Handle.Instance = RGB_PWM_TIM;
  TIM_Handle.Init.CounterMode = TIM_COUNTERMODE_UP;
  TIM_Handle.Init.Period = 255 - 1;
  TIM_Handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  TIM_Handle.Init.Prescaler =  2500 - 1;   
  // 初始化定时器TIM
  HAL_TIM_PWM_Init(&TIM_Handle);

  sConfig.OCMode = TIM_OCMODE_PWM1;
  //CHx的有效电平为高电平
  sConfig.OCPolarity = TIM_OCPOLARITY_LOW;
  //CHx在空闲状态下为低电平
  sConfig.OCIdleState = TIM_OCNIDLESTATE_SET;
  //比较器CCR的值
  sConfig.Pulse = 0;  
  //初始化输出比较通道
  HAL_TIM_PWM_ConfigChannel(&TIM_Handle, &sConfig, R_PWM_TIM_CH);
  HAL_TIM_PWM_ConfigChannel(&TIM_Handle, &sConfig, G_PWM_TIM_CH);
	HAL_TIM_PWM_ConfigChannel(&TIM_Handle, &sConfig, B_PWM_TIM_CH);

  HAL_TIM_PWM_Start(&TIM_Handle, R_PWM_TIM_CH);
  HAL_TIM_PWM_Start(&TIM_Handle, G_PWM_TIM_CH);
  HAL_TIM_PWM_Start(&TIM_Handle, B_PWM_TIM_CH);
}

//RGBLED显示颜色
void SetRGBColor(uint32_t rgb)
{
	uint8_t r=0,g=0,b=0;
	r=(uint8_t)(rgb>>16);
	g=(uint8_t)(rgb>>8);
	b=(uint8_t)rgb;
	TIM5->CCR1 = r;	//根据PWM表修改定时器的比较寄存器值
	TIM5->CCR2 = g;	//根据PWM表修改定时器的比较寄存器值        
	TIM5->CCR3 = b;	//根据PWM表修改定时器的比较寄存器值
}

//RGBLED显示颜色
void SetColorValue(uint8_t r,uint8_t g,uint8_t b)
{
	TIM5->CCR1 = r;	//根据PWM表修改定时器的比较寄存器值
	TIM5->CCR2 = g;	//根据PWM表修改定时器的比较寄存器值        
	TIM5->CCR3 = b;	//根据PWM表修改定时器的比较寄存器值
}

//停止pwm输出
void TIM_RGBLED_Close(void)
{
	SetColorValue(0,0,0);

  HAL_TIM_PWM_Stop(&TIM_Handle, R_PWM_TIM_CH);
  HAL_TIM_PWM_Stop(&TIM_Handle, G_PWM_TIM_CH);
  HAL_TIM_PWM_Stop(&TIM_Handle, B_PWM_TIM_CH);

  __HAL_TIM_ENABLE(&TIM_Handle);
  
  __TIM5_CLK_DISABLE();
	LED_GPIO_Config();
  
}


