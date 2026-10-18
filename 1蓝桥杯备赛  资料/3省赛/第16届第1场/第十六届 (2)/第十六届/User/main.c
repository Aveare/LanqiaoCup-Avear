/*头文件*/
#include <STC15F2K60S2.H>
#include <Seg.h>
#include <Led.h>
#include <Key.h>
#include "Ds18B20.h"
#include <Init.h>
#include <IIC.H>
#include "Ut_Wave.H"
#include <math.h>

/*变量区域*/
unsigned char Key_Up,Key_Down,Key_Val,Key_Old;
unsigned char Key_Slow_Down;
unsigned char Seg_Slow_Down;
unsigned char Seg_Buf[8]={1,2,3,4,5,6,7,8};
unsigned char Seg_Pos;
unsigned char Seg_Point[8]={0,0,0,0,0,0,0,0};
unsigned char ucled[8]={0,0,0,0,0,0,0,0};
idata unsigned char Seg_show_Mode;//数码管显示模式
idata unsigned char Para_show_Mode;//数码管参数显示模式
idata unsigned char T_Value;//当前温度
idata unsigned int T_Slow_Down;//温度减速
idata unsigned char Light_Level=1;//光强等级
idata unsigned char AD_Slow_Down;//ad减速
idata unsigned char Motion_State;//运动状态
idata unsigned char D_Value;//距离
idata unsigned char D_Value_Old;
idata unsigned int D_Slow_Down;//距离减速
idata unsigned char T_Para=30;//温度参数
idata unsigned char D_Para=30;//距离参数
idata unsigned int Relay_Control;//继电器次数
idata bit Relay_Led_Lock;//状态锁定
idata bit Timer_2000_Flag;//长按检测
idata unsigned int Timer_2000;//2秒
idata bit Hot_Flag;//高温判定
idata bit Led_Blink_Flag;//闪烁标志位
idata unsigned char Timer_Blink_100;
idata bit Close_Flag;//接近标志位
idata bit Run_State;//跑动状态
idata bit First_Get_D;//第一次获取的值
idata bit State_Change_Flag;//状态改变标志位
idata unsigned char Timer_3000;//状态改变
idata bit Stay_State;//静止状态
idata bit Par_State;//徘徊状态
idata bit Relay_Flag;//继电器标志位



/*按键区域*/
void Key_Proc()
{
	if(Key_Slow_Down<10) return;
	Key_Slow_Down=0;
	
	Key_Val=Key_Read();
	Key_Down=Key_Val&(Key_Val^Key_Old);
	Key_Up=~Key_Val&(Key_Val^Key_Old);
	Key_Old=Key_Val;
	
	if(Key_Down==4)
	{
		Relay_Led_Lock=(Seg_show_Mode==2);
		Para_show_Mode=0;
		Seg_show_Mode=(++Seg_show_Mode)%4;
	}
	
	if(Seg_show_Mode==2)
	{
		if(Key_Down==5)
				Para_show_Mode^=1;
		if(Key_Down==8)
		{
			switch(Para_show_Mode)
			{
				case 0:
					T_Para=(T_Para==80)
									?80
									:T_Para+1;
				break;
				
				case 1:
					D_Para=(D_Para==80)
									?80
									:D_Para+5;
				break;
			}
		}
		
		if(Key_Down==9)
		{
			switch(Para_show_Mode)
			{
				case 0:
					T_Para=(T_Para==20)
									?80
									:T_Para-1;
				break;
				
				case 1:
					D_Para=(D_Para==20)
									?80
									:D_Para-5;
				break;
			}
		}
		
	}
	
	if(Seg_show_Mode==3)
	{
		if(Timer_2000>=2000)
			Relay_Control=0;
		if(Key_Old==89)
			Timer_2000_Flag=1;
		else
		{
			Timer_2000_Flag=0;
			Timer_2000=0;
		}
	}
}


/*信息处理区域*/
void Seg_Proc()
{

	if(Seg_Slow_Down<500) return;
	Seg_Slow_Down=0;
	
	switch(Seg_show_Mode)
	{
		case 0:
			Seg_Buf[0]=11;
			Seg_Buf[1]=T_Value/10%10;
			Seg_Buf[2]=T_Value%10;
			Seg_Buf[3]=10;
			Seg_Buf[4]=10;
			Seg_Buf[5]=10;
			Seg_Buf[6]=12;
			Seg_Buf[7]=Light_Level;
		break;
	
		case 1:
			Seg_Buf[0]=13;
			Seg_Buf[1]=Motion_State;
			Seg_Buf[2]=10;
			Seg_Buf[3]=10;
			Seg_Buf[4]=10;
			Seg_Buf[5]=D_Value/100%10;
			Seg_Buf[6]=D_Value/10%10;
			Seg_Buf[7]=D_Value%10;
		break;
		
		case 2:
			Seg_Buf[0]=14;
			Seg_Buf[2]=10;
			Seg_Buf[3]=10;
			Seg_Buf[4]=10;
			Seg_Buf[5]=10;
			switch(Para_show_Mode)
			{
				case 0:
					Seg_Buf[1]=11;
					Seg_Buf[6]=T_Para/10%10;
					Seg_Buf[7]=T_Para%10;
				break;
				
				case 1:
					Seg_Buf[1]=13;
					Seg_Buf[6]=D_Para/10%10;
					Seg_Buf[7]=D_Para%10;
				break;
			}
		break;
		
		case 3:
			Seg_Buf[0]=12;
			Seg_Buf[1]=11;
			Seg_Buf[2]=10;
			Seg_Buf[3]=10;
			Seg_Buf[4]=(Relay_Control>1000)
									? Relay_Control/1000%10
									: 10;
			Seg_Buf[5]=(Relay_Control>100)
									? Relay_Control/100%10
									: 10;
			Seg_Buf[6]=(Relay_Control>10)
									? Relay_Control/10%10
									: 10;
			Seg_Buf[7]=Relay_Control%10;
		break;
		
	}

	
}


