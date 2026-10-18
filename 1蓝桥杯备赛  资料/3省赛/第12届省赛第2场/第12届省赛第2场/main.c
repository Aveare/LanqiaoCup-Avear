/*头文件区域*/
#include <STC15F2K60S2.H>
#include <Init.h>
#include <Key.h>
#include <Seg.h>
#include <Led.h>
#include <iic.h>
/*参数变量区域*/
unsigned char Key_Down,Key_Up,Key_Old,Key_Val;
unsigned char Key_Slow,Seg_Slow;
unsigned char Seg_Pos;
//数组
unsigned char Seg_Buf[] = {10,10,10,10,10,10,10,10};
unsigned char Seg_Point[] = {0,0,0,0,0,0,0,0};
unsigned char Led_Buf[] = {0,0,0,0,0,0,0,0};
//标志位
unsigned int Time_1s;
unsigned int Time_S7;
bit S7_flag;//s7长按标志位
bit Led_Flag;//led功能标志位
bit L1_flag;//L1点亮标志位
bit L2_flag;//L2点亮标志位
//数据
float flow;
float hd;
unsigned char Seg_Mode;//0频率1周期2电压
unsigned char Channel;//0通道1光敏电阻 1通道3滑动变阻器
unsigned int Freq;//ne555计算
unsigned int Freq_save;//保存的频率值
unsigned int Volt_save_hd;//保存的滑动变阻器电压值
unsigned int Tick;//周期
/*按键函数区域*/
void Key_Proc()
{
	
	if(Key_Slow) return;
	Key_Slow = 1;
	
	Key_Val = Key_Read();
	Key_Down = Key_Val & (Key_Old ^ Key_Val);
	Key_Up = ~Key_Val & (Key_Old ^ Key_Val);
	Key_Old = Key_Val;
	
	switch(Key_Down)
{
		case 4://显示模式切换
			if(++Seg_Mode == 3) Seg_Mode = 0;
		break;
		case 5://电压通道切换
			if(Seg_Mode == 2)//在电压界面有效
				Channel ^= 1;
		break;
		case 6://通道3滑动变阻器电压缓存
			Volt_save_hd = hd;
		break;
	}
	
	if(Key_Down == 7)S7_flag = 1;//长按S7超过1s禁用Led
	
	if(Key_Up == 7)
	{
		S7_flag = 0;//松手停止计时
		if(Time_S7>=1000)//长按超过1s
			Led_Flag ^= 1;//再次长按开启Led,默认0开启1关闭
		else Freq_save = Freq;//短按保存频率
		Time_S7 = 0;
	}
}
 
