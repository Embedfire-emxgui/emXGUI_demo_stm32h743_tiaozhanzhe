#ifndef _GUI_CAMERA_DIALOG_H_
#define _GUI_CAMERA_DIALOG_H_

#include "emXGUI.h"

typedef struct
{
  HWND Cam_Hwnd;
  HWND SetWIN;
  
  uint16_t *cam_buff0;
  uint16_t *cam_buff1;
  
  uint8_t fps;
  int flag;
  int cur_Resolution;
  int cur_LightMode;
  int cur_SpecialEffects;
  BOOL update_flag;
  BOOL focus_status;
  BOOL AutoFocus_Thread;
  BOOL Update_Thread;
}Cam_DIALOG_Typedef;


typedef enum
{
  eID_SET = 0x1000,
  eID_EXIT,
  eID_RETURN,
  eID_FPS,

	eID_SET1,            //自动对焦
	eID_SET2,            //亮度
	eID_SET3,            //饱和度
	eID_SET4,            //对比度
  eID_SET5,            //分辨率
  eID_SET6,            //光线模式
  eID_SET7,            //特殊效果
  eID_SCROLLBAR,       //亮度滑动条
  eID_SCROLLBAR1,      //饱和度滑动条
  eID_SCROLLBAR2,      //对比度滑动条  
  
  eID_TB1,             //当前分辨率显示
  eID_TB2,             //当前光线模式显示   
  eID_TB3,             //当前特殊效果显示
   
  eID_switch,          //自动对焦开关
  eID_Setting1,        //设置分辨率按钮
  eID_Setting2,        //设置光线模式按钮
  eID_Setting3,        //设置特殊效果按钮
  
  
  //单选框---分辨率
  eID_RB1,             //320*240
  eID_RB2,             //480*272
  eID_RB3,             //800*480（默认）
  //单选框---光线模式
  eID_RB4,             //自动
  eID_RB5,             //光照
  eID_RB6,             //阴天
  eID_RB7,             //办公室
  eID_RB8,             //家里  
  //单选框---特殊效果
   eID_RB9,              //冷色
  eID_RB10,             //暖色
  eID_RB11,             //黑白
  eID_RB12,             //泛黄
  eID_RB13,             //反色   
  eID_RB14,             //偏绿
  eID_RB15,             //过曝
  eID_RB16,             //正常  
  
  
  eID_BT1,             //分辨率界面返回按键
  eID_BT2,             //光线模式界面返回按键
  eID_BT3,             //特殊效果界面返回按键
}VideoDlg_Master_ID;

extern void GUI_Camera_DIALOGTest(void *param);
extern Cam_DIALOG_Typedef CamDialog;
extern GUI_SEM *cam_sem;//更新图像同步信号量（二值型）
#endif
