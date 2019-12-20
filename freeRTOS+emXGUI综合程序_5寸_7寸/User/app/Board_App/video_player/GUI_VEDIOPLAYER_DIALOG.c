#include "emXGUI.h"
#include "GUI_VEDIOPLAYER_DIALOG.h"
#include "Widget.h"
#include "emXGUI_JPEG.h"
#include "GUI_AppDef.h"
#include	"CListMenu.h"
#include "x_libc.h"
#include <string.h>
#include "Backend_vidoplayer.h"
#include "Backend_avifile.h"
#include "./sai/bsp_sai.h" 

GUI_SEM *Delete_VideoTask_Sem;//做任务同步,结束播放器前先关闭播放任务
TaskHandle_t VideoTask_Handle;
extern volatile uint8_t video_timeout;//视频播放引入

VIDEO_DIALOG_Typedef VideoDialog;
static SCROLLINFO video_sif_time;/*设置进度条的参数*/
char avi_playlist[20][100];//播放List
char lcdlist[20][100];//显示list
//static char lcdlist[20][100];//显示list
static SCROLLINFO video_sif_power;/*设置进度条的参数*/
static char path[100]="0:";//文件根目??
//图标管理数组
ICON_Typedef avi_icon[13] = {
   {"yinliang",         {20, 400,48,48},      FALSE},
   {"waifanglaba",      {732,400,48,48},      FALSE},     //外放喇叭
   {"back",             {294, 400, 64, 64},      FALSE},
   {"bofang",           {364, 400, 72, 72},      FALSE},
   {"next",             {442, 400, 64, 64},      FALSE},
   {"fenbianlv",        {0,40,380,40},   FALSE},
   {"zanting/bofang",   {300, 140, 200, 200}, FALSE},
   {"xiayishou",        {600, 200, 80, 80},   FALSE}, 
   {"bofangliebiao",    {675,400,48,48},      FALSE},   
//   {"mini_next",        {580, 4, 80, 80},     FALSE},
//   {"mini_Stop",        {652, 4, 80, 80},     FALSE},
//   {"mini_back",        {724, 3, 80, 80},     FALSE},  
//   {"上边栏",           {0 ,0, 800, 80},     FALSE},
//   {"下边栏",           {0 ,400, 800, 80},     FALSE}, 
   
};
VIDEO_DIALOG_Typedef VideoDialog ={
  .power = 20,
  
};
/*
 * @brief  绘制滚动条
 * @param  hwnd:   滚动条的句柄值
 * @param  hdc:    绘图上下文
 * @param  back_c：背景颜色
 * @param  Page_c: 滚动条Page处的颜色
 * @param  fore_c：滚动条滑块的颜色
 * @retval NONE
*/
static void draw_scrollbar(HWND hwnd, HDC hdc, COLOR_RGB32 Page_c, COLOR_RGB32 fore_c)
{
	RECT rc,rc_tmp;
   RECT rc_scrollbar;
	GetClientRect(hwnd, &rc);
	/* 背景 */
//	SetBrushColor(hdc, color_bg);
//	FillRect(hdc, &rc);
   GetClientRect(hwnd, &rc_tmp);//得到控件的位置
   GetClientRect(hwnd, &rc);//得到控件的位置
   WindowToScreen(hwnd, (POINT *)&rc_tmp, 1);//坐标转换
   
   BitBlt(hdc, rc.x, rc.y, rc.w, rc.h, VideoDialog.hdc_bk, rc_tmp.x, rc_tmp.y, SRCCOPY);
  
  
   rc_scrollbar.x = rc.x;
   rc_scrollbar.y = rc.h/2;
   rc_scrollbar.w = rc.w;
   rc_scrollbar.h = 2;
   
	SetBrushColor(hdc, MapRGB888(hdc, Page_c));
	FillRect(hdc, &rc_scrollbar);

	/* 滑块 */
	SendMessage(hwnd, SBM_GETTRACKRECT, 0, (LPARAM)&rc);

	SetBrushColor(hdc, MapRGB(hdc, 169, 169, 169));
	//rc.y += (rc.h >> 2) >> 1;
	//rc.h -= (rc.h >> 2);
	/* 边框 */
	//FillRoundRect(hdc, &rc, MIN(rc.w, rc.h) >> 2);
	FillCircle(hdc, rc.x + rc.h / 2, rc.y + rc.h / 2, rc.h / 2);
   InflateRect(&rc, -2, -2);

	SetBrushColor(hdc, MapRGB888(hdc, fore_c));
	FillCircle(hdc, rc.x + rc.h / 2, rc.y + rc.h / 2, rc.h / 2);
   //FillRoundRect(hdc, &rc, MIN(rc.w, rc.h) >> 2);
}
/*
 * @brief  自定义滑动条绘制函数
 * @param  ds:	自定义绘制结构体
 * @retval NONE
*/
static void vedio_scrollbar_ownerdraw(DRAWITEM_HDR *ds)
{
	HWND hwnd;
	HDC hdc;
	HDC hdc_mem;
	HDC hdc_mem1;
	RECT rc;
	RECT rc_cli;
	//	int i;

	hwnd = ds->hwnd;
	hdc = ds->hDC;
	GetClientRect(hwnd, &rc_cli);

	hdc_mem = CreateMemoryDC(SURF_SCREEN, rc_cli.w, rc_cli.h);
	hdc_mem1 = CreateMemoryDC(SURF_SCREEN, rc_cli.w, rc_cli.h);   
         
   	
	//绘制白色类型的滚动条
	draw_scrollbar(hwnd, hdc_mem1,  RGB888( 250, 250, 250), RGB888( 255, 255, 255));
	//绘制绿色类型的滚动条
	draw_scrollbar(hwnd, hdc_mem, RGB888( 250, 0, 0), RGB888( 250, 0, 0));
   SendMessage(hwnd, SBM_GETTRACKRECT, 0, (LPARAM)&rc);   

	//左
	BitBlt(hdc, rc_cli.x, rc_cli.y, rc.x, rc_cli.h, hdc_mem, 0, 0, SRCCOPY);
	//右
	BitBlt(hdc, rc.x + rc.w, 0, rc_cli.w - (rc.x + rc.w) , rc_cli.h, hdc_mem1, rc.x + rc.w, 0, SRCCOPY);

	//绘制滑块
	if (ds->State & SST_THUMBTRACK)//按下
	{
      BitBlt(hdc, rc.x, 0, rc.w, rc_cli.h, hdc_mem1, rc.x, 0, SRCCOPY);
		
	}
	else//未选中
	{
		BitBlt(hdc, rc.x, 0, rc.w, rc_cli.h, hdc_mem, rc.x, 0, SRCCOPY);
	}
	//释放内存MemoryDC
	DeleteDC(hdc_mem1);
	DeleteDC(hdc_mem);
}
static void vedio_text_ownerdraw(DRAWITEM_HDR *ds)
{
  HDC hdc; //控件窗口HDC
  HWND hwnd; //控件句柄 
  RECT rc_cli,rc_tmp;//控件的位置大小矩形
  HDC hdc_mem;
  WCHAR wbuf[128];
  hwnd = ds->hwnd;
  hdc = ds->hDC; 
  GetClientRect(hwnd, &rc_cli);

   //获取控件的位置大小信息
  hdc_mem = CreateMemoryDC(SURF_SCREEN, rc_cli.w,rc_cli.h); 
   
  GetWindowText(hwnd,wbuf,128); //获得按钮控件的文字  
 
  GetClientRect(hwnd, &rc_tmp);//得到控件的位置
  GetClientRect(hwnd, &rc_cli);//得到控件的位置
  WindowToScreen(hwnd, (POINT *)&rc_tmp, 1);//坐标转换
   
  BitBlt(hdc_mem, rc_cli.x, rc_cli.y, rc_cli.w, rc_cli.h, VideoDialog.hdc_bk, rc_tmp.x, rc_tmp.y, SRCCOPY);

  //设置按键的颜色
  SetTextColor(hdc_mem, MapRGB(hdc, 250,250,250));
  if(ds->ID == eID_TEXTBOX_RES)
   DrawText(hdc_mem, wbuf,-1,&rc_cli,DT_VCENTER|DT_RIGHT);
  else if(ds->ID == eID_TEXTBOX_FPS)
   DrawText(hdc_mem, wbuf,-1,&rc_cli,DT_VCENTER|DT_LEFT);
  else
   DrawText(hdc_mem, wbuf,-1,&rc_cli,DT_VCENTER|DT_CENTER);//绘制文字(居中对齐方式)
   
  BitBlt(hdc, rc_cli.x, rc_cli.y, rc_cli.w, rc_cli.h, hdc_mem, rc_cli.x, rc_cli.y, SRCCOPY);
  DeleteDC(hdc_mem);
}
static void vedio_exit_ownerdraw(DRAWITEM_HDR *ds) //绘制一个按钮外观
{
  HDC hdc;
  RECT rc;

	hdc = ds->hDC;   
	rc = ds->rc; 

  if (ds->State & BST_PUSHED)
	{ //按钮是按下状态
		SetPenColor(hdc, MapRGB(hdc, 120, 120, 120));      //设置文字色
	}
	else
	{ //按钮是弹起状态

		SetPenColor(hdc, MapRGB(hdc, 250, 250, 250));
	}

  SetPenSize(hdc, 2);

  OffsetRect(&rc,15,20);
	
  for(int i=0; i<4; i++)
  {	
    HLine(hdc, rc.x, rc.y ,58);//rc.w
    rc.y += 9;
  }

}
/**
  * @brief  vedio_button_ownerdraw 按钮控件的重绘制
  * @param  ds:DRAWITEM_HDR结构体
  * @retval NULL
  */
