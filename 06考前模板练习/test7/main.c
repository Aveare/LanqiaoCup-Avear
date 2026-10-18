#include <STC15F2K60S2.H>
#include <onewire.H>
#include <DS1302.H>
#include <iic.H>
#include <ultra.H>
#include <stdio.H>
#include <string.H>

/*底层变量*/
unsigned char Key_Slow,Key_Down,Key_Old,Key_Val,Key_Up;
unsigned int Seg_Slow;
unsigned char Seg_Pos;
unsigned char code seg_wei[8]={0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
unsigned char code seg_table[]={0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F,0x00,0x40};
unsigned char Seg_Buf[8]={10,10,10,10,10,10,10,10};
unsigned char Seg_Point[8]={0,0,0,0,0,0,0,0};
unsigned char ucLed[8]={0,0,0,0,0,0,0,0};

/*底层函数*/
void Init();
unsigned char Key_Read();
void Seg_Disp(unsigned char wei,unsigned char table,bit point);
void Led_Disp(unsigned char addr,enable);
void Beep(bit enable);
void Motor(bit enable);
void Relay(bit enable);
void UartInit(void);
/*变量*/
unsigned int freq;
unsigned char Timer200ms;
float temperature;
unsigned char ucRtc[3]={2,2,2};
unsigned char Lex,Vol;
unsigned char Key_Count=0;
unsigned int distance;
/*串口*/
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
	
	switch(Key_Down)
	{
		case 4:
		Key_Count++;
		EEPROM_Write(&Key_Count,0,1);
		break;
		case 5:
		Key_Count=0;
		EEPROM_Write(&Key_Count,0,1);
		break;
	}
}
/*数码管*/
void Seg_Proc()
{
	if(Seg_Slow)return;
	Seg_Slow=1;
	
	//NE555
//	Seg_Buf[0]=freq>=10000?freq/10000%10:10;
//	Seg_Buf[1]=freq>=1000?freq/1000%10:10;
//	Seg_Buf[2]=freq>=100?freq/100%10:10;
//	Seg_Buf[3]=freq>=10?freq/10%10:10;
//	Seg_Buf[4]=freq%10;
	//BS18B20
//		temperature=read_t();
//	Seg_Buf[0]=(unsigned char)temperature>=10?(unsigned char)temperature/10%10:10;
//	Seg_Buf[1]=(unsigned char)temperature%10;
//	Seg_Buf[2]=(unsigned int)(temperature*10)%10;
	//DS1302
//	Read_Rtc(ucRtc);
//	Seg_Buf[0]=ucRtc[0]/10%10;
//	Seg_Buf[1]=ucRtc[0]%10;
//	Seg_Buf[2]=11;
//	Seg_Buf[3]=ucRtc[1]/10%10;
//	Seg_Buf[4]=ucRtc[1]%10;
//	Seg_Buf[5]=11;
//	Seg_Buf[6]=ucRtc[2]/10%10;
//	Seg_Buf[7]=ucRtc[2]%10;
		
		//PCF8591
//	EA=0;
//	Lex=AD_Read(0x43);
//	Vol=AD_Read(0x41);//同时读交换顺序
//	EA=1;
//	Seg_Buf[0]=Lex/100%10;
//	Seg_Buf[1]=Lex/10%10;
//	Seg_Buf[2]=Lex%10;
//	Seg_Buf[3]=Vol/100%10;
//	Seg_Buf[4]=Vol/10%10;
//	Seg_Buf[5]=Vol%10;
		//DA_Write(255);
	//AT24c02
//	Seg_Buf[0]=Key_Count/10%10;
//	Seg_Buf[1]=Key_Count%10;
	//Ultra
//	distance=Ut_Wave_Data();
//	Seg_Buf[0]=distance/100%10;
//	Seg_Buf[1]=distance/10%10;
//	Seg_Buf[2]=distance%10;



}
/*Led*/
void Led_Proc()
{
	
}
void Uart_Proc()
{
	if(Uart_Rec_Index==0)return;
	if(Sys_Tick>=10)
	{
		Rec_Flag=Sys_Tick=0;
		
		printf("test");
		
		memset(Uart_Rec,0,Uart_Rec_Index);
		Uart_Rec_Index=0;
	}
}
/*定时器0初始化 NE555*/
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
/*定时器1初始化*/
void Timer1Init(void)		//0毫秒@12.000MHz
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
/*定时器1中断*/
void Timer1server(void) interrupt 3
{
	if(++Key_Slow==10)Key_Slow=0;
	if(++Seg_Slow==300)Seg_Slow=0;
	if(++Seg_Pos==8)Seg_Pos=0;
	Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],Seg_Point[Seg_Pos]);
	Led_Disp(Seg_Pos,ucLed[Seg_Pos]);
	//超时解析
	if(Rec_Flag)Sys_Tick++;
	
