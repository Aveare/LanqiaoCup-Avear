#include <STC15F2K60S2.H>
#include <onewire.h>
#include <ds1302.h>
#include <iic.h>
#include <uart.h>
#include <stdio.h>
#include <intrins.h>
#include <string.h>
/*底层变量声明*/
unsigned char Key_Slow;
unsigned int Seg_Slow;
unsigned char Seg_Pos;
unsigned char Key_Val,Key_Down,Key_Up,Key_Old;
unsigned char code seg_wei[8]={0x01,0x02,0x4,0x08,0x10,0x20,0x40,0x80};
unsigned char code seg_table[]={0xC0,0xF9,0xA4,0xB0,0x99,0x92,0x82,0xF8,0x80,0x90,0xff,0xbf};
unsigned char Seg_Buf[8]={10,10,10,10,10,10,10,10};
unsigned char Seg_Point[8]={0,0,0,0,0,0,0,0};
unsigned char ucLed[8]={0,0,0,0,0,0,0,0};
/*底层函数声明*/
void Init();
unsigned char Key_Read();
void Seg_Disp(unsigned char wei,unsigned char table,bit point);
void Led_Disp(unsigned char addr,bit enable);
void Beep(bit enable);
void Motor(bit enable);
void Relay(bit enable);
/*变量*/
unsigned int freq;
unsigned int Timer1s;

float temperature;

unsigned char ucRtc[3]={11,11,11};

unsigned char Lex;

unsigned char EEPROM_String[2];
unsigned char Count;

unsigned char distance;

