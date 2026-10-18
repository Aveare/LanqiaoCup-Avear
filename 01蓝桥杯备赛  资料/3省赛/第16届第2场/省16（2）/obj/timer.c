#include <STC15F2K60S2.H>
#include "display.h"

unsigned char pos;
unsigned char led_ms;
unsigned char key_ms;
unsigned int smg_ms;
unsigned char LED[];
unsigned char SMG[];
unsigned char dot[];
unsigned char start_plus;//启动累加标志位
unsigned char plus_flag;//累加
unsigned char time_para_disp=2;
unsigned char time_para_cltr=2;
unsigned int count_time_para;
unsigned char led_flash;//LED闪烁标志位
unsigned char count_100ms;

void Timer0_Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0x7F;			//定时器时钟12T模式
	TMOD &= 0xF0;			//设置定时器模式
	TL0 = 0x18;				//设置定时初始值
	TH0 = 0xFC;				//设置定时初始值
	TF0 = 0;				//清除TF0标志
	TR0 = 1;				//定时器0开始计时
	ET0=1;
	EA=1;
}



void Timer0_work()	interrupt 1
{
	led_ms++;
	key_ms++;
	smg_ms++;
	led(LED,pos);
	smg(SMG,dot,pos);
	if(++pos==8)
		pos=0;
	if(start_plus==1)
	{
		if(++count_100ms==100)
		{
			led_flash=1-led_flash;
		}
		count_time_para++;
		if(count_time_para==time_para_cltr*1000)
		{
			count_time_para=0;
			plus_flag=1;
		}
	}
	else 
		count_100ms=0;
		
}
	












