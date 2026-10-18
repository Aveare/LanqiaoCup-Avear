#ifndef _DS1302_H_
#define _DS1302_H_

#include "public.h"
unsigned char Read_Ds1302_Byte ( unsigned char address );
void Write_DS1320_Init(uchar hour,uchar min,uchar sec);
#endif
