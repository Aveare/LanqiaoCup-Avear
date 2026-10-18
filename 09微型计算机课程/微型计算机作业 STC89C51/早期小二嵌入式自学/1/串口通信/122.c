#include "reg51.h"

unsigned char recdat=0,flag=0;

void initson()
{
 SCON=0x50;  //0101 0000     
 TMOD=0x20;		 //0010 0000
 TH1=256-3;
 TL1=256-3;
 ES=1;
 EA=1;
 TR1=1;
}

void senddat()
{
	SBUF=recdat;
	while(!TI);
	TI=0;
}


void main()

{
  initson();
  while(1)
  	{
		if(flag==1)
		{
		senddat();
		}
	}
}

void scon_isr() interrupt 4
{
   recdat=SBUF;
	RI=0;
	flag=1;
}