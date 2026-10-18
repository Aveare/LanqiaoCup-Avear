#include <STC15F2K60S2.H>
#include <intrins.H>
#include <DS1302.H>

sbit SCK = P1^7;
sbit SDA = P2^3;
sbit RST = P1^3;

void DS302_Write(unsigned char temp)
{
	unsigned char i;
	for(i=0;i<8;i++)
	{
		SCK = 0;
		SDA = temp&0x01;
		temp>>=1;
		SCK = 1;
	}
}
void DS1302_Write_Byte(unsigned char position,unsigned char dat)//写入
{
	RST=0; _nop_();
	SCK=0; _nop_();
	RST=1; _nop_();
	DS302_Write(position);
	DS302_Write(dat);
	RST=0;
}
unsigned char DS1302_Read_Byte(unsigned char position)//读取
{
	unsigned char i,temp=0x00;
	RST=0; _nop_();
	SCK=0; _nop_();
	RST=1; _nop_();
	DS302_Write(position);
	
	for(i=0;i<8;i++)
	{
		SCK=0;
		temp>>=1;
		if(SDA)
			temp|=0x80;
		SCK=1;
	}
	RST=0; _nop_();
	SCK=0; _nop_();
	SCK=1; _nop_();
	SDA=0; _nop_();
	SDA=1; _nop_();
	
	return temp;
}



void Set_Rtc(unsigned char ucRtc[3])
{
	DS1302_Write_Byte(0x8e,0x00);//打开WP；WP拉低
	
	DS1302_Write_Byte(0x84,ucRtc[0]);//小时
	DS1302_Write_Byte(0x82,ucRtc[1]);
	DS1302_Write_Byte(0x80,ucRtc[2]);
	
	DS1302_Write_Byte(0x8e,0x80);
}
void Read_Rtc(unsigned char ucRtc[3])
{
	ucRtc[0]= DS1302_Read_Byte(0x85);//数组读取小时
	ucRtc[1]= DS1302_Read_Byte(0x83);//数组读取分钟
	ucRtc[2]= DS1302_Read_Byte(0x81);//数组读取秒钟
}

