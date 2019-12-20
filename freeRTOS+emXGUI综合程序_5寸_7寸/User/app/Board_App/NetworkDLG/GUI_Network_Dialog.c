#include <emXGUI.h>
#include <string.h>
#include <stdio.h>
#include "x_libc.h"
#include "GUI_AppDef.h"
#include "netconf.h"
#include "GUI_Network_Dialog.h"
#include "tcp_echoclient.h"
#include "tcp_echoserver.h"
#include "udp_echoclient.h"

int		number_input_box(int x, int y, int w, int h,
							const WCHAR *pCaption,
							WCHAR *pOut,
							int MaxCount,
							HWND hwndParent);

/* 单选框 ID */
#define ID_RB1    (0x1100 | (1<<16))
#define ID_RB2    (0x1101 | (1<<16))
#define ID_RB3    (0x1102 | (1<<16))

TaskHandle_t Network_Task_Handle=NULL;
TaskHandle_t TCPIP_Init_Task_Handle=NULL;
SemaphoreHandle_t Wait_TCPIP_Init_Sem;
extern void My_TCPIP_initialization(uint8_t *ipaddr_test);
int8_t NetworkTypeSelection = 0;

HWND Send_Handle;
HWND Receive_Handle;
HWND Network_Main_Handle;
HWND Message_Hwnd;

uint8_t network_start_flag=0;
uint8_t LWIP_Init_Start = 0;
uint8_t LWIP_Init_End = 0;
//extern __IO uint8_t EthLinkStatus;
__IO uint8_t EthLinkStatus;//用不到的变量
__IO uint32_t LocalTime = 0; /* this variable is used to create a time reference incremented by 10ms */
DRV_NETWORK drv_network,drv_network_server;

extern uint8_t IP_ADDRESS[4];

uint16_t bsp_result=0;

/* 从 Backend_vidoplayer.c 引入 */
extern void TIM3_Config(uint16_t period,uint16_t prescaler);
extern TIM_HandleTypeDef TIM3_Handle;
/* 从 sys_arch.c 引入 */


/* LWIP协议栈初始化,初始化成功即删除任务,否则会阻塞在函数内。 */
static void TCPIP_Init_Task(void *p)
{
	LWIP_Init_End = 1;
	if(network_start_flag == 0)//如果成功过一次,重新打开程序,不会再初始化。
	{
		My_TCPIP_initialization(IP_ADDRESS);
	}
	while(1)
	{
		vTaskDelay(5000);
	}
}

void Network_Dispose_Task(void *p) 
{
	EnableWindow(GetDlgItem(Network_Main_Handle, eID_LINK_STATE), DISABLE);
	
	if(network_start_flag == 0)
	{
	if(xSemaphoreTake( Wait_TCPIP_Init_Sem,5000) != pdTRUE)
		{
      network_start_flag=0;
      bsp_result |=1;
      /* 初始化出错 */
      SetTimer(Network_Main_Handle, 10, 100, TMR_SINGLE|TMR_START, NULL);
      vTaskSuspend(Network_Task_Handle);    // 挂起自己 不在执行 
    }
    else
    {
      network_start_flag=1;
      bsp_result &=~ 1;  
    }
	}
	DestroyWindow(Message_Hwnd);
	EnableWindow(GetDlgItem(Network_Main_Handle, eID_Network_EXIT), ENABLE);
	EnableWindow(GetDlgItem(Network_Main_Handle, eID_LINK_STATE), ENABLE);
	
#if 1
  if((drv_network.net_init==0)&&((bsp_result&1)==0))
  {
    /* Initilaize the LwIP stack */
		/* Config TCP Server IP and Local IP*/
    drv_network.net_local_ip1  = IP_ADDRESS[0];
    drv_network.net_local_ip2  = IP_ADDRESS[1];
    drv_network.net_local_ip3  = IP_ADDRESS[2];
    drv_network.net_local_ip4  = IP_ADDRESS[3];
    drv_network.net_local_port = LOCAL_PORT;
    
	  drv_network_server.net_local_ip1  = IP_ADDRESS[0];
    drv_network_server.net_local_ip2  = IP_ADDRESS[1];
    drv_network_server.net_local_ip3  = IP_ADDRESS[2];
    drv_network_server.net_local_ip4  = IP_ADDRESS[3];
    drv_network_server.net_local_port = LOCAL_PORT;
		drv_network_server.net_type = 1;//TCP_SERVER
		
    drv_network.net_remote_ip1  = DEST_IP_ADDR0;
    drv_network.net_remote_ip2  = DEST_IP_ADDR1;
    drv_network.net_remote_ip3  = DEST_IP_ADDR2;
    drv_network.net_remote_ip4  = DEST_IP_ADDR3;
    drv_network.net_remote_port = DEST_PORT;     
    
    drv_network.net_init=1;
  }
 #endif 
//  PostCloseMessage(GetDlgItem(Network_Main_Handle, ID_Hint_Win));

  InvalidateRect(Network_Main_Handle, NULL, TRUE);
  drv_network.net_connect=0;
  drv_network.net_type=0; 
//  TIM3_Config(999,899);//10ms定时器 
  LocalTime=0;
//  TIM_SetCounter(TIM3,0);
//  HAL_TIM_Base_Start_IT(&TIM3_Handle); 
  EthLinkStatus=0;
		/* 由中断处理接受到的数据 ---> HAL_ETH_RxCpltCallback */
	GUI_Thread_Delete(Network_Task_Handle);    // 删除网络处理任务
	GUI_Thread_Delete(TCPIP_Init_Task_Handle); //删除初始化任务		
	while(1){GUI_Yield();}
}

