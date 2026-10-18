#include <STC15F2K60S2.H>
#include <ut.h>
#include <intrins.h>
sbit Tx=P1^0;
sbit Rx=P1^1;
void Delay12us()		//@12.000MHz
{
	unsigned char i;

	_nop_();
	_nop_();
	i = 33;
	while (--i);
}
void Ut_Init()
{
	unsigned char i;
	for(i=0;i<8;i++)
	{
		Tx=1;Delay12us();
		Tx=0;Delay12us();
	}
}

unsigned char Ut_Wave_Data(unsigned char Speed_10x)
{
	unsigned int time;
	CH=CL=0;
	CMOD=0x00;
	Ut_Init();
	CR=1;
	while(Rx&&!CF);
	CR=0;
	if(!CF)
	{
		time=CH<<8|CL;
		return ((float)time/1000.0f*Speed_10x/200.0f);
	}
	else
	{
		CF=0;
		return 99;
	}
}