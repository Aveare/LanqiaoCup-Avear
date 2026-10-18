/* 头文件声明区 */
#include <STC15F2K60S2.H>
#include <Init.h>
#include <Led.h>
#include <Key.h>
#include <Seg.h>
#include <iic.h>
#include <ultrasonic.h>
/* 变量声明区 */
unsigned char Key_Val,Key_Down,Key_Old,Key_Up;//按键专用变量
unsigned char Key_Slow_Down;//按键减速专用变量
unsigned char Seg_Buf[8] = {10,10,10,10,10,10,10,10};//数码管显示数据存放数组
unsigned char Seg_Point[8] = {0,0,0,0,0,0,0,0};//数码管小数点数据存放数组
unsigned char Seg_Pos;//数码管扫描专用变量
unsigned int Seg_Slow_Down;//数码管减速专用变量
unsigned char ucLed[8] = {0,0,0,0,0,0,0,0};//Led显示数据存放数组
unsigned char Disp_Mode=0;//显示界面 0-测距界面 1-参数界面 2-记录界面
unsigned char Set_Mode=0;//参数界面模式 0-按键模式 1-旋钮模式
unsigned char Set_Low=10;//参数下限
unsigned char Set_High=60;//参数上线
unsigned int Wave_Data;//超声波距离变量
unsigned char Limit_Count;//报警次数
float Vol;//DAC转换的电压 0-5V
bit Time_Flag;//1开始
unsigned int Timer10ms;//10ms计时变量
bit Set_Low_Flag;
bit Set_High_Flag;
unsigned char Star_Flag;//闪烁标志位
unsigned char Timer100ms;//100ms计时标志位
unsigned char Set_FlagH;//报警数标志位H
unsigned char Set_FlagL;//报警数标志位L
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
		case 4://切换
			Disp_Mode=(Disp_Mode+1)%3;//0-2循环
		break;
		case 5://参数调整模式切换  记录模式下清零报警次数
			switch(Disp_Mode)
			{
				case 1://参数调整模式切换
					Set_Mode=(Set_Mode+1)%2;//0-1循环 0-按键模式 1-旋钮模式
				break;
				case 2://报警次数清0
					Limit_Count=0;
				break;
			}
		break;
	case 9://上限调整
			if(Disp_Mode==1&&Set_Mode==1)//参数界面的 旋钮模式
			{
				Time_Flag=1;        //启动计时
				Set_High_Flag=1;    //标记
				Timer10ms=0;       //重置计时
			}
			else if(Disp_Mode==1&&Set_Mode==0)//参数界面的 按键模式
			{
				Set_High+=10;
				if(Set_High>90)Set_High=50;
			}
		break;
		case 8://下限调整
			if(Disp_Mode==1&&Set_Mode==1)//参数界面的 旋钮模式
			{
				Time_Flag=1;        //启动计时
				Set_Low_Flag=1;     //标记
				Timer10ms=0;       //重置计时
			}
			else if(Disp_Mode==1&&Set_Mode==0)//按键模式
			{
				Set_Low+=10;
				if(Set_Low>40)Set_Low=0;
			}
		break;
	}

	
	if(Key_Up == 9) // S9松开
	{
		Time_Flag = 0;
		Set_High_Flag = 0;
		Timer10ms = 0;        //按键松开清除标志位
	}
	if(Key_Up == 8) // S8松开
	{
		Time_Flag = 0;
		Set_Low_Flag = 0;
		Timer10ms = 0;       //按键松开清除标志位
	}
}	

/* 信息处理函数 */
void Seg_Proc()
{
	if(Seg_Slow_Down) return;
	Seg_Slow_Down = 1;//数码管减速程序

	Wave_Data=Ut_Wave_Data();//读取超声波的距离
	switch(Disp_Mode)
	{
		case 0://测距界面
			Seg_Buf[0]=12;//A
			Seg_Buf[5]=Wave_Data>=100?Wave_Data/100%10:10;//百位高位熄灭
			Seg_Buf[6]=Wave_Data/10%10;
			Seg_Buf[7]=Wave_Data%10;
		break;
		case 1://参数界面
			Seg_Buf[0]=13;//P
			Seg_Buf[1]=Set_Mode+1;//1-按键模式 2-旋钮模式
			Seg_Buf[3]=Set_Low/10%10;
			Seg_Buf[4]=Set_Low%10;
			Seg_Buf[5]=11;//-
			Seg_Buf[6]=Set_High/10%10;
			Seg_Buf[7]=Set_High%10;
		break;
		case 2://记录界面
			Seg_Buf[0]=14;//E
			Seg_Buf[1]=Seg_Buf[2]=Seg_Buf[3]=Seg_Buf[4]=Seg_Buf[5]=Seg_Buf[6]=10;//熄灭
			if(Limit_Count<=9)Seg_Buf[7]=Limit_Count;//显示报警次数
				else  Seg_Buf[7]=11;//-
		break;
	}
	
	//报警数设置
	if(Wave_Data<=Set_High&&Wave_Data>=Set_Low)
	{
		Set_FlagH=Set_FlagL=1;//在范围内的标志位
	}
	if(Wave_Data>Set_High && Set_FlagH==1) Set_FlagH=2;
	if(Wave_Data<Set_Low  && Set_FlagL==1) Set_FlagL=2;//有变化，置标志位
	if(Set_FlagH==2)
	{
		Limit_Count++;
		Set_FlagH=0;
	}
	if(Set_FlagL==2)
	{
		Limit_Count++;
		Set_FlagL=0;
	}
}

/*旋钮调整参数函数*/
void Set_Proc()
{
	if(!Time_Flag)return; //未按下按键

	if(Timer10ms>=10)// 长按150ms后
	{
		Vol=AD_Read(0x03)/5.1; // 读取ADC值（0~255转换0-50 单位100mV）
		
		if(Set_High_Flag)//调整上限（S9按下）
		{
			if(Vol<10)        Set_High = 50;
			else if(Vol<=20)   Set_High = 60;
			else if(Vol<=30)   Set_High = 70;
			else if(Vol<=40)   Set_High = 80;
			else if(Vol<=50)  Set_High = 90;
		}
		
		if(Set_Low_Flag)//调整下限（S8按下）
		{
			if(Vol<10)        Set_Low = 0;
			else if(Vol<=20)   Set_Low = 10;
			else if(Vol<=30)   Set_Low = 20;
			else if(Vol<=40)   Set_Low = 30;
			else if(Vol<=50)  Set_Low = 40;
		}
	}
}
/*Led处理函数*/
void Led_Proc()
{	unsigned char i;
	for(i=0;i<3;i++)
	{
		ucLed[i]=(Disp_Mode==i);
	}
	
	if(Wave_Data>=Set_Low&&Wave_Data<=Set_High)
		ucLed[7]=1;
	else
		ucLed[7]=Star_Flag;
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
	
	if(Time_Flag)// 按键按下计时
	{
		if(++Timer10ms==10)
			Timer10ms=10;
	}
	else Timer10ms=0;
	
	if(++Timer100ms==100)//间隔100ms标志位取反
	{
		Timer100ms=0;
		Star_Flag^=1;
	}
}


/* Main */
void main()
{
	
	System_Init();
	Timer0Init();
	while (1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
		Set_Proc();
	}
}