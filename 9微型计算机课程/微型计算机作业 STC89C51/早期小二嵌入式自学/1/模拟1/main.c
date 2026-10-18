/* 头文件声明区 */
#include <AT89X52.H>
#include "Key.h"
#include "Seg.h"

/* 变量声明区 */
unsigned char Key_Slow_Down;//按键减速专用变量 10ms
unsigned int Seg_Slow_Down;//数码管减速专用变量 500ms
unsigned char Key_Val,Key_Down,Key_Old;//按键扫描专用变量
unsigned char Seg_Pos;//数码管扫描变量
unsigned char Seg_Buf[6] = {10,10,10,10,10,10};//数码管显示数据存放数组
unsigned char Seg_Mode;//数码管显示界面 0-显示 1-设置
unsigned int Timer_1000ms;//1000毫秒标志位
unsigned char Time_Count = 30;//系统计时变量
bit System_Flag;//系统标志位 0-暂停 1-开始
unsigned char Set_Dat[3] = {15,30,60};//设置参数储存数组
unsigned char Set_Dat_Index = 1;
unsigned int Timer_500ms;//500毫秒标志位
bit Seg_Flag;//数码管标志位

/* 按键处理函数 */
void Key_Proc()
{
    if(Key_Slow_Down) return;
    Key_Slow_Down = 1;//按键减速程序

    Key_Val = Key_Read();//读取按下的键码值
    Key_Down = Key_Val & (Key_Val ^ Key_Old);//捕捉下降沿
    Key_Old = Key_Val;//辅助扫描

   switch(Key_Down)
{
    case 1://启动
        if(Seg_Mode == 0)
            System_Flag = 1;
        break;
    case 3://切换
		if(Seg_Mode == 1)
			Time_Count = Set_Dat[Set_Dat_Index];
			Seg_Mode ^= 1;
		break;
    case 4://设置
        if(Seg_Mode == 1)
        {
            if(++Set_Dat_Index == 3)
                Set_Dat_Index = 0;
        }
        break;
	case 2:
		if(Seg_Mode == 0)
			Time_Count = Set_Dat[Set_Dat_Index];
    break;
}
}

/* 信息处理函数 */
void Seg_Proc()
{
    if(Seg_Slow_Down) return;
    Seg_Slow_Down = 1;//数码管减速程序

    Seg_Buf[0] = Seg_Mode+1;
    if(Seg_Mode == 0)//系统处于显示界面
    {
        Seg_Buf[4] = Time_Count / 10 % 10;//十位显示
        Seg_Buf[5] = Time_Count % 10;//个位显示
    }
  else//系统处于设置界面
	{
		if(Seg_Flag == 1)
		{
			Seg_Buf[4] = Set_Dat[Set_Dat_Index] / 10 % 10;
			Seg_Buf[5] = Set_Dat[Set_Dat_Index] % 10;
		}
		else
		{
			Seg_Buf[4] = 10;
			Seg_Buf[5] = 10;
		}
	}
}

/* 其他显示函数 */
void Led_Proc()
{
    if(Time_Count == 0)
    {
        P1 = 0x00;
        P2_3 = 0;
    }
    else
    {
        P1 = 0xff;
        P2_3 = 1;
    }
}

/* 定时器0初始化函数 */
void Timer0Init(void)  //1毫秒@12.000MHz
{
    TMOD &= 0xF0;  //设置定时器模式
    TMOD |= 0x01;  //设置定时器模式
    TL0 = 0x18;  //设置定时初始值
    TH0 = 0xFC;  //设置定时初始值
    TF0 = 0;  //清除TF0标志
    TR0 = 1;  //定时器0开始计时
    ET0 = 1;//允许定时器0中断 Enable T0
    EA = 1;//开启总中断
}


/* 定时器0中断服务函数 */
void Timer0Server() interrupt 1
{
    TL0 = 0x18;  //设置定时初始值
    TH0 = 0xFC;  //设置定时初始值
    if(++Key_Slow_Down == 10) Key_Slow_Down = 0;
    if(++Seg_Slow_Down == 500) Seg_Slow_Down = 0;

    if(++Seg_Pos == 6) Seg_Pos = 0;//6可为数码管显示有几位
    Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos]);

    if(System_Flag == 1)
    {
        if(++Timer_1000ms == 1000)
        {
            Timer_1000ms = 0;
            Time_Count--;
            if(Time_Count == 255)
                Time_Count = 0;
        }
    }
	if(++Timer_500ms == 500)
	{
		Timer_500ms = 0;
		Seg_Flag ^= 1;
	}
}

/* Main */
void main()
{
    Timer0Init();
    while(1)
    {
        Key_Proc();
        Seg_Proc();
        Led_Proc();
    }
}
