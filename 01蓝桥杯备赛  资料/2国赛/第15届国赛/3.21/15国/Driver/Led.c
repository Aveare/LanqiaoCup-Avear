#include <Led.h>

static unsigned char temp1=0x00;
static unsigned char temp1_old=0xff;

void Led_Disp(unsigned char addr,enable)
{
	static unsigned char temp=0x00;
	static unsigned char temp_old=0xff;
	if(enable)
		temp|=0x01<<addr;
	else
		temp&=~(0x01<<addr);
	if(temp_old!=temp)
	{
		P0=~temp;
		P2=P2&0x1f|0x80;
		P2&=0x1f;
		temp_old=temp;
	}
}

void Beep(unsigned char Flag)
{
	if(Flag)
		temp1|=0x40;
	else
		temp1&=~0x40;
	if(temp1_old!=temp1)
	{
		P0=temp1;
		P2=P2&0x1f|0xa0;
		P2&=0x1f;
		temp1_old=temp1;
	}
}

void Relay(unsigned char Flag)
{
	if(Flag)
		temp1|=0x10;
	else
		temp1&=~0x10;
	if(temp1_old!=temp1)
	{
		P0=temp1;
		P2=P2&0x1f|0xa0;
		P2&=0x1f;
		temp1_old=temp1;
	}
}