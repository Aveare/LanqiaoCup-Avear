#include<reg51.h>
sbit key0=P1^0;
unsigned char arr[]={0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};
unsigned char num=0,flag=0;
void delay(unsigned char n)
{
    unsigned char i = 0, j = 0; 
    for(i = 0; i < n; i++)     
    {
        for(j = 0; j < 120; j++);
    }
}


void key()
{
	if(key0==0&&flag==0)
	{
	flag=1;
	}	
	if(flag==1&&key0==1)
	{
	num++;
	flag=0;
	}					
}


void seg()
{
 	P2=arr[num];
	if(num==10)
	{
	num=0;
	}
}
 void main()
 {
 while(1)
 	{
 	key();
	seg();
	}
 }