/*串口数据*/
unsigned char Uart_Rec[10];
unsigned char Uart_Rec_Index;
bit Rec_Flag;
unsigned char Sys_Tick;
/*按键处理*/
void Key_Proc()
{
	if(Key_Slow)return;
	Key_Slow=1;
	Key_Val=Key_Read();
	Key_Down=Key_Val&(Key_Val^Key_Old);
	Key_Up=~Key_Val&(Key_Val^Key_Old);
	Key_Old=Key_Val;
	
}
/*信息处理*/
void Seg_Proc()
{
	if(Seg_Slow)return;
	Seg_Slow=1;
	temperature=read_t();
//	Seg_Buf[0]=(unsigned char)temperature>=10?(unsigned char)temperature/10%10:10;
//	Seg_Buf[1]=(unsigned char)temperature%10;
//	Seg_Buf[2]=(unsigned int)(temperature*10)%10;
	
//	Seg_Buf[3]=freq>=10000?freq/10000%10:10;
//	Seg_Buf[4]=freq>=1000?freq/1000%10:10;
//	Seg_Buf[5]=freq>=100?freq/100%10:10;
//	Seg_Buf[6]=freq>=10?freq/10%10:10;
//	Seg_Buf[7]=freq%10;
	
//	Read_Rtc(ucRtc);
//	Seg_Buf[0]=ucRtc[0]/10%10;
//	Seg_Buf[1]=ucRtc[0]%10;
//	Seg_Buf[2]=11;//-
//	Seg_Buf[3]=ucRtc[1]/10%10;
//	Seg_Buf[4]=ucRtc[1]%10;
//	Seg_Buf[5]=11;//-
//	Seg_Buf[6]=ucRtc[2]/10%10;
//	Seg_Buf[7]=ucRtc[2]%10;
	
//	Lex=AD_Read(0x41);
//	Seg_Buf[3]=Lex/10%10;
//	Seg_Buf[4]=Lex%10;

	distance=Ut_Wave_Data();
	Seg_Buf[5]=distance>=100?distance/100%10:10;
	Seg_Buf[6]=distance>=10?distance/10%10:10;
	Seg_Buf[7]=distance%10;
	
	
}
/*其他处理*/
void Led_Proc()
{
	ucLed[0]=1;
	Relay(1);
//	ucLed[1]=(Lex<=60);
//	DA_Write(255);
	
}
/*串口收发*/
void Uart_Proc()
{
	if(Uart_Rec_Index==0)return;
	if(Sys_Tick>=10)
	{
		Sys_Tick=Rec_Flag=0;
		
		printf("%d",(int)distance);
		
		
		memset(Uart_Rec,0,Uart_Rec_Index);
		Uart_Rec_Index=0;
	}
	
}
/*定时器0初始化函数*/
void Timer0Init(void)		//0毫秒@12.000MHz
{
	AUXR &= 0x7F;		//定时器时钟12T模式
	TMOD &= 0xF0;		//设置定时器模式
	TMOD|=0x05;
	TL0 = 0x00;		//设置定时初值
	TH0 = 0x00;		//设置定时初值
	TF0 = 0;		//清除TF0标志
	TR0 = 1;		//定时器0开始计时
}
/*定时器1初始化函数*/
void Timer1Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0xBF;		//定时器时钟12T模式
	TMOD &= 0x0F;		//设置定时器模式
	TL1 = 0x18;		//设置定时初值
	TH1 = 0xFC;		//设置定时初值
	TF1 = 0;		//清除TF1标志
	TR1 = 1;		//定时器1开始计时
	ET1=1;
	EA=1;
}
/*定时器1中断服务函数*/
void Timer1server(void) interrupt 3
{
	if(++Key_Slow==10)Key_Slow=0;
	if(++Seg_Slow==300)Seg_Slow=0;
	if(++Seg_Pos==8)Seg_Pos=0;
	Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],Seg_Point[Seg_Pos]);
	Led_Disp(Seg_Pos,ucLed[Seg_Pos]);
	//NE555
	if(++Timer1s>=1000)
	{
		Timer1s=0;
		freq=(TH0<<8|TL0);
		TH0=TL0=0;
	}
	//串口超时解析
	if(Rec_Flag)Sys_Tick++;
}
/*串口中断服务函数*/
void Uartserver(void) interrupt 4
{
	if(RI)
	{
		Rec_Flag=1;
		Sys_Tick=0;
		Uart_Rec[Uart_Rec_Index]=SBUF;
		Uart_Rec_Index++;
		RI=0;
	}
	if(Uart_Rec_Index>10)Uart_Rec_Index=0;
}
/*Main*/
void main()
{
	Init();
	Timer0Init();
	Timer1Init();
	UartInit();
	Set_Rtc(ucRtc);
	while(1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
		Uart_Proc();
	}
}
void Init()
{
	P2=0xa0;P0=0x00;P2=0x00;
	P2=0x80;P0=0xff;P2=0x00;
}
unsigned char Key_Read()
{
	unsigned char temp=0;
	P44=0;P42=1;P35=1;P34=1;
	if(P33==0)temp=4;
	if(P32==0)temp=5;
	if(P31==0)temp=6;
	if(P30==0)temp=7;
	P44=11;P42=0;P35=1;P34=1;
	if(P33==0)temp=8;
	if(P32==0)temp=9;
	if(P31==0)temp=10;
	if(P30==0)temp=11;
	P44=1;P42=1;P35=0;P34=1;
	if(P33==0)temp=12;
	if(P32==0)temp=13;
	if(P31==0)temp=14;
	if(P30==0)temp=15;
	P44=1;P42=1;P35=1;P34=0;
	if(P33==0)temp=16;
	if(P32==0)temp=17;
	if(P31==0)temp=18;
	if(P30==0)temp=19;
	return temp;
}
void Seg_Disp(unsigned char wei,unsigned char table,bit point)
{
	P2=0xe0;P0=0x00;P2=0x00;
	P2=0xc0;P0=seg_wei[wei];P2=0x00;
	P2=0xe0;
	P0=seg_table[table];
	if(point)P0&=~(1<<7);
	P2=0x00;
}
void Led_Disp(unsigned char addr,bit enable)
{
	unsigned char temp=0x00;
	if(enable)temp|=(0x01<<addr);
		else temp&=~(0x01<<addr);
	P2=0x80;P0=~temp;P2=0x00;
}
void Beep(bit enable)
{
	unsigned char temp=0x00;
	if(enable)temp|=0x40;
	else temp&=~0x40;
	P2=0xa0;P0=temp;P2=0x00;
}
void Motor(bit enable)
{
	unsigned char temp=0x00;
	if(enable)temp|=0x20;
	else temp&=~0x20;
	P2=0xa0;P0=temp;P2=0x00;
}
void Relay(bit enable)
{
	unsigned char temp=0x00;
	if(enable)temp|=0x10;
	else temp&=~0x10;
	P2=0xa0;P0=temp;P2=0x00;
}