#include <STC15F2K60S2.H>
#include <intrins.h>
#include <ultra.h>
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
void Wave_Init()
{
	unsigned char i;
	for(i=0;i<8;i++)
	{
		Tx=1;Delay12us();
		Tx=0;Delay12us();
	}
}

unsigned char Wave_Data()
{
	unsigned int time;
	CMOD=0x00;
	CH=CL=0;
	Wave_Init();
	CR=1;
	while(Rx&&!CF);
	CR=0;
	if(!CF)
	{
		time=CH<<8|CL;
		return (time*0.017);
	}
	else
	{
		CF=0;
		return 0;
	}
}