/* 头文件声明区 */
#include <STC15F2K60S2.H>
#include <Init.h>
#include <Seg.h>
/* 变量声明区 */
unsigned char Seg_Buf[8] = {10,10,10,10,10,10,10,10};//数码管显示数据存放数组
unsigned char Seg_Point[8] = {0,0,0,0,0,0,0,0};//数码管小数点数据存放数组
unsigned char Seg_Pos;//数码管扫描专用变量
unsigned int Seg_Slow_Down;//数码管减速专用变量
unsigned int Timer1000ms;//1s计时变量
unsigned long count=95;//计数变量
/* 信息处理函数 */
void Seg_Proc()
{
	unsigned char i;
	unsigned int temp_count = count;//临时赋值
	unsigned char start_pos = 7; 
	unsigned char digit[8]; //临时数组
	if(Seg_Slow_Down) return;
	Seg_Slow_Down = 1;//数码管减速程序
	
	for(i=0; i<7; i++)Seg_Buf[i] = 10;
	
	if(count == 0)
	{
		Seg_Buf[7] = 0;
		return;
	}

	digit[0] = (temp_count / 10000000) % 10;
	digit[1] = (temp_count / 1000000) % 10;
	digit[2] = (temp_count / 100000) % 10;
	digit[3] = (temp_count / 10000) % 10;
	digit[4] = (temp_count / 1000) % 10;
	digit[5] = (temp_count / 100) % 10;
	digit[6] = (temp_count / 10) % 10;		
	digit[7] = temp_count % 10;	
	
	for(i=0; i<7; i++)
	{
		if(digit[i] != 0)
		{
			start_pos = i;
			break;
		}
	}
	if(start_pos < 7)
	{
		for(i=start_pos; i<7; i++)Seg_Buf[i] = digit[i];
	}
	Seg_Buf[7] = digit[7];
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
	
	if(++Timer1000ms>=1000)
	{
		Timer1000ms=0;
		count++;
	}
}

/* Main */
void main()
{
	
	System_Init();
	Timer0Init();
	while (1)
	{
		Seg_Proc();
	}
}
