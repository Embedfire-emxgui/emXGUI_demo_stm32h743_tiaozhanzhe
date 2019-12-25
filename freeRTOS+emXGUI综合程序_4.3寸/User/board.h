#ifndef __BOARD_H__
#define __BOARD_H__

/*
*************************************************************************
*                             包含的头文件
*************************************************************************
*/
/* STM32 固件库头文件 */
#include "stm32h7xx.h"

/* 开发板硬件bsp头文件 */
#include "./led/bsp_led.h" 
//#include "./key/bsp_key.h" 
#include "./lcd/bsp_lcd.h"
#include "./sdram/bsp_sdram.h" 
#include "./touch/bsp_i2c_touch.h"
#include "./usart/bsp_usart.h"
//#include "./flash/bsp_spi_flash.h"
//#include "./font/fonts.h"
#include "./flash/bsp_qspi_flash.h"
#include "./touch/gt9xx.h"
#include "./cm_backtrace/cm_backtrace.h"
#include "./sd_card/bsp_sdio_sd.h"
#include "./mpu/bsp_mpu.h" 
#include "./beep/bsp_beep.h"   
#include "bsp_adc.h" 
#include "./app/Board_App/Phone_SMS/bsp_sim/bsp_gsm_usart.h"
#include "./app/Board_App/Phone_SMS/bsp_sim/bsp_gsm_gprs.h"
#include "./app/Board_App/clock/RTC/bsp_rtc.h"
#include "./app/Board_App/gyro/i2c_for_mpu6050/MPU6050_i2c.h"
#include "./app/Board_App/gyro/mpu6050/bsp_mpu_exti.h"
#include "./app/Board_App/camera/ov5640_AF.h"
#include "./app/Board_App/camera/bsp_ov5640.h"
#include "./wm8978/bsp_wm8978.h"  
#include "./i2c/i2c.h"

/*
*************************************************************************
*                               函数声明
*************************************************************************
*/
	

#endif /* __BOARD_H__ */
