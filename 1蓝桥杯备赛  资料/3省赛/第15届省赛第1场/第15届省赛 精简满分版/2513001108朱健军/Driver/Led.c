#include <Led.h>

void Led_Disp(unsigned char addr,enable)
{
	static unsigned char temp;
	static unsigned char temp_old;
	if(enable)temp|=0x01<<addr;
		else temp&=~(0x01<<addr);
	if(temp!=temp_old)
	{
		P0=~temp;
		P2=P2&=0x1f|0x80;
		P2&=0x1f;
		temp=temp_old;
	}
}