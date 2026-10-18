#include <Uart.h>

/* 串口初始化函数 */
void UartInit(void)		//9600bps@12.000MHz
{
	SCON = 0x50;		//8位数据,可变波特率
	AUXR |= 0x01;		//串口1选择定时器2为波特率发生器
	AUXR &= 0xFB;		//定时器2时钟为Fosc/12,即12T
	T2L = 0xE6;		//设定定时初值  65526-256=65270 (9600波特率)
	T2H = 0xFF;		//设定定时初值
	AUXR |= 0x10;		//启动定时器2
	ES = 1;             //使能串口中断
	EA = 1;             //开启总中断
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