static void vedio_button_ownerdraw(DRAWITEM_HDR *ds)
{
  HDC hdc; //控件窗口HDC
  HDC hdc_mem;//内存HDC，作为缓冲区
  HWND hwnd; //控件句柄 
  RECT rc_cli,rc_tmp;//控件的位置大小矩形
  WCHAR wbuf[128];
  hwnd = ds->hwnd;
	hdc = ds->hDC; 
  //获取控件的位置大小信息
  GetClientRect(hwnd, &rc_cli);
  //创建缓冲层，格式为SURF_ARGB4444
  hdc_mem = CreateMemoryDC(SURF_SCREEN, rc_cli.w, rc_cli.h);
   
	
   GetWindowText(ds->hwnd,wbuf,128); //获得按钮控件的文字  
   GetClientRect(hwnd, &rc_tmp);//得到控件的位置
   GetClientRect(hwnd, &rc_cli);//得到控件的位置
   WindowToScreen(hwnd, (POINT *)&rc_tmp, 1);//坐标转换   
   BitBlt(hdc_mem, rc_cli.x, rc_cli.y, rc_cli.w, rc_cli.h, VideoDialog.hdc_bk, rc_tmp.x, rc_tmp.y, SRCCOPY);
   //设置按键的颜色
   SetTextColor(hdc_mem, MapARGB(hdc_mem, 250,250,250,250));
   if((ds->ID == eID_Vedio_BACK || ds->ID == eID_Vedio_NEXT)&& ds->State & BST_PUSHED)
      SetTextColor(hdc_mem, MapARGB(hdc_mem, 250,105,105,105));
   if(ds->ID == eID_Vedio_BACK || ds->ID == eID_Vedio_NEXT)
   {
      SetFont(hdc_mem, ctrlFont64);

   }
   else if(ds->ID == eID_Vedio_START)
   {
      SetFont(hdc_mem, ctrlFont72);
   }
   else
   {
      //设置按钮字体
      SetFont(hdc_mem, ctrlFont48);
   }

   if((ds->State & BST_CHECKED) )
   { //按钮是按下状态
     SetTextColor(hdc_mem, MapRGB(hdc_mem, 105,105,105));      //设置文字色     
   }  
   DrawText(hdc_mem, wbuf,-1,&rc_cli,DT_VCENTER|DT_CENTER);//绘制文字(居中对齐方式)
   
   BitBlt(hdc, rc_cli.x, rc_cli.y, rc_cli.w, rc_cli.h, hdc_mem, 0, 0, SRCCOPY);
      
   DeleteDC(hdc_mem);  
}
/**
  * @brief  vedio_button_ownerdraw 按钮控件的重绘制
  * @param  ds:DRAWITEM_HDR结构体
  * @retval NULL
  */