//退出按钮重绘制
static void Ent_ExitButton_OwnerDraw(DRAWITEM_HDR *ds)
{
  HDC hdc;
  RECT rc;
//  HWND hwnd;

	hdc = ds->hDC;   
	rc = ds->rc; 
//  hwnd = ds->hwnd;

//  GetClientRect(hwnd, &rc_tmp);//得到控件的位置
//  WindowToScreen(hwnd, (POINT *)&rc_tmp, 1);//坐标转换

//  BitBlt(hdc, rc.x, rc.y, rc.w, rc.h, hdc_bk, rc_tmp.x, rc_tmp.y, SRCCOPY);

  if (ds->State & BST_PUSHED)
	{ //按钮是按下状态
		SetPenColor(hdc, MapRGB(hdc, 1, 191, 255));
	}
	else
	{ //按钮是弹起状态

		SetPenColor(hdc, MapRGB(hdc, 250, 250, 250));      //设置画笔色
	}

  SetPenSize(hdc, 2);

  OffsetRect(&rc,15,5);
	
  for(int i=0; i<4; i++)
  {	
    HLine(hdc, rc.x, rc.y ,58);//rc.w
    rc.y += 9;
  }

}

// 重绘普通按钮
static void Ent_Button_OwnerDraw(DRAWITEM_HDR *ds)
{
  HDC hdc;
  RECT rc;
  WCHAR wbuf[128];
  HWND hwnd;

	hdc = ds->hDC;   
	rc = ds->rc; 
  hwnd = ds->hwnd;

  SetBrushColor(hdc, MapRGB(hdc, 0, 202, 252));
  EnableAntiAlias(hdc, ENABLE);
  FillRoundRect(hdc, &rc, 5);
  EnableAntiAlias(hdc, DISABLE);

  if (ds->State & BST_PUSHED)
	{ //按钮是按下状态
    OffsetRect(&rc, 1, 1);
		SetTextColor(hdc, MapRGB(hdc, 200, 200, 200));
	}
	else
	{ //按钮是弹起状态

		SetTextColor(hdc, MapRGB(hdc, 255, 255, 255));      //设置画笔色
	}

  GetWindowText(hwnd, wbuf, 128);    // 得到原文本

  SetFont(hdc, defaultFont);
  DrawText(hdc, wbuf, -1, &rc, DT_VCENTER|DT_CENTER);     // 绘制文字(居中显示)
}

