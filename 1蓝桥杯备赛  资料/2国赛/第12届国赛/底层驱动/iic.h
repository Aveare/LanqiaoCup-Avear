#ifndef _IIC_H_
#define _IIC_H_

#include <STC15F2K60S2.H>
#include "intrins.h"

sbit sda = P2^1;
sbit scl = P2^0;
void I2C_Delay(unsigned char n);
void I2CStart(void);
void I2CSendByte(unsigned char byt);
void I2CStop(void);
unsigned char I2CWaitAck(void);
unsigned char I2CReceiveByte(void);
void I2CSendAck(unsigned char ackbit);

#endif