static void List_button_ownerdraw(DRAWITEM_HDR *ds)
{
  HDC hdc; //控件窗口HDC
  HDC hdc_mem;//内存HDC，作为缓冲区
  HWND hwnd; //控件句柄 
  RECT rc_cli,rc_tmp;//控件的位置大小矩形
  WCHAR wbuf[128];
  hwnd = ds->hwnd;
	hdc = ds->hDC; 
  //获取控件的位置大小信息
  GetClientRect(hwnd, &rc_cli);
  //创建缓冲层，格式为SURF_ARGB4444
  hdc_mem = CreateMemoryDC(SURF_SCREEN, rc_cli.w, rc_cli.h);
   
	
   GetWindowText(ds->hwnd,wbuf,128); //获得按钮控件的文字  
   GetClientRect(hwnd, &rc_tmp);//得到控件的位置
   GetClientRect(hwnd, &rc_cli);//得到控件的位置
   WindowToScreen(hwnd, (POINT *)&rc_tmp, 1);//坐标转换   
//   BitBlt(hdc_mem, rc_cli.x, rc_cli.y, rc_cli.w, rc_cli.h, VideoDialog.hdc_bk, rc_tmp.x, rc_tmp.y, SRCCOPY);
   SetBrushColor(hdc_mem, MapRGB(hdc_mem, 54, 54, 54));
   FillRect(hdc_mem, &rc_cli);   
   //设置按键的颜色
   SetTextColor(hdc_mem, MapARGB(hdc_mem, 250,250,250,250));
   if((ds->ID == eID_Vedio_BACK || ds->ID == eID_Vedio_NEXT)&& ds->State & BST_PUSHED)
      SetTextColor(hdc_mem, MapARGB(hdc_mem, 250,105,105,105));
   if(ds->ID == eID_Vedio_BACK || ds->ID == eID_Vedio_NEXT)
   {
      SetFont(hdc_mem, ctrlFont64);

   }
   else if(ds->ID == eID_Vedio_START)
   {
      SetFont(hdc_mem, ctrlFont72);
   }
   else
   {
      //设置按钮字体
      SetFont(hdc_mem, ctrlFont48);
   }

   if((ds->State & BST_CHECKED) )
   { //按钮是按下状态
     SetTextColor(hdc_mem, MapRGB(hdc_mem, 105,105,105));      //设置文字色     
   }  
   DrawText(hdc_mem, wbuf,-1,&rc_cli,DT_VCENTER|DT_CENTER);//绘制文字(居中对齐方式)
   
   BitBlt(hdc, rc_cli.x, rc_cli.y, rc_cli.w, rc_cli.h, hdc_mem, 0, 0, SRCCOPY);
      
   DeleteDC(hdc_mem);  
}
static int Set_Widget_VCENTER(int y0, int h)
{
  return y0-h/2;
}
static void video_return_ownerdraw(DRAWITEM_HDR *ds) //绘制一个按钮外观
{
	HWND hwnd;
	HDC hdc;
	RECT rc;
	WCHAR wbuf[128];

	hwnd = ds->hwnd; //button的窗口句柄.
	hdc = ds->hDC;   //button的绘图上下文句柄.
	rc = ds->rc;     //button的绘制矩形区.

  SetBrushColor(hdc, MapRGB(hdc, 0,0,0));
	FillRect(hdc, &rc); //用矩形填充背景

	if (IsWindowEnabled(hwnd) == FALSE)
	{
		SetTextColor(hdc, MapRGB(hdc, COLOR_INVALID));
	}
	else if (ds->State & BST_PUSHED)
	{ //按钮是按下状态
		SetTextColor(hdc, MapRGB(hdc, 105, 105, 105));      //设置文字色
	}
	else
	{ //按钮是弹起状态
		SetTextColor(hdc, MapRGB(hdc, 255, 255, 255));
	}

	  /* 使用控制图标字体 */
	SetFont(hdc, ctrlFont48);

	GetWindowText(ds->hwnd, wbuf, 128); //获得按钮控件的文字

	DrawText(hdc, wbuf, -1, &rc, DT_VCENTER);//绘制文字(居中对齐方式)
   rc.x = 35; 
//   rc.y = 20;
  /* 恢复默认字体 */
	SetFont(hdc, defaultFont);
  DrawText(hdc, L"返回", -1, &rc, DT_VCENTER);
}
/**
  * @brief  scan_files 递归扫描sd卡内的视频文??
  * @param  path:初始扫描路径
  * @retval result:文件系统的返回??
  */
