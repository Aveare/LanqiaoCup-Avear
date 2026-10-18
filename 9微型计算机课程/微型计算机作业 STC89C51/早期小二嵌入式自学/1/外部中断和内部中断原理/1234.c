#include "reg51.h"
sbit ex=P3^2;
unsigned char s[]={0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};
unsigned char num=0;
void initex()
{
	IT0=1;
	EX0=1;
	EA=1;
	ex=1;
}

display()
{
  P2=s[num];
  if(num==10)
  {
   num=0;
  }
}


void main()
{
 initex();
 while(1)
 	{
	 display();
	}
}

void ex_isr() interrupt 0
{
 num++;
}