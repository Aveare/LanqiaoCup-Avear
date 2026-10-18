/* 头文件声明区 */
#include "key.h" /* 键盘专用 */
#include "init.h" /* 初始化专用 */
#include "seg.h" /* 数码管专用 */
#include "onewire.h" /* 温度专用 */
#include "iic.h" /* 单总线专用 */
#include "ds1302.h" /* 时钟专用 */
#include "uart.h" /* 串口专用 */
#include "stdio.h" /* 字符串专用 */
#include "led.h"	/* LED专用 */

/* 变量声明区 */
xdata unsigned long int Ms_Tick;//系统计时器
xdata unsigned char Key_Slow_Down;//键盘减速变量 10ms
xdata unsigned int Seg_Slow_Down;//信息处理减速变量 500ms
xdata unsigned int Uart_Slow_Down;//串口减速变量 500ms
xdata unsigned char Data_Buf[6][5];//数据读取数组
xdata unsigned char EEPROM_Data_Buf[6][5];//eeprom储存数据数组
xdata unsigned char ucLed[8] = {0};//LED储存数组
unsigned char Key_Down,Key_Old,Key_Val;//键盘减速变量
unsigned char Seg_Pos;//数码管扫描变量
unsigned char Seg_Buf[8] = {11,11,11,11,11,11,11,11};//数码管显示数据储存数组
unsigned char Seg_Disp_Mode;//数码管显示模式 0-温度湿度信息 1-时间信息 2-停留时间
unsigned char Rb2_Dat;//实时电压值
unsigned char Rb1_Dat;//实时光敏值
unsigned char ucRtc[3] = {0x23,0x59,0x55};//时钟数据存放数组
unsigned char Time;//计时器
unsigned char Dwell_Tm;//物体停留时间
unsigned char UartData_Buf[7];//串口缓存数组
unsigned char UartData_Buf_Index;//缓存数组索引
unsigned char EEPROM_Data_Buf_Index;//eeprom储存数据数组索引
unsigned char Uart_Data[24];//串口信息发送数据
float Temperature;//实时温度值
bit Works_Mode;//工作模式 0-自动传输模式 1-自动记录模式
bit Seg_Star_Flag;//数码管闪烁标志位
bit Bright_Dark_Flag;//环境亮暗标志位

/* 键盘扫描函数 */
void Key_Proc()
{
	if(Key_Slow_Down) return;
	Key_Slow_Down = 1;//键盘减速程序
	Key_Val = Key_Read();//实时读取键码值
	Key_Down = Key_Val & (Key_Val ^ Key_Old);//捕捉下降沿跳变
	Key_Old = Key_Val;//辅助扫描
	if(Key_Down == 5)//数码管显示状态切换
	{
		if(++Seg_Disp_Mode == 3)
			Seg_Disp_Mode = 0;
	}
	if(Key_Down == 4)//工作模式切换
		Works_Mode ^= 1;
}

/* 信息处理函数 */
void Seg_Proc()
{
	unsigned char i,j;//用于循环
	if(Seg_Slow_Down) return;
	Seg_Slow_Down = 1;//信息处理减速程序	 
	/* 信息采集 */
	Temperature = Rd_Temperature();//温度
	Rb2_Dat = Ad_Read(0x41);//电压
	Rb1_Dat = Ad_Read(0x43);//光敏
	Read_Rtc(ucRtc);//时钟
	Seg_Star_Flag = ucRtc[2] % 2;
	if(Rb1_Dat <= 100)//第一次有物体停留
	{
		/* EEPROM数据储存 */
		if(Works_Mode == 1 && Bright_Dark_Flag == 0)
		{
			for(i=0;i<3;i++)
				EEPROM_Data_Buf[2+i][EEPROM_Data_Buf_Index] = ucRtc[i]/16*10 + ucRtc[i]%16;
		}
		Bright_Dark_Flag = 1;
	}
	else if(Bright_Dark_Flag == 1)//物体离开
	{
		Bright_Dark_Flag = 0;
		Dwell_Tm = Time;
		/* EEPROM数据储存 */
		if(Works_Mode == 1)
		{
			EEPROM_Data_Buf[0][EEPROM_Data_Buf_Index] = Temperature;//温度值
			EEPROM_Data_Buf[1][EEPROM_Data_Buf_Index] = (unsigned char)(Rb2_Dat / 2.57);//湿度值
			EEPROM_Data_Buf[5][EEPROM_Data_Buf_Index] = Dwell_Tm;//停留时间
			if(++EEPROM_Data_Buf_Index == 5)
				EEPROM_Data_Buf_Index = 0;
//			for(i=0;i<5;i++)
//			{
//				for(j=0;j<6;j++)
//				EEPROM_Write(EEPROM_Data_Buf[j][i],(8*i);
//			}
		}
		Time = 0;
	}
	/* 数据处理 */
	switch(Seg_Disp_Mode)
	{
		case 0://温度湿度
			Seg_Buf[3] = Seg_Buf[4] = 11;
			Seg_Buf[0] = (unsigned char)Temperature / 10 % 10;
			Seg_Buf[1] = (unsigned char)Temperature % 10;
			Seg_Buf[2] = 12;
			Seg_Buf[5] = (unsigned char)(Rb2_Dat / 2.57) / 10 % 10;
			Seg_Buf[6] = (unsigned char)(Rb2_Dat / 2.57) % 10;
			Seg_Buf[7] = 13;
		break;
		case 1://时钟
			for(i=0;i<3;i++)
			{
				Seg_Buf[i*3] = ucRtc[i] / 16;
				Seg_Buf[i*3+1] = ucRtc[i] % 16;
			}
			Seg_Buf[2] = Seg_Buf[5] = Seg_Star_Flag?10:11;
		break;
		case 2://停留时间
			for(i=0;i<3;i++)
				Seg_Buf[i] = 11;
			Seg_Buf[3] = 10;
			Seg_Buf[4] = Dwell_Tm / 1000 % 10;
			Seg_Buf[5] = Dwell_Tm / 100 % 10;
			Seg_Buf[6] = Dwell_Tm / 10 % 10;
			Seg_Buf[7] = Dwell_Tm % 10;
		break;
	}
}

