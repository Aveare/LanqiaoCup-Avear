#include "stc15f2k60s2.h"

extern unsigned char uart_buffer[30];
extern unsigned char uart_pos;

void SendString(unsigned char *str);
void Uart1_Init(void);	//9600bps@12.000MHz