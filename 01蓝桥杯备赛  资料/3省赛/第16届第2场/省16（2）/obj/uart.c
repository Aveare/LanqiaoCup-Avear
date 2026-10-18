#include <STC15F2K60S2.H>
#include <stdio.h>

//unsigned char Uart_Buf[20];
//unsigned char Uart_Rx_Index;

//void Uart1_Init(void) // 9600bps@12MHz
//{
//    SCON = 0x50;  // 8位数据,可变波特率
//    AUXR |= 0x01; // 串口1选择定时器2为波特率发生器
//    AUXR |= 0x04; // 定时器时钟1T模式
//    T2L = 0xC7;   // 设置定时初始值
//    T2H = 0xFE;   // 设置定时初始值
//    AUXR |= 0x10; // 定时器2开始计时
//    ES = 1;       // 打开串口中断
//    EA = 1;       // 打开总中断
//}


//void uart_send(unsigned char*arr)
//{
//	while(*arr!='\0')
//	{
//		SBUF=*arr;
//		while(TI==0);
//		TI=0;
//		arr++;
//	}
//}


//void uart_work()	interrupt 4
//{
//	if(RI)
//	{
//		Uart_Buf[Uart_Rx_Index]=SBUF;
//		Uart_Rx_Index++;
//		RI=0;
//	}
//}

//extern char putchar(char ch)
//{
//	SBUF=ch;
//	while(TI==0);
//	TI=0;
//	return ch;
//}

