#include"reg51.h"
unsigned char s[]={0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};
unsigned char num=99;

void delay(unsigned int ms)
{
    unsigned int i, j;
    for(i = 0; i < ms; i++)
        for(j = 0; j < 120; j++);
}

void key_scan()
{
	unsigned char temp=0,temp0=0,temp1=0;
 	P1=0xF0;
 	if(P1!=0xF0)
 	{
	 delay(20);
	 temp0=P1;
	 P1=0x0f;
	 if(P1!=0x0f)
		{
		 temp1=P1;	 	
		}
	}
		   	temp=temp0+temp1;
			if(temp == 0xEE) num = 0;
            else if(temp == 0xED) num = 1;
            else if(temp == 0xEB) num = 2;
            else if(temp == 0xE7) num = 3;
}


void display()
{
 P2=s[num];
}

void main()
{
 while(1)
 	{
	 key_scan();
	 display();
	}

}