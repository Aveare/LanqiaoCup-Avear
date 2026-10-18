#include <STC15F2K60S2.H>
#include <iic.h>
#include <ut.h>
/*底层变量*/
static unsigned char temp;
unsigned char Key_Slow,Key_Val,Key_Old,Key_Down,Key_Up;
unsigned int Seg_Slow;
unsigned char Seg_Buf[9]={10,10,10,10,10,10,10,10};
unsigned char Seg_Pos;
unsigned char Seg_Point[8]={0,0,0,0,0,0,0,0};
unsigned char ucLed[8]={0,0,0,0,0,0,0,0};
unsigned char code seg_wei[8]={0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
unsigned char code seg_table[]={0xC0,0xF9,0xA4,0xB0,0x99,0x92,0x82,0xF8,0x80,0x90,0xff,0xbf,0x8e,0x89,0x88,0x8c};
/*底层函数声明*/
void Init();
unsigned char Key_Read();
void Seg_Disp(unsigned char wei,unsigned char table,bit point);
void Led_Disp(unsigned char addr,bit enable);
void Motor(enable);
void Relay(enable);
/*变量*/
unsigned int freq,kfreq_10x;
unsigned char Timer200ms;
unsigned char Disp_Mode;//0-频率 1-湿度 2-测距 3-参数
bit Khz_Flag;//0-hz 1-Khz
bit M_FLag;//0-cm 1-m
unsigned char Para[3]={90,40,6};
unsigned char Set_Mode;//0-频率参数 10-120 1-湿度参数10-60 2-距离参数1-12
unsigned char Humid=40;//湿度
unsigned int distance=60;//距离
bit Long_Flag=0;
unsigned int Timer1s;
unsigned char Relay_Count;
bit Relay_Flag=0;
bit Star_Flag=0;
unsigned char Timer100ms=0;
unsigned char PWM_Level=0;
unsigned char PWM_Count=0;
/*按键处理*/
void Key_Proc()
{
	if(Key_Slow)return;
	Key_Slow=1;
	Key_Val=Key_Read();
	Key_Down=Key_Val&(Key_Val^Key_Old);
	Key_Up=~Key_Val&(Key_Val^Key_Old);
	Key_Old=Key_Val;
	switch(Key_Down)
	{
		case 4://显示界面切换
		Disp_Mode=(Disp_Mode+1)%4;//0-3循环
		Set_Mode=0;
		break;
		case 5:
		Set_Mode=(Set_Mode+1)%3;//0-2循环	
		break;
		case 6://+
			if(Disp_Mode==2)M_FLag^=1;
			if(Disp_Mode==3)
			{
				if(Set_Mode==0)//频率参数状态
				{
					Para[Set_Mode]+=5;
					if(Para[Set_Mode]==125)Para[Set_Mode]=10;
				}
				if(Set_Mode==1)//湿度参数状态
				{
					Para[Set_Mode]+=10;
					if(Para[Set_Mode]==70)Para[Set_Mode]=10;
				}
				if(Set_Mode==2)//距离参数状态
				{
					Para[Set_Mode]+=1;
					if(Para[Set_Mode]==13)Para[Set_Mode]=1;
				}
			}
		break;
		case 7://-
			if(Disp_Mode==0)Khz_Flag^=1;
			if(Disp_Mode==1)Long_Flag=1;//湿度界面下长按
			if(Disp_Mode==3)
			{
				if(Set_Mode==0)//频率参数状态
				{
					Para[Set_Mode]-=5;
					if(Para[Set_Mode]==5)Para[Set_Mode]=120;
				}
				if(Set_Mode==1)//湿度参数状态
				{
					Para[Set_Mode]-=10;
					if(Para[Set_Mode]==0)Para[Set_Mode]=60;
				}
				if(Set_Mode==2)//距离参数状态
				{
					Para[Set_Mode]-=1;
					if(Para[Set_Mode]==0)Para[Set_Mode]=12;
				}
			}
		break;	
	}
	//湿度界面下长按S7>1s
	if(Key_Up==7)
	{
		if(Timer1s>1000)
		{
			Relay_Count=0;
			EEPROM_Write(&Relay_Count,0,1);
		}
		Long_Flag=Timer1s=0;
	}
}

/*信息处理*/
void Seg_Proc()
{
	if(Seg_Slow)return;
	Seg_Slow=1;
	//重要信息读取
	kfreq_10x=freq/100;
	
	EA=0;Humid=AD_Read(0x43)/51.0*20.0;EA=1;
	distance=Ut_Wave_Data();
	
	switch(Disp_Mode)
	{
		case 0://频率界面
			Seg_Buf[0]=12;//F
			Seg_Buf[1]=10;
			if(!Khz_Flag)//hz模式
			{
				Seg_Buf[2]=freq>=100000?freq/100000%10:10;
				Seg_Buf[3]=freq>=10000?freq/10000%10:10;
				Seg_Buf[4]=freq>=1000?freq/1000%10:10;
				Seg_Buf[5]=freq>=100?freq/100%10:10;
				Seg_Buf[6]=freq>=10?freq/10%10:10;
				Seg_Buf[7]=freq%10;
				Seg_Point[6]=0;
			}
			else//Khz模式
			{
			Seg_Buf[2]=kfreq_10x>100000?kfreq_10x/100000%10:10;
			Seg_Buf[3]=kfreq_10x>10000?kfreq_10x/10000%10:10;
			Seg_Buf[4]=kfreq_10x>1000?kfreq_10x/1000%10:10;
			Seg_Buf[5]=kfreq_10x>100?kfreq_10x/100%10:10;
			Seg_Buf[6]=kfreq_10x>10?kfreq_10x/10%10:10;
			Seg_Buf[7]=kfreq_10x%10;
			if(freq<1000)
			{
				Seg_Buf[6]=0;
				Seg_Buf[7]=freq/100%10;
			}
			Seg_Point[6]=1;
		}
		break;
		case 1://湿度界面
			Seg_Buf[0]=13;//H
			Seg_Buf[1]=Seg_Buf[2]=Seg_Buf[3]=Seg_Buf[4]=Seg_Buf[5]=10;
			if(Humid==100)Humid=99;
			Seg_Buf[6]=Humid/10%10;
			Seg_Buf[7]=Humid%10;
		Seg_Point[6]=0;
		break;
		case 2://测距界面
			Seg_Buf[0]=14;//A
			Seg_Buf[1]=Seg_Buf[2]=Seg_Buf[3]=Seg_Buf[4]=10;
		if(!M_FLag)//cm模式
		{
			Seg_Buf[5]=distance>=100?distance/100%10:10;
			Seg_Buf[6]=distance>=10?distance/10%10:10;
			Seg_Buf[7]=distance%10;
			Seg_Point[5]=0;
		}
		else //M模式
		{
				Seg_Buf[5]=distance>=100?distance/100%10:0;
				Seg_Buf[6]=distance>=10?distance/10%10:0;
				Seg_Buf[7]=distance%10;
				Seg_Point[5]=1;
		}
		break;
		case 3://参数界面
			Seg_Buf[0]=15;//P
			Seg_Buf[1]=Set_Mode+1;
			switch(Set_Mode)
			{
				case 0://频率参数
				Seg_Buf[2]=Seg_Buf[3]=Seg_Buf[4]=10;
				Seg_Buf[5]=Para[Set_Mode]>=100?Para[Set_Mode]/100%10:10;
				Seg_Buf[6]=Para[Set_Mode]>=10?Para[Set_Mode]/10%10:0;
				Seg_Buf[7]=Para[Set_Mode]%10;
				Seg_Point[6]=1;
				break;
				case 1://温度参数
				Seg_Buf[2]=Seg_Buf[3]=Seg_Buf[4]=Seg_Buf[5]=10;	
				Seg_Buf[6]=Para[Set_Mode]/10%10;
				Seg_Buf[7]=Para[Set_Mode]%10;
				Seg_Point[6]=0;
				break;
				case 2://距离参数
				Seg_Buf[2]=Seg_Buf[3]=Seg_Buf[4]=Seg_Buf[5]=10;	
				Seg_Buf[6]=Para[Set_Mode]>=10?Para[Set_Mode]/10%10:0;
				Seg_Buf[7]=Para[Set_Mode]%10;
				Seg_Point[6]=1;
				break;
			}
			Seg_Point[5]=0;
		break;
	}
	
}
/*其他处理*/
void Led_Proc()
{
	unsigned char i;
	//Led
	for(i=0;i<3;i++)ucLed[i]=Disp_Mode<3?(i==Disp_Mode):(i==Set_Mode)&Star_Flag;
	ucLed[3]=(freq>Para[0]*1000);
	ucLed[4]=(Humid>Para[1]);
	ucLed[5]=(distance/10>Para[2]);
	ucLed[6]=ucLed[7]=0;
	//继电器
	if(ucLed[5])
	{
		if(Relay_Flag==0)
		{
			Relay(1);
			Relay_Flag=1;
			Relay_Count++;
			EEPROM_Write(&Relay_Count,0,1);
		}
	}
	else
	{
		if(Relay_Flag==1)
		{
			Relay(0);
			Relay_Flag=0;
			Relay_Count++;
			EEPROM_Write(&Relay_Count,0,1);
		}
	}
	//DA
	if(Humid<Para[1])DA_Write(51);
	else if(Humid>80)DA_Write(255);
	else DA_Write(((Humid-Para[1])/(80.0-Para[1])*4.0+1.0)*51);
	//MOTOR 产生方波
	PWM_Level=ucLed[3]?8:2;
}

/*定时器0初始化函数*/
void Timer0Init(void)		//0毫秒@12.000MHz
{
	AUXR &= 0x7F;		//定时器时钟12T模式
	TMOD &= 0xF0;		//设置定时器模式
	TMOD|=0x05;
	TL0 = 0x00;		//设置定时初值
	TH0 = 0x00;		//设置定时初值
	TF0 = 0;		//清除TF0标志
	TR0 = 1;		//定时器0开始计时
}
/*定时器1初始化*/
void Timer1Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0xBF;		//定时器时钟12T模式
	TMOD &= 0x0F;		//设置定时器模式
	TL1 = 0x18;		//设置定时初值
	TH1 = 0xFC;		//设置定时初值
	TF1 = 0;		//清除TF1标志
	TR1 = 1;		//定时器1开始计时
	ET1=1;
	EA=1;
}
/* 定时器2中断初始化 */
void Timer2Init(void)		//100微秒@12.000MHz
{
	AUXR &= 0xFB;		//定时器时钟12T模式
	T2L = 0x9C;		//设置定时初始值
	T2H = 0xFF;		//设置定时初始值
	AUXR |= 0x10;		//定时器2开始计时
	IE2 |= (1<<2);   //定时器中断2打开
}
/*定时器1中断服务函数*/
void Timer1server(void) interrupt 3
{
	if(++Key_Slow==10)Key_Slow=0;
	if(++Seg_Slow==300)Seg_Slow=0;
	if(++Seg_Pos==8)Seg_Pos=0;
	Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],Seg_Point[Seg_Pos]);
	Led_Disp(Seg_Pos,ucLed[Seg_Pos]);
	//NE555
	if(++Timer200ms==200)
	{
		freq=TH0<<8|TL0;
		freq*=5;
		TH0=TL0=0;
		Timer200ms=0;
	}
	//长按S7
	if(Long_Flag)
	{
		Timer1s++;
		if(Timer1s==1000)Timer1s=1001;
	}
	else Timer1s=0;
	//闪烁
	if(++Timer100ms==100)
	{
		Timer100ms=0;
		Star_Flag^=1;
	}
}
/* 定时器2中断服务函数 */
void Timer2Server() interrupt 12
{
	if(++PWM_Count == 10)PWM_Count = 0;//脉冲周期为1MS
	Motor(PWM_Count<PWM_Level);//PWM输出
}
/*Main*/
void main()
{
	Init();
	Timer0Init();
	Timer1Init();
	Timer2Init();
	EEPROM_Read(&Relay_Count,0,1);
	while(1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
	}
}
void Init()
{
	P2=0x80;P0=0xff;P2=0x00;
	P2=0xa0;P0=0x00;P2=0x00;
}
unsigned char Key_Read()
{
	unsigned char temp=0;
	if(P33==0)temp=4;
	if(P32==0)temp=5;
	if(P31==0)temp=6;
	if(P30==0)temp=7;
	return temp;
}
void Seg_Disp(unsigned char wei,unsigned char table,bit point)
{
	P2=0xe0;P0=0x00;P2=0x00;
	P2=0xc0;P0=seg_wei[wei];P2=0x00;
	P2=0xe0;P0=seg_table[table];
	if(point)P0&=~(1<<7);
	P2=0x00;
}
void Led_Disp(unsigned char addr,bit enable)
{
	static unsigned char temp;
	if(enable)temp|=0x01<<addr;
	else temp&=~0x01<<addr;
	P2=0x80;P0=~temp;P2=0x00;
}
void Motor(enable)
{
	if(enable)temp|=0x20;
	else temp&=~0x20;
	P0=temp;P2=0xa0;P2=0x00;
}
void Relay(enable)
{
	if(enable)temp|=0x10;
	else temp&=~0x10;
	P0=temp;P2=0xa0;P2=0x00;
}