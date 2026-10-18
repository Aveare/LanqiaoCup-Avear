/*头文件引用*/
#include <STC15F2K60S2.H>
#include <Init.h>
#include <Led.h>
#include <Key.h>
/*变量声明*/
unsigned char Key_Val,Key_Down,Key_Old,Key_Up;//按键
unsigned char Key_Slow_Down;//按键减速
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
		case 4: //关闭蜂鸣器
			P2=0x40;Beep(0);
		break;
		case 5: //打开蜂鸣器
			P2=0x40;Beep(1);
		break;
		case 6: //打开led
			P2 = P2 & 0x1f | 0x80;P0=0x00;
		break;
		case 7: //关闭led
			P2 = P2 & 0x1f | 0x80;P0=0xff;
		break;
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

}


/* Main */
void main()
{
	Timer0Init();
	System_Init();
	while (1)
	{
		Key_Proc();
	}
}