/* 头文件声明区 */
#include <STC15F2K60S2.H>
#include <Init.h>
#include <Led.h>
#include "onewire.h"
#include <Uart.h>
#include <stdio.h>
/* 变量声明区 */
unsigned char Seg_Pos;
unsigned char ucLed[8] = {0,0,0,0,0,0,0,0};//Led显示数据存放数组
unsigned char Uart_Slow_Down;//串口减速专用变量
unsigned char Uart_Recv[10];//串口接受数组储存数据
unsigned char Uart_Recv_Index;//串口接收数组指针
unsigned char Uart_Send[30];//串口发送数组储存数组
bit L1_Flag,L2_Flag,L3_Flag,L4_Flag,L5_Flag,L6_Flag,L7_Flag,L8_Flag;
bit Uart_Send_Flag;
unsigned int Timer1000ms;
float Temp;//温度变量
/* 信息处理函数 */
void Seg_Proc()
{
	Temp=read_t();
}
/* 其他显示函数 */
void Led_Proc()
{
	ucLed[0]=L1_Flag;
	ucLed[1]=L2_Flag;
	ucLed[2]=L3_Flag;
	ucLed[3]=L4_Flag;
	ucLed[4]=L5_Flag;
	ucLed[5]=L6_Flag;
	ucLed[6]=L7_Flag;
	ucLed[7]=L8_Flag;
}

/*串口处理函数*/
void Uart_Proc()
{
	unsigned char i;
	if (Uart_Recv_Index > 0)
    {
        for(i=0;i<Uart_Recv_Index;i++)
        {
            switch(Uart_Recv[i])
            {
                case '1': L1_Flag = 1; break;
                case '2': L2_Flag = 1; break;
                case '3': L3_Flag = 1; break;
                case '4': L4_Flag = 1; break;
                case '5': L5_Flag = 1; break;
                case '6': L6_Flag = 1; break;
                case '7': L7_Flag = 1; break;
                case '8': L8_Flag = 1; break;
                default: break;
            }
        }
        Uart_Recv_Index = 0; // 处理完复位指针
    }
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
    if(++Seg_Pos == 8) Seg_Pos = 0;//数码管显示专用
	Led_Disp(Seg_Pos,ucLed[Seg_Pos]);
	
	if(++Timer1000ms==1000)
	{
		Timer1000ms=0;
		sprintf(Uart_Send,"temperature:%.3f\r\n",Temp);
		Uart_Send_String(Uart_Send);
	}
}

/*串口1中断服务函数*/
void Uart1Server(void) interrupt 4
{
	if(RI == 1)//串口接受数据
	{
		Uart_Recv[Uart_Recv_Index]=SBUF;
		Uart_Recv_Index++;
		RI=0;
	}
}

/* Main */
void main()
{
	System_Init();
	Timer0Init();
	UartInit();
	while (1)
	{
		Seg_Proc();
		Led_Proc();
		Uart_Proc();
	}
}