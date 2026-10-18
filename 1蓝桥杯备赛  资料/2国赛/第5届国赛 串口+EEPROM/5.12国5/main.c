#include <STC15F2K60S2.H>
#include <onewire.h>
#include <ds1302.h>
#include <iic.h>
#include <stdio.h>
#include <string.h>
/*底层变量*/
unsigned char Key_Val,Key_Slow,Key_Down,Key_Up,Key_Old;
unsigned int Seg_Slow;
unsigned char Seg_Pos;
unsigned char Seg_Buf[8]={10,10,10,10,10,10,10,10};
unsigned char Seg_Point[8]={0,0,0,0,0,0,0,0};
unsigned char ucLed[8]={0,0,0,0,0,0,0,0};
unsigned char code seg_wei[8]={0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
unsigned char code seg_table[]={0xC0,0xF9,0xA4,0xB0,0x99,0x92,0x82,0xF8,0x80,0x90,0xff,0xbf,0xC6,0x89};

/*底层函数声明*/
void Init();
unsigned char Key_Read();
void Seg_Disp(unsigned char wei,unsigned char table,bit point);
void Led_Disp(unsigned char addr,bit enable);
void UartInit(void);
extern char putchar (char ch);
/*变量*/
unsigned char ucRtc[3]={23,59,55};
float temperature;//温度
unsigned char Disp_Mode=0;//显示模式 0-温湿度 1-时间 2-停留时间
bit Work_Mode=0;//设备工作模式 0-自动传输模式 1-自动记录模式
unsigned char Humid;//湿度
unsigned char Lex;//光亮
bit Near_Flag=0;//靠近标志位
unsigned int Timer1s;
bit Star_Flag=0;
unsigned char Stay_Time;//停留时间
bit Stay_Time_Flag;//停留时间标志位
unsigned int Stay_Time_Ms;
bit Timer1_Flag;
unsigned int Timer1;
bit Record_Flag;//开始记录标志位
bit Time_Flag;//ms-s赋值标志位
/*Uart*/
unsigned char Uart_Rec[10];
unsigned char Uart_Rec_Index=0;
unsigned char Sys_Tick=0;
bit Rec_Flag=0;
bit Send_Flag;
/*EEPROM*/
unsigned char EEPROM_String[5][6];//储存
unsigned char Record_Index=0;//存储 最多5组
unsigned char EEPROM_Send[5][6];//读取要发送的数据
/*Key*/
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
		case 4://设备工作模式切换
			Work_Mode^=1;
		break;
		case 5://显示模式切换
			Disp_Mode=(Disp_Mode+1)%3;//0-2循环
		break;
	}
}
/*Seg*/
void Seg_Proc()
{
	unsigned char i,j;
//	if(Seg_Slow)return;
//	Seg_Slow=1;
	//重要信息读取
	switch(Seg_Slow)
	{
		case 100:
			Seg_Slow+=1;
			Read_Rtc(ucRtc);
			temperature=read_t();
		break;
		case 200:
			Seg_Slow+=1;
			Humid=AD_Read(0x41)/51.0*20*0.99;
		break;
		case 300:
			Seg_Slow+=1;
			Lex=AD_Read(0x43);
			Near_Flag=(Lex<50);
		if(Work_Mode)
		{
			if(Near_Flag)
			{
				Record_Flag=1;//靠近
				Stay_Time_Flag=1;//开始计时停留时间
				EEPROM_String[Record_Index][2]=(unsigned char)ucRtc[0];
				EEPROM_String[Record_Index][3]=(unsigned char)ucRtc[1];
				EEPROM_String[Record_Index][4]=(unsigned char)ucRtc[2];
			}
			else if(Record_Flag)//远离，并且上一状态为靠近
			{
				Stay_Time_Flag=Record_Flag=0;
				EEPROM_String[Record_Index][0]=(unsigned char)temperature;
				EEPROM_String[Record_Index][1]=(unsigned char)Humid;
				if(++Record_Index==5)Record_Index=0;//0-4循环 只存5组数据
				for(i=0;i<6;i++)
				{
					for(j=0;j<5;j++)EEPROM_Write(&EEPROM_String[j][i],(8*i),1);
				}
				
			}
		}
		break;
	}
	switch(Disp_Mode)
	{
		case 0://温湿度界面
			Seg_Buf[0]=(unsigned char)temperature/10%10;
			Seg_Buf[1]=(unsigned char)temperature%10;
			Seg_Buf[2]=12;//C
			Seg_Buf[3]=Seg_Buf[4]=10;
			Seg_Buf[5]=Humid/10%10;
			Seg_Buf[6]=Humid%10;
			Seg_Buf[7]=13;//H
		break;
		case 1://时钟界面
			Seg_Buf[0]=ucRtc[0]/10%10;
			Seg_Buf[1]=ucRtc[0]%10;
			Seg_Buf[2]=Star_Flag?11:10;//-or灭
			Seg_Buf[3]=ucRtc[1]/10%10;
			Seg_Buf[4]=ucRtc[1]%10;
			Seg_Buf[5]=Star_Flag?11:10;//-or灭
			Seg_Buf[6]=ucRtc[2]/10%10;
			Seg_Buf[7]=ucRtc[2]%10;
		break;
		case 2://靠近时间界面
			Seg_Buf[0]=Seg_Buf[1]=Seg_Buf[2]=10;
			Seg_Buf[3]=11;//-
			Seg_Buf[4]=Stay_Time/1000%10;
			Seg_Buf[5]=Stay_Time/100%10;
			Seg_Buf[6]=Stay_Time/10%10;
			Seg_Buf[7]=Stay_Time%10;
		break;
	}
}
/*Other*/
void Led_Proc()
{
	ucLed[0]=!Work_Mode;
	ucLed[1]=Work_Mode;
	ucLed[2]=Near_Flag;
	ucLed[3]=ucLed[4]=ucLed[5]=ucLed[6]=ucLed[7]=0;
}
/*Uart*/
void Uart_Proc()
{
	if(Uart_Rec_Index==0)return;
	if(Sys_Tick>10)
	{
		Rec_Flag=Sys_Tick=0;
		if(strcmp(Uart_Rec,"AAASSS")==0)Send_Flag=1;//判断AAASSS
		memset(Uart_Rec,0,Uart_Rec_Index);
		Uart_Rec_Index=0;
	}
}
/*SendData*/
void Send_Proc()
{	
	int i,j;
	if(Send_Flag)//收到AAASSS
	{
		if(!Work_Mode)//自动传输模式下
		{
			Timer1_Flag=1;//1s发送一次
			if(Timer1>1000)
			{
				printf("{%02d-%02bu%%}{%02bu-%02bu-%02bu}{%d}\n",
				(int)temperature,Humid,
				ucRtc[0],ucRtc[1],ucRtc[2],
				(int)Near_Flag);
				Timer1_Flag=Timer1=0;
			}
		}
		else//自动记录模式
		{
			for(i=0;i<5;i++)
			{
				for(j=0;j<6;j++)EEPROM_Send[i][j]=EEPROM_String[i][j];
				printf("{%02bu-%02bu%%}{%02bu-%02bu-%02bu}{%bu}\n",EEPROM_Send[i][0],EEPROM_Send[i][1],EEPROM_Send[i][2],EEPROM_Send[i][3],EEPROM_Send[i][4],EEPROM_Send[i][5]);
			}
			Send_Flag=0;
		}
	}
}
/*Timer0Init*/
void Timer0Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0x7F;		//定时器时钟12T模式
	TMOD &= 0xF0;		//设置定时器模式
	TL0 = 0x18;		//设置定时初值
	TH0 = 0xFC;		//设置定时初值
	TF0 = 0;		//清除TF0标志
	TR0 = 1;		//定时器0开始计时
	ET0=1;
	EA=1;
}
/*Timer0server*/
void Timer0server(void) interrupt 1
{
	if(++Key_Slow==10)Key_Slow=0;
	if(++Seg_Slow==400)Seg_Slow=0;
	if(++Seg_Pos==8)Seg_Pos=0;
	Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],Seg_Point[Seg_Pos]);
	Led_Disp(Seg_Pos,ucLed[Seg_Pos]);
	//超时解析
	if(Rec_Flag)Sys_Tick++;
	//闪烁
	if(++Timer1s==1000)
	{
		Star_Flag^=1;
		Timer1s=0;
	}
	//停留时间
	if(Stay_Time_Flag)
	{
		Stay_Time_Ms++;
		Time_Flag=1;
	}
	else if(Time_Flag)
	{
		Stay_Time=Stay_Time_Ms/1000;
		EEPROM_String[Record_Index][5]=(unsigned char)Stay_Time;
		Stay_Time_Ms=0;
		Time_Flag=0;
	}
	//1s发送
	if(Timer1_Flag)
	{
		Timer1++;
		if(Timer1==1000)Timer1=1001;
	}
}
/*Uart1server*/
void uart1server (void) interrupt 4
{
    if(RI)
    {
      Rec_Flag=1;
			Sys_Tick=0;
			Uart_Rec[Uart_Rec_Index]=SBUF;
			Uart_Rec_Index++;
			RI=0;
    }
		if(Uart_Rec_Index>=10)Uart_Rec_Index=0;
}
/*Main*/
void main()
{
	unsigned char i,j;
	Init();
	Timer0Init();
	UartInit();
	Set_Rtc(ucRtc);
	for(i=0;i<5;i++)
	{
		for(j=0;j<6;j++)EEPROM_Read(&EEPROM_String[i][j],(8*i),1);
	}
	while(1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
		Uart_Proc();
		Send_Proc();
	}
}
void Init()
{
	P2=0x80;P0=0xff;P2=0x00;
	P2=0xa0;P0=0x00;P2=0x00;
}
unsigned char Key_Read()
{
	unsigned char temp=0;
	if(P33==0)temp=4;
	if(P32==0)temp=5;
	return temp;
}
void Seg_Disp(unsigned char wei,unsigned char table,bit point)
{
	P2=0xe0;P0=0xff;P2=0x00;
	P2=0xc0;P0=seg_wei[wei];P2=0x00;
	P2=0xe0;P0=seg_table[table];
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
void UartInit(void)		//1200bps@12.000MHz
{
	SCON = 0x50;		//8位数据,可变波特率
	AUXR |= 0x40;		//定时器1时钟为Fosc,即1T
	AUXR &= 0xFE;		//串口1选择定时器1为波特率发生器
	TMOD &= 0x0F;		//设定定时器1为16位自动重装方式
	TL1 = 0x3C;		//设定定时初值
	TH1 = 0xF6;		//设定定时初值
	ET1 = 0;		//禁止定时器1中断
	TR1 = 1;		//启动定时器1
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
