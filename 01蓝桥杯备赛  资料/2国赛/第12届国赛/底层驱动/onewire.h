#ifndef _ONEWIRE_H_
#define _ONEWIRE_H_

#include <STC15F2K60S2.H>

sbit DQ = P1^4;
void Delay_OneWire(unsigned int t) ;
void Write_DS18B20(unsigned char dat);
unsigned char Read_DS18B20(void);
bit init_ds18b20(void);

#endif
