#include "reg51.h"
unsigned char s[]={0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};
unsigned char num;
void initcounter()
{
 TMOD=0x06;
 TH0=256-3;	  //0000 0110
 TL0=256-3;
 ET0=1;
 TR0 = 1;
 EA=1;
}

void display()
{
 P2=s[num];
 	if(num==10)
	{
	num=0;
	}
}

void main()
{
 initcounter();
 while(1)
 	{
	display();
 	}
}

void counter_isr() interrupt 1
{
 num++;
}