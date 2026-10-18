#include <Led.h>

/*Led显示*/
void Led_Disp(unsigned char addr,enable)
{
    static unsigned char temp = 0x00;
    if(enable) temp |= 0x01 << addr;
	else temp &=~(0x01 << addr);
    P0 = ~temp;P2 = 0x80;P2 = 0x00;
}
/*蜂鸣器*/
void Beep(bit enable)
{
    static unsigned char temp = 0x00;
    if(enable)temp |= 0x40;
    else temp &= ~0x40;
    P0 = temp;P2 = 0xa0;P2 = 0x00;
}
/*电机驱动方波生成PWM*/ 
void MOTOR(bit enable)
{
    static unsigned char temp = 0x00;
    if (enable)temp |= 0x20;
    else temp &= ~0x20;
	P0 = temp; P2 = 0xa0;P2 = 0x00;
}
/*继电器*/
void Relay(bit enable)
{
    static unsigned char temp = 0x00;
    if(enable)temp |= 0x10;
    else temp &= ~0x10;
	P0 = temp;P2 = 0xa0;P2 = 0x00;
}
