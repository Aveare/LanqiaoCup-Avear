#include "public.h"

sbit TX = P1^0;
sbit RX = P1^1;

void Timer2Init(void)		//1毫秒@12.000MHz
{
	AUXR |= 0x04;		//定时器时钟1T模式
	T2L = 0x20;		//设置定时初始值
	T2H = 0xD1;		//设置定时初始值
	AUXR |= 0x10;		//定时器2开始计时
	IE2 |= 0x04;
	EA = 1;
}


uchar Read_Key(void)
{
	static uchar last_trg = 0,cnt = 0;
	uchar trg = 0,cur = 0,value = 3,key_x = 0,key_y = 0;
	
	P3 = 0x0f;P4 = 0x00;
	if(!P30) key_x = 3;
	else if(!P31) key_x = 2;
	else if(!P32) key_x = 1;
	else if(!P33) key_x = 0;
	
	P3 = 0xf0;P4 = 0xff;
	if(!P34) key_y = 4;
	else if(!P35) key_y = 3;
	else if(!P42) key_y = 2;
	else if(!P44) key_y = 1;
	
	cur = key_y ^ 0x00;
	trg = cur^cnt&cur;
	cnt = cur;
	if((last_trg^trg&last_trg) && cur)
	{
		value = key_x + key_y * 4;
	}
	last_trg = trg;
	return value;
}

void Delay12us()		//@12.000MHz
{
	unsigned char i;

	_nop_();
	_nop_();
	i = 33;
	while (--i);
}


void Send_Wave(void)
{
	uchar i = 0;
	for(i = 0;i<8;i++)
	{
		TX = 0;
		Delay12us();
		TX = 1;
		Delay12us();
	}
}


uint Get_Distance(void)
{
	uint dis = 0;
	TMOD &= 0xf0;
	TH0 = TL0 = 0x00;
	Send_Wave();
	TR0 = 1;
	while((RX == 1) &&(TF0 == 0));
	TR0 = 0;
	if(TF0 == 0)
	{
		dis = (TH0 << 8)|TL0;
		dis = dis * 0.017 +0.5;
	}
	else
	{
		TF0 = 0;
		dis = 999;
	}
	return dis;
}