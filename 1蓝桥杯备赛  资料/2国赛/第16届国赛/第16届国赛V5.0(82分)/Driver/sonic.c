#include "sonic.h"
#include "onewire.h"

void Timer1_Init(void)		//12微秒@12.000MHz
{
	AUXR &= 0xBF;			//定时器时钟12T模式
	TMOD &= 0x0F;			//设置定时器模式
	TL1 = 0xF4;				//设置定时初始值
	TH1 = 0xFF;				//设置定时初始值
	TF1 = 0;				//清除TF1标志
	TR1 = 0;				//定时器1开始计时
}

double WaveRcvd()
{
	unsigned ucnum = 10;
	Tx = 0;
	TL1 = 0xF4;
	TH1 = 0xFF;	
	TR1 = 1;
	while(ucnum--)
	{
		while(!TF1);
		Tx ^= 1;
		TF1 = 0;
	}
	TR1 = 0;
	TL1 = TH1 = 0;
	TR1 = 1;
	
	while(Rx && !TF1);
	TR1 = 0;
	if(TF1)
		return 0;
	else
		return (TH1<<8|TL1)*0.0000005*(330+0.6*ReadTemprature());
}