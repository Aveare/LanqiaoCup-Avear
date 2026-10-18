#include "reg51.h"
unsigned char s[]={0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};
unsigned char LED [8] = {0xfe,0xfd,0xfb,0xf7,0xef,0xdf,0xbf,0x7f};
unsigned char count=0;
unsigned char num=0,temp=1,temp0=0;
unsigned char led_index;
void inittimer(void) 
{
	TMOD=0x01;	 
	TH0=(65536-50000)/256;	
	TL0=(65536-50000)%256;	 
	ET0=1;	
	EA=1;	
	TR0=1;
}

void display()
{
	unsigned char led_index = (temp - 1) % 8; 
	P2=s[num];
	if(temp < 30) 
    {
        led_index = (temp - 1) % 8; 
        P0 = LED[led_index];  
    }
    else  
    {
        P0 = 0xFF; 
    }

 	if(num==10)
	{
	 num=0;
	}


}


void timer_isr() interrupt 1	
{
 	TH0=(65536-50000)/256;
	TL0=(65536-50000)%256;
	count++;
		if(count==20)
		{
			num++;
			count=0;
		}
	
	temp0++;
		if(temp0==20)
		{
		temp++;
			temp0=0;
		}
}


void main()
{
inittimer();
while(1)
	{
	 display();
	}
}