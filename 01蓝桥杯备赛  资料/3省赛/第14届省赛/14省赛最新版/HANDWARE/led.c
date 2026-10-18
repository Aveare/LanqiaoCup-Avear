#include "led.h"

void led(uchar *LED,uchar pos)
{
	static uchar temp=0xff;
	if(LED[pos])
		temp &=~(0x01<<pos);
	else
		temp |= 0x01<<pos;
	P0=temp;
	hc573(4);
	
	
	
}