#include <STC15F2K60S2.H>
#include <intrins.h>
sbit Tx=P1^0;
sbit Rx=P1^1;

void Delay12us(void)	//@12.000MHz
{
	unsigned char data i;

	_nop_();
	_nop_();
	i = 33;
	while (--i);
}


void cs_wave_init()
{
	unsigned char i;
	for(i=0;i<8;i++)
	{
		Tx=1;
		Delay12us();
		Tx=0;
		Delay12us();
	}
}


unsigned char cs_wave_data()
{
	unsigned int time;
	TMOD&=0x0f;
	TH1=TL1=0;
	cs_wave_init();
	TR1=1;
	while((TF1==0)&&(Rx==1));
	TR1=0;
	if(TF1==0)
	{
		time=TH1<<8|TL1;
		return time*0.017;
	}
	else 
	{
		TF1=0;
		return 0;
	}
}










