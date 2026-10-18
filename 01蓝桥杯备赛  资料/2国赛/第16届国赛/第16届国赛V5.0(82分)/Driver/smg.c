#include "smg.h"

code unsigned char SmgCode[] = {0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F,0x00};
unsigned char SmgBuf[8];
unsigned char DisplayChars[20] = "        ";

void SmgScanTask()
{
	static unsigned char i = 0;
	
	DeviceCtrl(0xc0,0x00);
	DeviceCtrl(0xe0,~SmgBuf[i]);
	DeviceCtrl(0xc0,0x01<<i);
	
	i++;
	if(i>=8)
	{
		i = 0;
	}
}

void SmgUpdate()
{
	unsigned char i,j;
  unsigned char tcode = SmgCode[10];
	for(;i<=8;i++)
	{
		switch(DisplayChars[j])
		{
			case '0':tcode = SmgCode[0];break;
			case '1':tcode = SmgCode[1];break;
			case '2':tcode = SmgCode[2];break;
			case '3':tcode = SmgCode[3];break;
			case '4':tcode = SmgCode[4];break;
			case '5':tcode = SmgCode[5];break;
			case '6':tcode = SmgCode[6];break;
			case '7':tcode = SmgCode[7];break;
			case '8':tcode = SmgCode[8];break;
			case '9':tcode = SmgCode[9];break;
			case ' ':tcode = SmgCode[10];break;
			case '-':tcode = 0x40;break;
			case 'P':tcode = 0x73;break;
			case 'E':tcode = 0x79;break;
			default:break;
		}
		if(DisplayChars[j+1] == '.')
		{
			tcode |= 0x80;
			j+=2;
		}
		else
		{
			j++;
		}
		SmgBuf[i] = tcode;
	}
}
