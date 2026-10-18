#include "reg51.h"
unsigned char s[]={0x39,0x5b,0x3f,0x6f};//C209
unsigned char num;
void delay(unsigned int ms);

void display()
{ 
	unsigned char num;  
    for(num = 0; ; num++)  
    {
		num %= 4; 
		P2=0x01<<num;//选择			
        P0 = ~s[num];//显示
        delay(10);//视觉暂留10ms为静态显示，1s以上为动态显示
		}
}
	
void delay(unsigned int ms)
{
    unsigned int i, j;
    for(i = 0; i < ms; i++)
        for(j = 0; j < 120; j++);
}


void main()
{
	
	while(1)
	{
		display();
	}
}