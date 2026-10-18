#ifndef __IIC_H
#define __IIC_H


void IIC_Start(void);

void IIC_Stop(void);
void IIC_SendAck(bit ackbit);
bit IIC_WaitAck(void);
void IIC_SendByte(unsigned char byt);
unsigned char IIC_RecByte(void);
unsigned char Ad_Read(unsigned char addr);
void Da_Write(unsigned char addr);

#endif



