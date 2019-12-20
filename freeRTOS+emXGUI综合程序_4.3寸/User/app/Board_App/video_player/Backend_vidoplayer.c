#include <stdio.h>
#include <string.h>
#include "stm32h7xx.h"
//#include "ff.h"
#include "Backend_avifile.h"
#include "Backend_vidoplayer.h"
#include "./sai/bsp_sai.h" 
#include "./Bsp/wm8978/bsp_wm8978.h"  
#include "emXGUI.h"
#include "emXGUI_JPEG.h"
#include "GUI_VEDIOPLAYER_DIALOG.h"
#include "x_libc.h"
//#include "./mjpegplayer/GUI_AVIList_DIALOG.h"
//#include "./mjpegplayer/GUI_AVIPLAYER_DIALOG.h"
FIL       fileR ;
UINT      BytesRD;
#define   Frame_Buf_Size    (1024*30)
uint8_t   *Frame_buf;

static volatile uint8_t audiobufflag=0;
//__align(4) uint8_t   Sound_buf[4][1024*5]	__attribute__((at(0xd1bc0000)));
__align(4) uint8_t   Sound_buf[4][1024*5]	__EXRAM;

static uint8_t   *pbuffer;

uint32_t  mid;
uint32_t  Strsize;
uint16_t  Strtype;

TIM_HandleTypeDef TIM3_Handle;

volatile uint8_t video_timeout;
extern WAVEFORMAT*   wavinfo;
extern avih_TypeDef* avihChunk;
//extern HWND avi_wnd_time;
//extern int avi_chl;
void MUSIC_SAI_DMA_TX_Callback(void);

extern GUI_MUTEX*	AVI_JPEG_MUTEX;    // 用于确保一帧图像用后被释放完在退出线程

static volatile int frame=0;
static volatile int t0=0;
volatile int avi_fps=0;

volatile BOOL bDrawVideo=FALSE;
extern SURFACE *pSurf1;

