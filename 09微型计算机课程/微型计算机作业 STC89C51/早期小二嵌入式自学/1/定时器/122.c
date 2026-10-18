#include "reg51.h"
unsigned char s[]={0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};
unsigned char count=0;
unsigned char num=0;
void inittimer(void)  //初始化定时器
{
	TMOD=0x01;	 //0000 0001   定时器0开启，16位定时器
	TH0=(65536-50000)/256;	 //高8位
	TL0=(65536-50000)%256;	 //低8位
	ET0=1;	//开启定时器0的中断
	EA=1;	//开启总中断
	TR0=1;	//启动定时器0
}

void display()
{
 P2=s[num];
 	if(num==10)
	{
	 num=0;
	}
}

void main()
{
inittimer();
while(1)
	{
	 display();
	}
}

void timer_isr() interrupt 1	//0表示外部中断；1表示定时器中断0；2表示外部中断1；3表示定时器中断1；4表示串口中断
{
 	TH0=(65536-50000)/256;
	TL0=(65536-50000)%256;
	count++;
		if(count==20)
		{
			num++;
			count=0;
		}

}