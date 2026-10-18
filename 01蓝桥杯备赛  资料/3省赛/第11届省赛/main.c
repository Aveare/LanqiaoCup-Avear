/* 头文件声明区 */
#include <STC15F2K60S2.H>//15单片机寄存器专用头文件
#include <Init.h>//初始化底层驱动专用头文件
#include <Led.h>//Led底层驱动专用头文件
#include <Key.h>//按键底层驱动专用头文件
#include <Seg.h>//数码管底层驱动专用头文件
#include "iic.h"

/* 变量声明区 */
unsigned char Key_Val,Key_Down,Key_Old,Key_Up;//按键专用变量
unsigned char Key_Slow_Down;//按键减速专用变量
unsigned char Seg_Buf[8] = {10,10,10,10,10,10,10,10};//数码管显示数据存放数组
unsigned char Seg_Point[8] = {0,0,0,0,0,0,0,0};//数码管小数点数据存放数组
unsigned char Seg_Pos;//数码管扫描专用变量
unsigned int Seg_Slow_Down;//数码管减速专用变量
unsigned char ucLed[8] = {0,0,0,0,0,0,0,0};//Led显示数据存放数组
unsigned char Disp_Mode;//0-数据界面 1-参数界面 2-计数界面
float Vol;//电压变量
float Vol_Set;//电压参数
float Vol_P;//电压标准
unsigned char Vol_Save;//保存电压
unsigned char Count;//计数值
bit Count_flag;//计数标志位
unsigned int Timer5000ms;//5s计时变量
unsigned char Error_Count;//错误操作变量
/* 键盘处理函数 */
void Key_Proc()
{
	if(Key_Slow_Down) return;
	Key_Slow_Down = 1;//键盘减速程序

	Key_Val = Key_Read();//实时读取键码值
	Key_Down = Key_Val & (Key_Old ^ Key_Val);//捕捉按键下降沿
	Key_Up = ~Key_Val & (Key_Old ^ Key_Val);//捕捉按键上降沿
	Key_Old = Key_Val;//辅助扫描变量

	 switch(Key_Down)
	 {
		 case 12://显示界面切换
			 if(Disp_Mode==0)Vol_Set=Vol_P;
			 else if(Disp_Mode==1)
				 {
					Count_flag=0;
				  Vol_P=Vol_Set;
					Vol_Save=Vol_P*10;
					EEPROM_Write(&Vol_Save,0,1);
				 }
			 Disp_Mode=(Disp_Mode+1)%3;//0-2循环
		 break;
		 case 13://计数值清0
			 if(Disp_Mode==2)
				Error_Count=Count=0;
			 else
				 Error_Count++;
		 break;
		 case 16://+
			 if(Disp_Mode==1)
			 {
				 Error_Count=0;
				Vol_Set += 0.5;
				if(Vol_Set > 5.0) Vol_Set = 0.0;
				Vol_P = Vol_Set; 
			 }
			 else
				 Error_Count++;
		 break;
		 case 17://-
			 if(Disp_Mode==1)
			 {
				 Error_Count=0;
				Vol_Set -= 0.5;
				if(Vol_Set < 0.0) Vol_Set = 5.0;
				Vol_P = Vol_Set;
				 }
			 else
				 Error_Count++;
		 break;
	 }
	 
}

/* 信息处理函数 */
void Seg_Proc()
{
	unsigned char i;
	if(Seg_Slow_Down) return;
	Seg_Slow_Down = 1;//数码管减速程序
	
	Vol=AD_Read(3) / 51.0f;
	
	switch(Disp_Mode)
	{
		case 0://数据界面
			Seg_Buf[0]=12;//U
			Seg_Buf[5]=(unsigned char)Vol%10;
			Seg_Buf[6]=(unsigned int)(Vol*10)%10;
			Seg_Buf[7]=(unsigned int)(Vol*100)%10;
		Seg_Point[5]=1;
		break;
		case 1://参数界面
			Seg_Buf[0]=13;//P
			Seg_Buf[5]=(unsigned char)Vol_Set%10;
			Seg_Buf[6]=(unsigned int)(Vol_Set*10)%10;
			Seg_Buf[7]=(unsigned int)(Vol_Set*100)%10;
		break;
		case 2://计数界面
			Seg_Point[5]=0;
			Seg_Buf[0]=14;//U
			Seg_Buf[3]=Count/10000%10;
      Seg_Buf[4]=Count/1000%10;
      Seg_Buf[5]=Count/100%10;
      Seg_Buf[6]=Count/10%10;
      Seg_Buf[7]=Count%10;
			for(i=3; Seg_Buf[i]==0&&i<7;i++)
        Seg_Buf[i]=10;
		break;
		
	}
}

/* 其他显示函数 */
void Led_Proc()
{
	ucLed[1]=(Count%2!=0);
	ucLed[2]=(Error_Count>=3);
	
	if(Vol>Vol_P)Count_flag=1;
  else if(Count_flag==1)
  {
   Count_flag=0;
    Count++;
  }
}

/* 定时器0中断初始化函数 */
void Timer0Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0x7F;		//定时器时钟12T模式
	TMOD &= 0xF0;		//设置定时器模式
	TL0 = 0x18;		//设置定时初始值
	TH0 = 0xFC;		//设置定时初始值
	TF0 = 0;		//清除TF0标志
	TR0 = 1;		//定时器0开始计时
	ET0 = 1;    //定时器中断0打开
	EA = 1;     //总中断打开
}

/* 定时器0中断服务函数 */
void Timer0Server() interrupt 1
{  
	if(++Key_Slow_Down == 10) Key_Slow_Down = 0;//键盘减速专用
	if(++Seg_Slow_Down == 500) Seg_Slow_Down = 0;//数码管减速专用
	if(++Seg_Pos == 8) Seg_Pos = 0;//数码管显示专用
	Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],Seg_Point[Seg_Pos]);
	Led_Disp(Seg_Pos,ucLed[Seg_Pos]);
	
	if(Vol>=Vol_P)
		ucLed[0]=Timer5000ms=0;
	else if(++Timer5000ms==5000)
		ucLed[0]=1;
	
}


/* Main */
void main()
{
	System_Init();
		EEPROM_Read(&Vol_Save,0,1);//上电读取
		Vol_P=Vol_Save/10.0;
	AD_Read(3);
	Timer0Init();
	while (1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
	}
}
