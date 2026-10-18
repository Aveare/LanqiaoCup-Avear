/* 头文件申明区 */
#include "key.h"
#include "init.h"
#include "ds1302.h"
#include "seg.h"
#include "onewire.h"
#include "iic.h"
#include "led.h"

/* 变量申明区 */
unsigned long int Ms_Tick,Ms_Tick_Dark_Point,Ms_Tick_Light_Point;
unsigned int Key_Slow_Down,Seg_Slow_Down;
unsigned char Key_Val,Key_Old,Key_Down;
unsigned char Seg_Pos;
unsigned char Seg_String[8];//数码管显示数据存放数组
unsigned char Seg_Point[8] = {0,0,0,0,0,0,0,0};//数码管显示小数点数据存放数组
unsigned char Rtc_Data[8];//时钟实时数据存放数组
unsigned char ucRtc_Set[3] = {0x16,0x59,0x50};//时钟设置数据存放数组
unsigned char Seg_Disp_Mode;//数码管显示模式 0-时间显示 1-温度显示 2-亮度显示 3-参数设置显示
unsigned char Param_Data[3] = {17,25,4};//参数显示数据存放数组 0-小时参数 1-温度参数 2-LED参数
unsigned char Param_Data_Ctrl[3] = {17,25,4};//参数控制数据存放数组 0-小时参数 1-温度参数 2-LED参数
unsigned char Param_Data_Min[3] = {0,1,4};//参数设置最小边界
unsigned char Param_Data_Max[3] = {23,99,8};//参数设置最大边界
unsigned char Param_Data_Inedx;//参数设置指针
bit Brightness_And_Darkness;//亮暗程度 0-亮 1-暗
bit Brightness_And_Darkness_Old;//用于判断状态持续
bit Light_Sts_Flag;//亮状态标志位
bit Dark_Sts_Flag;//暗状态标志位
float Temperature;//温度显示数值
float Voltage_Val;//实时电压
/* 键盘处理函数 */
void Key_Proc()
{
	unsigned char i;//用于赋值的for循环
	if(Key_Slow_Down) return;
	Key_Slow_Down = 1; //键盘减速程序
	Key_Val = Key_Read();//获取键码值
	Key_Down = Key_Val & (Key_Val ^ Key_Old);//检测下降沿
	Key_Old = Key_Val;
	
	switch(Key_Down)
	{
		case 4://界面 在数据界面和参数界面之间切换
			if(Seg_Disp_Mode != 3)//处于数据显示界面
			{
				Param_Data_Inedx = 0;//默认为时间参数设置界面
				Seg_Disp_Mode = 3;//切换到默认参数设置界面
			}
			else//处于参数设置界面
			{
				for(i=0;i<3;i++)
					Param_Data_Ctrl[i] = Param_Data[i];//退出参数设置界面之后将设置好的参数赋值给真实比较参数数组
				Seg_Disp_Mode = 0;//切换到默认数据显示界面
				if((Rtc_Data[0] * 10 + Rtc_Data[1]) >= Param_Data_Ctrl[0])//设置之后重新比较一遍时间,若不满足条件则关闭LED，若满足条件则原状态不变
					Led_Disp(1,0);
			}
		break;
		case 5://切换 切换不同的显示内容
			if(Seg_Disp_Mode != 3)//处于数据显示界面
			{
				if(++Seg_Disp_Mode == 3)//显示模式在0-2之间循环切换
					Seg_Disp_Mode = 0;
			}
			else//处于参数设置界面
			{
				if(++Param_Data_Inedx == 3)//参数模式在0-2之间循环切换
					Param_Data_Inedx = 0;
			}
		break;
		case 8://参数减一
			if(Seg_Disp_Mode == 3)//处于参数设置界面
			{
				if(--Param_Data[Param_Data_Inedx] == Param_Data_Min[Param_Data_Inedx] - 1)//当参数自减到参数下限
					Param_Data[Param_Data_Inedx] = Param_Data_Min[Param_Data_Inedx];//将最小值赋给设置参数
			}
		break;
		case 9://参数加一
			if(Seg_Disp_Mode == 3)//处于参数设置界面
			{
				if(++Param_Data[Param_Data_Inedx] == Param_Data_Max[Param_Data_Inedx] + 1)//当参数自加到参数上限
					Param_Data[Param_Data_Inedx] = Param_Data_Max[Param_Data_Inedx];//将最大值赋给设置参数
			}
		break;
	}
}

