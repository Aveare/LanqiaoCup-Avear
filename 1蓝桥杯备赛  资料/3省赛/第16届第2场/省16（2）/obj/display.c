#include <STC15F2K60S2.H>
#include "sys.h"

code unsigned char duanma[]={
0xC0,
0xF9,
0xA4,
0xB0,
0x99,
0x92,
0x82,
0xF8,
0x80,
0x90,
0x88,//A	
0x83,//B
0xC6,//C
0xA1,//D
0x86,//E
0x8E,//F
0xff,//Ï¨Ãð
0x8c //P-17
};

void led(unsigned char*LED,unsigned char pos)
{
	static unsigned char temp=0xff;
	if(LED[pos])
		temp&=~(0x01<<pos);
	else
		temp|=0x01<<pos;
	P0=temp;
	hc573(4);
}

void smg(unsigned char*SMG,unsigned char*dot,unsigned char pos)
{
	P0=0xff;
	hc573(7);
	P0=0x01<<pos;
	hc573(6);
	if(dot[pos])
		P0=duanma[SMG[pos]]&0x7f;
	else
		P0=duanma[SMG[pos]];
	hc573(7);
}

void beep(unsigned char enable)
{
	unsigned char temp;
	if(enable==0)
		temp&=~(0x40);
	else
		temp|=0x40;
	P0=temp;
	hc573(5);
}

void relay(unsigned char enable)
{
	unsigned char temp;
	if(enable==0)
		temp&=~(0x10);
	else
		temp|=0x10;
	P0=temp;
	hc573(5);
}


