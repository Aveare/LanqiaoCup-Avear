#include "device.h"

void DeviceCtrl(unsigned char p2data,unsigned char p0data)
{
	P0 = p0data;
	P2 = (P2&0x1f) | p2data;
	P2 &= 0x1f;
}