static FRESULT scan_files (char* path) 
{ 
  FRESULT res; 		//部分在递归过程被修改的变量，不用全局变量	
  FILINFO fno; 
  DIR dir; 
  int i; 
  char *fn; 
  char file_name[100];	
	
#if _USE_LFN 
  static char lfn[_MAX_LFN * (0x81 ? 2 : 1) + 1]; 	//长文件名支持
  fno.lfname = lfn; 
  fno.lfsize = sizeof(lfn); 
#endif  
  res = f_opendir(&dir, path); //打开目录
  if (res == FR_OK) 
  { 
    i = strlen(path); 
    for (;;) 
    { 
      res = f_readdir(&dir, &fno); 										//读取目录下的内容
     if (res != FR_OK || fno.fname[0] == 0)
		 {
			 f_closedir(&dir);
			 break; 	//为空时表示所有项目读取完毕，跳出
		 }
#if _USE_LFN 
      fn = *fno.lfname ? fno.lfname : fno.fname; 
#else 
      fn = fno.fname; 
#endif 
      if(strstr(path,"recorder")!=NULL)continue;       //逃过录音文件
      if (*fn == '.') continue; 											//点表示当前目录，跳过			
      if (fno.fattrib & AM_DIR) 
			{ 																							//目录，递归读取
        sprintf(&path[i], "/%s", fn); 							//合成完整目录??
        res = scan_files(path);											//递归遍历 
        if (res != FR_OK) 
			{
			    f_closedir(&dir);
					break; 																		//打开失败，跳出循??
			}
        path[i] = 0; 
      } 
      else 
		{ 
											//输出文件??
				if(strstr(fn,".avi")||strstr(fn,".AVI"))//判断是否AVI文件
				{
					if ((strlen(path)+strlen(fn)<100)&&(VideoDialog.avi_file_num<20))
					{
						sprintf(file_name, "%s/%s", path, fn);
            printf("%s%s\r\n", path, fn);	
						memcpy(avi_playlist[VideoDialog.avi_file_num],file_name,strlen(file_name));
            memcpy(lcdlist[VideoDialog.avi_file_num],fn,strlen(fn));
//						memcpy(lcdlist[VideoDialog.avi_file_num],fn,strlen(fn));						
						//memcpy(lcdlist1[avi_file_num],fn,strlen(fn));lcdlist_wnd
					}
               VideoDialog.avi_file_num++;//记录文件个数
				}//if 
      }//else
     } //for
  } 
  return res; 
}
static int thread_PlayVideo = 0;

static void App_PlayVideo(void *param)
{
	int app=0;

	while(thread_PlayVideo) //线程已创建了
	{
		if(app==0)
		{
			app=1;    
      AVI_play(avi_playlist[VideoDialog.playindex]);
      app = 0;
			vTaskDelay(1);
		}
  }
	while(1)//任务结束,程序在这执行
	{
		vTaskDelay(200);
	}
}

static LRESULT Dlg_VideoList_WinProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  static struct __obj_list *menu_list = NULL;
  static WCHAR (*wbuf)[128];
  switch(msg)
  {
    case WM_CREATE:
    {
      HWND wnd;
      int i = 0;
      list_menu_cfg_t cfg;
	   	RECT rc;
      GetClientRect(hwnd, &rc);
      CreateWindow(BUTTON, L"F", BS_FLAT | BS_NOTIFY|WS_OWNERDRAW |WS_TRANSPARENT|WS_VISIBLE,
                   0, 0, 240, 80, hwnd, eID_VIDEO_RETURN, NULL, NULL);  
      /* 需要分配N+1项，最后一项为空 */
      menu_list = (struct __obj_list *)GUI_VMEM_Alloc(sizeof(struct __obj_list)*(VideoDialog.avi_file_num+1));
      wbuf = (WCHAR (*)[128])GUI_VMEM_Alloc(sizeof(WCHAR *)*VideoDialog.avi_file_num * 128);
      if(menu_list == NULL || wbuf == NULL)
       return 0;
      for(;i < VideoDialog.avi_file_num; i++)
      {
        char p[128] ;
        strcpy(p, lcdlist[i]);
        int t, L;
        L = (int)strlen(p);
        if (L > 13)
        {
          for (t = L; t > 13; t --)
          {
          p[t] = p[t - 1];
          }
          p[13] = '\0';
          p[L + 1] = '\0';
        }  
        x_mbstowcs_cp936(wbuf[i], p, 100);
        menu_list[i].pName = wbuf[i];
        menu_list[i].cbStartup = NULL;
        menu_list[i].icon = L"K";
        menu_list[i].bmp = NULL;
        menu_list[i].color = RGB_WHITE;
      } 
      /* 最后一项为空 */
      menu_list[i].pName = NULL;
      menu_list[i].cbStartup = NULL;
      menu_list[i].icon = NULL;
      menu_list[i].bmp = NULL;
      menu_list[i].color = NULL;         
         
      cfg.list_objs = menu_list; 
      cfg.x_num = 3;
      cfg.y_num = 2; 
      cfg.bg_color = 0x363636;
      wnd = CreateWindow(&wcex_ListMenu,
                  L"ListMenu1",
                  WS_VISIBLE | LMS_ICONFRAME|LMS_PAGEMOVE|WS_TRANSPARENT,
                  rc.x + 100, rc.y + 80, rc.w - 200, rc.h-80,
                  hwnd,
                  eID_FileList,
                  NULL,
                  &cfg);         
      SendMessage(wnd, MSG_SET_SEL, VideoDialog.playindex, 0); 
      
      wnd= CreateWindow(BUTTON, L"L", BS_FLAT | BS_NOTIFY | WS_OWNERDRAW|WS_TRANSPARENT |WS_VISIBLE,
                      0, rc.h * 1 / 2, 70, 70, hwnd, ICON_VIEWER_ID_PREV, NULL, NULL);
      SetWindowFont(wnd, ctrlFont48); 
      
      wnd = CreateWindow(BUTTON, L"K", BS_FLAT | BS_NOTIFY | WS_OWNERDRAW|WS_TRANSPARENT | WS_VISIBLE,
            rc.w - 65, rc.h * 1 / 2, 70, 70, hwnd, ICON_VIEWER_ID_NEXT, NULL, NULL);
      SetWindowFont(wnd, ctrlFont48);
         
      break;
    }
    case WM_DRAWITEM:
    {
       DRAWITEM_HDR *ds;

       ds = (DRAWITEM_HDR*)lParam;
       if(ds->ID == eID_VIDEO_RETURN)
          video_return_ownerdraw(ds); //执行自绘制按钮
       else if(ds->ID == ICON_VIEWER_ID_NEXT || ds->ID == ICON_VIEWER_ID_PREV)
          List_button_ownerdraw(ds);
//       else if(ds->ID == ID_HOME )
//          home_owner_draw(ds);
       return TRUE;    
    }
    case WM_ERASEBKGND:
    {
       HDC hdc = (HDC)wParam;
       RECT rc_top  = {0, 0, 800, 80};
       RECT rc_text = {200, 0, 400, 80};
       RECT rc_cli =*(RECT*)lParam;
       
       //hdc_mem = CreateMemoryDC(SURF_ARGB4444, rc_cli.w, rc_cli.h);
       SetBrushColor(hdc, MapRGB(hdc, 54, 54, 54));
       FillRect(hdc, &rc_cli);
//         if(res!=FALSE)
//            BitBlt(hdc, rc_cli.x, rc_cli.y, rc_cli.w, rc_cli.h,
//                   hdc_bk, rc_cli.x, rc_cli.y, SRCCOPY);         
       
       SetBrushColor(hdc, MapRGB(hdc,0, 0, 0));
       FillRect(hdc, &rc_top);
       SetFont(hdc, defaultFont);
       SetTextColor(hdc, MapRGB(hdc, 250, 250, 250));
       DrawText(hdc, L"播放列表", -1, &rc_text, DT_SINGLELINE| DT_CENTER | DT_VCENTER);

       break;
    }     
    case WM_NOTIFY:
    {
       u16 code, id;	
       LM_NMHDR *nm;
       code = HIWORD(wParam);
       id = LOWORD(wParam); 

       nm = (LM_NMHDR*)lParam;

       if (code == LMN_CLICKED)
       {
          switch (id)
          {
             case eID_FileList:
             {
                
                VideoDialog.playindex = nm->idx;
                VideoDialog.SWITCH_STATE = 1;
                PostCloseMessage(hwnd); //产生WM_CLOSE消息关闭主窗口
             }
             case ICON_VIEWER_ID_LIST:
             {
                PostCloseMessage(hwnd); //产生WM_CLOSE消息关闭主窗口
             }
             break;
          }

       }

       
       if (code == BN_CLICKED && id == ICON_VIEWER_ID_PREV)
       {
          SendMessage(GetDlgItem(hwnd, eID_FileList), MSG_MOVE_PREV, TRUE, 0);
       }

       if (code == BN_CLICKED && id == ICON_VIEWER_ID_NEXT)
       {
          SendMessage(GetDlgItem(hwnd, eID_FileList), MSG_MOVE_NEXT, TRUE, 0);
       }
       if (code == BN_CLICKED && id == eID_VIDEO_RETURN )
       {
          PostCloseMessage(hwnd);
       }

       break;
    }    
    
    case WM_DESTROY:
    {
      VideoDialog.LIST_STATE = 0;
      GUI_VMEM_Free(menu_list);
      GUI_VMEM_Free(wbuf);
      return PostQuitMessage(hwnd);
    }               
    default:
      return DefWindowProc(hwnd, msg, wParam, lParam);
  }
  return WM_NULL;
}

