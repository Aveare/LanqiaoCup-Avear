#include "reg51.h"

void delay(unsigned int ms)
{
    unsigned int i, j;
    for(i = 0; i < ms; i++)
        for(j = 0; j < 120; j++);
}

int readkey()
{
	int p,key_value=0xff;
	P1=0xf7;
	p=P1;
	if(p!=0xf7)
	{
		delay(100);
		p=P1;
		if(p!=0xf7)switch(p)
		{
			case 0xe7: key_value=10; break;
			case 0xd7: key_value=1; break;
			case 0xb7: key_value=4; break;
			case 0x77: key_value=7; break;
		}
	}
	
	P1=0xfb;
	p=P1;
	if(p!=0xfb)
	{
		delay(100);
		p=P1;
		if(p!=0xfb)switch(p)
		{
			case 0xeb: key_value=0; break;
			case 0xdb: key_value=2; break;
			case 0xbb: key_value=5; break;
			case 0x7b: key_value=8; break;
		}
	}
	
	P1=0xfd;
	p=P1;
	if(p!=0xfd)
	{
		delay(100);
		p=P1;
		if(p!=0xfd)switch(p)
		{
			case 0xed: key_value=11; break;
			case 0xdd: key_value=3; break;
			case 0xbd: key_value=6; break;
			case 0x7d: key_value=9; break;
		}
	}
	
	P1=0xfe;
	p=P1;
	if(p!=0xfe)
	{
		delay(100);
		p=P1;
		if(p!=0xfe)switch(p)
		{
			case 0xee: key_value=15; break;
			case 0xde: key_value=14; break;
			case 0xbe: key_value=13; break;
			case 0x7e: key_value=12; break;
		}
	}
	return key_value;
}
	
void main()
{
	int key;
	long i,j;
	int t=1;
	int shuzi;
	int num[]={0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f,0x77,0x7c,0x58,0x5e,0x79,0x71};
	int show[8];
	for(i=0;i<8;i++)show[i]=0x00;
	j=1;
	while(1)
	{
		key=readkey();
		if(key!=0xff)
		{
			for(i=0;i<7;i++)show[i]=show[i+1];
			show[7]=num[key];
			key=0xff;
		}
		j=0;
		t=1;
		while(j<8)
		{
			P3=0xff-t;
			P2=show[j++];
			i=150;
			while(i--);
			t=t*2;
		}
	}
}