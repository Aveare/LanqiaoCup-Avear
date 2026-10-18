#include "stc15f2k60s2.h"

sbit SCK = P1^7;
sbit SDA = P2^3;
sbit RST = P1^3;

void SetClock(unsigned char hour,unsigned char min,unsigned char sec);
unsigned char Read_Ds1302_Byte ( unsigned char address );

#define BcdToDec(bcd) (bcd/16*10 + bcd%16)
#define DecToBcd(dec) (dec/10*16 + dec%10)