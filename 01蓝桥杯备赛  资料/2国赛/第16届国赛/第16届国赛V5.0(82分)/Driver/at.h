#include "stc15f2k60s2.h"
#include "device.h"
#include "timer.h"
#include "smg.h"
#include "kbt.h"
#include "uart.h"
#include "onewire.h"
#include "sonic.h"
#include "ds1302.h"
#include "iic.h"
#include "ne555.h"
#include "math.h"
#include "string.h"

typedef struct
{
	unsigned char bit0:1;
	unsigned char bit1:1;
	unsigned char bit2:1;
	unsigned char bit3:1;
	unsigned char bit4:1;
	unsigned char bit5:1;
	unsigned char bit6:1;
	unsigned char bit7:1;
	unsigned char bit8:1;
} BitGroup;

typedef union
{
	unsigned char byte;
	BitGroup bits;
} ByteGroup;