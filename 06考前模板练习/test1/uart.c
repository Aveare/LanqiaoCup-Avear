#include "uart.h"
void Uart1_Init(void)  // 9600bps@12.000MHz
{
	SCON = 0x50;		//8位数据,可变波特率
	AUXR |= 0x01;		//串口1选择定时器2为波特率发生器
	AUXR &= 0xFB;		//定时器2时钟为Fosc/12,即12T
	T2L = 0xE6;		//设定定时初值
	T2H = 0xFF;		//设定定时初值
	AUXR |= 0x10;		//启动定时器2
//isp生成后加ES EA 为1
  ES = 1;        // 使能串口1中断
  EA = 1;
}

extern char putchar (char ch)
{
	SBUF=ch;
	while(TI==0);
	TI=0;
	return ch;
}