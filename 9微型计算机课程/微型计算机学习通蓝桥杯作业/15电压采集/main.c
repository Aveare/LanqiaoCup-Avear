/* 头文件声明区 */
#include <STC15F2K60S2.H>
#include <Init.h>
#include <Seg.h>
#include "iic.h"

/* 变量声明区 */
unsigned char Key_Val,Key_Down,Key_Old,Key_Up;//按键专用变量
unsigned char Key_Slow_Down;//按键减速专用变量
unsigned char Seg_Buf[8] = {10,10,10,10,10,10,10,10};//数码管显示数据存放数组
unsigned char Seg_Point[8] = {0,0,0,0,0,0,0,0};//数码管小数点数据存放数组
unsigned char Seg_Pos;//数码管扫描专用变量
unsigned char Timer100ms;// 100ms计时变量
unsigned long Vol;//电压

/* 键盘处理函数 */
void Seg_Proc()
{
	Seg_Buf[0]=Seg_Buf[2]=11;
	Seg_Buf[1]=1;
	Seg_Buf[3]=10;//???
	Seg_Buf[4]=Vol/1000%10;
	Seg_Buf[5]=Vol/100%10;
	Seg_Buf[6]=Vol/10%10;
	Seg_Buf[7]=Vol%10;
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
	if(++Seg_Pos == 8) Seg_Pos = 0;//数码管显示专用
	Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],Seg_Point[Seg_Pos]);
	
	if(++Timer100ms>=100)
	{
		Timer100ms=0;
		Vol=Pcf8591_Adc(0x03);
		Vol=Vol*5000/255;//计算
	}
}


/* Main */
void main()
{
	System_Init();
	Timer0Init();
	Vol=Pcf8591_Adc(0x03);
	while (1)
	{
		Seg_Proc();
	}
}