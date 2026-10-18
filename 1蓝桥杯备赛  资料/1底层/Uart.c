#include <Uart.h>

/* 串口初始化函数 */
void UartInit(void)		//9600bps@12.000MHz
{
	SCON = 0x50;		//8位数据,可变波特率
	AUXR |= 0x01;		//串口1选择定时器2为波特率发生器
	AUXR |= 0x04;		//定时器时钟1T模式
	T2L = 0xC7;		//设置定时初始值
	T2H = 0xFE;		//设置定时初始值
	AUXR |= 0x10;		//定时器2开始计时
	//串口1选择定时器2为波特率发生器 开ES和EA
	ES = 1;
	EA = 1;
}

/* 字节发送函数 */
void SendByte(unsigned char dat)
{
	SBUF=dat;//将dat数据赋给SBUF，将数据发送出去
	while(TI == 0);//等待数据发送
	TI = 0;//将发送标志位清零
}

/* 字符串发送函数 */
void Uart_Send_String(unsigned char *dat)
{
	while(*dat != '\0')//当字符不为空时，继续发送
		SendByte(*dat++);//发送后指针dat加1，指向下一个字节
}

/*
前面定义，注意长度
unsigned char Uart_Slow_Down;//串口减速专用变量
unsigned char Uart_Recv[10];//串口接收数据储存数组 默认10个字节 若接收数据较长 可更改最大字节数
unsigned char Uart_Recv_Index;//串口接收数组指针
unsigned char Uart_Send[10];//串口接收数据储存数组 默认10个字节 若发送数据较长 可更改最大字节数

扫描里也加上减速
	if(++Uart_Slow_Down == 200) Uart_Slow_Down = 0;//串口减速专用

  串口处理函数 
void Uart_Proc()
{
	if(Uart_Slow_Down) return;
	Uart_Slow_Down = 1;//串口减速程序	
	写程序！！！
}
中断服务函数里写
  串口1中断服务函数 
void Uart1Server() interrupt 4
{
	if(RI == 1) //串口接收数据
	{
		Uart_Recv[Uart_Recv_Index] = SBUF;
		Uart_Recv_Index++;
		RI = 0;
	}
}
*/