/* 信息处理函数 */
void Seg_Proc()
{
	unsigned char i;//用于赋值的for循环
	if(Seg_Slow_Down) return;
	Seg_Slow_Down = 1; //信息处理减速程序
	
	Read_Rtc(Rtc_Data);//将时钟数据读入放入到Rtc_Data数组中
	Temperature = Temperature_Read();//读取实时温度数据
	Voltage_Val = Ad_Read(0x41) / 51.0;//读取实时电压值
	if(Voltage_Val <= 0.1)//判断亮灭程度
		Brightness_And_Darkness = 1;
	else
		Brightness_And_Darkness = 0;
	switch(Seg_Disp_Mode)//数码管显示模式 0-时间显示 1-温度显示 2-亮度显示 3-参数设置显示
	{
		case 0:
			Seg_Point[2] = 0;//关闭小数点显示
			for(i=0;i<8;i++)
				Seg_String[i] = Rtc_Data[i];//将时钟数据显示
		break;
		case 1:
			Seg_Point[6] = 1;//第七位数码管显示小数点
			Seg_String[0] = 12;//C
			for(i=1;i<5;i++)
				Seg_String[i] = 11;//熄灭
			Seg_String[5] = (unsigned char)Temperature / 10 % 10;//显示温度十位
			Seg_String[6] = (unsigned char)Temperature % 10;//显示温度个位
			Seg_String[7] = (unsigned int)(Temperature *100) % 10;//显示温度十分位
		break;
		case 2:
			Seg_Point[6] = 0;//关闭小数点显示
			Seg_Point[2] = 1;//第三位数码管显示小数点
			Seg_String[0] = 13;//E
			Seg_String[1] = Seg_String[5] = Seg_String[6] =11;//熄灭
			Seg_String[2] = (unsigned char)Voltage_Val % 10;//显示电压十位
			Seg_String[3] = (unsigned int)(Voltage_Val *100) / 10 % 10;//显示电压个位
			Seg_String[4] = (unsigned int)(Voltage_Val *100) % 10;//显示电压十分位
			Seg_String[7] = Brightness_And_Darkness;//亮暗程度
		break;
		case 3:
			Seg_Point[2] = Seg_Point[6] = 0;//关闭小数点显示
			Seg_String[0] = 14;//P
			Seg_String[1] = Param_Data_Inedx+1;//通道编号
			for(i=2;i<6;i++)
				Seg_String[i] = 11;//熄灭		
			if(Param_Data_Inedx != 2)//非LED参数设置
			{
				Seg_String[6] = Param_Data[Param_Data_Inedx] / 10 % 10;//显示相关参数十位
				Seg_String[7] = Param_Data[Param_Data_Inedx] % 10;//显示相关参数个位
			}
			else//LED参数设置
			{
				Seg_String[6] = 11;//熄灭
				Seg_String[7] = Param_Data[Param_Data_Inedx] % 10;//显示相关参数个位				
			}
		break;
	}
	
}

/* 其他显示函数 */
void Led_Proc()
{
	if((Rtc_Data[0] * 10 + Rtc_Data[1]) < Param_Data_Ctrl[0])//当前小时数据小于设置参数
		Led_Disp(1,1);//点亮L1
	if((Rtc_Data[0] * 10 + Rtc_Data[1]) == 8)//每到达一个八点
		Led_Disp(1,0);//熄灭L1
	
	if(Temperature < Param_Data_Ctrl[1])//当前温度数据小于设置参数
		Led_Disp(2,1);//点亮L2
	else
		Led_Disp(2,0);//熄灭L2
	
	if(Brightness_And_Darkness)//亮度处于暗状态
		Led_Disp(Param_Data_Ctrl[2],1);//点亮参数LED
	else
		Led_Disp(Param_Data_Ctrl[2],0);//熄灭参数LED
	
	if(Brightness_And_Darkness)//亮度处于暗状态
	{
		if(Dark_Sts_Flag == 0)//检测是否第一次进入暗状态
		{
			Ms_Tick_Dark_Point = Ms_Tick;//计时器复位
			Dark_Sts_Flag = 1;
			Light_Sts_Flag = 0;
		}
		if(Ms_Tick - Ms_Tick_Dark_Point >= 3000)
			Led_Disp(3,1);
	}
	else//亮度处于亮状态
	{
		if(Light_Sts_Flag == 0)//检测是否第一次进入暗状态
		{
			Ms_Tick_Light_Point = Ms_Tick;//计时器复位
			Dark_Sts_Flag = 0;
			Light_Sts_Flag = 1;
		}
		if(Ms_Tick - Ms_Tick_Light_Point >= 3000)
			Led_Disp(3,0);
	}
}

/* 定时器0初始化函数 */
void Timer0Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0x7F;		//定时器时钟12T模式
	TMOD &= 0xF0;		//设置定时器模式
	TL0 = 0x18;		//设置定时初始值
	TH0 = 0xFC;		//设置定时初始值
	TF0 = 0;		//清除TF0标志
	TR0 = 1;		//定时器0开始计时
	ET0 = 1;    //定时器0中断打开
	EA = 1;     //总中断打开
}

/* 定时器0中断服务函数 */
void Timer0Server() interrupt 1
{
	Ms_Tick++;
	if(++Seg_Pos == 8) Seg_Pos = 0;
	if(++Key_Slow_Down == 10) Key_Slow_Down = 0;
	if(++Seg_Slow_Down == 500) Seg_Slow_Down = 0;
	Seg_Disp(Seg_String[Seg_Pos],Seg_Pos,Seg_Point[Seg_Pos]);
}

/* Main */
void main()
{
	Cls_Init();//关闭外设
	Set_Rtc(ucRtc_Set);//设置时钟参数
	Timer0Init();//定时器0初始化
	while(1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
	}
}

