#include <Uart.h>



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
