/*头文件声明*/
#include <REGX51.H>
#include <Key.h>
/*变量声明*/
unsigned char Key_Val,Key_Down,Key_Old,Key_Up;
unsigned char count;
unsigned char seg_dula[] = {0xc0,0xf9,0xa4,0xb0,0x99,0x92,0x82,0xf8,0x80,0x90,0xff};
unsigned char seg_wela[] = {0x01,0x02};
unsigned char Seg_Pos;//数码管扫描变量
unsigned char Seg_Buf[2];
/*按键处理函数*/
void Key_Proc()
{

	Key_Val = Key_Read();//实时读取键码值
	Key_Down = Key_Val & (Key_Old ^ Key_Val);//捕捉按键下降沿
	Key_Up = ~Key_Val & (Key_Old ^ Key_Val);//捕捉按键上降沿
	Key_Old = Key_Val;//辅助扫描变量

	switch(Key_Down)
	{
		case 1:
			count=0;
		break;
		case 2:
			count=1;
		break;
		case 3:
			count=2;
		break;
		case 4:
			count=3;
		break;
		case 5:
			count=4;
		break;
		case 6:
			count=5;
		break;
		case 7:
			count=6;
		break;
		case 8:
			count=7;
		break;
		case 9:
			count=8;
		break;
		case 10:
			count=9;
		break;
		case 11:
			count=10;
		break;
		case 12:
			count=11;
		break;
		case 13:
			count=12;
		break;
		case 14:
			count=13;
		break;
		case 15:
			count=14;
		break;
		case 16:
			count=15;
		break;
	}
}
/*数码管显示函数*/
void Seg_Disp(unsigned char wela,dula)
{
	  P0 = 0xff;  // 消影：段码全灭
    P2 = 0x00;  // 消影：位选全关
    
    P0 = seg_dula[dula]; // 送段码
    P2 = seg_wela[wela]; // 送位选
}
/*数码管处理函数*/
void Seg_Proc()
{
	Seg_Buf[0]=count/10%10;//显示十位
	Seg_Buf[1]=count/1%10;//显示个位
}

/* 定时器0中断初始化函数 */
void Timer0Init(void)		//1毫秒@12.000MHz
{
	TMOD &= 0xF0;		//设置定时器模式
	TMOD |= 0x01;		//设置定时器模式
	TL0 = 0x18;		//设置定时初始值
	TH0 = 0xFC;		//设置定时初始值
	TF0 = 0;		//清除TF0标志
	TR0 = 1;		//定时器0开始计时
	ET0 = 1;    //定时器0中断打开
	EA = 1;     //总中断打开
}

/* 定时器0中断服务函数 */
void Timer0Server() interrupt 1
{
 	TL0 = 0x18;		//设置定时初始值
	TH0 = 0xFC;		//设置定时初始值  	
	if(++Seg_Pos == 2) Seg_Pos = 0;//数码管显示专用
	Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos]);//扫描数码管实现动态显示
}
/*Main*/
void main()
{
	Timer0Init();
	while(1)
	{
		Key_Proc();
		Seg_Proc();
	}
}
