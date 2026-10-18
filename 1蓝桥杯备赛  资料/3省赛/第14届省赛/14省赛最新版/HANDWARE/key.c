#include "key.h"
bit key_up=1;
uint key_time;
uchar flag1;
uchar key_scan()
{
	uchar key;
	P44=P42=1;
	P32=P33=0;
	if(key_up==1&&(P44==0||P42==0))
	{
		key_up=0;
		if(P44==0) key=1;
		else if(P42==0)
		{
			flag1=1;
			key=2;
		}
		P44=P42=0;
	    P32=P33=1;
		if(P32==0||P33==0)
		{
		  if(P32==0) 
			{
				if(flag1==1)
					flag1=2;
				return key+8;
				
			}
		else if(P33==0) return key+12;
		}
	}
	else if(P44==1&&P42==1)
	{
		key_up=1;
		if(flag1==2&&key_time>=2000)
		{
			flag1=0;
			key_time=0;
			return 40;
		}
		//也可以这样写，但触发界面不同，没必要
//		else if(flag1==2&&key_time<=2000)
//		{
//			flag1=0;
//			key_time=0;
//			return 10;
//		}
		flag1=0;
		key_time=0;
	}
	return 0;
	
	
}