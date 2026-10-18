/* 头文件声明区 */
#include <STC15F2K60S2.H>
#include <Init.h>
#include <Led.h>
#include <Key.h>
#include <Seg.h>
#include "onewire.h"
#include "ds1302.h"  		
/* 变量声明区 */
unsigned char Key_Val,Key_Down,Key_Old,Key_Up;//按键专用变量
unsigned char Key_Slow_Down;//按键减速专用变量
unsigned char Seg_Buf[8] = {10,10,10,10,10,10,10,10};//数码管显示数据存放数组
unsigned char Seg_Point[8] = {0,0,0,0,0,0,0,0};//数码管小数点数据存放数组
unsigned char Seg_Pos;//数码管扫描专用变量
unsigned int Seg_Slow_Down;//数码管减速专用变量
unsigned char ucLed[8] = {0,0,0,0,0,0,0,0};//Led显示数据存放数组
unsigned char Disp_Mode=0;//显示模式 0-参数设置界面 1-时钟显示界面 2-温度采集界面
unsigned char ucRtc[3]={0x23,0x59,0x50};//1302初始化时间
unsigned char ucRtc_Set[6];//时间设置变量
unsigned char T_Set[4]={1,5,30,60};//温度采集间隔
unsigned char T_Set_Index=0;//温度采集间隔索引
unsigned char T_Set_Index_Con;//温度采集间隔确定
bit Star_Flag;//闪烁标志位
unsigned int Timer1000ms;//1s计时变量
float Temper;//温度变量
unsigned int Timer_Set;//采集温度间隔实现 1s 5s 30s 60s
unsigned char Temper_Count;//采集次数计数
unsigned char Temper_Save[10];//采集温度保存
bit L1_Flag;
unsigned char count;
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
		case 4://温度采集间隔时间切换
			if(Disp_Mode==0)
				T_Set_Index=(T_Set_Index+1)%4;//0-3循环
		break;
		case 5:
			if(Disp_Mode==0)
			{
				T_Set_Index_Con=T_Set_Index;//保存温度采集间隔时间
				Disp_Mode=1;//进入时钟显示界面
			}
		break;
		case 6:
			if(Disp_Mode==2)	L1_Flag=0;//按下 S6，L1 熄灭
		break;
		case 7:
			if(Disp_Mode==2)Disp_Mode=0;
		break;
	}
}

/* 信息处理函数 */
void Seg_Proc()
{
	if(Seg_Slow_Down) return;
	Seg_Slow_Down = 1;//数码管减速程序

	Read_Rtc(ucRtc);//读取时钟
	switch(Disp_Mode)
	{
		case 0://参数设置界面
			Seg_Buf[5]=11;//-
			Seg_Buf[6]=T_Set[T_Set_Index]/10%10;
			Seg_Buf[7]=T_Set[T_Set_Index]%10;
		break;
		case 1://时钟显示界面
			Seg_Buf[0]=ucRtc[0]/16;
			Seg_Buf[1]=ucRtc[0]%16;
			Seg_Buf[3]=ucRtc[1]/16;
			Seg_Buf[4]=ucRtc[1]%16;
			Seg_Buf[6]=ucRtc[2]/16;
			Seg_Buf[7]=ucRtc[2]%16;
			Seg_Buf[2]=Seg_Buf[5]=Star_Flag?11:10;//-or灭  闪烁1s
		break;
		case 2://温度采集显示界面
			Seg_Buf[0]=Seg_Buf[5]=11;//-
			Seg_Buf[1]=0;
			Seg_Buf[2]=count;
			Seg_Buf[3]=Seg_Buf[4]=10;//熄灭
			Seg_Buf[6]=Temper_Save[count]/10%10;
			Seg_Buf[7]=Temper_Save[count]%10;
		break;
	}
	
	if(Temper_Count==9)
	{
		Disp_Mode=2;//采集到10个数据后进入数码管温度采集显示界面
		L1_Flag=1;
	}
}

/* 其他显示函数 */
void Led_Proc()
{
	ucLed[0]=L1_Flag;//采集到 10 个数据后，指示灯 L1 闪烁
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
	if(++Seg_Slow_Down == 250) Seg_Slow_Down = 0;//数码管减速专用
	if(++Seg_Pos == 8) Seg_Pos = 0;//数码管显示专用
	Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],Seg_Point[Seg_Pos]);
	Led_Disp(Seg_Pos,ucLed[Seg_Pos]);
	
	if(++Timer1000ms==1000)
	{
		Timer1000ms=0;
		Star_Flag^=1;
		if(++count==10)count=0;
	}
	if(Disp_Mode==1)//在时钟显示界面
	{
		if(++Timer_Set==T_Set[T_Set_Index_Con]*1000)
		{
			Timer_Set=0;
			Temper_Save[Temper_Count]=read_t();//读取温度
			Temper_Count++;
			if(Temper_Count==10)Temper_Count=0;//0-9循环
		}
	}
}


/* Main */
void main()
{
	Set_Rtc(ucRtc);
	System_Init();
	Timer0Init();
	while (1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
	}
}