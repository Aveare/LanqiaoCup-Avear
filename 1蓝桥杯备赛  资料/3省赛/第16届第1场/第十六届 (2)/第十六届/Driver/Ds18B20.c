#include "Ds18B20.h"
#include "REG52.H"

sbit DQ=P1^4;	

/*延时函数*/
void Delay_OneWire(unsigned int t)
{
	t*=12;
	while(t--);
}

/*写字节*/
void Wirte_Ds18B20(unsigned char dat)
{
	unsigned char i;
	for(i=0;i<8;i++)
	{
		DQ=0;
		DQ=dat&0x01;
		Delay_OneWire(5);
		DQ=1;
		dat>>=1;
	}
	Delay_OneWire(5);
}

/*读字节*/
unsigned char Read_Ds18B20(void)
{
	unsigned char i;
	unsigned char dat;
	for(i=0;i<8;i++)
	{
		DQ=0;
		dat>>=1;
		DQ=1;
		if(DQ)
		{
			dat|=0x80;
		}	
		Delay_OneWire(5);
	}
	

	return dat;
}


/*初始化*/
bit init_Ds18B20(void)
{
	bit initflag=0;
	
	DQ=1;
	Delay_OneWire(12);
	DQ=0;
	Delay_OneWire(80);
	DQ=1;
	Delay_OneWire(10);
	initflag=DQ;
	return initflag;
}

float Read_t()
{
	unsigned char low,high;
	init_Ds18B20();
	Wirte_Ds18B20(0xCC);
	Wirte_Ds18B20(0x44);
	
	init_Ds18B20();
	Wirte_Ds18B20(0xCC);
	Wirte_Ds18B20(0xBE);
	low=Read_Ds18B20();
	high=Read_Ds18B20();
	
	return ((high<<8)|low)/16.0;
}







