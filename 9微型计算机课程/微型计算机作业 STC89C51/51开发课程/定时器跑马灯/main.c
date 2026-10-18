/*头文件声明*/
#include <REGX51.H>
/*变量声明*/
unsigned int Timer1s;//1s变量
unsigned char count=0;//计时变量
/*Led处理函数*/
void Led_Proc()
{
	P0=~(0x01<<(count%8));//只在八个灯之间流转
}
/*定时器0初始化*/
void Timer0Init(void)		//1毫秒@12.000MHz
{
	TMOD &= 0xF0;		//设置定时器模式
	TMOD |= 0x01;		//设置定时器模式
	TL0 = 0x18;		//设置定时初始值
	TH0 = 0xFC;		//设置定时初始值
	TF0 = 0;		//清除TF0标志
	TR0 = 1;		//定时器0开始计时
	ET0 = 1;    //定时器0中断打开
	EA = 1;     //总中断打开
}
/*定时器0中断服务函数*/
void TImer0_isr(void) interrupt 1
{
	TL0 = 0x18;		//设置定时初始值
	TH0 = 0xFC;		//设置定时初始值
	
	if(++Timer1s==1000)//计时1s
	{
		Timer1s=0;
		count++;
		if(count==65534)count=0;//重置，防止溢出
	}
}
void main()
{
	Timer0Init();
	while(1)
	{
		Led_Proc();
	}
}