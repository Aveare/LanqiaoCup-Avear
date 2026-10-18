#include <STC15F2K60S2.H>

unsigned char key_scan1()
{
	static unsigned char key_up=1;
	if(key_up==1&&(P30==0||P31==0||P32==0||P33==0))
	{
		key_up=0;
		if(P30==0)		return 7;
		else if(P31==0)	return 6;
		else if(P32==0)	return 5;
		else if(P33==0)	return 4;
	}
	else if(P30==1&&P31==1&&P32==1&&P33==1)
		key_up=1;
	return 0;
}

unsigned char key_scan2()
{
	static unsigned char key_up=1;
	unsigned char key;
	P44=P42=P35=P34=1;
	P30=P31=P32=P33=0;
	if(key_up==1)
	{
		if(P44==0)		key=1;
		else if(P42==0)	key=2;
		else if(P35==0)	key=3;
		else if(P34==0)	key=4;
		else	return 0;
		key_up=0;
		P44=P42=P35=P34=0;
		P30=P31=P32=P33=1;
		if(P30==0)		key=key;
		else if(P31==0)	key=key+4;
		else if(P32==0)	key=key+8;
		else if(P33==0)	key=key+12;
		return key;
	}
	else if(P44==1&&P42==1&&P35==1&&P34==1)
		key_up=1;
	return 0;
}









