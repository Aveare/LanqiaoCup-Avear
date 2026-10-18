#ifndef _PUBLIC_H_
#define _PUBLIC_H_
#include <STC15F2K60S2.H>
#include "intrins.h"

typedef unsigned char uchar;
typedef unsigned int uint;
#define Control_IO(x,y) P0 = y;P2 = x;P2 = 0

void Timer2Init(void);		//1∫¡√Î@12.000MHz
uchar Read_Key(void);
void Delay12us();		//@12.000MHz
uint Get_Distance(void);
#endif
