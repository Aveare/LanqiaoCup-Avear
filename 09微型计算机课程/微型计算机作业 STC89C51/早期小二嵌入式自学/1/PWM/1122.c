#include "reg51.h"
sbit led=P1^0;

main()
{
	int i,j;
	
	j=1000;   //0%¡¡
	while(j--)
	{
			
			led=0;
			i=100;
			while(i--);
	}
	
	j=1000;   //50%¡¡
	while(j--)
	{
			led=0;
			i=50;
			while(i--);
	
			led=1;
			i=50;
			while(i--);
	}
 
 j=1000;   //100%¡¡
	while(j--)
	{
			
			led=1;
			i=100;
			while(i--);
	}
}