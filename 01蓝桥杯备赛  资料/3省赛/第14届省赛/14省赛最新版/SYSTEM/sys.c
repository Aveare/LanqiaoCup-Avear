#include "sys.h"

void hc573(uchar channel)
{
	switch(channel)
	{
		case 4: P2 =P2& 0x1f |0x80;break;
		case 5: P2 =P2& 0x1f |0xa0;break;
		case 6: P2 =P2& 0x1f |0xc0;break;
		case 7: P2 =P2& 0x1f |0xe0;break;
		
	}
	P2 =P2& 0x1f;
}

void sys_init()
{
	P0=0x00;
	hc573(5);
	P0=0xff;
	hc573(4);
	P0=0x00;
	hc573(6);
	
	
	
}