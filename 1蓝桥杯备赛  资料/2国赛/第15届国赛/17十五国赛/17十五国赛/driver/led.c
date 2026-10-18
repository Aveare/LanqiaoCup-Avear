#include <led.H>

void led_disp(unsigned char wei,enable)
{
	static unsigned char temp=0x00;
	static unsigned char temp_old=0xff;
	unsigned char temp_i;
	if(enable)
		temp|=(0x01<<wei);
	else
		temp&=~(0x01<<wei);
	if(temp!=temp_old)
	{
		P0=~temp;
		temp_i=P2&0x1f;
		temp_i=temp_i|0x80;
		P2=temp_i;
		temp_i=P2&0x1f;
		P2=temp_i;
		temp_old=temp;
	}
}

void relay(unsigned char flat)
{
	static unsigned char temp=0x00;
	static unsigned char temp_old=0xff;
	unsigned char temp_i;
	if(flat)
		temp|=0x10;
	else
		temp&=~(0x10);
	if(temp!=temp_old)
	{
		P0=temp;
		temp_i=P2&0x1f;
		temp_i=temp_i|0xa0;
		P2=temp_i;
		temp_i=P2&0x1f;
		P2=temp_i;
		temp_old=temp;
	}
}