/* 其他显示函数 */
void Led_Proc()
{
	ucLed[0] = !Works_Mode;
	ucLed[1] = Works_Mode;
	ucLed[2] = Bright_Dark_Flag;
}

/* 串口信息函数 */
void Uart_Proc()
{
	unsigned char i,j;//用于循环
	if(Uart_Slow_Down) return;
	Uart_Slow_Down = 1;//串口减速程序
	if(UartData_Buf[0] == 'A' && UartData_Buf[1] == 'A' && UartData_Buf[2] == 'A' && UartData_Buf[3] == 'S' && UartData_Buf[4] == 'S' && UartData_Buf[5] == 'S')
	{
		if(Works_Mode == 0)//自动传输模式
			sprintf(Uart_Data,"{%02d-%02d%}{%02d-%02d-%02d}{%d}\r\n",(unsigned int)Temperature,(unsigned int)(Rb2_Dat / 2.57),(unsigned int)(ucRtc[0]/16*10+ucRtc[0]%16),(unsigned int)(ucRtc[1]/16*10+ucRtc[1]%16),(unsigned int)(ucRtc[2]/16*10+ucRtc[2]%16),(unsigned int)Bright_Dark_Flag);
		else//自动记录模式
		{
			for(i=0;i<6;i++)
			{
				for(j=0;j<5;j++)
//					Data_Buf[j][i] = EEPROM_Read(8*i);
					Data_Buf[i][j] = EEPROM_Data_Buf[i][j];
			}
			if(UartData_Buf_Index == 6)
			{
				for(i=0;i<5;i++)
				{
					sprintf(Uart_Data,"{%02d-%02d%}{%02d-%02d-%02d}{%d}\r\n",(unsigned int)Data_Buf[0][i],(unsigned int)Data_Buf[1][i],(unsigned int)Data_Buf[2][i],(unsigned int)Data_Buf[3][i],(unsigned int)Data_Buf[4][i],(unsigned int)Data_Buf[5][i]);
					Uart_Send(Uart_Data);
				}
				UartData_Buf_Index = 0;
			}
		}
	}
}

/* 定时器1中断初始化函数 */
void Timer1Init(void)		//1毫秒@12.000MHz
{
	TMOD &= 0x0F;		//设置定时器模式
	TL1 = 0x18;		//设置定时初始值
	TH1 = 0xFC;		//设置定时初始值
	TF1 = 0;		//清除TF1标志
	TR1 = 1;		//定时器1开始计时
	ET1 = 1;		//定时器1中断打开
	EA = 1;			//总中断打开
}

/* 定时器1中断服务函数 */
void Timer1Server() interrupt 3
{
	Ms_Tick++;
	/* 减速相关 */
	if(++Key_Slow_Down == 10) Key_Slow_Down = 0;
	if(++Seg_Slow_Down == 500) Seg_Slow_Down = 0;
	if(++Uart_Slow_Down == 500) Uart_Slow_Down = 0;
	/* 数码管相关 */
	if(++Seg_Pos == 8) Seg_Pos = 0;
	Seg_Disp(Seg_Buf[Seg_Pos],Seg_Pos);
	/* LED相关 */
	Led_Disp(Seg_Pos,ucLed[Seg_Pos]);
	/* 停留时间计时 */
	if(!(Ms_Tick % 1000))
	{
		if(Bright_Dark_Flag == 1)//开始计时
			Time++;
		if(Works_Mode == 0)//自动传输模式
		{
			Uart_Send(Uart_Data);
			UartData_Buf_Index = 0;
		}
	}
}

/* 串口服务函数 */
void UartServer() interrupt 4
{
	if(RI)
	{
		RI = 0;
		UartData_Buf[UartData_Buf_Index] = SBUF;
		UartData_Buf_Index++;
	}
}

/* Main */
void main()
{
	Cls_Init();//关闭外设
	Set_Rtc(ucRtc);//设置时钟
	UartInit();//串口初始化
	Timer1Init();//定时器1初始化
	while(1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
		Uart_Proc();
	}
}