#include <csb.H>

sbit tx=P1^0;
sbit rx=P1^1;

void Delay12us()		//@12.000MHz
{
	unsigned char i;

	_nop_();
	_nop_();
	i = 38;
	while (--i);
}

void init_csb()
{
	unsigned char i;
	for(i=0;i<8;i++)
	{
		tx=1;
		Delay12us();
		tx=0;
		Delay12us();
	}
}

unsigned char read_csb()
{
	unsigned int time;
	CMOD=0;
	CH=CL=0;
	CR=1;
	init_csb();
	while(rx==1&&CF==0);
	CR=0;
	if(CF==0)
	{
		time=(CH<<8)|CL;
		return (time*0.017);
	}
	else
	{
		CF=0;
		return 0;
	}
}
