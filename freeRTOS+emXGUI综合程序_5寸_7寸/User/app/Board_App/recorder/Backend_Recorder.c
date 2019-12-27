/*
******************************************************************************
* @file    recorder.c
* @author  fire
* @version V1.0
* @date    2015-xx-xx
* @brief   WM8978录音功能测试
******************************************************************************
* @attention
*
* 实验平台:野火  STM32 H743 开发板  
* 论坛    :http://www.firebbs.cn
* 淘宝    :https://fire-stm32.taobao.com
*
******************************************************************************
*/
#include "board.h"
#include "ff.h" 
#include "Backend_Recorder.h"
#include "./app/Board_App/mp3_player/Backend_mp3Player.h"

/* 音频格式切换列表(可以自定义) */
#define FMT_COUNT	6		/* 音频格式数组元素个数 */

/* 录音文件路径全称：初始化为rec001.wav */
char recfilename[25]={"0:/recorder/rec001.wav"};   
extern REC_TYPE Recorder;          /* 录音设备 */
WavHead rec_wav;            /* WAV设备  */
uint8_t Isread=0;           /* DMA传输完成标志 */
uint8_t bufflag=0;          /* 数据缓存区选择标志 */
uint32_t wavsize=0;         /* wav音频数据大小 */
__attribute__((at(0x24008000))) uint16_t record_buffer0[RECBUFFER_SIZE];//	__attribute__((at(0x24008000)));//__EXRAM;  /* 数据缓存区1 ，实际占用字节数：RECBUFFER_SIZE*2 BUF不放在外部会有不能读取SD卡的BUG*/
__attribute__((at(0x24004000))) uint16_t record_buffer1[RECBUFFER_SIZE];//	__attribute__((at(0x24004000)));//__EXRAM;  /* 数据缓存区2 ，实际占用字节数：RECBUFFER_SIZE*2 */

FIL record_file	__EXRAM;			/* file objects */
extern FRESULT result; 
extern UINT bw;            					/* File R/W count */

uint32_t g_FmtList[FMT_COUNT][3] =
{
	{I2S_STANDARD_PHILIPS, I2S_DATAFORMAT_16B, I2S_AUDIOFREQ_8K},
	{I2S_STANDARD_PHILIPS, I2S_DATAFORMAT_16B, I2S_AUDIOFREQ_16K},
	{I2S_STANDARD_PHILIPS, I2S_DATAFORMAT_16B, I2S_AUDIOFREQ_22K},
	{I2S_STANDARD_PHILIPS, I2S_DATAFORMAT_16B, I2S_AUDIOFREQ_44K},
	{I2S_STANDARD_PHILIPS, I2S_DATAFORMAT_16B, I2S_AUDIOFREQ_96K},
	{I2S_STANDARD_PHILIPS, I2S_DATAFORMAT_16B, I2S_AUDIOFREQ_192K},
};

//extern const uint16_t recplaybuf[4];//2个16位数据,用于录音时I2S Master发送.循环发送0.
__EXRAM uint16_t recplaybuf[4]={0X0000,0X0000};
/* 仅允许本文件内调用的函数声明 */

void MusicPlayer_SAI_DMA_TX_Callback(void);

/**
  * @brief  配置WM8978和STM32的I2S开始录音。
  * @param  无
  * @retval 无
  */
