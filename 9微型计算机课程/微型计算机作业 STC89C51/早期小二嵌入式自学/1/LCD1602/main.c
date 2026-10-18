#include"reg51.h"
sbit RS=P3^0;
sbit RW=P3^1;
sbit E=P3^2;

unsigned char str[]={"hello world!"};
unsigned char i=0;
void delay(unsigned int t)
{
	unsigned int i=0,j=0;
	for(i=0;i<t;i++)
	{
		for(j=0;j<120;j++)
		{
		
		}
	}	
}


void writedat(unsigned char dat)
{
	RS=1;
	RW=0;
	E=0;
	P2=dat;
	delay(5);
	E=1;
	E=0;
	
}

void writecom(unsigned char com)
{
	RS=0;
	RW=0;
	E=0;
	P2=com;
	delay(5);
	E=1;
	E=0;
	
}

void initlcd()
{
	writecom(0x38);//设置16x2显示，5x7点阵，8位数据接口
	writecom(0x0c);//设置开显示，不显示光标
	writecom(0x06);//写一个字符后地址指针+1
	writecom(0x01);//显示清0，数值指针清0
}

void display()
{
	writecom(0x80+2+0x40);
	delay(5);
//	writedat('A');
//	delay(5);
//	writedat(0x31);
//	delay(5);
	while(str[i]!='\0')
	{
		writedat(str[i]);
		delay(5);
		i++;
	}
}

void main()
{
	initlcd();
	while(1)
	{
		display();
	}
}