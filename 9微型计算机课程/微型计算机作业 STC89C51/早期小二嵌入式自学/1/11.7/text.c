#include "regx51.h"
#include "intrins.h"


unsigned char DUANMA[]=
{0xC0,0xF9,0xA4,0xB0,0x99,0x92,0x82,0xF8,0x80,0x90,0xff};//0-9的数字
unsigned char wei[]={0x01,0x02,0x04,0x08};
unsigned int led_flag=0x01,count=0,num=30;
unsigned int i,n,led_pos=1;

void delay(unsigned int ms)
{
    unsigned int i, j;
    for(i = 0; i < ms; i++)
        for(j = 0; j < 120; j++);
}

	
void inittimer(void)  //初始化定时器
{
	TMOD=0x01;	 //0000 0001   定时器0开启，16位定时器
	TH0=(65536-50000)/256;	 //高8位
	TL0=(65536-50000)%256;	 //低8位
	ET0=1;	//开启定时器0的中断
	EA=1;	//开启总中断
	TR0=1;	//启动定时器0
}

void timer0_isr() interrupt 1
{
	TH0=(65535-50000)/256;
	TL0=(65535-50000)%256;
	count++;
	if(count>=20)
	{
		count=0;
		num--;
		if (num == 0) num = 0; 
	}	
		
	if(count==10)
	{
		 if (num<=30&&num>20) 
			 {
				;	
			 } 
		 else if (num<20 && num>10) 
			 {
				led_pos = (led_pos + 1) % 8; // 0~7循环
			 } 
		 else if (num >= 0 && num <= 10) 
			 {
			if (count % 10 == 0) 
				{
				led_pos = (led_pos + 2) % 8; // 每次+2，实现1、3、5、7循环
				}
			}
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


void led()
{
	if(num<=30&&num>20)
	{
		
		 P1 = (count < 10) ? 0xFF : 0x00;
		
//		P0=0xFF;
//		if(num%2==1)
//		{
//			P0=~P0;
//		}
	}
	
	else if(num<20 && num>10)
	{
			P1 = ~(0x01 << led_pos); // 左移定位，取反使对应位亮
//		led_flag=0x01;
//		led_flag=_crol_(led_flag,1);
	}
	
	if(num>0&&num<11)
	{
		P1 = ~(0x01 << led_pos);
//		led_flag=0x01;
//		led_flag=_crol_(led_flag,2);
	}
	else
	{
	P1=0x00;
	}
}


void main()
{
	inittimer();
	while(1)
	{
	led();
		
		// 在主循环的数码管扫描中，前两位显示熄灭（段码0xff）
for(i = 0; i < 4; i++)
{
    if(i < 2)  // 前两位不亮
        P0 = DUANMA[10];  // 10对应0xff（熄灭）
    else        // 后两位显示十位和个位
        P0 = DUANMA[get_digit(num, i)];
    P3 = wei[i];
    delay(1);  // 缩短延时，减少闪烁（建议移到定时器中断中扫描）
    P3 = 0x00; // 关闭位选，防止串扰（原代码P2=0x00无效，应操作位选口P3）
}
	
//	 for(i = 0; i < 4; i++)
//			{
//				P0 = DUANMA[get_digit(num, i)];
//				P3 = wei[i];
//				delay(16);                    
//				P2 = 0x00;
//			}

		
	}
}
