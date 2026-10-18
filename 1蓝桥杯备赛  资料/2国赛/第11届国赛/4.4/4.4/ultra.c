#include <ultra.h>
#include <intrins.h>
sbit Tx=P1^0;
sbit Rx=P1^1;
void Delay12us()		//@12.000MHz
{
	unsigned char i;
	_nop_();
	_nop_();
	i = 38;
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

unsigned char Ut_Wave_Data()
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
		CH=CL=0;
		return time*0.017;
	}
	else
	{
		CF=0;
		return 0;
	}
}