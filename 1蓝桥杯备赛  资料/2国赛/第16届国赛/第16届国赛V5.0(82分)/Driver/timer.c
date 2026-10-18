#include "timer.h"

void PcaInit()
{
	CMOD = 0x01;
	CCON = 0x00;
	
	CH = (65535 - 1000)>>8;
	CL = (65535 - 1000);
	CR = 1;
}