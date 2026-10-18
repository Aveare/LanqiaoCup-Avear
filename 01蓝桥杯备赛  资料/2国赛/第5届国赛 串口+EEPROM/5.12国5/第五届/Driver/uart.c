#include "uart.h"

void UartInit(void)		//4800bps@12.000MHz
{
	SCON = 0x50;		//8位数据,可变波特率
	AUXR |= 0x01;		//串口1选择定时器2为波特率发生器
	AUXR |= 0x04;		//定时器时钟1T模式
	T2L = 0x8F;		//设置定时初始值
	T2H = 0xFD;		//设置定时初始值
	AUXR |= 0x10;		//定时器2开始计时
	ES = 1;
}

void Uart_Send(unsigned char *Uart_String)
{
	while(*Uart_String != '\0')
	{
		SBUF = *Uart_String;
		while(TI == 0);
		TI = 0;
		Uart_String++;
	}
}