//static u32 alltime = 0;		//总时长 
//u32 cur_time; 		//当前播放时间 
uint8_t tmp=0;	
u32 pos;//文件指针位置
s32 time_sum = 0;
void AVI_play(char *filename)
{
	GUI_SemWait(Delete_VideoTask_Sem,0);//清空一下信号量,确保信号量是由当前播放程序释放的,保证退出的信号量的准确性
	
  FRESULT  res;
  uint32_t offset;
  uint16_t audiosize;
  uint8_t avires=0;
  uint8_t audiosavebuf;
  WCHAR buff[128];

	HDC hdc1;

  bDrawVideo=FALSE;
  Frame_buf = (uint8_t *)GUI_VMEM_Alloc(Frame_Buf_Size);
  
  hdc1 =CreateDC(pSurf1,NULL);
  
  pbuffer=Frame_buf;
//  GUI_DEBUG("%s", filename);
  res=f_open(&fileR,filename,FA_READ);
  if(res!=FR_OK)
  {
    GUI_VMEM_Free(Frame_buf);
    DeleteDC(hdc1);
		GUI_SemPost(Delete_VideoTask_Sem);//文件打开失败,返回前释放程序可以关闭的信号量
    return;    
  }

  res=f_read(&fileR,pbuffer,20480,&BytesRD);


  avires=AVI_Parser(pbuffer);//解析AVI文件格式
  if(avires)
  {
    return;    
  }
  
  avires=Avih_Parser(pbuffer+32);//解析avih数据块
  if(avires)
  {
    return;    
  }
  //strl列表
  avires=Strl_Parser(pbuffer+88);//解析strh数据块
  if(avires)
  {
    return;    
  }
  
  avires=Strf_Parser(pbuffer+164);//解析strf数据块
  if(res!=FR_OK)
  {
    return;    
  }
  
  mid=Search_Movi(pbuffer);//寻找movi ID	（数据块）	
  if(mid==0)
  {
    return;    
  }
//  
  Strtype=MAKEWORD(pbuffer+mid+6);//流类型（movi后面有两个字符）
  Strsize=MAKEDWORD(pbuffer+mid+8);//流大小
  if(Strsize%2)Strsize++;//奇数加1
  f_lseek(&fileR,mid+12);//跳过标志ID  
//  
  offset=Search_Auds(pbuffer);
  if(offset==0)
  {
    return;    
  }  
  audiosize=*(uint8_t *)(pbuffer+offset+4)+256*(*(uint8_t *)(pbuffer+offset+5));
  if(audiosize==0)
  {
    offset=(uint32_t)pbuffer+offset+4;
    mid=Search_Auds((uint8_t *)offset);
    if(mid==0)
    {
      return;    
    }
    audiosize=*(uint8_t *)(mid+offset+4)+256*(*(uint8_t *)(mid+offset+5));
  }
  
  SAI_Play_Stop();			/* 停止I2S录音和放音 */
	wm8978_Reset();		/* 复位WM8978到复位状态 */	
  	/* 配置WM8978芯片，输入为DAC，输出为耳机 */
	wm8978_CfgAudioPath(DAC_ON, SPK_ON|EAR_LEFT_ON | EAR_RIGHT_ON);
  wm8978_OutMute(0);
	/* 调节音量，左右相同音量 */
	wm8978_SetOUT1Volume(VideoDialog.power);

	/* 配置WM8978音频接口为飞利浦标准I2S接口，16bit */
	wm8978_CfgAudioIF(SAI_I2S_STANDARD, 16);
  SAI_GPIO_Config();
  SAIxA_Tx_Config(SAI_I2S_STANDARD, SAI_PROTOCOL_DATASIZE_16BIT, wavinfo->SampleRate);
  SAI_DMA_TX_Callback=MUSIC_SAI_DMA_TX_Callback;			//回调函数指wav_i2s_dma_callback
//  I2S_Play_Stop();
  SAIA_TX_DMA_Init((uint32_t )Sound_buf[1],(uint32_t )Sound_buf[2],audiosize/2);
  audiobufflag=0;	    
  video_timeout=0;
  audiosavebuf=0;
  audiobufflag=0;
  TIM3_Config((avihChunk->SecPerFrame/100)-1,20000-1);
  SAI_Play_Start();  
	
	t0= GUI_GetTickCount();

  
//   //歌曲总长度=每一帧需要的时间（s）*帧总数
  VideoDialog.alltime=(avihChunk->SecPerFrame/1000)*avihChunk->TotalFrame;
  VideoDialog.alltime/=1000;//单位是秒
//  WCHAR buff[128];
//  //char *str = NULL;
// // RECT rc0 = {0, 367,120,30};//当前时间
  x_wsprintf(buff, L"分辨率：%d*%d", videoplayer_img_w, videoplayer_img_h);
  SetWindowText(GetDlgItem(VideoDialog.Video_Hwnd, eID_TEXTBOX_RES), buff);

//  char *ss;
//  int length1=strlen(filename);
//  int length2=strlen(File_Path);
//  if(strncpy(filename,File_Path,length2))//比较前n个字符串，类似strcpy
//  {
//    ss = filename + length2;
//  }
  x_mbstowcs_cp936(buff, lcdlist[VideoDialog.playindex], 200);
  SetWindowText(GetDlgItem(VideoDialog.Video_Hwnd, eID_TEXTBOX_ITEM), buff);
  x_wsprintf(buff, L"%02d:%02d:%02d",
             VideoDialog.alltime/3600,(VideoDialog.alltime%3600)/60,VideoDialog.alltime%60);
  SetWindowText(GetDlgItem(VideoDialog.Video_Hwnd, eID_TEXTBOX_ALLTIME), buff);
//  
  while(!VideoDialog.SWITCH_STATE)//播放循环
  {

    if(!(avi_icon[3].state == FALSE))
    {
      GUI_msleep(10);
      continue;
    }    
		int t1;
    if(!VideoDialog.avi_chl)
    {

        
//        
//   //fptr存放着文件指针的位置，fsize是文件的总大小，两者之间的比例和当前时间与总时长的比例相同（fptr/fsize = cur/all）     
   VideoDialog.curtime=((double)fileR.fptr/fileR.fsize)*VideoDialog.alltime;
//   //更新进度条
   //GUI_DEBUG("%d", VideoDialog.curtime*255/VideoDialog.alltime);
   if(!(SendMessage(VideoDialog.SBN_TIMER_Hwnd, SBM_GETSTATE,0,0)&SST_THUMBTRACK))
    SendMessage(VideoDialog.SBN_TIMER_Hwnd, SBM_SETVALUE, TRUE, VideoDialog.curtime*255/VideoDialog.alltime);     
   //InvalidateRect(VideoDialog.Video_Hwnd, NULL, FALSE);
   
   x_wsprintf(buff, L"%02d:%02d:%02d",///%02d:%02d:%02d alltime/3600,(alltime%3600)/60,alltime%60
              VideoDialog.curtime/3600,(VideoDialog.curtime%3600)/60,VideoDialog.curtime%60); 
   if(!VideoDialog.LIST_STATE)
    SetWindowText(GetDlgItem(VideoDialog.Video_Hwnd, eID_TEXTBOX_CURTIME), buff);    
	 if(Strtype==T_vids)//显示帧
    {    	
			frame++;
			t1 =GUI_GetTickCount();
			if((t1 - t0) >= 1000)
			{
				
				avi_fps =frame;
				t0 =t1;
				frame =0;
			}

      //HDC hdc_mem,hdc;
      pbuffer=Frame_buf;
      

      res = f_read(&fileR,Frame_buf,Strsize+8,&BytesRD);//读入整帧+下一数据流ID信息
//      GUI_DEBUG("%d", GUI_GetTickCount()-tt0);
      
      if(res != FR_OK)
      {
        GUI_DEBUG("E\n");
      }
         
			video_timeout=0;
		
			if(frame&1)
			{	
#if 1		//直接写到窗口方式.	

				HWND hwnd=VideoDialog.Video_Hwnd;
				//HDC hdc;
				//hdc =GetDC(VideoDialog.Video_Hwnd);
				
				//hdc =BeginPaint(hwnd,&ps);
        
        GUI_MutexLock(AVI_JPEG_MUTEX,0xFFFFFFFF);    // 获取互斥量
				//  JPEG_Out(hdc,160,89,Frame_buf,BytesRD);
				JPEG_Out(hdc1,0,0,Frame_buf,BytesRD);
//            ClrDisplay(hdc, &rc0, MapRGB(hdc, 0,0,0));
//            SetTextColor(hdc, MapRGB(hdc,255,255,255));
//            DrawText(hdc, buff,-1,&rc0,DT_VCENTER|DT_CENTER);
            

//           SetWindowText(GetDlgItem(VideoPlayer_hwnd, ID_TB5), buff);
        x_wsprintf(buff, L"帧率：%dFPS/s", avi_fps);
        if(!VideoDialog.LIST_STATE)
          SetWindowText(GetDlgItem(VideoDialog.Video_Hwnd, eID_TEXTBOX_FPS), buff);

        bDrawVideo=TRUE;
//        GUI_msleep(10);
        InvalidateRect(hwnd,NULL,FALSE); //产生无效区...

//			  ReleaseDC(VideoDialog.Video_Hwnd,hdc);
			 // EndPaint(hwnd,&ps);
        GUI_MutexUnlock(AVI_JPEG_MUTEX);              // 解锁互斥量				
#endif

			}

//			while(bDrawVideo==TRUE)
//			{
//				GUI_msleep(5);
//			}

      while(video_timeout==0)
      {   
				//rt_thread_delay(1); //不要死等，最好用信号量.				
        GUI_msleep(5);
      }      
	  bDrawVideo=FALSE;

      video_timeout=0;
    }//显示帧
    else if(Strtype==T_auds)//音频输出
    { 
      uint8_t i;
      audiosavebuf++;
      if(audiosavebuf>3)
			{
				audiosavebuf=0;
			}	
      do
      {
				//rt_thread_delay(1); 
        i=audiobufflag;
        if(i)
					i--;
        else 
					i=3; 

      }while(audiobufflag==i);
      AVI_DEBUG("S\n");

      res = f_read(&fileR,Sound_buf[audiosavebuf],Strsize+8,&BytesRD);//读入整帧+下一数据流ID信息
      if(res != FR_OK)
      {
        GUI_DEBUG("E\n");
      }
      
      pbuffer=Sound_buf[audiosavebuf];      
      
    }
    else 
      break;
					   	
    }
    else
    {
         pos = fileR.fptr;
         //根据进度条调整播放位置				
         tmp=SendMessage(VideoDialog.SBN_TIMER_Hwnd, SBM_GETVALUE, NULL, NULL); 
         time_sum = fileR.fsize/VideoDialog.alltime*(tmp*VideoDialog.alltime/249-VideoDialog.curtime);//跳过多少数据 计算公式：文件总大小/需要跳过的数据量 = 总时间/当前的时间
         //如果当前文件指针未到最后
        	if(pos<fileR.fsize)pos+=time_sum; 
         //如果文件指针到了最后30K内容
          if(pos>(fileR.fsize-1024*30))
          {
            pos=fileR.fsize-1024*30;
          }
         
          f_lseek(&fileR,pos);
//      
      #if 0
         if(pos == 0)
            mid=Search_Movi(Frame_buf);//寻找movi ID  判断自己是不是还在数据段
         else 
            mid = 0;  
        int iiii= 0;//计算偏移量
         while(1)
         {
            //每次读512个字节，直到找到数据帧的帧头
            u16 temptt = 0;//计算数据帧的位置
            AVI_DEBUG("S\n");

            f_read(&fileR,Frame_buf,512,&BytesRD);
            AVI_DEBUG("E\n");

            temptt = Search_Fram(Frame_buf,BytesRD);
            iiii++;
            if(temptt)
            {            
               AVI_DEBUG("S temptt =%d\n",temptt);
               AVI_DEBUG("S Frame_buf[temptt] =%c %c %c %c\n",
                                      Frame_buf[temptt],
                                      Frame_buf[temptt+1],
                                      Frame_buf[temptt+2],
                                      Frame_buf[temptt+3]);
               /* 多读取512数据，防止标志在边界时出错 */
               f_read(&fileR,(u8*)Frame_buf+BytesRD,512,&BytesRD);
               AVI_DEBUG("E\n");
                pbuffer = Frame_buf;
               Strtype=MAKEWORD(pbuffer+temptt+2);//流类型
               Strsize=MAKEDWORD(pbuffer+temptt+4);//流大小
               mid += temptt + 512*iiii-512;//加上偏移量
//               if(temptt == 16)
//                  continue;
               break;
            }

         }
         #else
         f_read(&fileR,Frame_buf,1024*30,&BytesRD);
         AVI_DEBUG("E\n");
         if(pos == 0)
            mid=Search_Movi(Frame_buf);//寻找movi ID
         else 
            mid = 0;
         mid += Search_Fram(Frame_buf,1024*30);
         pbuffer = Frame_buf;
         Strtype=MAKEWORD(pbuffer+mid+2);//流类型
         Strsize=MAKEDWORD(pbuffer+mid+4);//流大小
         #endif
         
         if(Strsize%2)Strsize++;//奇数加1
         f_lseek(&fileR,pos+mid+8);//跳过标志ID  
         AVI_DEBUG("S Strsize=%d\n",Strsize);

         f_read(&fileR,Frame_buf,Strsize+8,&BytesRD);//读入整帧+下一数据流ID信息 
         
//         
         VideoDialog.avi_chl = 0;    
     }
         //判断下一帧的帧内容 
         Strtype=MAKEWORD(pbuffer+Strsize+2);//流类型
         Strsize=MAKEDWORD(pbuffer+Strsize+4);//流大小									
         if(Strsize%2)Strsize++;//奇数加1		  
//        
     }
//  
// 

	GUI_VMEM_Free(Frame_buf);
  DeleteDC(hdc1);
  if(!VideoDialog.SWITCH_STATE)
  {
    VideoDialog.playindex++;
    if(VideoDialog.playindex > VideoDialog.avi_file_num - 1) 
      VideoDialog.playindex = 0;
  }
  else
    VideoDialog.SWITCH_STATE = 0;
  SAI_Play_Stop();
	wm8978_Reset();	/* 复位WM8978到复位状态 */
  HAL_TIM_Base_Stop_IT(&TIM3_Handle); //停止定时器3更新中断
  f_close(&fileR);
	GUI_SemPost(Delete_VideoTask_Sem);

}

