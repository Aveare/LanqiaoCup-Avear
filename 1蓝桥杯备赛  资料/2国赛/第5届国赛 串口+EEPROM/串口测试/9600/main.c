/* ͷ�ļ������� */
#include <STC15F2K60S2.H>
#include <Init.h>
#include <Led.h>
#include "onewire.h"
#include <Uart.h>
#include <stdio.h>
#include <string.h>

unsigned char Seg_Pos;
unsigned char ucLed[8] = {0,0,0,0,0,0,0,0};

unsigned char Uart_Slow_Down;
unsigned char Uart_Recv[10];
unsigned char Uart_Recv_Index;
unsigned char Uart_Send[30];

unsigned int Timer1000ms;
float Temp;

void Seg_Proc()
{
	Temp=read_t();
}

void Led_Proc()
{
	
}


void Uart_Proc()
{
	if(Uart_Slow_Down)return;
	Uart_Slow_Down=1;

	if (Uart_Recv_Index > 0)
    {
        if(strcmp(Uart_Recv,"AAASSS")==0)
		{
			sprintf(Uart_Send,"temperature:%.3f\r\n",Temp);
			Uart_Send_String(Uart_Send);
		}
        Uart_Recv_Index = 0;
    }
}

void Timer0Init(void)		
{
	AUXR &= 0x7F;		
	TMOD &= 0xF0;		
	TL0 = 0x18;		
	TH0 = 0xFC;		
	TF0 = 0;		
	TR0 = 1;		
	ET0 = 1;    
	EA = 1;     
}

void Timer0Server() interrupt 1
{  
	if(++Uart_Slow_Down == 8) Uart_Slow_Down = 0;
    if(++Seg_Pos == 8) Seg_Pos = 0;
	Led_Disp(Seg_Pos,ucLed[Seg_Pos]);
	
	if(++Timer1000ms==1000)
	{
		Timer1000ms=0;
		
	Uart_Send_String("Uart Init OK\r\n");
	}
}

void Uart1Server(void) interrupt 4
{
	if(RI == 1)
	{
		Uart_Recv[Uart_Recv_Index]=SBUF;
		Uart_Recv_Index++;
		// 添加字符串终止符，确保strcmp能正常工作
		if(Uart_Recv_Index < 10)
			Uart_Recv[Uart_Recv_Index] = '\0';
		RI=0;
	}
}

/* Main */
void main()
{
	System_Init();
	Timer0Init();
	UartInit();

	// 清空串口接收缓冲区
	memset(Uart_Recv, 0, Uart_Recv_Index);
	Uart_Recv_Index = 0;

	while (1)
	{
		Seg_Proc();
		Led_Proc();
		Uart_Proc();
	}
}