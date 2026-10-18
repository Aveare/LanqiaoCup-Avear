#ifndef _IIC_H
#define _IIC_H

unsigned char Ad_Read(unsigned char addr);
void EEPROM_Write(unsigned int dat,addr);
unsigned char EEPROM_Read(unsigned int addr);

void IIC_Start(void); 
void IIC_Stop(void);  
bit IIC_WaitAck(void);  
void IIC_SendAck(bit ackbit); 
void IIC_SendByte(unsigned char byt); 
unsigned char IIC_RecByte(void); 

#endif