//	//NE555
//	if(++Timer200ms==200)
//	{
//		Timer200ms=0;
//		freq=TH0<<8|TL0;
//		freq*=5;
//		TH0=TL0=0;
//	}
}
void Uart_Isr(void) interrupt 4
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
	EEPROM_Read(&Key_Count,0,1);
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
	P2=0x80;P2=0xff;P2=0x00;
	P2=0xa0;P0=0x00;P2=0x00;
}
unsigned char Key_Read()
{
	unsigned char temp=0;
	P44=0;P42=1;P35=1;//P34=1;
	if(P33==0)temp=4;
	if(P32==0)temp=5;
	if(P31==0)temp=6;
	if(P30==0)temp=7;
	P44=1;P42=0;P35=1;//P34=1;
	if(P33==0)temp=8;
	if(P32==0)temp=9;
	if(P31==0)temp=10;
	if(P30==0)temp=11;
	P44=1;P42=1;P35=0;//P34=1;
	if(P33==0)temp=12;
	if(P32==0)temp=13;
	if(P31==0)temp=14;
	if(P30==0)temp=15;
	P44=1;P42=1;P35=1;//P34=0;
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
	P2=0xe0;P0=~seg_table[table];
	if(point)P0&=~(1<<7);
	P2=0x00;
}
void Led_Disp(unsigned char addr,enable)
{
	static unsigned char temp_led=0x00;
	if(enable)temp_led|=(0x01<<addr);
	else temp_led&=~(0x01<<addr);
	P2=0x80;P0=~temp_led;P2=0x00;
}
void Beep(bit enable)
{
	static unsigned char temp=0x00;
	static unsigned char temp_old=0xff;
	if(enable)temp|=0x40;
	else temp&=~0x40;
	if(temp!=temp_old)
	{
		P2=temp;
		P2=P2&0x1f|0xA0;
		P2&=0x1f;
		temp_old=temp;
	}
}
void Motor(bit enable)
{
	static unsigned char temp=0x00;
	static unsigned char temp_old=0xff;
	if(enable)temp|=0x20;
	else temp&=~0x20;
	if(temp!=temp_old)
	{
		P2=temp;
		P2=P2&0x1f|0xA0;
		P2&=0x1f;
		temp_old=temp;
	}
}
void Relay(bit enable)
{
	static unsigned char temp=0x00;
	static unsigned char temp_old=0xff;
	if(enable)temp|=0x10;
	else temp&=~0x10;
	if(temp!=temp_old)
	{
		P2=temp;
		P2=P2&0x1f|0xA0;
		P2&=0x1f;
		temp_old=temp;
	}
}

void UartInit(void)		//9600bps@12.000MHz
{
	SCON = 0x50;		//8位数据,可变波特率
	AUXR |= 0x01;		//串口1选择定时器2为波特率发生器
	AUXR &= 0xFB;		//定时器2时钟为Fosc/12,即12T
	T2L = 0xE6;		//设定定时初值
	T2H = 0xFF;		//设定定时初值
	AUXR |= 0x10;		//启动定时器2
	ES=1;
	EA=1;
}

extern char putchar (char ch)
{
	SBUF=ch;
	while(TI==0);
	TI=0;
	return ch;
}