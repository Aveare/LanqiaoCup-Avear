#include <Ultra.h>
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
void Ultra_Init()
{
	unsigned char i;
	for(i=0;i<8;i++)
	{
		Tx=1;Delay12us();
		Tx=0;Delay12us();
	}
}

unsigned int Ut_Wave_Data(unsigned int Ut_Speed)
{
	unsigned long int time;
	unsigned int temp;
	CH=CL=0;
	CMOD=0x00;
	Ultra_Init();
	CR=1;
	while(Rx&&!CF);
	CR=0;
	if(!CF)
	{
		time=CH<<8|CL;
		temp=time*Ut_Speed/20000;
		return temp;
	}
	else
	{
		CF=0;
		return 99;
	}
}