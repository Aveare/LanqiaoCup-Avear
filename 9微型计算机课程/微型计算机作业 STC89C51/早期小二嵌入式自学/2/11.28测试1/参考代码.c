//头文件声明区
#include <STC15F2K60S2.H>
#include "Init.h"
#include "Key.h"
#include "Seg.h"
#include "Led.h"
#include "ds1302.h"
#include "onewire.h"
#include "intrins.h"
//变量定义区
unsigned char Proc_Flag;//子程序运行标志位
unsigned int Proc_Slow_Down;//子函数减速专用变量
unsigned char Key_Val,Key_Down,Key_Up,Key_Old;//按键专用变量
unsigned char Seg_Buf[8]={10,10,10,10,10,10,10,10};
unsigned char Seg_Point[8]={0,0,0,0,0,0,0,0};
unsigned char Seg_Pos;//数码管扫描专用变量
unsigned char Led_Buf[8]={0,0,0,0,0,0,0,0};
unsigned char Seg_Disp_Mode;//数码管界面显示 0-温度显示 1-时间显示 2-参数设置
float Temperature;//温度
unsigned char Rtc_Buf[3]={0x23,0x59,0x40};//DS1302初始时间 23时59分50秒
unsigned char T_Set=23;//温度设置值
unsigned char T_Real;//温度真实值
unsigned char Control_Mode;//控制模式 0-温度控制 1-时间控制
bit Second_Flag;//显示分秒标志位
unsigned int Relay_T_5000ms;//5s计时变量(继电器)
bit Relay_T_5000ms_Run;//5s计时启动标志位（继电器）
unsigned int Led_T_5000ms;//5s计时变量(LED)
bit Led_T_5000ms_Run;//5s计时启动标志位（LED）
bit Relay_ON_Flag;//继电器吸合标志位
bit  T_100ms_Run;//计时100ms启动标志位
unsigned char T_100ms;//计时100ms
bit L3_ON_Flag=1;//L3亮标志位
//按键处理函数
void Key_Proc()
{
	if(Proc_Flag != 1) return;
	Proc_Flag=0;
	Key_Val=Key_Read();
	Key_Down = Key_Val & (Key_Val^Key_Old);
	Key_Up = ~Key_Val & (Key_Val^Key_Old);
	Key_Old=Key_Val;
	if(Seg_Disp_Mode==1)//在时间显示界面，按下S17，则显示分秒界面
	{
		if(Key_Old==17)
		{
			Second_Flag=1;
		}
		if(Key_Up==17)//松开S17，则显示时分界面
		{
			Second_Flag=0;
		}
	}
	switch(Key_Down)
	{
		case 12://界面切换
		Seg_Disp_Mode++;
		Seg_Disp_Mode%=3;
		break;
		case 13://模式切换  0-温度控制 1-时间控制
		Control_Mode++;
		Control_Mode%=2;
		break;
		case 16://+
		if(Seg_Disp_Mode==2)//仅在参数设置界面有效
		{
			T_Set++;
			if(T_Set>99) T_Set=10;//温度参数在10~99内变化
		}
		break;
		case 17://-
		if(Seg_Disp_Mode==2)//仅在参数设置界面有效
		{
			T_Set--;
			if(T_Set<10) T_Set=99;//温度参数在10~99内变化
		}
		break;

	}
}
//信息处理函数
void Seg_Proc()
{
	if(Proc_Flag != 2) return;
	Proc_Flag=0;
	Temperature=Temperature_Read();//温度读取
	Rtc_Read(Rtc_Buf);//时间读取
	switch(Seg_Disp_Mode)
	{
		case 0:// 0-温度显示 
		Seg_Buf[0]=17;//U
		Seg_Buf[1]=1;
		Seg_Buf[2]=10;
		Seg_Buf[3]=10;
		Seg_Buf[4]=10;
		Seg_Buf[5]=(unsigned char)Temperature/10%10;
		Seg_Buf[6]=(unsigned char)Temperature%10;
		Seg_Buf[7]=(unsigned int)(Temperature*10)%10;
		Seg_Point[6]=1;
		break;
		case 1://1-时间显示 
		Seg_Buf[0]=17;//U
		Seg_Buf[1]=2;
		Seg_Buf[2]=10;
		Seg_Buf[3]=Second_Flag?Rtc_Buf[1]/16:Rtc_Buf[0]/16;//如果显示分秒标志位为1，则显示分，否则显示时
		Seg_Buf[4]=Second_Flag?Rtc_Buf[1]%16:Rtc_Buf[0]%16;
		Seg_Buf[5]=18;
		Seg_Buf[6]=Second_Flag?Rtc_Buf[2]/16:Rtc_Buf[1]/16;//如果显示分秒标志位为1，则显示秒，否则显示分
		Seg_Buf[7]=Second_Flag?Rtc_Buf[2]%16:Rtc_Buf[1]%16;
		Seg_Point[6]=0;
		break;
		case 2://2-参数设置
		Seg_Buf[0]=17;//U
		Seg_Buf[1]=3;
		Seg_Buf[2]=10;
		Seg_Buf[3]=10;
		Seg_Buf[4]=10;
		Seg_Buf[5]=10;
		Seg_Buf[6]=T_Set/10;
		Seg_Buf[7]=T_Set%10;
		Seg_Point[6]=0;
		break;

	}
}
//其他处理函数
void Led_Proc()
{
	if(Control_Mode==0)//在温度控制界面
	{
		if(Temperature>T_Set*1.0) Relay(1);//如果当前温度大于温度设置值，则继电器吸合，否则不吸合
		else Relay(0);
	}
	else if(Control_Mode==1)//在时间控制界面
	{
		if(Rtc_Buf[1]==0 && Rtc_Buf[2]==0)//如果是整点
		{	
			Relay(1);//吸合
			Relay_ON_Flag=1;//继电器吸合标志位置1
			Relay_T_5000ms_Run=1;//计时标志位置1
		}
	}
	if(Rtc_Buf[1]==0 && Rtc_Buf[2]==0)//如果是整点
	{	
		Led_Buf[0]=1;//L1亮5s后熄灭
		Led_T_5000ms_Run=1;
	}
	if(Control_Mode==0) Led_Buf[1]=1;//如果是温度控制模式，L2亮
	else Led_Buf[1]=0;

	Led_Buf[2]=Relay_ON_Flag?L3_ON_Flag:0;
	Led_Buf[3]=0;//其他灯均熄灭
	Led_Buf[4]=0;
	Led_Buf[5]=0;
	Led_Buf[6]=0;
	Led_Buf[7]=0;
}
//定时器1初始化函数
void Timer1_Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0xBF;			//定时器时钟12T模式
	TMOD &= 0x0F;			//设置定时器模式
	TL1 = 0x18;				//设置定时初始值
	TH1 = 0xFC;				//设置定时初始值
	TF1 = 0;				//清除TF1标志
	TR1 = 1;				//定时器1开始计时

	EA=1;//打开总中断
	ET1=1;//打开定时器1中断
}
//定时器1中断服务函数
void Timer1_Service() interrupt 3
{
	if(Relay_T_5000ms_Run==1)
	{
		Relay_T_5000ms++;
		if(Relay_T_5000ms==5000)
		{
			Relay_T_5000ms=0;
			Relay(0);
			Relay_ON_Flag=0;
		}
	}
	if(Led_T_5000ms_Run==1)
	{
		Led_T_5000ms++;
		if(Led_T_5000ms==5000)
		{
			Led_T_5000ms=0;
			Led_Buf[0]=0;
		}
	}
	if(Relay_ON_Flag==1)
	{
		T_100ms++;
		if(T_100ms==100)
		{
			T_100ms=0;
			L3_ON_Flag^=1;
		}
	}
	else 
	{
		T_100ms=0;
		L3_ON_Flag=0;
		Led_Buf[2]=0;
	}
	if(++Proc_Slow_Down==500) Proc_Slow_Down=0;
	if(Proc_Slow_Down % 9==0) Proc_Flag=1;
	switch (Proc_Slow_Down)
	{
		case 50: Proc_Flag=2; break;
	}
	if(++Seg_Pos==8) Seg_Pos=0;
	Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],Seg_Point[Seg_Pos]);
	Led_Disp(Seg_Pos,Led_Buf[Seg_Pos]);
}

void Delay750ms()		//@12.000MHz
{
	unsigned char i, j, k;

	_nop_();
	_nop_();
	i = 35;
	j = 51;
	k = 182;
	do
	{
		do
		{
			while (--k);
		} while (--j);
	} while (--i);
}

//主函数
void main()
{
	System_Init();//系统初始化
	Timer1_Init();//定时器1初始化
	Set_Rtc(Rtc_Buf);//设置初始时间值
	Temperature_Read();//温度读取
	Delay750ms();//等待温度转换，避免上电显示默认温度
	while(1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
	}
}
