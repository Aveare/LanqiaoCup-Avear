#include "kbt.h"

unsigned char GetIoState()
{
	unsigned char keyio = 0xff;
	P3 = 0xf0; P4 = 0xff;
	if(P44 == 0)keyio = 0x70;
	if(P42 == 0)keyio = 0xb0;
	if(P35 == 0)keyio = 0xd0;
	
	P3 = 0x1f;P4 = 0x00;
	if(P33 == 0)keyio |= 0x07;
	if(P32 == 0)keyio |= 0x0b;
	if(P31 == 0)keyio |= 0x0d;
	if(P30 == 0)keyio |= 0x0e;
	
	return keyio;
}

unsigned char Ktrg = 0;
unsigned char Kcont = 0;

void KbtThree()
{
	unsigned char readdata = GetIoState()^0xff;
	Ktrg = readdata&(readdata^Kcont);
	Kcont = readdata;
}