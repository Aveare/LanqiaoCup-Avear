/*头文件声明*/
#include <REGX51.H>

sbit PWM=P1^0;
/*变量声明*/
unsigned char count;
unsigned char kv=90;//占空比

/*延迟函数*/
void delay(unsigned int temp)
{
	unsigned int i,j;
	for(i=0;i<i;i++)
		for(j=0;j<120;j++);
}
/*定时器0初始化函数*/
void Timer0Init()
{
	TMOD=0x01;
	TH0=(65535-500)/256;
	TL0=(65535-500)%256;
	ET0=1;
	TR0=1;
	EA=1;
}
/*定时器0中断服务函数*/
void Timer0isr(void) interrupt 1
{
	TH0=(65535-500)/256;
	TL0=(65535-500)%256;
	if(count<kv)PWM=1;//占空比为90%
	else PWM=0;
	count=(count+1)%100;
}
/*Main*/
void main() 
{
  Timer0Init();
	while(1);
}