/*
 * @brief  重绘显示亮度的透明文本
 * @param  ds:	自定义绘制结构体
 * @retval NONE
*/
static void Brigh_Textbox_OwnerDraw(DRAWITEM_HDR *ds) //绘制一个按钮外观
{
	HWND hwnd;
	HDC hdc;
	RECT rc;
	WCHAR *wbuf;
  uint32_t WinTexeLen = 0;

	hwnd = ds->hwnd; //button的窗口句柄.
	hdc = ds->hDC;   //button的绘图上下文句柄.
  rc = ds->rc;

  SetTextColor(hdc, MapRGB(hdc, 170, 170, 170));
	SetBrushColor(hdc,MapRGB(hdc,250,250,250));
  if (ds->ID == ID_TEXTBOX_Receive)
  {
    DrawText(hdc, L"接收区", -1, &rc, DT_VCENTER|DT_CENTER);  // 绘制文字
  }
  else
  {
    SetPenColor(hdc, MapRGB(hdc, 121, 121, 121));
		FillRect(hdc,&rc);
    EnableAntiAlias(hdc, ENABLE);
    DrawRoundRect(hdc, &rc, 7);
    EnableAntiAlias(hdc, DISABLE);
    DrawText(hdc, L"发送区", -1, &rc, DT_VCENTER|DT_CENTER);  // 绘制文字
  }
  
  OffsetRect(&rc, 5, 5);
  SetTextColor(hdc, MapRGB(hdc, 0, 0, 0));
  WinTexeLen = GetWindowTextLength(hwnd);               // 获取文本长度

  if (WinTexeLen > 0)
  {
    wbuf = (WCHAR *)GUI_VMEM_Alloc(WinTexeLen*2);         // 申请文本长度 + 新消息长度的内存
    GetWindowText(hwnd, wbuf, WinTexeLen+1);    // 得到原文本

    SetFont(hdc, defaultFont);
    DrawText(hdc, wbuf, -1, &rc, DT_TOP|DT_LEFT|DT_WORDBREAK);     // 绘制文字( 顶部对齐，左对齐，当字符超出矩形边界时,自动换行)
  }
}

extern void TCP_Echo_Init(void);
extern int SelectDialogBox(HWND hwndParent, RECT rc,const WCHAR *pText,const WCHAR *pCaption,const MSGBOX_OPTIONS *ops);
WCHAR I[128];