extern uint8_t *Frame_buf;
extern UINT BytesRD;
extern volatile BOOL bDrawVideo;
GUI_MUTEX*	AVI_JPEG_MUTEX = NULL;    // 用于确保一帧图像用后被释放完在退出线程
SURFACE *pSurf1;
u16 *vbuf;
static LRESULT video_win_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch(msg)
  {
    case WM_CREATE:
    { 
			 Delete_VideoTask_Sem = GUI_SemCreate(0, 1);
			
			 AVI_JPEG_MUTEX = GUI_MutexCreate();    // 创建一个互斥信号量

       //音量icon（切换静音模式），返回控件句柄值
       avi_icon[0].rc.y = Set_Widget_VCENTER(440, avi_icon[0].rc.h);
       CreateWindow(BUTTON,L"A",WS_OWNERDRAW|WS_VISIBLE,//按钮控件，属性为自绘制和可视
                                avi_icon[0].rc.x,avi_icon[0].rc.y,//位置坐标和控件大小
                                avi_icon[0].rc.w,avi_icon[0].rc.h,//由avi_icon[0]决定
                                hwnd,eID_Vedio_Power,NULL,NULL);//父窗口hwnd,ID为ID_BUTTON_Power，附加参数为： NULL
       
      avi_icon[1].rc.y = Set_Widget_VCENTER(440, avi_icon[1].rc.h);                         
       //喇叭icon
       CreateWindow(BUTTON,L"P",BS_NOTIFY|BS_CHECKBOX|WS_OWNERDRAW |WS_VISIBLE,
                    avi_icon[1].rc.x,avi_icon[1].rc.y,//位置坐标
                    avi_icon[1].rc.w,avi_icon[1].rc.h,//控件大小
                   hwnd,eID_Vedio_TRUM,NULL,NULL);       
       avi_icon[2].rc.y = Set_Widget_VCENTER(440, avi_icon[2].rc.h);
       //上一首icon
       CreateWindow(BUTTON,L"S",WS_OWNERDRAW|WS_VISIBLE, //按钮控件，属性为自绘制和可视
                    avi_icon[2].rc.x,avi_icon[2].rc.y,//位置坐标
                    avi_icon[2].rc.w,avi_icon[2].rc.h,//控件大小
                    hwnd,eID_Vedio_BACK,NULL,NULL);//父窗口hwnd,ID为ID_BUTTON_List，附加参数为： NULL
       avi_icon[3].rc.y = Set_Widget_VCENTER(440, avi_icon[3].rc.h);
       
       //播放icon
       CreateWindow(BUTTON,L"U",WS_OWNERDRAW|WS_VISIBLE, //按钮控件，属性为自绘制和可视
                    avi_icon[3].rc.x,avi_icon[3].rc.y,//位置坐标
                    avi_icon[3].rc.w,avi_icon[3].rc.h,//控件大小
                    hwnd,eID_Vedio_START,NULL,NULL);//父窗口hwnd,ID为ID_BUTTON_List，附加参数为： NULL
       avi_icon[4].rc.y = Set_Widget_VCENTER(440, avi_icon[4].rc.h);
       //下一首icon
       CreateWindow(BUTTON,L"V",WS_OWNERDRAW|WS_VISIBLE, //按钮控件，属性为自绘制和可视
                    avi_icon[4].rc.x,avi_icon[4].rc.y,//位置坐标
                    avi_icon[4].rc.w,avi_icon[4].rc.h,//控件大小
                    hwnd,eID_Vedio_NEXT,NULL,NULL);//父窗口hwnd,ID为ID_BUTTON_List，附加参数为： NULL
       
       CreateWindow(BUTTON,L"分辨率：0*0",WS_OWNERDRAW|WS_VISIBLE,
                    0,40,380,40,hwnd,eID_TEXTBOX_RES,NULL,NULL);
                    
       avi_icon[8].rc.y = Set_Widget_VCENTER(440, avi_icon[8].rc.h);
       //
       CreateWindow(BUTTON,L"D",WS_OWNERDRAW|WS_VISIBLE, //按钮控件，属性为自绘制和可视
                    avi_icon[8].rc.x,avi_icon[8].rc.y,//位置坐标
                    avi_icon[8].rc.w,avi_icon[8].rc.h,//控件大小
                    hwnd,eID_Vedio_List,NULL,NULL);
       
       //歌曲名字
       CreateWindow(BUTTON,L"",WS_OWNERDRAW|WS_VISIBLE,
                    100,0,600,40,hwnd,eID_TEXTBOX_ITEM,NULL,NULL);
               
       //总时间        
       CreateWindow(BUTTON,L"00:00:00",WS_OWNERDRAW|WS_VISIBLE,
                    680,Set_Widget_VCENTER(382, 30) ,120,30,hwnd,eID_TEXTBOX_ALLTIME,NULL,NULL);
         
       //当前时间           
       CreateWindow(BUTTON,L"00:00:00",WS_OWNERDRAW|WS_VISIBLE,
                    0,Set_Widget_VCENTER(382, 30) ,120,30,hwnd,eID_TEXTBOX_CURTIME,NULL,NULL);
   
                 
       CreateWindow(BUTTON,L"帧率:0FPS/s",WS_OWNERDRAW|WS_VISIBLE,
                    420,40,300,40,hwnd,eID_TEXTBOX_FPS,NULL,NULL);
      #if 1  
       /*********************歌曲进度条******************/
       video_sif_time.cbSize = sizeof(video_sif_time);
       video_sif_time.fMask = SIF_ALL;
       video_sif_time.nMin = 0;
       video_sif_time.nMax = 249;
       video_sif_time.nValue = 0;//初始值
       video_sif_time.TrackSize = 35;//滑块值
       video_sif_time.ArrowSize = 0;//两端宽度为0（水平滑动条）          
       VideoDialog.SBN_TIMER_Hwnd = CreateWindow(SCROLLBAR, L"SCROLLBAR_Time",  WS_OWNERDRAW|WS_VISIBLE, 
                       120, 365, 560, 35, hwnd, eID_SBN_TIMER, NULL, NULL);
       SendMessage(VideoDialog.SBN_TIMER_Hwnd, SBM_SETSCROLLINFO, TRUE, (LPARAM)&video_sif_time);
       /*********************音量值滑动条******************/
       video_sif_power.cbSize = sizeof(video_sif_power);
       video_sif_power.fMask = SIF_ALL;
       video_sif_power.nMin = 0;
       video_sif_power.nMax = 63;//音量最大值为63
       video_sif_power.nValue = 20;//初始音量值
       video_sif_power.TrackSize = 31;//滑块值
       video_sif_power.ArrowSize = 0;//两端宽度为0（水平滑动条）
       CreateWindow(SCROLLBAR, L"SCROLLBAR_R", WS_OWNERDRAW,
                          70, Set_Widget_VCENTER(440, 31), 150, 31, hwnd, eID_SBN_POWER, NULL, NULL);
       SendMessage(GetDlgItem(hwnd, eID_SBN_POWER), SBM_SETSCROLLINFO, TRUE, (LPARAM)&video_sif_power);         
       
       
       CreateWindow(BUTTON, L"O",WS_OWNERDRAW|WS_VISIBLE|WS_TRANSPARENT,
                      720, 5, 80, 80, hwnd, eID_VIDEO_EXIT, NULL, NULL);            
			
       #endif
       u8 *jpeg_buf;
       u32 jpeg_size;
       JPG_DEC *dec;
       VideoDialog.LOAD_STATE = RES_Load_Content(GUI_RGB_BACKGROUNG_PIC, (char**)&jpeg_buf, &jpeg_size);
       VideoDialog.hdc_bk = CreateMemoryDC(SURF_SCREEN, 800, 480);
       if(VideoDialog.LOAD_STATE)
       {
          /* 根据图片数据创建JPG_DEC句柄 */
          dec = JPG_Open(jpeg_buf, jpeg_size);

          /* 绘制至内存对象 */
          JPG_Draw(VideoDialog.hdc_bk, 0, 0, dec);

          /* 关闭JPG_DEC句柄 */
          JPG_Close(dec);
       }
       /* 释放图片内容空间 */
       RES_Release_Content((char **)&jpeg_buf);         
       vbuf =GUI_GRAM_Alloc(800*480*2);
       pSurf1 =CreateSurface(SURF_SCREEN,800,480,800*2,vbuf);
			 pSurf1->GL->FillArea(pSurf1,0,0,LCD_XSIZE,LCD_YSIZE,pSurf1->CC->MapRGB(0,0,0)); 
//       App_PlayVideo(NULL);
			BaseType_t xReturn = pdPASS;/* 定义一个创建信息返回值，默认为pdPASS */
  
			/* 创建视频播放任务 */
			xReturn = xTaskCreate((TaskFunction_t )App_PlayVideo,  /* 任务入口函数 */
														(const char*    )"App_PlayVideo",/* 任务名字 */
														(uint16_t       )3*1024,  /* 任务栈大小 */
														(void*          )NULL,/* 任务入口函数参数 */
														(UBaseType_t    )5, /* 任务的优先级 */
														(TaskHandle_t*  )&VideoTask_Handle);/* 任务控制块指针 */ 
       
			if(pdPASS != xReturn)
			{
				GUI_ERROR("VIDEOPLAYER Start Failed!Play Reset\r\n");
				while(1);//停止工作
			}else
			{
				thread_PlayVideo = 1;
			}
			
      break;
    }
    case WM_NOTIFY:
    {
       u16 code, id, ctr_id;
       id  =LOWORD(wParam);//获取消息的ID码
       code=HIWORD(wParam);//获取消息的类型
        if(code == BN_CLICKED && id == eID_Vedio_TRUM)
        {
          avi_icon[1].state = ~avi_icon[1].state;
          if(avi_icon[1].state == FALSE)
          {
            wm8978_CfgAudioPath(DAC_ON,  SPK_ON|EAR_LEFT_ON|EAR_RIGHT_ON);
          }
          //当音量icon被按下时，设置为静音模式
          else
          {                
						wm8978_CfgAudioPath(DAC_ON,  EAR_LEFT_ON|EAR_RIGHT_ON);
          }         
        }      
       //发送单击
       if(code == BN_CLICKED)
       { 
          switch(id)
          {
             //音量icon处理case
             case eID_Vedio_Power:
             {

                avi_icon[0].state = ~avi_icon[0].state;
                //InvalidateRect(hwnd, &music_icon[0].rc, TRUE);
                //当音量icon未被按下时
                if(avi_icon[0].state == FALSE)
                {
                     ShowWindow(GetDlgItem(hwnd,eID_SBN_POWER), SW_HIDE); //窗口隐藏
                }
                //当音量icon被按下时，设置为静音模式
                else
                {                
                     ShowWindow(GetDlgItem(hwnd,eID_SBN_POWER), SW_SHOW); //窗口显示
                }
                break;
             }              
             case eID_Vedio_START:
             {

                avi_icon[3].state = ~avi_icon[3].state;
                //当音量icon未被按下时
                if(avi_icon[3].state == FALSE)
                {
                   SAI_Play_Start();
                   HAL_TIM_Base_Start_IT(&TIM3_Handle);                       
                   
                   SetWindowText(GetDlgItem(hwnd, eID_Vedio_START), L"U");
                            
                }
                //当音量icon被按下时，暂停
                else
                {          
                   SAI_Play_Stop();
                   HAL_TIM_Base_Stop_IT(&TIM3_Handle);               
                   SetWindowText(GetDlgItem(hwnd, eID_Vedio_START), L"T");
                }
                //
                break;
             }
             case eID_Vedio_BACK:
             {
          
                VideoDialog.playindex--;
                if(VideoDialog.playindex < 0)
                   VideoDialog.playindex = VideoDialog.avi_file_num - 1;  
//                sw_flag = 1;   

                VideoDialog.SWITCH_STATE = 1;               

                break;
             }
             case eID_Vedio_NEXT:
             {                  
                VideoDialog.playindex++;
                
                if(VideoDialog.playindex > VideoDialog.avi_file_num - 1)
                   VideoDialog.playindex = 0;
//                sw_flag = 1;
                VideoDialog.SWITCH_STATE = 1;
//                sif_time.nValue = 0;//设置为0
//                SendMessage(avi_wnd_time, SBM_SETSCROLLINFO, TRUE, (LPARAM)&sif_time);                    
                break;
             }
             case eID_VIDEO_EXIT:
             {
                PostCloseMessage(hwnd);
                break;
             }
          }
       }
       NMHDR *nr;  
       ctr_id = LOWORD(wParam); //wParam低16位是发送该消息的控件ID. 
       nr = (NMHDR*)lParam; //lParam参数，是以NMHDR结构体开头.
       //进度条处理case
       if (ctr_id == eID_SBN_TIMER)
       {
          NM_SCROLLBAR *sb_nr;
          int i = 0;
          sb_nr = (NM_SCROLLBAR*)nr; //Scrollbar的通知消息实际为 NM_SCROLLBAR扩展结构,里面附带了更多的信息.
          switch (nr->code)
          {
             case SBN_THUMBTRACK: //R滑块移动
             {
                i = sb_nr->nTrackValue; //获得滑块当前位置值                
                SendMessage(nr->hwndFrom, SBM_SETVALUE, TRUE, i); //设置进度值
                //置位进度条变更位置
                VideoDialog.avi_chl = 1;//滑动标志
             }
             break;
          }
       }
      if(id==eID_Vedio_List && code==BN_CLICKED)
      {
        
        WNDCLASS wcex;
        VideoDialog.LIST_STATE = 1;
        wcex.Tag	 		= WNDCLASS_TAG;
        wcex.Style			= CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc	= (WNDPROC)Dlg_VideoList_WinProc;
        wcex.cbClsExtra		= 0;
        wcex.cbWndExtra		= 0;
        wcex.hInstance		= NULL;
        wcex.hIcon			= NULL;
        wcex.hCursor		= NULL;
        if(1)
        {
          RECT rc;

          GetClientRect(hwnd,&rc);

          VideoDialog.List_Hwnd = CreateWindowEx(NULL,
                                                    &wcex,L"VideoList",
                                                    WS_TRANSPARENT|WS_CLIPCHILDREN|WS_VISIBLE,
                                                    0,0,800,480,
                                                    hwnd,0,NULL,NULL);
          
        }
      }       
       //音量条处理case
       if (ctr_id == eID_SBN_POWER)
       {
          NM_SCROLLBAR *sb_nr;
          sb_nr = (NM_SCROLLBAR*)nr; //Scrollbar的通知消息实际为 NM_SCROLLBAR扩展结构,里面附带了更多的信息.
          static int NoVol_flag = 0;
          switch (nr->code)
          {
             case SBN_THUMBTRACK: //R滑块移动
             {
                VideoDialog.power= sb_nr->nTrackValue; //得到当前的音量值
                if(VideoDialog.power == 0) 
                {
                   wm8978_OutMute(1);//静音
                   SetWindowText(GetDlgItem(hwnd, eID_Vedio_Power), L"J");
                   NoVol_flag = 1;
                   
                }
                else
                {
                   if(NoVol_flag == 1)
                   {
                      SetWindowText(GetDlgItem(hwnd, eID_Vedio_Power), L"A");
                      NoVol_flag = 0;
                   }
                   wm8978_OutMute(0);
                   wm8978_SetOUT1Volume(VideoDialog.power);//设置WM8978的音量值
                } 
                SendMessage(nr->hwndFrom, SBM_SETVALUE, TRUE, VideoDialog.power); //发送SBM_SETVALUE，设置音量值
             }
             break;
          }
       }   
       break;
    }     
    case WM_DRAWITEM:
    {     
       DRAWITEM_HDR *ds;
       ds = (DRAWITEM_HDR*)lParam;
       if (ds->ID == eID_SBN_TIMER || ds->ID == eID_SBN_POWER)
       {
          vedio_scrollbar_ownerdraw(ds);
          return TRUE;
       }//         
       if(ds->ID == eID_VIDEO_EXIT)
       {
          vedio_exit_ownerdraw(ds);
          return TRUE;         
       }
       else if ((ds->ID >= eID_Vedio_Power && ds->ID<= eID_Vedio_START))
       {
          vedio_button_ownerdraw(ds);
          return TRUE;
       }
       if(ds->ID >= eID_TEXTBOX_ITEM && ds->ID <= eID_TEXTBOX_ALLTIME)
       {
          vedio_text_ownerdraw(ds);
          return TRUE;
       }

    }   
    case WM_ERASEBKGND:
    {
       HDC hdc =(HDC)wParam;
       RECT rc =*(RECT*)lParam;
       
       if(VideoDialog.LOAD_STATE!=FALSE)
          BitBlt(hdc, rc.x, rc.y, rc.w, rc.h, VideoDialog.hdc_bk, rc.x, rc.y, SRCCOPY);
  


       return TRUE;

    }

    case WM_PAINT:
    {
    	PAINTSTRUCT ps;
    	HDC hdc;
    	RECT rc;


    	hdc =BeginPaint(hwnd,&ps);
    	SetBrushColor(hdc,MapRGB(hdc,250,0,0));
        rc.x =160;
        rc.y =89;
        rc.w =120;
        rc.h =80;
        FillRect(hdc,&rc);

    //	if(bDrawVideo)
    	{
    		HDC hdc_avi;

    		hdc_avi =CreateDC(pSurf1,NULL);

    		OffsetRect(&rc,rc.w+4,0);
    		SetBrushColor(hdc,MapRGB(hdc,0,250,0));
    		FillRect(hdc,&rc);

    		//JPEG_Out(hdc,160,89,Frame_buf,BytesRD);
    		BitBlt(hdc,160,89,480,272,hdc_avi,0,0,SRCCOPY);

    		DeleteDC(hdc_avi);

    		bDrawVideo=FALSE;
    	}
    	EndPaint(hwnd,&ps);
    	return TRUE;
    }

    case WM_DESTROY:
    {		
			thread_PlayVideo = 0;  //结束音乐播放线程
			VideoDialog.SWITCH_STATE = 0xff;//切换状态,退出循环播放
      video_timeout =1;//恢复任务
			
			GUI_SemWait(Delete_VideoTask_Sem,0xFFFFFFFF);//死等,同步结束播放线程
			
			/* 关闭硬件 */
			SAI_Play_Stop();	
      HAL_TIM_Base_Stop_IT(&TIM3_Handle);    
			vTaskDelete(VideoTask_Handle);
			
			/* 软件复位 */
      VideoDialog.playindex = 0;
			VideoDialog.SWITCH_STATE = 0;
			avi_icon[3].state = FALSE;
			
      DeleteDC(VideoDialog.hdc_bk);
      GUI_GRAM_Free(vbuf);
      DeleteSurface(pSurf1);
			
			GUI_MutexDelete(AVI_JPEG_MUTEX);//删除互斥量
			GUI_SemDelete(Delete_VideoTask_Sem);//删除信号量
			
      return PostQuitMessage(hwnd);	
    }  
    default:
      return DefWindowProc(hwnd, msg, wParam, lParam);
  }
  return WM_NULL;
}


