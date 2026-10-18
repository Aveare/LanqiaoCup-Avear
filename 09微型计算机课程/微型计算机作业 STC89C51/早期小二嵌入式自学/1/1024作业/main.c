#include "reg51.h"
unsigned int s[4];
unsigned char arr[]={0xC0,0xF9,0xA4,0xB0,0x99,0x92,0x82,0xF8,0x80,0x90};
unsigned char wei[]={0x01,0x02,0x04,0x08};
unsigned int num=0;
unsigned char i=0;
void delay(unsigned int ms)
{
    unsigned int i, j;
    for(i = 0; i < ms; i++)
        for(j = 0; j < 120; j++);
}



unsigned char get_digit(unsigned int n, unsigned char pos)
{
    switch(pos)
    {
        case 0: return n / 1000 % 10;
        case 1: return n / 100 % 10;
        case 2: return n / 10 % 10;
        case 3: return n % 10;
        default: return 0;
    }
}

void main()
{
    while(1)
    {
        for(i = 0; i < 4; i++)
        {
            P0 = arr[get_digit(num, i)];  //利用get_digit函数得到各个位
            P2 = wei[i];                  
            delay(15);                    
            P2 = 0x00;  //消除屏幕闪烁                 
        }
        num++; 
    }
}
