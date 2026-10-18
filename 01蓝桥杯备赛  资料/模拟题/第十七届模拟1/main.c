/* 头文件声明区 */
#include <STC15F2K60S2.H>
#include <Init.h>
#include <Led.h>
#include <Key.h>
#include <Seg.h>
#include <ds1302.h>
#include <ultrasonic.h>

/* 变量声明区 */
unsigned char Key_Val,Key_Down,Key_Old,Key_Up;//按键专用变量
unsigned char Key_Slow_Down;//按键减速专用变量
unsigned char Seg_Buf[8] = {10,10,10,10,10,10,10,10};//数码管显示数据存放数组
unsigned char Seg_Point[8] = {0,0,0,0,0,0,0,0};//数码管小数点数据存放数组
unsigned char Seg_Pos;//数码管扫描专用变量
unsigned int Seg_Slow_Down;//数码管减速专用变量
unsigned char ucLed[8] = {0,0,0,0,0,0,0,0};//Led显示数据存放数组
unsigned char Disp_Mode;//0-数据界面 1-时间界面 2-记录界面
unsigned char Ut_Data;//超声波距离
unsigned char ucRtc[3]={0x23,0x59,0x50};//时钟数据存放数组 23：59：06
unsigned char ucRtc_Set[3][6];//时钟数据设置数组
unsigned char Mode3_Index;//记录界面索引
bit Beep_Flag;//蜂鸣器标志位
bit Ut_Flag;//超声波触发标志位

void ucRtc_Init()
{
	unsigned char i,j;
	for(i=0;i<3;i++)
	{
		for(j=0;j<6;j++)
		ucRtc_Set[i][j]=11;
	}
}
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
		case 4://显示模式切换
			Disp_Mode=(Disp_Mode+1)%3;//0-2循环
		break;
		case 5://查询
			Mode3_Index=(Mode3_Index+1)%3;//0-2循环
		break;
		case 8://清空 所有触发时间
			if(Disp_Mode==2)ucRtc_Init();
			
		break;
		case 9:
			if(Beep_Flag)
			{
				Beep(0);//如果报警，按下按键关闭报警
				Beep_Flag=0;
			}
		break;
	}
}

/* 信息处理函数 */
void Seg_Proc()
{
	if(Seg_Slow_Down) return;
	Seg_Slow_Down = 1;//数码管减速程序
	Ut_Data=Ut_Wave_Data();//超声波读距
	
	Read_Rtc(ucRtc);//读取实时时钟数据
	
	switch(Disp_Mode)
	{
		case 0://数据界面
			Seg_Buf[0]=12;//L
			Seg_Buf[5]=Ut_Data>=100?Ut_Data/100%10:0;
			Seg_Buf[6]=Ut_Data/10%10;
			Seg_Buf[7]=Ut_Data%10;
		
			Seg_Buf[1]=Seg_Buf[2]=Seg_Buf[3]=Seg_Buf[4]=10;
		break;
		case 1://时间界面
			Seg_Buf[0]=ucRtc[0]/16;
			Seg_Buf[1]=ucRtc[0]%16;
				Seg_Buf[2]=11;//-
			Seg_Buf[3]=ucRtc[1]/16;
			Seg_Buf[4]=ucRtc[1]%16;
				Seg_Buf[5]=11;//-
			Seg_Buf[6]=ucRtc[2]/16;
			Seg_Buf[7]=ucRtc[2]%16;
		break;
		case 2://记录界面
			Seg_Buf[0]=13;//A
			Seg_Buf[1]=Mode3_Index+1;//0-2通过加一实现1-3
			switch(Mode3_Index)
			{
				case 0://index为1
					Seg_Buf[2]=ucRtc_Set[0][0];
					Seg_Buf[3]=ucRtc_Set[0][1];
					Seg_Buf[4]=ucRtc_Set[0][2];
					Seg_Buf[5]=ucRtc_Set[0][3];
					Seg_Buf[6]=ucRtc_Set[0][4];
					Seg_Buf[7]=ucRtc_Set[0][5];
				break;
				case 1://index为2
					Seg_Buf[2]=ucRtc_Set[1][0];
					Seg_Buf[3]=ucRtc_Set[1][1];
					Seg_Buf[4]=ucRtc_Set[1][2];
					Seg_Buf[5]=ucRtc_Set[1][3];
					Seg_Buf[6]=ucRtc_Set[1][4];
					Seg_Buf[7]=ucRtc_Set[1][5];
				break;
				case 2://index为3
					Seg_Buf[2]=ucRtc_Set[2][0];
					Seg_Buf[3]=ucRtc_Set[2][1];
					Seg_Buf[4]=ucRtc_Set[2][2];
					Seg_Buf[5]=ucRtc_Set[2][3];
					Seg_Buf[6]=ucRtc_Set[2][4];
					Seg_Buf[7]=ucRtc_Set[2][5];
				break;
				
			}
		break;
	}
	//触发与记录功能实现
	if(Ut_Data<30)Ut_Flag=1;
		else Ut_Flag=0;
	if(Ut_Flag)
	{
		unsigned char i;
		for(i=0;i<3;i++)
		{
			ucRtc_Set[Mode3_Index][0]=ucRtc[0]/16;
			ucRtc_Set[Mode3_Index][1]=ucRtc[0]%16;
			ucRtc_Set[Mode3_Index][2]=ucRtc[1]/16;
			ucRtc_Set[Mode3_Index][3]=ucRtc[1]%16;
			ucRtc_Set[Mode3_Index][4]=ucRtc[2]/16;
			ucRtc_Set[Mode3_Index][5]=ucRtc[2]%16;
			if(++Mode3_Index==3)Mode3_Index=0;
		}
	}
}

/* 其他显示函数 */
void Led_Proc()
{
	unsigned char i;
	if(Ut_Data<30)
	{
		Beep(1);
		Beep_Flag=1;//置标志位
	}
	else 
	{
		Beep(0);            //报警功能实现
		Beep_Flag=0;
	}
	
	for(i=0;i<3;i++)
		ucLed[i]=(i==Disp_Mode);//互斥点亮
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


/* Main */
void main()
{
	System_Init();
	Timer0Init();
	Set_Rtc(ucRtc);//设置上电初始化时间
	ucRtc_Init();
	while (1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
	}
}