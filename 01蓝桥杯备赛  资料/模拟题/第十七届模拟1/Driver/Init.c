#include <Init.h>

void System_Init()//关闭外设子函数
{
	P0 = 0xff;//led全灭
	P2 = P2 & 0x1f | 0x80;//选中led通道
	P2 &= 0x1f;//关闭通道
	
	P0 = 0x00;//关闭继电器，蜂鸣器
	P2 = P2 & 0x1f | 0xa0;//选中外设通道
	P2 &= 0x1f;//关闭通道
}