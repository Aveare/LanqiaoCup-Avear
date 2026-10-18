/* 头文件声明区 */
#include <STC15F2K60S2.H>
#include <Init.h>
#include <Led.h>
#include <Key.h>
#include <Seg.h>
#include "onewire.h"
#include <iic.h>
/* 变量声明区 */
unsigned char Key_Val,Key_Down,Key_Old,Key_Up;//按键专用变量
unsigned char Key_Slow_Down;//按键减速专用变量
unsigned char Seg_Buf[8] = {10,10,10,10,10,10,10,10};//数码管显示数据存放数组
unsigned char Seg_Point[8] = {0,0,0,0,0,0,0,0};//数码管小数点数据存放数组
unsigned char Seg_Pos;//数码管扫描专用变量
unsigned int Seg_Slow_Down;//数码管减速专用变量
unsigned char ucLed[8] = {0,0,0,0,0,0,0,0};//Led显示数据存放数组
bit Disp_Mode=0;//0-检测界面 1-阈值界面
bit Disp_Mode_in=0;//0-温度阈值界面 1-光照阈值界面
unsigned char Temper;//实时温度变量
unsigned int Lux;//实时光照变量
unsigned char Temper_Limit=30;//温度阈值 10-50  上电默认30
unsigned int Lux_Limit=100;//光照阈值 100-300  上电默认100
float Vol;//采集模拟电压值变量
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
		case 4://显示界面切换
			Disp_Mode^=1;
		if(Disp_Mode==1 && Disp_Mode_in==1)Disp_Mode_in=0;//进入阈值界面，默认在温度阈值
		break;
		case 5://子模式切换
			Disp_Mode_in^=1;
		break;
		case 8://+
			if(Disp_Mode==1 && Disp_Mode_in==0)//在温度阈值界面
			{
				Temper_Limit+=5;
				if(Temper_Limit>50)Temper_Limit=50;
			}
			else if(Disp_Mode==1 && Disp_Mode_in==1)//在光照阈值界面
			{
				Lux_Limit+=20;
				if(Lux_Limit>300)Lux_Limit=300;
			}
		break;
		case 9://-
			if(Disp_Mode==1 && Disp_Mode_in==0)//在温度阈值界面
			{
				Temper_Limit-=5;
				if(Temper_Limit<10)Temper_Limit=10;
			}
			else if(Disp_Mode==1 && Disp_Mode_in==1)//在光照阈值界面
			{
				Lux_Limit-=20;
				if(Lux_Limit<100)Lux_Limit=100;
			}
		break;
	}
}

/* 信息处理函数 */
void Seg_Proc()
{
	if(Seg_Slow_Down) return;
	Seg_Slow_Down = 1;//数码管减速程序

	Temper = read_t();

	if(Disp_Mode==0)               //检测界面
	{
		Seg_Buf[0]=12;//C
		Seg_Buf[1]=Temper/10%10;
		Seg_Buf[2]=Temper%10;       //实时显示环境温度
		Seg_Buf[4]=13;//H
		Seg_Buf[5]=Lux>=100?Lux/100%10:10;
		Seg_Buf[6]=Lux>=10?Lux/10%10:10;
		Seg_Buf[7]=Lux%10;
	}
	else if(Disp_Mode==1 && Disp_Mode_in==0)//显示阈值 温度阈值
	{
		Seg_Buf[0]=14;//P
		Seg_Buf[1]=12;//C
		Seg_Buf[2]=Seg_Buf[3]=Seg_Buf[4]=Seg_Buf[5]=10;//熄灭
		Seg_Buf[6]=Temper_Limit/10%10;
		Seg_Buf[7]=Temper_Limit%10;
	}
	else if(Disp_Mode==1 && Disp_Mode_in==1)//显示阈值 光照阈值
	{
		Seg_Buf[0]=14;//P
		Seg_Buf[1]=13;//H
		Seg_Buf[2]=Seg_Buf[3]=Seg_Buf[4]=10;//熄灭
		Seg_Buf[5]=Lux_Limit/100%10;
		Seg_Buf[6]=Lux_Limit/10%10;
		Seg_Buf[7]=Lux_Limit%10;        
	}
	/*AD功能区*/
	Vol=AD_Read(0x43) /51.0;//采集RB2模拟电压
	if(Vol<0.3)Lux=10;
	else if(Vol>5)Lux=500;
	else
	{
		 Lux=10+(Vol-0.3)*(490/4.7);  //斜率计数Lux
	}
}

/* 其他显示函数 */
void Led_Proc()
{
	ucLed[0]=~Disp_Mode;
	ucLed[1]=Disp_Mode;
	if(Temper>Temper_Limit && Lux>Lux_Limit)ucLed[7]=1;
		else ucLed[7]=0;
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
}
/*延迟函数*/
void Delay750ms()		//@12.000MHz
{
	unsigned char i, j, k;
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

/* Main */
void main()
{
	System_Init();
	read_t();
	Delay750ms();
	Timer0Init();
	while (1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
	}
}