void StartRecord(const char *filename)
{
#if 1
	uint8_t ucRefresh;	/* 通过串口打印相关信息标志 */
	DIR dir;
	
	Recorder.ucStatus=STA_IDLE;    /* 开始设置为空闲状态  */
	Recorder.ucInput=0;            /* 缺省MIC输入  */
	Recorder.ucFmtIdx=3;           /* 缺省飞利浦I2S标准，16bit数据长度，44K采样率  */
	Recorder.ucVolume=50;          /* 缺省耳机音量  */
	if(Recorder.ucInput==0) //MIC 
	{
		Recorder.ucGain=50;          /* 缺省MIC增益  */
		rec_wav.wChannels=2;         /* 缺省MIC单通道 */
	}
	else                    //LINE
	{
		Recorder.ucGain=6;           /* 缺省线路输入增益 */
		rec_wav.wChannels=2;         /* 缺省线路输入双声道 */
	}
	
	rec_wav.riff=0x46464952;       /* “RIFF”; RIFF 标志 */
	rec_wav.size_8=0;              /* 文件长度，未确定 */
	rec_wav.wave=0x45564157;       /* “WAVE”; WAVE 标志 */ 
	
	rec_wav.fmt=0x20746d66;        /* “fmt ”; fmt 标志，最后一位为空 */
	rec_wav.fmtSize=16;            /* sizeof(PCMWAVEFORMAT) */ 
	rec_wav.wFormatTag=1;          /* 1 表示为PCM 形式的声音数据 */ 
	/* 每样本的数据位数，表示每个声道中各个样本的数据位数。 */
	rec_wav.wBitsPerSample=16;
	/* 采样频率（每秒样本数） */
	rec_wav.dwSamplesPerSec=g_FmtList[Recorder.ucFmtIdx][2];
	/* 每秒数据量；其值为通道数×每秒数据位数×每样本的数据位数／ 8。 */
	rec_wav.dwAvgBytesPerSec=rec_wav.wChannels*rec_wav.dwSamplesPerSec*rec_wav.wBitsPerSample/8;  
	/* 数据块的调整数（按字节算的），其值为通道数×每样本的数据位值／8。 */
	rec_wav.wBlockAlign=rec_wav.wChannels*rec_wav.wBitsPerSample/8; 
	
	rec_wav.data=0x61746164;       /* “data”; 数据标记符 */
	rec_wav.datasize=0;            /* 语音数据大小 目前未确定*/
	
	/*  初始化并配置I2S  */
	I2S_Stop();
  I2S_GPIO_Config();
  I2Sx_Mode_Config(g_FmtList[Recorder.ucFmtIdx][0],g_FmtList[Recorder.ucFmtIdx][1],g_FmtList[Recorder.ucFmtIdx][2]);
	I2Sxext_Mode_Config(g_FmtList[Recorder.ucFmtIdx][0],g_FmtList[Recorder.ucFmtIdx][1],g_FmtList[Recorder.ucFmtIdx][2]);
	I2S_Play_Stop();
	I2Sxext_Recorde_Stop();
	
	ucRefresh = 1;
	bufflag=0;
	Isread=0;
#endif

	printf("当前录音文件 -> %s\n",filename);
	result=f_open(&record_file,filename,FA_CREATE_ALWAYS|FA_WRITE);
	if(result!=FR_OK)
	{
		printf("Open wavfile fail!!!->%d\r\n",result);
		result = f_close (&record_file);
		Recorder.ucStatus = STA_ERR;
		return;
	}
	
	// 写入WAV文件头，这里必须写入写入后文件指针自动偏移到sizeof(rec_wav)位置，
	// 接下来写入音频数据才符合格式要求。
	result=f_write(&record_file,(const void *)&rec_wav,sizeof(rec_wav),&bw);
	
	GUI_msleep(10);		/* 延迟一段时间，等待I2S中断结束 */
	I2S_Stop();			/* 停止I2S录音和放音 */	
	wm8978_Reset();		/* 复位WM8978到复位状态 */
  wm8978_CtrlGPIO1(0);
	Recorder.ucStatus = STA_RECORDING;		/* 录音状态 */
		
	/* 调节放音音量，左右相同音量 */
	wm8978_SetOUT1Volume(Recorder.ucVolume);

	if(Recorder.ucInput == 1)   /* 线输入 */
	{
		/* 配置WM8978芯片，输入为线输入，输出为耳机 */
		wm8978_CfgAudioPath(LINE_ON | ADC_ON, EAR_LEFT_ON | EAR_RIGHT_ON);
		wm8978_SetLineGain(Recorder.ucGain);
	}
	else   /* MIC输入 */
	{
		/* 配置WM8978芯片，输入为Mic，输出为耳机 */
		//wm8978_CfgAudioPath(MIC_LEFT_ON | ADC_ON, EAR_LEFT_ON | EAR_RIGHT_ON);
		//wm8978_CfgAudioPath(MIC_RIGHT_ON | ADC_ON, EAR_LEFT_ON | EAR_RIGHT_ON);
		wm8978_CfgAudioPath(MIC_LEFT_ON | MIC_RIGHT_ON | ADC_ON, EAR_LEFT_ON | EAR_RIGHT_ON);	
		wm8978_SetMicGain(Recorder.ucGain);	
	}
		
	/* 配置WM8978音频接口为飞利浦标准I2S接口，16bit */
	 wm8978_CfgAudioIF(I2S_STANDARD_PHILIPS, 16);
	
	I2Sxext_Mode_Config(g_FmtList[Recorder.ucFmtIdx][0],g_FmtList[Recorder.ucFmtIdx][1],g_FmtList[Recorder.ucFmtIdx][2]);

	I2Sxext_RX_DMA_Init((uint32_t)record_buffer0,(uint32_t)record_buffer1,RECBUFFER_SIZE);
  	
	I2Sxext_Recorde_Start();
}

void MusicPlayer_I2S_DMA_RX_Callback(void)
{
	if(Recorder.ucStatus == STA_RECORDING)
	{
		if(I2Sx_RX_DMA_STREAM->CR&(1<<19)) //当前使用Memory1数据
		{
			bufflag=0;                       //可以将数据读取到缓冲区0
		}
		else                               //当前使用Memory0数据
		{
			bufflag=1;                       //可以将数据读取到缓冲区1
		}
		Isread=1;                          // DMA传输完成标志
	}
}

/***************************** (END OF FILE) *********************************/