static LRESULT	win_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch(msg)
  {
    case WM_CREATE:
    {
      RECT rc;
      GetClientRect(hwnd, &rc); 
      HWND Temp_Handle;
			
			Wait_TCPIP_Init_Sem = GUI_SemCreate(0,1);
			BaseType_t xReturn = pdPASS;
		  if(LWIP_Init_End == 0)
			{
				xReturn = xTaskCreate((TaskFunction_t )TCPIP_Init_Task,      /* 任务入口函数 */
															(const char*    )"TCPIP Init Task",    /* 任务名字 */
															(uint16_t       )1024,                  /* 任务栈大小FreeRTOS的任务栈以字为单位 */
															(void*          )NULL,                      /* 任务入口函数参数 */
															(UBaseType_t    )6,                         /* 任务的优先级 */
															(TaskHandle_t*  )&TCPIP_Init_Task_Handle);     /* 任务控制块指针 */
															
			 if(xReturn != pdPASS)  
				{
					GUI_ERROR("Fail to create Network_Dispose_Task!\r\n");
				}			
				
				xReturn = xTaskCreate((TaskFunction_t )Network_Dispose_Task,      /* 任务入口函数 */
															(const char*    )"Network Dispose Task",    /* 任务名字 */
															(uint16_t       )1024,                  /* 任务栈大小FreeRTOS的任务栈以字为单位 */
															(void*          )NULL,                      /* 任务入口函数参数 */
															(UBaseType_t    )6,                         /* 任务的优先级 */
															(TaskHandle_t*  )&Network_Task_Handle);     /* 任务控制块指针 */
				if(xReturn != pdPASS)  
				{
					GUI_ERROR("Fail to create Network_Dispose_Task!\r\n");
				}		
			}
			else
			{
				if(network_start_flag == 0)
				{
					SetTimer(Network_Main_Handle, 10, 50, TMR_SINGLE|TMR_START, NULL);
					LWIP_Init_End = 2;
				}
			}		
														
      CreateWindow(BUTTON, L"O", WS_TRANSPARENT|BS_FLAT | BS_NOTIFY |WS_OWNERDRAW|WS_VISIBLE,
                  720, 5, 80, 80, hwnd, eID_Network_EXIT, NULL, NULL); 

      /* 创建一组单选宽 */
      rc.x = 415;
      rc.y = 86;
      rc.w = 155;
      rc.h = 28;
      CreateWindow(BUTTON,L"TCP Server",BS_RADIOBOX|WS_VISIBLE,
      rc.x,rc.y,rc.w,rc.h,hwnd,ID_RB1,NULL,NULL);
      
      OffsetRect(&rc, rc.w+2, 0);
      CreateWindow(BUTTON,L"TCP Client",BS_RADIOBOX|WS_VISIBLE,
      rc.x,rc.y,rc.w,rc.h,hwnd,ID_RB2,NULL,NULL);
			SendMessage(GetDlgItem(hwnd, ID_RB2&0xFFFF), BM_SETSTATE, BST_CHECKED, 0);    // 默认选中

      OffsetRect(&rc, rc.w, 0);
      rc.w = 80;
      CreateWindow(BUTTON,L"UDP",BS_RADIOBOX|WS_VISIBLE,
      rc.x,rc.y,rc.w,rc.h,hwnd,ID_RB3,NULL,NULL);
      
      CreateWindow(BUTTON, L"未连接", WS_TRANSPARENT | BS_NOTIFY|WS_VISIBLE|WS_OWNERDRAW|WS_DISABLED,
                  702, 218, 95, 30, hwnd, eID_LINK_STATE, NULL, NULL);
      
      /* 数据发送文本窗口 */
      rc.w = 384;
      rc.h = 224;
      rc.x = 412;
      rc.y = 251;
      Send_Handle = CreateWindow(TEXTBOX, L"你好！这里是野火开发板 ^_^",WS_VISIBLE|WS_OWNERDRAW, rc.x, rc.y, rc.w, rc.h, hwnd, ID_TEXTBOX_Send, NULL, NULL);

      /* 创建接收窗口 */
      rc.w = 400;
      rc.h = 380;
      rc.x = 5;
      rc.y = 55;
      Receive_Handle = CreateWindow(TEXTBOX, L"", WS_TRANSPARENT|WS_VISIBLE|WS_OWNERDRAW, rc.x, rc.y, rc.w, rc.h, hwnd, ID_TEXTBOX_Receive, NULL, NULL);
      
      rc.x = 720;
      rc.y = 440;
      rc.w = 71;
      rc.h = 30;
      CreateWindow(BUTTON, L"发送", WS_TRANSPARENT | BS_NOTIFY|WS_VISIBLE|WS_OWNERDRAW,
                         rc.x,rc.y,rc.w,rc.h, hwnd, eID_Network_Send, NULL, NULL); 
                         
      rc.x = 289;
      rc.h = 30;
      rc.w = 111;
      rc.y = 440;
      CreateWindow(BUTTON, L"清空接收", WS_TRANSPARENT | BS_NOTIFY|WS_VISIBLE|WS_OWNERDRAW,
                         rc.x,rc.y,rc.w,rc.h, hwnd, eID_Receive_Clear,       NULL, NULL); 

      /* 数据发送文本窗口 */
      rc.w = 48;
      rc.h = 28;
      rc.x = 414;
      rc.y = 219;
      Temp_Handle = CreateWindow(TEXTBOX, L"192", WS_VISIBLE|WS_BORDER, rc.x, rc.y, rc.w, rc.h, hwnd, ID_TEXTBOX_RemoteIP1, NULL, NULL);//
      SendMessage(Temp_Handle, TBM_SET_TEXTFLAG, 0, DT_VCENTER | DT_CENTER | DT_BKGND);

      OffsetRect(&rc, rc.w+9, 0);
      Temp_Handle = CreateWindow(TEXTBOX, L"168", WS_VISIBLE|WS_BORDER, rc.x, rc.y, rc.w, rc.h, hwnd, ID_TEXTBOX_RemoteIP2, NULL, NULL);//
      SendMessage(Temp_Handle, TBM_SET_TEXTFLAG, 0, DT_VCENTER | DT_CENTER | DT_BKGND);

      OffsetRect(&rc, rc.w+9, 0);
      Temp_Handle = CreateWindow(TEXTBOX, L"000", WS_VISIBLE|WS_BORDER, rc.x, rc.y, rc.w, rc.h, hwnd, ID_TEXTBOX_RemoteIP3, NULL, NULL);//
      SendMessage(Temp_Handle, TBM_SET_TEXTFLAG, 0, DT_VCENTER | DT_CENTER | DT_BKGND);

      OffsetRect(&rc, rc.w+9, 0);
      Temp_Handle = CreateWindow(TEXTBOX, L"138", WS_VISIBLE|WS_BORDER, rc.x, rc.y, rc.w, rc.h, hwnd, ID_TEXTBOX_RemoteIP4, NULL, NULL);//
      SendMessage(Temp_Handle, TBM_SET_TEXTFLAG, 0, DT_VCENTER | DT_CENTER | DT_BKGND);

      OffsetRect(&rc, rc.w+9 , 0);
      rc.w = 69;
      Temp_Handle = CreateWindow(TEXTBOX, L"8080", WS_VISIBLE|WS_BORDER, rc.x, rc.y, rc.w-12, rc.h, hwnd, ID_TEXTBOX_RemotePort, NULL, NULL);//
      SendMessage(Temp_Handle, TBM_SET_TEXTFLAG, 0, DT_VCENTER | DT_CENTER | DT_BKGND);

			if(LWIP_Init_Start == 0)
			{
				LWIP_Init_Start = 1;
				RECT RC;
				RC.w = 280;
				RC.h = 170;
				RC.x = (GUI_XSIZE - RC.w) >> 1;
				RC.y = (GUI_YSIZE - RC.h) >> 1;
				Message_Hwnd = CreateWindow(TEXTBOX, L"正在初始化\r\n\n请等待...", WS_VISIBLE|WS_BORDER|WS_OVERLAPPED, RC.x, RC.y-20, RC.w, RC.h, hwnd, ID_TEXTBOX_Wait, NULL, NULL);//
				SendMessage(Message_Hwnd, TBM_SET_TEXTFLAG, 0, DT_VCENTER | DT_CENTER | DT_BKGND);
				EnableWindow(GetDlgItem(hwnd, eID_Network_EXIT), DISABLE);
			}
      break;
    } 
    case WM_TIMER:
    {
      int tmr_id;

      tmr_id = wParam;    // 定时器 ID

      if (tmr_id == 10)    // 以太网初始化错误
      {
        RECT RC;
        MSGBOX_OPTIONS ops;
        const WCHAR *btn[] ={L"确认",L"取消"};      //对话框内按钮的文字

        ops.Flag =MB_ICONERROR;
        ops.pButtonText =btn;
        ops.ButtonCount =2;
        RC.w = 300;
        RC.h = 200;
        RC.x = (GUI_XSIZE - RC.w) >> 1;
        RC.y = (GUI_YSIZE - RC.h - 36) >> 1;
        SelectDialogBox(hwnd, RC, L"以太网初始化失败\n请重新检查连接\r\n并复位开发板。", L"错误", &ops);    // 显示错误提示框
        PostCloseMessage(hwnd);                                                          // 发送关闭窗口的消息
      }
      
      break;
    }

    case WM_PAINT:
    {
      HDC hdc;
      PAINTSTRUCT ps;
      WCHAR tempstr[30];
      
      RECT rc =  {0, 0, GUI_XSIZE, GUI_YSIZE};
      // hdc_pointer = CreateMemoryDC(SURF_SCREEN, PANEL_W, PANEL_H);
      hdc = BeginPaint(hwnd, &ps);
      
      SetBrushColor(hdc, MapRGB(hdc, 255, 255, 255));
      FillRect(hdc, &rc);

      rc.h = 50;
      GradientFillRect(hdc, &rc, MapRGB(hdc, 1, 218, 254), MapRGB(hdc, 1, 168, 255), FALSE);
      SetTextColor(hdc, MapRGB(hdc, 255, 255, 255));
      DrawText(hdc, L"以太网", -1, &rc, DT_VCENTER|DT_CENTER);

      SetPenColor(hdc, MapRGB(hdc, 121, 121, 121));

      rc.x = 5;
      rc.y = 55;
      rc.w = 400;
      rc.h = 420;
      EnableAntiAlias(hdc, ENABLE);
      DrawRoundRect(hdc, &rc, 7);     // 绘制接收区的外框
      EnableAntiAlias(hdc, DISABLE);
      
      SetFont(hdc, defaultFont);
      SetTextColor(hdc, MapRGB(hdc, 0x16, 0x9B, 0xD5));

      rc.x = 412;
      rc.y = 57;
      rc.w = 120;
      rc.h = 28;
      DrawText(hdc, L"通讯协议：", -1, &rc, DT_LEFT|DT_TOP);

      // rc.x = 550;
      // rc.y = 36;
      // rc.w = 60;
      // rc.h = 30;
      // DrawText(hdc, L"数据发送：", -1, &rc, DT_LEFT|DT_TOP);
      
      rc.w = 207;
      rc.h = 28;
      rc.x = 412;
      rc.y = 120;
      DrawText(hdc, L"本地IP地址&端口：", -1, &rc, DT_LEFT|DT_TOP);
      
      rc.y = 188;
      DrawText(hdc, L"远端IP地址&端口：", -1, &rc, DT_LEFT|DT_TOP);
      
      SetTextColor(hdc, MapRGB(hdc, 10, 10, 10));
//      x_wsprintf(tempstr, L"[%d.%d.%d.%d:%d]",drv_network.net_local_ip1,drv_network.net_local_ip2,\
//                                       drv_network.net_local_ip3,drv_network.net_local_ip4,\
//                                       drv_network.net_local_port);
      x_wsprintf(tempstr, L"[%d.%d.%d.%d:%d]",IP_ADDRESS[0],IP_ADDRESS[1],IP_ADDRESS[2],IP_ADDRESS[3],LOCAL_PORT);
      rc.w = 300;
      rc.h = 30;
      rc.x = 412;
      rc.y = 152;
      DrawText(hdc, tempstr, -1, &rc, DT_LEFT|DT_TOP);
      
      SetTextColor(hdc, MapRGB(hdc, 0x16, 0x9B, 0xD5));

      // rc.w = 120;
      // rc.h = 30;
      // rc.x = 10;
      // rc.y = 210;
      // DrawText(hdc, L"数据接收：", -1, &rc, DT_LEFT|DT_TOP);

      rc.w = 7;
      rc.h = 28;
      rc.x = 463-4;
      rc.y = 223;
      DrawText(hdc, L".", -1, &rc, DT_LEFT|DT_BOTTOM);
      
      rc.x = 520-4;
      DrawText(hdc, L".", -1, &rc, DT_LEFT|DT_BOTTOM);
      
      rc.x = 577-4;
      DrawText(hdc, L".", -1, &rc, DT_LEFT|DT_BOTTOM);
      
      rc.x = 634-4;
      DrawText(hdc, L":", -1, &rc, DT_LEFT|DT_BOTTOM);
      
      EndPaint(hwnd, &ps);
      break;
    }

    case WM_DRAWITEM:
    {
       DRAWITEM_HDR *ds;
       ds = (DRAWITEM_HDR*)lParam;
       switch(ds->ID)
       {
          case eID_Network_EXIT:
          {
            Ent_ExitButton_OwnerDraw(ds);
            return TRUE;             
          }  

          case ID_TEXTBOX_Send:
          case ID_TEXTBOX_Receive:
          {
            Brigh_Textbox_OwnerDraw(ds);
            return TRUE;   
          }

          case eID_LINK_STATE:
          case eID_Network_Send:
          case eID_Receive_Clear:
          {
            Ent_Button_OwnerDraw(ds);
            return TRUE;   
          }
       }

       break;
    }

    //设置TEXTBOX的背景颜色以及文字颜色
		case	WM_CTLCOLOR:
		{
			/* 控件在绘制前，会发送 WM_CTLCOLOR到父窗口.
			 * wParam参数指明了发送该消息的控件ID;lParam参数指向一个CTLCOLOR的结构体指针.
			 * 用户可以通过这个结构体改变控件的颜色值.用户修改颜色参数后，需返回TRUE，否则，系统
			 * 将忽略本次操作，继续使用默认的颜色进行绘制.
			 *
			 */
			u16 id;
			id =LOWORD(wParam);
      
			if(id >= ID_TEXTBOX_RemoteIP1 && id <= ID_TEXTBOX_RemotePort)
			{
				CTLCOLOR *cr;
				cr =(CTLCOLOR*)lParam;
				cr->TextColor =RGB888(0, 0, 0);//文字颜色（RGB888颜色格式)
				cr->BackColor =RGB888(255, 255, 255);//背景颜色（RGB888颜色格式)
				cr->BorderColor =RGB888(10,10,10);//边框颜色（RGB888颜色格式)
				return TRUE;				
			}

      return FALSE;
		}

    case WM_NOTIFY:
    {
      u16 code, id;
      id  =LOWORD(wParam);//获取消息的ID码
      code=HIWORD(wParam);//获取消息的类型    
      if(code == BN_CLICKED && id == eID_Network_EXIT)    // 退出按钮按下
      {
				switch(NetworkTypeSelection)
				{
					case 0:
						tcp_echoserver_close();
						break;
					case 1:
						tcp_echoclient_disconnect();
						break;
					case 2:
						udp_echoclient_disconnect();	
						break;   
				}
        PostCloseMessage(hwnd);    // 发送关闭窗口的消息
        break;
      }
      if(code == BN_CLICKED && id == eID_Receive_Clear)    // 清空接收的窗口的按钮被按下
      {
        SetWindowText(Receive_Handle, L"");
        break;
      }
      
      if(code == TBN_CLICKED && id == ID_TEXTBOX_RemoteIP1)    // IP1 编辑框被按下
      {
        number_input_box(0, 0, GUI_XSIZE, GUI_YSIZE, L"IP1", I, 3, hwnd);
        SetWindowText(GetDlgItem(hwnd, ID_TEXTBOX_RemoteIP1), I);
        break;
      }
      
      if(code == TBN_CLICKED && id == ID_TEXTBOX_RemoteIP2){    // IP2 编辑框被按下
        number_input_box(0, 0, GUI_XSIZE, GUI_YSIZE, L"IP2", I, 3, hwnd);
        SetWindowText(GetDlgItem(hwnd, ID_TEXTBOX_RemoteIP2), I);
        break;
      }
      
      if(code == TBN_CLICKED && id == ID_TEXTBOX_RemoteIP3){    // IP3 编辑框被按下
        number_input_box(0, 0, GUI_XSIZE, GUI_YSIZE, L"IP3", I, 3, hwnd);
        SetWindowText(GetDlgItem(hwnd, ID_TEXTBOX_RemoteIP3), I);
        break;
      }
      
      if(code == TBN_CLICKED && id == ID_TEXTBOX_RemoteIP4){    // IP4 编辑框被按下
        number_input_box(0, 0, GUI_XSIZE, GUI_YSIZE, L"IP4", I, 3, hwnd);
        SetWindowText(GetDlgItem(hwnd, ID_TEXTBOX_RemoteIP4), I);
        break;
      }
      
      if(code == TBN_CLICKED && id == ID_TEXTBOX_RemotePort)    // 端口 编辑框被按下
      {
        number_input_box(0, 0, GUI_XSIZE, GUI_YSIZE, L"PORT", I, 5, hwnd);
        SetWindowText(GetDlgItem(hwnd, ID_TEXTBOX_RemotePort), I);
        break;
      }
      
      if(code == BN_CLICKED && id == eID_LINK_STATE)
      {
        if((bsp_result&1)||EthLinkStatus)
          {
            break;
          }
          if(drv_network.net_connect==0)
          {
            uint8_t connectflag;
            WCHAR wbuf[128];
            char buf[128];

            GetWindowText(GetDlgItem(hwnd, ID_TEXTBOX_RemoteIP1), wbuf, 128);    // 获取文本框的文本
            x_wcstombs_cp936(buf, wbuf, 128);                                    // 将宽字符串转为单字符串
            drv_network.net_remote_ip1 = x_atoi(buf);                            // 字符串转整型
//						drv_network.net_local_ip1 = x_atoi(buf);
            GetWindowText(GetDlgItem(hwnd, ID_TEXTBOX_RemoteIP2), wbuf, 128);
            x_wcstombs_cp936(buf, wbuf, 128);
            drv_network.net_remote_ip2 = x_atoi(buf);
//						drv_network.net_local_ip2 = x_atoi(buf);
            GetWindowText(GetDlgItem(hwnd, ID_TEXTBOX_RemoteIP3), wbuf, 128);
            x_wcstombs_cp936(buf, wbuf, 128);
            drv_network.net_remote_ip3 = x_atoi(buf);
//						drv_network.net_local_ip3 = x_atoi(buf);
            GetWindowText(GetDlgItem(hwnd, ID_TEXTBOX_RemoteIP4), wbuf, 128);
            x_wcstombs_cp936(buf, wbuf, 128);
            drv_network.net_remote_ip4 = x_atoi(buf);
//						drv_network.net_local_ip4 = x_atoi(buf);
            GetWindowText(GetDlgItem(hwnd, ID_TEXTBOX_RemotePort), wbuf, 128);
            x_wcstombs_cp936(buf, wbuf, 128);
            drv_network.net_remote_port = x_atoi(buf);
//						drv_network.net_local_port = x_atoi(buf);
            drv_network.net_type=NetworkTypeSelection;
            switch(drv_network.net_type)
            {
              case 0:
                /*create tcp server */ 
                connectflag=tcp_echoserver_init(drv_network_server);
                break;
              case 1:
                /*connect to tcp server */
                connectflag=tcp_echoclient_connect(drv_network);
                break;
              case 2:
                /* Connect to tcp server */ 
                connectflag=udp_echoclient_connect(drv_network);		
                break;            
            }
            if(connectflag==0)    // 连接成功
            {
              drv_network.net_connect=1; 
              SetWindowText(GetDlgItem(hwnd, eID_LINK_STATE), L"点击断开");
            }else
						{
						
						}
          }
          else
          {
            /* 断开连接 */
            SetWindowText(GetDlgItem(hwnd, eID_LINK_STATE), L"未连接");
            switch(drv_network.net_type)
            {
              case 0:
                tcp_echoserver_close();
                break;
              case 1:
                tcp_echoclient_disconnect();
                break;
              case 2:
                udp_echoclient_disconnect();	
                break;            
            }
            drv_network.net_connect=0;
          }
        }
        if(code == BN_CLICKED && id == eID_Network_Send)
        {
          if(drv_network.net_connect==1)          
          {
            WCHAR wbuf[128];
            char comdata[128];

            /* 获取发送窗口的字符串 */
            GetWindowText(GetDlgItem(hwnd, ID_TEXTBOX_Send), wbuf, 128);
            x_wcstombs_cp936(comdata, wbuf, 128);

            /* 发送消息 */
            switch(drv_network.net_type)
            {
              case 0:
                network_tcpserver_send((char *)comdata);
                break;
              case 1:
                network_tcpclient_send((char *)comdata);
                break;
              case 2:
                udp_echoclient_send((char *)comdata);
                break;            
            }
          }
        }
      
        /* 单选按钮被按下 */
        if( (id >= (ID_RB1 & ~(1<<16))) && (id <= (ID_RB3 & ~(1<<16))))
        {
          if (code == BN_CLICKED)
          {
            NetworkTypeSelection = id & 3;
          }
        }

      break;
    } 

    case WM_DESTROY:
    { 
      NetworkTypeSelection = 0;                  // 复位默认的选项
      udp_echoclient_disconnect();
			tcp_echoclient_disconnect();
			tcp_echoserver_close();
			GUI_SemDelete(Wait_TCPIP_Init_Sem);

      return PostQuitMessage(hwnd);	
    } 

    default:
      return	DefWindowProc(hwnd, msg, wParam, lParam);   
  }
  
  return WM_NULL;
  
}

void GUI_NetworkDLG_Dialog(void)
{
	
	WNDCLASS	wcex;
	MSG msg;
	wcex.Tag = WNDCLASS_TAG;

	wcex.Style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = win_proc; //设置主窗口消息处理的回调函数.
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = NULL;//hInst;
	wcex.hIcon = NULL;//LoadIcon(hInstance, (LPCTSTR)IDI_WIN32_APP_TEST);
	wcex.hCursor = NULL;//LoadCursor(NULL, IDC_ARROW);
   
	//创建主窗口
	Network_Main_Handle = CreateWindowEx(WS_EX_NOFOCUS|WS_EX_FRAMEBUFFER,
                              &wcex,
                              L"GUI_Network_Dialog",
                              WS_VISIBLE|WS_CLIPCHILDREN,
                              0, 0, GUI_XSIZE, GUI_YSIZE,
                              NULL, NULL, NULL, NULL);

   //显示主窗口
	ShowWindow(Network_Main_Handle, SW_SHOW);

	//开始窗口消息循环(窗口关闭并销毁时,GetMessage将返回FALSE,退出本消息循环)。
	while (GetMessage(&msg, Network_Main_Handle))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}


