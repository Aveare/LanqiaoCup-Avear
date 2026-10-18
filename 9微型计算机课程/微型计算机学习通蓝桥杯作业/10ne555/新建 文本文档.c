#include <STC15F2K60S2.H>
#include <absacc.h>

void selectHC(unsigned char n);
void show(unsigned char w,unsigned char n);
void delay_ms(int n);
void Init_Timer();
void show_F();
unsigned char table[]={0xc0,0xf9,0xa4,0xb0,0x99,0x92,0x82,0xf8,0x80,0x90,0xbf};//0xbf为数码管显示 ——

unsigned int f=0;//计算频率
unsigned int dat_f=0;//存放1s的频率
unsigned char count=0;//计算时间

void main()
{
	selectHC(4);P0=0XFF;P2&=0X1F; //关闭LED
	selectHC(5);P0=0X00;P2&=0X1F; //关闭蜂鸣器，继电器
	Init_Timer();
	while(1)
	{
		show_F();
  }
}

//数码管显示频率值
void show_F()
{
	show(0,0x8e); //显示F
	delay_ms(1);
	
	show(1,0xff); 
	delay_ms(1);
	
	show(2,0xff); 
	delay_ms(1);
	
	if(dat_f>9999)
	{
		show(3,table[dat_f/10000]); 
		delay_ms(1);
  }
	
	if(dat_f>999)
	{
		show(4,table[dat_f/1000%10]); 
		delay_ms(1);
  }
	
	if(dat_f>99)
	{
		show(5,table[dat_f/100%10]); 
		delay_ms(1);
  }
	
	if(dat_f>9)
	{
		show(6,table[dat_f/10%10]); 
		delay_ms(1);
  }
	
	show(7,table[dat_f/1%10]); 
	delay_ms(1);
}

void service_T0() interrupt 1
{
	f++;  //累加1s的频率
}

void service_T1() interrupt 3
{
	TH1=(65536-5000)/256;
	TL1=(65536-5000)%256;
	count++;
	if(count==20) //20*50ms，1s时间到取出这一秒的频率数，放到dat中
	{
		dat_f=f;
		f=0;
		count=0;
  }
}

//计数器，定时器初始化函数
void Init_Timer()
{
	//定时器0用作计数，8位重装方式
	TH0=0XFF;
	TL0=0XFF;
	
	//定时器1用作定时，定时时间是50ms
	TH1=(65536-5000)/256;
	TL1=(65536-5000)%256;
	
	TMOD=0x16;  //0001 0110  高四位控制T1，低四位控制T0
	
	ET0=1;
	ET1=1;
	EA=1;
	TR0=1;
	TR1=1;
}


//寄存器选择
void selectHC(unsigned char n)
{
	switch(n)
	{
		case 4: P2=(P2&0X1f)|0x80; break;
		case 5: P2=(P2&0X1f)|0xa0; break;
		case 6: P2=(P2&0X1f)|0xc0; break;
		case 7: P2=(P2&0X1f)|0xe0; break;
	}
}

//第w个数码管显示n,n为对应16进制
void show(unsigned char w,unsigned char n)
{
	selectHC(6);
	P0=0X01<<w;
	selectHC(7);
	P0=n;
	delay_ms(5);
	P0=0xff;
	P2&=0x1f;
}

//延时函数
void delay_ms(int n)
{
	int i,j;
	for(i=n;i>0;i--)
		for(j=110;j>0;j--);
}