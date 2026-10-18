/*头文件引用*/
#include <STC15F2K60S2.H>
#include <Init.h>
#include <Key.h>
/*变量声明*/
unsigned char Pwm_t=0;//Pwm周期循环变量
unsigned char Led_d=0;//led亮度等级  取值0-10
unsigned int Timer50ms;//50ms计数变量
unsigned char Led_t;//呼吸灯0-20周期变化
bit Timer_Flag;//标志位

/* 定时器0中断初始化*/
void Timer0Init(void)
{
    AUXR &= 0x7F;        // 定时器时钟12T模式
    TMOD &= 0xF0;        // 设置定时器0为16位模式
    TL0 = 0x00;          // 12MHz 1ms定时初始值
    TH0 = 0xFC;          // 12MHz 1ms定时初始值
    TF0 = 0;             // 清除溢出标志
    TR0 = 1;             // 启动定时器0
    ET0 = 1;             // 开启定时器0中断
    EA = 1;              // 开启总中断
}
/* 定时器0中断服务函数 */
void Timer0Server() interrupt 1
{
		TL0 = 0x00;          // 12MHz 1ms定时初始值
    TH0 = 0xFC;          // 12MHz 1ms定时初始值
	
		Pwm_t=(Pwm_t+1)%10;   //周期10ms
		P2=P2&0x1f|0x80;//选中led通道
		if(Led_d>Pwm_t)
			P0=0x00;//led全开
		else 
			P0=0xff;//全灭
		if(++Timer50ms>=50)
		{
			Timer50ms=0;
			Timer_Flag=1;
		}
}
 void Pwm()
 {
		if(Timer_Flag)
		{
			Led_t=(Led_t+1)%20;
//led_t 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20
//led_d 0 1 2 3 4 5 6 7 8 9 10  9  8  7  6  5  4  3  2  1  0
			if(Led_t<10)Led_d=Led_t;
			else Led_d=20-Led_t;
			Timer_Flag=0;
		}
 }

/*Main*/
void main()
{
	System_Init();
	Timer0Init();
	while (1)
	{
		Pwm();
	}
}