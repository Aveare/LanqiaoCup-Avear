#ifndef __SYS_H
#define __SYS_H

#define uint unsigned int
#define uchar unsigned char
	
#include <STC15F2K60S2.H>
#include "intrins.h"
#include "stdio.h"

void hc573(uchar channel);
void sys_init();


#endif