void MUSIC_SAI_DMA_TX_Callback(void)
{
  audiobufflag++;
  if(audiobufflag>3)
	{
		audiobufflag=0;
	}
	
	if(DMA1_Stream2->CR&(1<<19)) //当前读取Memory1数据
	{
		//DMA_MemoryTargetConfig(DMA1_Stream2,(uint32_t)Sound_buf[audiobufflag], DMA_Memory_0);
    HAL_DMAEx_ChangeMemory(&h_txdma, (uint32_t)Sound_buf[audiobufflag], MEMORY0);
	}
	else
	{
    HAL_DMAEx_ChangeMemory(&h_txdma, (uint32_t)Sound_buf[audiobufflag], MEMORY1); 
	} 
}

/**
  * @brief  通用定时器3中断初始化
  * @param  period : 自动重装值。
  * @param  prescaler : 时钟预分频数
  * @retval 无
  * @note   定时器溢出时间计算方法:Tout=((period+1)*(prescaler+1))/Ft us.
  *          Ft=定时器工作频率,为SystemCoreClock/2=90,单位:Mhz
  */
  
void TIM3_Config(uint16_t period,uint16_t prescaler)
{	
  __TIM3_CLK_ENABLE();

  TIM3_Handle.Instance = TIM3;
  /* 累计 TIM_Period个后产生一个更新或者中断*/		
  //当定时器从0计数到4999，即为5000次，为一个定时周期
  TIM3_Handle.Init.Period = period;
  //定时器时钟源TIMxCLK = 2 * PCLK1  
  //				PCLK1 = HCLK / 4 
  //				=> TIMxCLK=HCLK/2=SystemCoreClock/2=200MHz
  // 设定定时器频率为=TIMxCLK/(TIM_Prescaler+1)=10000Hz
  TIM3_Handle.Init.Prescaler =  prescaler;
  TIM3_Handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  TIM3_Handle.Init.CounterMode = TIM_COUNTERMODE_UP;
  // 初始化定时器TIM
  HAL_TIM_Base_Init(&TIM3_Handle);
  
  HAL_NVIC_SetPriority(TIM3_IRQn, 2, 0);
  HAL_NVIC_EnableIRQ(TIM3_IRQn); 
  // 开启定时器更新中断
  HAL_TIM_Base_Start_IT(&TIM3_Handle);  
   
  
}

/**
  * @brief  定时器3中断服务函数
  * @param  无
  * @retval 无
  */
void TIM3_IRQHandler(void)
{
  HAL_TIM_IRQHandler(&TIM3_Handle);
}