/*数码管函数区域*/
void Seg_Proc()
{
 
	if(Seg_Slow) return;
	Seg_Slow = 1;
	
	hd = AD_Read(0x43)*5.0/255.0;//滑动变阻器
	hd = AD_Read(0x43)*5.0/255.0;//连读消除ad的读取问题
	flow = AD_Read(0x41)*5.0/255.0;//读取光敏电阻电压值
	flow = AD_Read(0x41)*5.0/255.0;
	
	if(hd>Volt_save_hd)//当前通道3电压值大于记录值保存值
		L1_flag = 1;
	else L1_flag = 0;
	
	switch(Seg_Mode)
	{
		case 0://频率显示模式
			Seg_Point[5] = 0;
			Seg_Buf[0] = 12;//F
			Seg_Buf[1] = 10;
			Seg_Buf[2] = 10;
			Seg_Buf[3] = Freq>10000?Freq/10000:10;
			Seg_Buf[4] = Freq>1000?Freq%10000/1000:10;
			Seg_Buf[5] = Freq>100?Freq%1000/100:10;
			Seg_Buf[6] = Freq>10?Freq%100/10:10;
			Seg_Buf[7] = Freq%10;
		break;
		case 1://周期显示模式
			Seg_Point[5] = 0;
			Seg_Buf[0] = 13;//N
			Seg_Buf[1] = 10;
			Seg_Buf[2] = 10;
			Seg_Buf[3] = Tick>10000?Tick/10000%10:10;
			Seg_Buf[4] = Tick>1000?Tick%10000/1000:10;
			Seg_Buf[5] = Tick>100?Tick%1000/100:10;
			Seg_Buf[6] = Tick>10?Tick%100/10:10;
			Seg_Buf[7] = Tick%10;
		break;
		case 2://电压显示模式
			Seg_Buf[0] = 14;//U
			Seg_Buf[1] = 11;//-
			Seg_Buf[3] = Seg_Buf[4] = 10;
			if(Channel == 0) //光敏电阻
			{
				Seg_Buf[2] = 1;//通道编号
				Seg_Buf[5] = (unsigned char)flow;
				Seg_Point[5] = 1;
				Seg_Buf[6] =(unsigned char)(flow*10)%10;
				Seg_Buf[7] =(unsigned char)(flow*100)%10;
			}
			else  //滑动变阻器
			{
				Seg_Buf[2] = 3;
				Seg_Buf[5] = (unsigned char)hd;
				Seg_Point[5] = 1;
				Seg_Buf[6] =(unsigned char)(hd*10)%10;
				Seg_Buf[7] =(unsigned char)(hd*100)%10;
			}
		break;
	}
	
}
/*Led函数区域*/
void Led_Proc(){
	if(Led_Flag==0)//默认开启led功能//默认开启led功能
	{
		Led_Buf[0] = L1_flag;
		Led_Buf[1] = L2_flag;
		Led_Buf[2] = (Seg_Mode==0);
		Led_Buf[3] = (Seg_Mode==1);
		Led_Buf[4] = (Seg_Mode==2);
	}
	else
	{
		Led_Buf[0] = 0;
		Led_Buf[1] = 0;
		Led_Buf[2] = 0;
		Led_Buf[3] = 0;
		Led_Buf[4] = 0;
	}
}
/*定时器0初始化函数*/
void Timer0_Init(void)		//1微秒@12.000MHz
{
	AUXR &= 0x7F;			//定时器时钟12T模式
	TMOD &= 0xF0;			//设置定时器模式
	TMOD |= 0x05;			//切换计数模式
	TL0 = 0x00;				//设置定时初始值
	TH0 = 0x00;				//设置定时初始值
	TF0 = 0;				//清除TF0标志
	TR0 = 1;				//定时器0开始计时
}
/*定时器1初始化函数*/
void Timer1_Init(void)		//100微秒@12.000MHz
{
	AUXR &= 0xBF;			//定时器时钟12T模式
	TMOD &= 0x0F;			//设置定时器模式
	TL1 = 0x18;				//设置定时初始值
	TH1 = 0xFC;				//设置定时初始值
	TF1 = 0;				//清除TF1标志
	TR1 = 1;				//定时器1开始计时
	ET1 = 1;
	EA = 1;
}
 
/*定时器1中断服务函数*/
void Timer1_Service() interrupt 3
{
	
	if(++Seg_Slow == 50) Seg_Slow = 0;
	if(++Key_Slow == 50) Key_Slow = 0;
	
	if(++Seg_Pos == 8) Seg_Pos = 0;
	Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],Seg_Point[Seg_Pos]);
	Led_Disp(Seg_Pos,Led_Buf[Seg_Pos]);
	
	if(++Time_1s == 1000)
	{
		Time_1s = 0;
		Freq = (TH0<<8)|TL0;
		Tick = 1000000/Freq;
		TH0 = TL0 = 0;
		if(Freq>Freq_save) L2_flag = 1;
		else L2_flag = 0;
	}
	
	if(S7_flag)
	{
		if(++Time_S7 >= 1000) Time_S7 =1010;//锁死s7
	}
}
 
 
/*主函数区域*/
void main()
{
	System_Init();
	Timer1_Init();
	Timer0_Init();
	while(1)
	{
		Led_Proc();
		Seg_Proc();
		Key_Proc();
	}
}