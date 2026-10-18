/* 头文件声明区 */
#include <STC15F2K60S2.H>
#include <Init.h>
#include <Key.h>
#include <Seg.h>

/* 变量声明区 */
unsigned char Key_Val,Key_Down,Key_Old,Key_Up;//按键专用变量
unsigned char Seg_Buf[8] = {10,10,10,10,10,10,10,10};//数码管显示数据存放数组
unsigned char Seg_Point[8] = {0,0,0,0,0,0,0,0};//数码管小数点数据存放数组
unsigned char Seg_Pos;//数码管扫描专用变量
unsigned int Seg_Slow_Down;//数码管减速专用变量
unsigned int count=100;//显示数值
unsigned char Timer20ms;//20ms计时变量
bit key_flag=0;//按键扫描标志位
/* 键盘处理函数 */
void Key_Proc()
{
	Key_Val = Key_Read();//实时读取键码值
	Key_Down = Key_Val & (Key_Old ^ Key_Val);//捕捉按键下降沿
	Key_Up = ~Key_Val & (Key_Old ^ Key_Val);//捕捉按键上降沿
	Key_Old = Key_Val;//辅助扫描变量

	switch(Key_Down)
	{
		case 5:
			count++;
		break;
		case 6:
			count--;
		break;
	}
}

/* 信息处理函数 */
void Seg_Proc()
{
	if(Seg_Slow_Down) return;
	Seg_Slow_Down = 1;//数码管减速程序
	
	Seg_Buf[0]=count>=10000000?count/10000000%10:10;
	Seg_Buf[1]=count>=1000000?count/100000%10:10;
	Seg_Buf[2]=count>=100000?count/100000%10:10;
	Seg_Buf[3]=count>=10000?count/10000%10:10;
	Seg_Buf[4]=count>=1000?count/1000%10:10;
	Seg_Buf[5]=count>=100?count/100%10:10;
	Seg_Buf[6]=count>=10?count/10%10:10;
	Seg_Buf[7]=count%10;
}

/* 定时器0中断初始化函数 */
void Timer0Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0x7F;		//定时器时钟12T模式
	TMOD &= 0xF0;		//设置定时器模式
	TL0 = 0x18;		//设置定时初始值
	TH0 = 0xFC;		//设置定时初始值
	TF0 = 0;		//清除TF0标志
	TR0 = 1;		//定时器0开始计时
	ET0 = 1;    //定时器中断0打开
	EA = 1;     //总中断打开
}

/* 定时器0中断服务函数 */
void Timer0Server() interrupt 1
{  
	if(++Seg_Slow_Down == 500) Seg_Slow_Down = 0;//数码管减速专用
	if(++Seg_Pos == 8) Seg_Pos = 0;//数码管显示专用
	Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],Seg_Point[Seg_Pos]);
	
		if(++Timer20ms>=20)//周期20ms扫描一次
			{
				key_flag = 1;
				Timer20ms = 0;
			}
}

/* Main */
void main()
{
	System_Init();
	Timer0Init();
	while (1)
	{
		Seg_Proc();
		if(key_flag)//确认扫描标志位再读取按键值
		{
			key_flag=0;
			Key_Proc();
		}
	}
}