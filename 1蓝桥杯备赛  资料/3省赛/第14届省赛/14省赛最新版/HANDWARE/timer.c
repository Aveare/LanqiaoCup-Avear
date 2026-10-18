#include "timer.h"
uint fre,sec;
uint tim;

void timer0_init()
{
	AUXR &= 0x7F;		
	TMOD &= 0xF0;	
  TMOD |= 0x05;	
	TL0 = 0x00;		
	TH0 = 0x00;		
	TF0 = 0;		
	TR0 = 1;		
	
}
void timer1_init()
{
	AUXR &= 0x7F;		
	TMOD &= 0x0F;	
  TMOD |= 0x05;	
	TL1 = 0x18;		
	TH1 = 0xFC;		
	TF1 = 0;		
	TR1 = 1;		
	ET1=1;
	EA=1;
}

void timer1() interrupt 3
{
	if(++sec==1000)
	{
		fre= (TH0<<8) |TL0;
		sec=0;
		TH0=TL0=0;
	}
	if(++tim==100)
	{
		state4 ^=1;
		tim=0;
		
	}
	if(trig==1)
		sec_trig++;
	if(flag1==2)
		key_time++;
	key_dely++;
	smg_dely++;
	dat_dely++;
	pos=(pos+1)%8;
	smg(temp,dot,pos);
	led(LED,pos);
	
	
	
}