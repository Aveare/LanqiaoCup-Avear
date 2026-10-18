/*头文件声明区*/
#include <STC15F2K60S2.H>
#include <Key.h>
#include <Seg.h>
#include <Init.h>
#include <Led.h>
#include <onewire.h>
/*变量声明区*/
unsigned char Key_Slow_Down;//按键减速专用变量
unsigned char Key_Val,Key_Old,Key_Down,Key_Up;//按键扫描专用变量
unsigned int Seg_Slow_Down;//数码管减速专用变量
unsigned char Seg_Buf[8] = {10,10,10,10,10,10,10,10};//数码管显示数据存放数组
unsigned char Seg_Point[8] = {0,0,0,0,0,0,0,0};//数码管小数点数据存放数组
unsigned char Seg_Pos;//数码管扫描专用变量
unsigned char ucLed_Buf[8] = {1,1,1,1,1,1,1,1};//Led显示数据存放数组
unsigned char Seg_Disp_Mode;//数码管显示模式  0-温度显示界面 1-参数设置界面
unsigned char P_Dat[2]={30,20};//参数数组储存数组
unsigned char P_Contol[2]={30,20};//真实参数控制数组
unsigned char P_Dat_Index;//参数数据数组指针 0-上限 1-下线
float Temperature;//温度储存变量
bit Seg_Star_Flag;//数码管闪烁标志位
unsigned int Timer_500ms;//500ms计时变量
unsigned int Key_Time;//记录按键时间变量
bit Key_Flag;//按键使能标志位
unsigned char Led_Pwm;
unsigned char level;
/*按键处理函数*/
void Key_Proc()
{
	if(Key_Slow_Down) return;
	Key_Slow_Down = 1;//键盘减速程序

	Key_Val = Key_Read();//实时读取键码值
	Key_Down = Key_Val & (Key_Old ^ Key_Val);//捕捉按键下降沿
	Key_Up = ~Key_Val & (Key_Old ^ Key_Val);//捕捉按键上降沿
	Key_Old = Key_Val;//辅助扫描变量
		
	if(Seg_Disp_Mode==1)
	{
		if(Key_Down==14||Key_Down==15)//产生下降沿
			Key_Flag=1;//拉高标志位，开始计时
	}
	
	if(Key_Time<500)//在500毫秒之内
	{
		if(Key_Up==14)//短按
		Key_Flag=Key_Time=0;
		if(++P_Dat[P_Dat_Index]==71)
			P_Dat[P_Dat_Index]=10;	
		
		if(Key_Up==15)//短按
		Key_Flag=Key_Time=0;
		if(--P_Dat[P_Dat_Index]==9)
			P_Dat[P_Dat_Index]=70;	
	}
	else//在500毫秒之外
	{
		if(Key_Old==14)
		{
			if(++P_Dat[P_Dat_Index]==71)
			P_Dat[P_Dat_Index]=10;	
		}
		
		if(Key_Old==15)
		{
			if(--P_Dat[P_Dat_Index]==9)
			P_Dat[P_Dat_Index]=70;	
		}
		if(Key_Up==14||Key_Up==15)
			Key_Flag=Key_Time=0;
	}
	
	switch(Key_Down)
	{
		case 12://界面切换
			if(++Seg_Disp_Mode==2)Seg_Disp_Mode=0;
			if(++Seg_Disp_Mode==0)//从参数设置界面跳转刀温度显示界面
			{
				P_Dat_Index=0;
				if(P_Dat[0]>P_Dat[1])
				{
					P_Contol[0]=P_Dat[0];
					P_Contol[1]=P_Dat[1];
				}
			}
		break;
		case 13://参数切换
			if(Seg_Disp_Mode==1)
			{
			P_Dat_Index^=1;
			}
		break;
			case 14://+
			if(Seg_Disp_Mode==1)
			{
				if(++P_Dat[P_Dat_Index]==71)
				{
					P_Dat[P_Dat_Index]=10;
				}
			}
		break;
			case 16://重置
			P_Contol[0]=30;
			P_Contol[1]=20;
		break;	
	}
}

/* 信息处理函数 */
void Seg_Proc()
{
	if(Seg_Slow_Down) return;
	Seg_Slow_Down = 1;//数码管减速程序
		
	/* 信息获取区域 */
	Temperature=rd_temperature();
	
	/* 数据显示区域 */
	switch(Seg_Disp_Mode)
	{
		case 0:// 温度显示界面
			Seg_Buf[0]=11;//c
			Seg_Buf[4]=(unsigned char)Temperature/10%10;
			Seg_Buf[5]=(unsigned char)Temperature%10;
			Seg_Buf[6]=(unsigned int)(Temperature*10)%10;
			Seg_Buf[7]=11;//c
			Seg_Point[5]=1;
		break;
		case 1:
			Seg_Buf[0]=12;//P
			Seg_Buf[3]=P_Dat[0]/10%10;
			Seg_Buf[4]=P_Dat[0]%10;
			Seg_Buf[5]=13;//-
			Seg_Buf[6]=P_Dat[1]/10%10;
			Seg_Buf[7]=P_Dat[1]%10;
			
		
			if(P_Dat_Index==0)
			{
				Seg_Buf[3]=Seg_Star_Flag?P_Dat[0]/10%10:10;
				Seg_Buf[4]=Seg_Star_Flag?P_Dat[0]/10%10:10;
			}
			else
			{
				Seg_Buf[5]=Seg_Star_Flag?P_Dat[1]/10%10:10;
				Seg_Buf[6]=Seg_Star_Flag?P_Dat[1]/10%10:10;
			}
			Seg_Point[5]=0;
		break;
	}
	
	
}

/* 其他显示函数 */
void Led_Proc()
{
	ucLed_Buf[0]=(unsigned char)Temperature/P_Contol[0];
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
	Led_Disp(Seg_Pos,ucLed_Buf[Seg_Pos]);
	
	if(++Timer_500ms==500)
	{
		Timer_500ms=0;
		Seg_Star_Flag^=1;
	}
	
	if(Key_Flag==1)
	{
		if(++Key_Time==600)
			Key_Time=600;
	}
	
	if(++Led_Pwm==12)Led_Pwm=1;
	if(Led_Pwm<level)
		Led_Disp(Seg_Pos,ucLed_Buf[Seg_Pos]);
	else
		Led_Disp(Seg_Pos,0);
}

void Delay750us()		//@12.000MHz
{
	unsigned char i, j;

	i = 9;
	j = 190;
	do
	{
		while (--j);
	} while (--i);
}


/* Main */
void main()
{
	rd_temperature();
	Delay750us();
	Sys_Init();
	Timer0Init();
	while (1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
	}
}