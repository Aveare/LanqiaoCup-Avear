#include "stc15f2k60s2.h"

double WaveRcvd();
void Timer1_Init(void);
sbit Tx = P1^0;
sbit Rx = P1^1;