/*其他处理函数*/
void led_Proc()
{
	bit temp_Relay_Flag;
	if(!Relay_Led_Lock)
	{
		ucled[0]=(Light_Level>=1);
		ucled[1]=(Light_Level>=2);
		ucled[2]=(Light_Level>=3);
		ucled[3]=(Light_Level>=4);
		temp_Relay_Flag=(Close_Flag&&Hot_Flag);
		if(Relay_Flag==0&&temp_Relay_Flag==1)
			Relay_Control++;
		Relay_Flag=temp_Relay_Flag;
		Relay(Relay_Flag);
	}
	
	if(Stay_State) ucled[7]=0;
	else if(Par_State) ucled[7]=1;
	else if(Run_State) ucled[7]=Led_Blink_Flag;	 
	
	
	
}

void Get_D()
{
	unsigned char DL_Chazhi;//两次测量的差
	unsigned char temp_Motion_State;//临时的运动状态
	if(D_Slow_Down<999) return;
	D_Slow_Down=0;
	if(First_Get_D==1)
	{
		D_Value=Ut_Wave_Data();
		DL_Chazhi=abs(D_Value_Old-D_Value);
		D_Value_Old=D_Value;
		Close_Flag=(D_Value<D_Para);
		if(!State_Change_Flag)
		{
			if(DL_Chazhi<5) temp_Motion_State=0;
			else if(DL_Chazhi<10) temp_Motion_State=1;
			else  temp_Motion_State=2;
			State_Change_Flag=(temp_Motion_State!=Motion_State);
			Motion_State=temp_Motion_State;
		}
		if(!Relay_Led_Lock)
		{
			Stay_State=(Motion_State=1);
			Par_State=(Motion_State=2);
			Run_State=(Motion_State=3);
		}
	}
	else
	{
		First_Get_D=1;
		D_Value=Ut_Wave_Data();
		D_Value_Old=Ut_Wave_Data();
	}
	
}

void Get_AD()
{
	unsigned char temp;
	if(AD_Slow_Down<100) return;
		AD_Slow_Down=0;
	
	temp=Ad_Read(0x43);
	if(temp>=3*51) Light_Level=1;
	else if(temp>=2*51) Light_Level=2;
	else if(temp>=0.5*51) Light_Level=3;
	else  Light_Level=4;
}

void Get_T()
{
	float temp;
	if(T_Slow_Down<300) return;
		T_Slow_Down=0;
	temp=Read_t();
	T_Value=temp;
	Hot_Flag=(temp>T_Para);
	
	
}


/*定时器初始化*/
void Timer0Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0x7F;		//定时器时钟12T模式
	TMOD &= 0xF0;		//设置定时器模式
	TL0 = 0x18;		//设置定时初值
	TH0 = 0xFC;		//设置定时初值
	TF0 = 0;		//清除TF0标志
	TR0 = 1;		//定时器0开始计时
	ET0=1;
	EA=1;
}

/*中断服务函数*/
void Timer_Server() interrupt 1
{
	AD_Slow_Down++;
	D_Slow_Down++;
	T_Slow_Down++;
	Key_Slow_Down++;
	Seg_Slow_Down++;
	
	if(++Seg_Pos==8) Seg_Pos=0;
	Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],Seg_Point[Seg_Pos]);
	Led_Disp(ucled);

	if(Timer_2000_Flag)
	{
		if(++Timer_2000>2000)
			Timer_2000=2001;
	}
	else
	{
		Timer_2000=0;
	}
	
	if(Run_State)
	{
		if(++Timer_Blink_100==100)
		{
			Timer_Blink_100=0;
			Led_Blink_Flag^=1;
		}
	}
	else
	{
		Timer_Blink_100=0;
		Led_Blink_Flag=0;
	}
	
	if(State_Change_Flag)
	{
		if(++Timer_3000==3000)
		{
			Timer_3000=0;
			State_Change_Flag=0;
		}
	}
	else State_Change_Flag=0;
}



/*主函数*/
void main()
{
	Timer0Init();	
	System_Init();
	while(1)
	{
		Key_Proc();
		Seg_Proc();
		led_Proc();
		Get_T();
		Get_AD();
		Get_D();
	}

}