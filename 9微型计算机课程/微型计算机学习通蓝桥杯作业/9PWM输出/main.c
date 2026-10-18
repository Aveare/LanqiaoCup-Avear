/* 头文件声明区 */
#include <STC15F2K60S2.H>//15单片机寄存器专用头文件
#include <Init.h>//初始化底层驱动专用头文件
#include <Led.h>//Led底层驱动专用头文件
#include <Key.h>//按键底层驱动专用头文件
#include <Seg.h>//数码管底层驱动专用头文件
/* 变量声明区 */
unsigned char Key_Val,Key_Down,Key_Old,Key_Up;//按键专用变量
unsigned char Key_Slow_Down;//按键减速专用变量
unsigned char Seg_Buf[8] = {10,10,10,10,10,10,10,10};//数码管显示数据存放数组
unsigned char Seg_Point[8] = {0,0,0,0,0,0,0,0};//数码管小数点数据存放数组
unsigned char Seg_Pos;//数码管扫描专用变量
unsigned int Seg_Slow_Down;//数码管减速专用变量
unsigned char ucLed[8] = {0,0,0,0,0,0,0,0};//Led显示数据存放数组
unsigned char Mode;//0-睡眠风 1-自然风 2-常风
unsigned char zhankong[3]={25,35,75};
unsigned char Pwm_Count;//Pwm循环变量
sbit Pwm = P3^4;
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
		case 4://模式切换
			Mode=(Mode+1)%3;  //0-2循环
		break;
		
	}
}

/* 信息处理函数 */
void Seg_Proc()
{
	if(Seg_Slow_Down) return;
	Seg_Slow_Down = 1;//数码管减速程序

	switch(Mode)
	{
			case 0://睡眠风模式
			Seg_Buf[0]=11;//显示-
			Seg_Buf[1]=Mode+1;
			Seg_Buf[2]=11;//显示-
			Seg_Buf[3]=10;//熄灭
			Seg_Buf[4]=Seg_Buf[5]=0;
			Seg_Buf[6]=zhankong[Mode]/10%10;
			Seg_Buf[7]=zhankong[Mode]%10;
		break;
			case 1://自然风模式
			Seg_Buf[0]=11;//显示-
			Seg_Buf[1]=Mode+1;
			Seg_Buf[2]=11;//显示-
			Seg_Buf[3]=10;//熄灭
			Seg_Buf[4]=Seg_Buf[5]=0; 
			Seg_Buf[6]=zhankong[Mode]/10%10;
			Seg_Buf[7]=zhankong[Mode]%10;
		break;
				case 2://常风模式
			Seg_Buf[0]=11;//显示-
			Seg_Buf[1]=Mode+1;
			Seg_Buf[2]=11;//显示-
			Seg_Buf[3]=10;//熄灭
			Seg_Buf[4]=Seg_Buf[5]=0;
			Seg_Buf[6]=zhankong[Mode]/10%10;
			Seg_Buf[7]=zhankong[Mode]%10;
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
	if(++Seg_Slow_Down == 500) Seg_Slow_Down = 0;//数码管减速专用
	if(++Seg_Pos == 8) Seg_Pos = 0;//数码管显示专用
	Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],Seg_Point[Seg_Pos]);
	Led_Disp(Seg_Pos,ucLed[Seg_Pos]);
}

/* 定时器1初始化函数*/
void Timer1_Init(void)
{
	AUXR |= 0x80;			//定时器时钟1T模式=
	TMOD &= 0x0F;			//保留定时器0设置，清除定时器1的旧配置
	TMOD |= 0x10;			//设置定时器1为模式1（16位定时器）
	TH1=(65535-50)/256;
	TL1=(65535-50)%256;
	TF1 = 0;				//清除TF1标志
	TR1 = 1;				//定时器1开始计时
	ET1 = 1;				//使能定时器1中断
}

/* 定时器1中断函数*/
void Timer1Server() interrupt 3
{
	TH1=(65535-50)/256;
	TL1=(65535-50)%256;
	if(Pwm_Count<=zhankong[Mode]) 
	{
		ucLed[0]=1;
		Pwm=1;
	}
	else
	{
		ucLed[0]=0;
		Pwm=0;
	}
	
	
	Pwm_Count=(Pwm_Count+1)%100;//设置周期为100
}


/* Main */
void main()
{
	System_Init();
	Timer0Init();
	Timer1_Init();
	while (1)
	{
		Key_Proc();
		Seg_Proc();
	}
}