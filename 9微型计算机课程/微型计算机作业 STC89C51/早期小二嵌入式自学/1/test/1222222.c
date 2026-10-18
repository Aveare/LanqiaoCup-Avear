#include "reg51.h"
#include "intrins.h"


unsigned char DUANMA[]=
{0xC0,0xF9,0xA4,0xB0,0x99,0x92,0x82,0xF8,0x80,0x90,0xff};//0-9的数字
unsigned char wei[]={0x01,0x02,0x04,0x08};
unsigned int led_flag=0x01,count=0,num=30;
unsigned int i;

unsigned char get_digit(unsigned int n, unsigned char pos)


void timer0_init()
{
   
    TMOD = 0x01; 
    TH0 = (65536 - 50000) / 256; 
    TL0 = (65536 - 50000) % 256;
    ET0 = 1;       // 允许T0中断
    EA = 1;        // 允许总中断
    TR0 = 1;       // 启动T0
}

void timer0_isr() interrupt 1
{
	TH0=(65535-50000)/256;
	TL0=(65535-50000)%256;
	count++;
	if(count==20)
	{
		num--;
	}
	
	if(num<=30&&num>20)
	{
		P0=0x00;
		if(num%2==1)
		{
			P0=~P0;
		}
	}
	
	if(num<=20&&num>10)
	{	
		led_flag=0x01;
		led_flag=_crol_(led_flag,1);
	}
	
	if(num>0&&num<11)
	{
		led_flag=0x01;
		led_flag=_crol_(led_flag,2);
	}
	

	for(i = 0; i < 4; i++)
	{
		P0 = ~DUANMA[get_digit(num, i)];  
		P3 = 0x01<<i;                                      
		P0 = 0x00;                  
	}
	

}

unsigned char get_digit(unsigned int n, unsigned char pos)
{
    switch(pos)
    {
        case 0: return n / 1000 % 10;
        case 1: return n / 100 % 10;
        case 2: return n / 10 % 10;
        case 3: return n % 10;
        default: return 0;
    }
}



void main()
{
	while(1)
	{
		P1=led_flag;
	}
}