void	GUI_VideoPlayer_DIALOG(void*param)
{	
	WNDCLASS	wcex;
	MSG msg;
	VideoDialog.avi_file_num = 0;
  scan_files(path);
	if (wm8978_Init()==0)
	{
		GUI_DEBUG("检测不到WM8978芯片!!!\n");
		while (1);	/* 停机 */
	}  
	wcex.Tag = WNDCLASS_TAG;

	wcex.Style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = video_win_proc; //设置主窗口消息处理的回调函数.
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = NULL;//hInst;
	wcex.hIcon = NULL;//LoadIcon(hInstance, (LPCTSTR)IDI_WIN32_APP_TEST);
	wcex.hCursor = NULL;//LoadCursor(NULL, IDC_ARROW);

	//创建主窗口
	VideoDialog.Video_Hwnd = CreateWindowEx(WS_EX_NOFOCUS|WS_EX_FRAMEBUFFER,
                                    &wcex,
                                    L"GUI_MUSICPLAYER_DIALOG",
                                    WS_VISIBLE|WS_CLIPCHILDREN,
                                    0, 0, GUI_XSIZE, GUI_YSIZE,
                                    NULL, NULL, NULL, NULL);

	//显示主窗口
	ShowWindow(VideoDialog.Video_Hwnd, SW_SHOW);

	//开始窗口消息循环(窗口关闭并销毁时,GetMessage将返回FALSE,退出本消息循环)。
	while (GetMessage(&msg, VideoDialog.Video_Hwnd))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}
