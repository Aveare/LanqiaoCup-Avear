#include <Init.h>

void System_Init()//关闭外设子函数
{
	P2=0xa0;P0=0x00;P2=0x00;//关外设（继电器+ 蜂鸣器）
	P2=0x80;P0=0xff;P2=0x00;//关Led
}