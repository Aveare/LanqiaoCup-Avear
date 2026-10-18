/* 头文件声明区 */
#include <STC15F2K60S2.H>//15单片机寄存器专用头文件
#include <Init.h>//初始化底层驱动专用头文件
#include <Seg.h>//数码管底层驱动专用头文件
#include "onewire.h"

/* 变量声明区 */
unsigned char Seg_Buf[8] = {10,10,10,10,10,10,10,10};//数码管显示数据存放数组
unsigned char Seg_Point[8] = {0,0,0,0,0,0,0,0};//数码管小数点数据存放数组
unsigned char Seg_Pos;//数码管扫描专用变量
unsigned int Seg_Slow_Down;//数码管减速专用变量
float Temper;//温度变量
unsigned long temp_val;
/* 信息处理函数 */
void Seg_Proc()
{
		if(Seg_Slow_Down) return;
			Seg_Slow_Down = 1;//数码管减速程序

    Temper=read_t();
    temp_val=(unsigned long)(Temper*10000);
    Seg_Buf[2]=(temp_val/100000)%10; //十位
    Seg_Buf[3]=(temp_val/10000)%10;  //个位
    Seg_Buf[4]=(temp_val/1000)%10;   //小数第1位
    Seg_Buf[5]=(temp_val/100)%10;    //小数第2位
    Seg_Buf[6]=(temp_val/10)%10;     //小数第3位
    Seg_Buf[7]=temp_val%10;          //小数第4位

    Seg_Point[3] = 1;
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
	if(++Seg_Slow_Down == 500) Seg_Slow_Down = 0;//数码管减速专用
	if(++Seg_Pos == 8) Seg_Pos = 0;//数码管显示专用
	Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],Seg_Point[Seg_Pos]);
}


/* Main */
void main()
{
	System_Init();
	read_t();
	Timer0Init();
	while (1)
	{
		Seg_Proc();
	}
}