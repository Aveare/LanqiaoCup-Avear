/* 头文件声明区 */
#include <STC15F2K60S2.H>
#include <Init.h>
#include <Led.h>
#include <Key.h>
#include <Seg.h>
#include "ds1302.h" 	
#include "onewire.h"
/* 变量声明区 */
unsigned char Key_Val,Key_Down,Key_Old,Key_Up;//按键专用变量
unsigned char Key_Slow_Down;//按键减速专用变量
unsigned char Seg_Buf[8] = {10,10,10,10,10,10,10,10};//数码管显示数据存放数组
unsigned char Seg_Point[8] = {0,0,0,0,0,0,0,0};//数码管小数点数据存放数组
unsigned char Seg_Pos;//数码管扫描专用变量
unsigned int Seg_Slow_Down;//数码管减速专用变量
unsigned char ucLed[8] = {0,0,0,0,0,0,0,0};//Led显示数据存放数组
unsigned char Disp_Mode=0;//显示界面切换 0-温度 1-时间 2-参数
bit Mode_Control=0;//模式控制 0-温度控制 1-时间控制
float Temper;//实时温度变量
unsigned char T_Set=23;//温度参数变量
unsigned char ucRtc[3]={0x00,0x59,0x55};//时钟显示变量
bit Time_Flag;//按下标志位
bit Relay_Flag;//蜂鸣器打开标志位
bit Relay_5s;//蜂鸣器5s标志位
unsigned int Timer5000ms;//5s计时变量
unsigned char Timer100ms;//100ms计时变量
bit Star_Flag;//闪烁标志位
bit Relay_Buf;
bit Relay_State; // 继电器实际状态（0-断开，1-吸合）
bit L1_5s_Flag;  // L1 5秒标志
bit L1_On_Flag;  // L1 点亮标志
/* 键盘处理函数 */
void Key_Proc()
{
	if(Key_Slow_Down) return;
	Key_Slow_Down = 1;//键盘减速程序

	Key_Val = Key_Read();//实时读取键码值
	Key_Down = Key_Val & (Key_Old ^ Key_Val);//捕捉按键下降沿
	Key_Up = ~Key_Val & (Key_Old ^ Key_Val);//捕捉按键上降沿
	Key_Old = Key_Val;//辅助扫描变量

	if(Disp_Mode==1)
		{
			if(Key_Old==17) Time_Flag=1;//按下状态
			else Time_Flag=0;
		}
	
	switch(Key_Down)
	{
		case 12://显示界面切换
			Disp_Mode=(Disp_Mode+1)%3;
		break;
		case 13://模式切换
			Mode_Control^=1;
		break;
		case 16://+
			if(Disp_Mode==2)
			{
				T_Set++;
				if(T_Set>99) T_Set=99;//限制上限
			}
		break;
		case 17://-
			if(Disp_Mode==2)
			{
				T_Set--;
				if(T_Set<10) T_Set=10;//限制下限
			}
		break;
	}
}

/* 信息处理函数 */
void Seg_Proc()
{
	if(Seg_Slow_Down) return;
	Seg_Slow_Down = 1;//数码管减速程序

	Temper=read_t();
	Read_Rtc(ucRtc);
	switch(Disp_Mode)
	{
		case 0://温度显示界面
			Seg_Buf[0]=12;//U
			Seg_Buf[1]=1;
			Seg_Buf[5]=(unsigned char)Temper/10%10;
			Seg_Buf[6]=(unsigned char)Temper%10;
			Seg_Buf[7]=(unsigned char)(Temper*10)%10;
		Seg_Point[6]=1;
		break;
		case 1://时间显示界面
			Seg_Buf[0]=12;//U
			Seg_Buf[1]=2;
			Seg_Buf[3]=ucRtc[0]/16;
			Seg_Buf[4]=ucRtc[0]%16;
			Seg_Buf[5]=11;//-
			Seg_Buf[6]=ucRtc[1]/16;
			Seg_Buf[7]=ucRtc[1]%16;
		if(Time_Flag)//按键按下，显示分 秒
		{
			Seg_Buf[3]=ucRtc[1]/16;
			Seg_Buf[4]=ucRtc[1]%16;
			Seg_Buf[6]=ucRtc[2]/16;
			Seg_Buf[7]=ucRtc[2]%16;
		}
			Seg_Point[6]=0;
		break;
		case 2://参数设置界面
			Seg_Buf[0]=12;//U
			Seg_Buf[1]=3;
			Seg_Buf[3]=Seg_Buf[4]=Seg_Buf[5]=10;//熄灭
			Seg_Buf[6]=T_Set/10%10;
			Seg_Buf[7]=T_Set%10;
		break;
	}
}

/* 其他显示函数 */
void Led_Proc()
{
    //继电器状态判断
    if(Mode_Control == 0) // 温度控制模式
    {
        Relay_State = (Temper > T_Set) ? 1 : 0;
        Relay(Relay_State);
    }
    else // 时间控制模式
    {
        // 整点判断（分=00，秒=00）
        if((ucRtc[1] == 0x00) && (ucRtc[2] == 0x00))
        {
            Relay_5s = 1;
            Timer5000ms = 0; // 重置5秒计时
        }
        
        if(Relay_5s)
        {
            Relay_State = 1;
            Relay(1);
        }
        else
        {
            Relay_State = 0;
            Relay(0);
        }
    }

    // L1 控制：整点点亮5秒
    if((ucRtc[1] == 0x00) && (ucRtc[2] == 0x00))
    {
        L1_5s_Flag = 1;
        Timer5000ms = 0;
    }

    if(L1_5s_Flag)L1_On_Flag = 1;
    else L1_On_Flag = 0;
    ucLed[0] = L1_On_Flag;

    // L2 控制：温度控制模式亮，否则灭
    ucLed[1] = (Mode_Control == 0) ? 1 : 0;

    // L3 控制：继电器吸合时0.1秒闪烁，否则灭
    if(Relay_State)
    {
        ucLed[2] = Star_Flag;
    }
    else
    {
        ucLed[2] = 0;
    }

    // 其余LED熄灭
    ucLed[3]=ucLed[4]=ucLed[5]=ucLed[6]=ucLed[7]=0;
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

    // 5秒计时（继电器 + L1）
    if(Relay_5s || L1_5s_Flag)
    {
        if(++Timer5000ms == 5000) // 5秒到
        {
            Relay_5s = 0;
            L1_5s_Flag = 0;
            Timer5000ms = 0;
        }
    }

    // 0.1秒闪烁（L3）
    if(++Timer100ms == 100)
    {
        Timer100ms = 0;
        Star_Flag ^= 1; // 翻转闪烁标志
    }
}
/*延迟函数*/
void Delay750ms()		//@12.000MHz
{
	unsigned char i, j, k;
	i = 35;
	j = 51;
	k = 182;
	do
	{
		do
		{
			while (--k);
		} while (--j);
	} while (--i);
}

/* Main */
void main()
{
	read_t();
	Delay750ms();//防止上电85
	Set_Rtc(ucRtc);//初始化上电时间
	System_Init();
	Timer0Init();

    // 初始化标志位
    L1_5s_Flag = 0;
    L1_On_Flag = 0;
    Star_Flag = 0;
    Relay_5s = 0;
    Timer5000ms = 0;
    Timer100ms = 0;

	while (1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
	}
}