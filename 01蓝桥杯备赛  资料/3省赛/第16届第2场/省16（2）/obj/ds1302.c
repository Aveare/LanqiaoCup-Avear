#include <STC15F2K60S2.H>
#include <intrins.h>
sbit SCK=P1^7;
sbit SDA=P2^3;
sbit RST=P1^3;
unsigned char write_addr[]={0x80,0x82,0x84};
unsigned char read_addr[]={0x81,0x83,0x85};
unsigned char Time[]={0x55,0x59,0x23};
//
void Write_Ds1302(unsigned  char temp) 
{
	unsigned char i;
	for (i=0;i<8;i++)     	
	{ 
		SCK = 0;
		SDA = temp&0x01;
		temp>>=1; 
		SCK=1;
	}
}   

//
void Write_Ds1302_Byte( unsigned char address,unsigned char dat )     
{
 	RST=0;	_nop_();
 	SCK=0;	_nop_();
 	RST=1; 	_nop_();  
 	Write_Ds1302(address);	
 	Write_Ds1302(dat);		
 	RST=0; 
}

//
unsigned char Read_Ds1302_Byte ( unsigned char address )
{
 	unsigned char i,temp=0x00;
 	RST=0;	_nop_();
 	SCK=0;	_nop_();
 	RST=1;	_nop_();
 	Write_Ds1302(address);
 	for (i=0;i<8;i++) 	
 	{		
		SCK=0;
		temp>>=1;	
 		if(SDA)
 		temp|=0x80;	
 		SCK=1;
	} 
 	RST=0;	_nop_();
 	SCK=0;	_nop_();
	SCK=1;	_nop_();
	SDA=0;	_nop_();
	SDA=1;	_nop_();
	return (temp);			
}


void time_set()
{
	unsigned char i;
	Write_Ds1302_Byte(0x8e,0x00);
	for(i=0;i<3;i++)
		Write_Ds1302_Byte(write_addr[i],Time[i]);
	Write_Ds1302_Byte(0x8e,0x80);
}

void time_read()
{
	unsigned char i;
	for(i=0;i<3;i++)
		Time[i]=Read_Ds1302_Byte(read_addr[i]);
}


