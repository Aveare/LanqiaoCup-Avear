#include <STC15F2K60S2.H>
#include <intrins.h>
sbit DQ=P1^4;
void Delay24us(void)	//@12.000MHz
{
	unsigned char data i;

	_nop_();
	_nop_();
	i = 69;
	while (--i);
}

//
void Delay_OneWire(unsigned int t)  
{
	unsigned char i;
	while(t--){
		for(i=0;i<12;i++);
	}
}

//
void Write_DS18B20(unsigned char dat)
{
	unsigned char i;
	for(i=0;i<8;i++)
	{
		DQ = 0;
		DQ = dat&0x01;
		Delay_OneWire(5);
		DQ = 1;
		dat >>= 1;
	}
	Delay_OneWire(5);
}

//
unsigned char Read_DS18B20(void)
{
	unsigned char i;
	unsigned char dat;
  
	for(i=0;i<8;i++)
	{
		DQ = 0;
		dat >>= 1;
		DQ = 1;
		if(DQ)
		{
			dat |= 0x80;
		}	    
		Delay_OneWire(5);
	}
	return dat;
}

//
bit init_ds18b20(void)
{
  	bit initflag = 0;
  	
  	DQ = 1;
  	Delay_OneWire(12);
  	DQ = 0;
  	Delay_OneWire(80);
  	DQ = 1;
  	Delay_OneWire(10); 
    initflag = DQ;     
  	Delay_OneWire(5);
  
  	return initflag;
}

float read_tempera()
{
	unsigned char tml;
	unsigned char tmh;
	init_ds18b20();
	Write_DS18B20(0xcc);
	Write_DS18B20(0x44);
	Delay24us();
	init_ds18b20();
	Write_DS18B20(0xcc);
	Write_DS18B20(0xbe);
	tml=Read_DS18B20();
	tmh=Read_DS18B20();
	return ((tmh<<8)|tml)/16.0;
}




