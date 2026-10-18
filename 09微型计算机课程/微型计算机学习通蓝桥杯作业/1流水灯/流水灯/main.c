/*头文件引用*/
#include <STC15F2K60S2.H>

/*变量声明*/
unsigned int Timer1000ms;//1000ms计时
unsigned char count=0,Pos=0;
/*初始化关闭外设*/
void System_Init()//关闭外设子函数
{
	P0=0xff;//led全灭
	P2=P2&0x1f|0x80;//选中led通道
	P2&=0x1f;//关闭通道
	
	P0=0x00;//关闭继电器，蜂鸣器
	P2=P2&0x1f|0xa0;//选中外设通道
	P2&=0x1f;//关闭通道
}

/* 定时器0中断初始化函数 */
void Timer0Init(void)		//1毫秒@12.000MHz
{
	AUXR&=0x7F;		//定时器时钟12T模式
	TMOD&=0xF0;		//设置定时器模式
	TL0=0x18;		//设置定时初始值
	TH0=0xFC;		//设置定时初始值
	TF0=0;		//清除TF0标志
	TR0=1;		//定时器0开始计时
	ET0=1;    //定时器中断0打开
	EA=1;     //总中断打开
}

/* 定时器0中断服务函数 */
void Timer0Server() interrupt 1
{  
	if(++Timer1000ms>=1000)       //计时1s
	{
		Timer1000ms=0;
		P2=0x80;                   //选中Led通道
		P0=~(0x01<<Pos);
		P2=0x00;
		if(++Pos>=8)Pos=0;
	}
}

/*Main*/
void main()
{
	System_Init();
	Timer0Init();
	while(1);
}