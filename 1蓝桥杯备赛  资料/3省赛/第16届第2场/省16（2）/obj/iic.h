#ifndef __IIC_H__
#define __IIC_H__
void at24c02_write(unsigned char addr,unsigned char dat);
unsigned char at24c02_read(unsigned char addr);
void pcf8591_write(unsigned char dat);
unsigned char pcf8591_read(unsigned char control);
#endif