#include "onewire.h"
#include "reg52.h"

sbit DQ = P1^4;  


//单总线内部延时函数
void Delay_OneWire(unsigned int t)  
{
	//t *= 12;//仿真tick和真实tick不同，硬件上需要t*=12
	while(t--);
}

//单总线写操作
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

//单总线读操作
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
			dat |= 0x80;//默认先读高位
		}	    
		Delay_OneWire(5);
	}
	return dat;
}

//DS18B20初始化
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

float read_t()
{
	unsigned char low,high;//返回的温度数据的高低八位
	init_ds18b20();//初始化
	Write_DS18B20(0xcc);//跳过ROM
	Write_DS18B20(0x44);//进行温度转换

	init_ds18b20();//初始化
	Write_DS18B20(0xcc);//跳过ROM
	Write_DS18B20(0xbe);//读取温度

	low = Read_DS18B20();//读取低位
	high = Read_DS18B20();//读取高位
	return ((high << 8) | low) / 16.0;//默认先输出低位再输出高位
}

/*温度精度修改*/ 
void Precision_Set(unsigned char dat)
{
	init_ds18b20();//初始化
	Write_DS18B20(0xcc);//跳过ROM
	Write_DS18B20(0x4e);//写暂存器指令4E
	Write_DS18B20(0x7f);//写高速缓存器TH高温限值
	Write_DS18B20(0x00);//写高速缓存器TL高温限值
	Write_DS18B20(dat);//写告诉缓存器精度      //9 位填0xFC（0.5）、10 位填0xFD（0.25）、11 位填0xFE（0.125）、12 位填0xFF（0.0625）
}