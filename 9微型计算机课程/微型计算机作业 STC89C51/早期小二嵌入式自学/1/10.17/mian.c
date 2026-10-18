#include <reg52.h>
#include<intrins.h>
void delay(unsigned int ms)
{
    unsigned int i, j;
    for(i = 0; i < ms; i++)
        for(j = 0; j < 112; j++);  // 11.0592MHz¾§ÕñÏÂÔ¼1msÑÓ³Ù
}

void main()
 { 
 int i;
  P1=0xFE;
  delay(1000);
  for(i=0;i<8;i++)
  {
  P1=_crol_(P1,1);
  delay(1000);
  }
   
}