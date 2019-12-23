/**
  ******************************************************************
  * @file    bsp_adcd.c
  * @author  fire
  * @version V1.1
  * @date    2018-xx-xx
  * @brief   adc应用函数接口
  ******************************************************************
  * @attention
  *
  * 实验平台:野火 STM32H743开发板 
  * 公司    :http://www.embedfire.com
  * 论坛    :http://www.firebbs.cn
  * 淘宝    :https://fire-stm32.taobao.com
  *
  ******************************************************************
  */
#include "bsp_adc.h" 
#include "emXGUI.h"

extern float ADC_vol;

ADC_HandleTypeDef ADC_Handle;
DMA_HandleTypeDef hdma_adc;
__attribute__ ((at(0x24002000))) __IO uint16_t ADC_ConvertedValue=0 ;//__EXRAM;//


/**
  * @brief  ADC引脚配置函数
  * @param  无
  * @retval 无
  */  
static void ADC_GPIO_Mode_Config(void)
{
    /* 定义一个GPIO_InitTypeDef类型的结构体 */
    GPIO_InitTypeDef  GPIO_InitStruct;
    /* 使能ADC引脚的时钟 */
    RHEOSTAT_ADC_GPIO_CLK_ENABLE();
    
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG; 
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Pin = RHEOSTAT_ADC_PIN; 
    /* 配置为模拟输入，不需要上拉电阻 */ 
    HAL_GPIO_Init(RHEOSTAT_ADC_GPIO_PORT, &GPIO_InitStruct);
  
}

/**
  * @brief  ADC工作模式配置函数
  * @param  无
  * @retval 无
  */ 
static void ADC_Mode_Config(void)
{
    ADC_ChannelConfTypeDef ADC_Config;
  
    RCC_PeriphCLKInitTypeDef RCC_PeriphClkInit;  
  	RCC_PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;		
		RCC_PeriphClkInit.AdcClockSelection = RCC_ADCCLKSOURCE_CLKP; 		
		HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphClkInit);  
	
    /* 使能ADC时钟 */
    RHEOSTAT_ADC_CLK_ENABLE();
    /* 使能DMA时钟 */
    RHEOSTAT_ADC_DMA_CLK_ENABLE();
    
    //选择DMA1的Stream1
    hdma_adc.Instance = RHEOSTAT_ADC_DMA_Base;
    //ADC1的DMA请求
    hdma_adc.Init.Request = RHEOSTAT_ADC_DMA_Request;
    //传输方向：外设-》内存
    hdma_adc.Init.Direction = DMA_PERIPH_TO_MEMORY;
    //外设地址不自增
    hdma_adc.Init.PeriphInc = DMA_PINC_DISABLE;
    //内存地址不自增
    hdma_adc.Init.MemInc = DMA_PINC_DISABLE;
    //外设数据宽度：半字
    hdma_adc.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    //内存数据宽度：半字
    hdma_adc.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    //DMA循环传输
    hdma_adc.Init.Mode = DMA_CIRCULAR;
    //DMA的软件优先级：低
    hdma_adc.Init.Priority = DMA_PRIORITY_LOW;
    //FIFO模式关闭
    hdma_adc.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    //DMA初始化
    HAL_DMA_Init(&hdma_adc);
    //hdma_adc和ADC_Handle.DMA_Handle链接
    __HAL_LINKDMA(&ADC_Handle,DMA_Handle,hdma_adc);    
      
    
    ADC_Handle.Instance = RHEOSTAT_ADC;
    //使能Boost模式
    ADC_Handle.Init.BoostMode = ENABLE;
    //ADC时钟1分频
    ADC_Handle.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
    //使能连续转换模式
    ADC_Handle.Init.ContinuousConvMode = ENABLE;
    //数据存放在数据寄存器中
    ADC_Handle.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DMA_CIRCULAR;
    //关闭不连续转换模式
    ADC_Handle.Init.DiscontinuousConvMode = DISABLE;
    //单次转换
    ADC_Handle.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    //软件触发
    ADC_Handle.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    //关闭低功耗自动等待
    ADC_Handle.Init.LowPowerAutoWait = DISABLE;
    //数据溢出时，覆盖写入
    ADC_Handle.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
    //不使能过采样模式
    ADC_Handle.Init.OversamplingMode = DISABLE;
    //分辨率为：16bit
    ADC_Handle.Init.Resolution = ADC_RESOLUTION_16B;
    //不使能多通道扫描
    ADC_Handle.Init.ScanConvMode = DISABLE;
    //初始化 ADC
    HAL_ADC_Init(&ADC_Handle);
          
    //使用通道18
    ADC_Config.Channel = RHEOSTAT_ADC_CHANNEL;
    //转换顺序为1
    ADC_Config.Rank = ADC_REGULAR_RANK_1;
    //采样周期为64.5个周期
    ADC_Config.SamplingTime = ADC_SAMPLETIME_64CYCLES_5;
    //不使用差分输入的功能
    ADC_Config.SingleDiff = ADC_SINGLE_ENDED ;
    //配置ADC通道
    HAL_ADC_ConfigChannel(&ADC_Handle, &ADC_Config);    
    
    //使能ADC1、2
    ADC_Enable(&ADC_Handle);
    
    HAL_ADC_Start_DMA(&ADC_Handle, (uint32_t*)&ADC_ConvertedValue, 1);
    
}
/**
  * @brief  ADC中断优先级配置函数
  * @param  无
  * @retval 无
  */  
void Rheostat_ADC_NVIC_Config(void)
{
    HAL_NVIC_SetPriority(Rheostat_ADC12_IRQ, 6, 0);
    HAL_NVIC_EnableIRQ(Rheostat_ADC12_IRQ);
}

/**
  * @brief  ADC初始化函数
  * @param  无
  * @retval 无
  */
void ADC_Init(void)
{
    
    ADC_GPIO_Mode_Config();
  
    ADC_Mode_Config();
  
		Rheostat_ADC_NVIC_Config();
	
    HAL_ADC_Start(&ADC_Handle);
}

/**
  * @brief  转换完成中断回调函数（非阻塞模式）
  * @param  AdcHandle : ADC句柄
  * @retval 无
  */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* AdcHandle)
{
  /* 获取结果 */
//    ADC_ConvertedValue = HAL_ADC_GetValue(AdcHandle); 
}

void Rheostat_DISABLE(void)
{
	// 使能ADC DMA
	HAL_ADC_Stop(&ADC_Handle);
	
	ADC_Disable(&ADC_Handle);//他停止采集
	
	HAL_ADC_Stop_DMA(&ADC_Handle);
	
}

void RHEOSTAT_ADC_DMA_IRQHandler()
{
	HAL_DMA_IRQHandler(ADC_Handle.DMA_Handle);
}

/*********************************************END OF FILE**********************/


