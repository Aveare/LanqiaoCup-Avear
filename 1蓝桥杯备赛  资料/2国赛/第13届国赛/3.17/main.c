/*头文件引用*/
#include <STC15F2K60S2.H>
#include <iic.h>
#include <onewire.h>
#include <ultra.h>

/*底层函数变量*/
unsigned char temp=0x00;
unsigned char Key_Val,Key_Down,Key_Old,Key_Up;
unsigned char Key_Slow;
unsigned int Seg_Slow;
unsigned char Seg_Pos;
unsigned char code seg_wei[8]={0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
unsigned char code seg_table[]={0xC0,0xF9,0xA4,0xB0,0x99,0x92,0x82,0xF8,0x80,0x90,0xff,0xbf,0x8e,0x89,0x88,0x8c};
unsigned char Seg_Buf[8]={10,10,10,10,10,10,10,10};
unsigned char Seg_Point[8]={0,0,0,0,0,0,0,0};
unsigned char ucLed[8]={0,0,0,0,0,0,0,0};
/*底层函数声明*/
void Init();
unsigned char Key_Read();
void Seg_Disp(unsigned char wei,unsigned char table,bit point);
void Led_Disp(unsigned char addr,bit enable);
void MOTOR(enable);
void Relay(enable);
void Delay10ms();
/*变量*/
unsigned int freq;
unsigned int Timer1s;
unsigned char Disp_Mode;//显示界面 0-频率界面 1-湿度界面 2-测距界面 3-参数界面
unsigned char Set_Mode;// 参数界面 0-频率参数 1-湿度参数 2-距离参数
unsigned char humid;
unsigned int distance;
unsigned char freq_set_10x=90;//频率参数 1.0K-12.0K   10-120存
unsigned char humid_set=40;//湿度参数 10-60
unsigned char distance_set_10x=6;//距离参数 0.1-1.2 1-12存
bit CM_Mode=0;//cm-m标志位
bit Hz_Mode=0;//Hz-KHz标志位
unsigned int freq_khz_10x;
bit Long_Flag;//长按标志位
unsigned int Timer1000ms;
unsigned char Relay_Count=0;//继电器开关次数
bit Relay_Flag=0;
float dac_dat;
unsigned char da_val;
unsigned char PWM_Level;//PWM占空比
unsigned char PWM_Count;
bit Star_Flag=0;//闪烁标志位
unsigned char Timer100ms=0;
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
		Disp_Mode=(Disp_Mode+1)%4;
		Set_Mode=0;
		break;
		case 5://参数界面切换
		Set_Mode=(Set_Mode+1)%3;	
		break;
		case 6://+
		if(Disp_Mode==3)
		{
			if(Set_Mode==0)//在频率参数下
			{
				freq_set_10x+=5;
				if(freq_set_10x>120)freq_set_10x=10;
			}
			else if(Set_Mode==1)//在湿度参数下
			{
				humid_set+=10;
				if(humid_set>60)humid_set=10;
			}
			else if(Set_Mode==2)
			{
				distance_set_10x++;
				if(distance_set_10x>12)distance_set_10x=1;
			}
		}
		if(Disp_Mode==2)//测距界面
			 CM_Mode^=1;
		break;
		case 7://-
		if(Disp_Mode==3)
		{
			if(Set_Mode==0)//在频率参数下
			{
				freq_set_10x-=5;
				if(freq_set_10x<10)freq_set_10x=120;
			}
			else if(Set_Mode==1)//在湿度参数下
			{
				humid_set-=10;
				if(humid_set<10)humid_set=60;
			}
			else if(Set_Mode==2)
			{
				distance_set_10x--;
				if(distance_set_10x<1)distance_set_10x=12;
			}
		}
		if(Disp_Mode==0)//测距界面
			Hz_Mode^=1;
		if(Disp_Mode==1)//湿度界面
			Long_Flag=1;
		break;
	}
//长按
	if(Key_Up==7)
{
    if(Long_Flag && Timer1000ms > 1000)
    {
        Relay_Count = 0;
        EEPROM_Write(&Relay_Count,0,1);Delay10ms();
    }
    Long_Flag = 0;
    Timer1000ms = 0;
}
	
}
/*信息处理*/
void Seg_Proc()
{
	if(Seg_Slow)return;
	Seg_Slow=1;
	//重要信息读取
	EA=0;AD_Read(0x03);humid = (unsigned char)(AD_Read(0x03) * 100.0 / 255.0 + 0.5);EA=1;
	if(humid>99) humid = 99;//限制湿度上限
	distance=Ultra_Wave_Data();
	freq_khz_10x=freq/100;
	switch(Disp_Mode)
	{
		case 0://频率界面
		Seg_Buf[0]=12;//F
		Seg_Buf[1]=10;
		Seg_Point[6]=Hz_Mode;
		if(Hz_Mode==0)
		{
		Seg_Buf[2]=freq>100000?freq/100000%10:10;
		Seg_Buf[3]=freq>10000?freq/10000%10:10;
		Seg_Buf[4]=freq>1000?freq/1000%10:10;
		Seg_Buf[5]=freq>100?freq/100%10:10;
		Seg_Buf[6]=freq>10?freq/10%10:10;
		Seg_Buf[7]=freq%10;
		}
		else
		{
		Seg_Buf[2]=freq_khz_10x>100000?freq_khz_10x/100000%10:10;
		Seg_Buf[3]=freq_khz_10x>10000?freq_khz_10x/10000%10:10;
		Seg_Buf[4]=freq_khz_10x>1000?freq_khz_10x/1000%10:10;
		Seg_Buf[5]=freq_khz_10x>100?freq_khz_10x/100%10:10;
		Seg_Buf[6]=freq_khz_10x>10?freq_khz_10x/10%10:10;
		Seg_Buf[7]=freq_khz_10x%10;
		if(freq<1000)
		{
			Seg_Buf[6]=0;
			Seg_Buf[7]=freq/100%10;
		}
		}
		break;
		case 1://湿度界面
		Seg_Buf[0]=13;//H
		Seg_Buf[1]=Seg_Buf[2]=Seg_Buf[3]=Seg_Buf[4]=Seg_Buf[5]=10;
		Seg_Buf[6]=humid/10%10;
		Seg_Buf[7]=humid%10;
		Seg_Point[6]=0;
		break;
		case 2://测距界面
		Seg_Buf[0]=14;//A
		Seg_Buf[1]=Seg_Buf[2]=Seg_Buf[3]=Seg_Buf[4]=10;
		Seg_Point[5]=CM_Mode;
		if(CM_Mode==0)
		{
		Seg_Buf[5]=distance>100?distance/100%10:10;
		Seg_Buf[6]=distance>10?distance/10%10:10;
		Seg_Buf[7]=distance%10;
		}
		else 
		{
		Seg_Buf[5]=distance>100?distance/100%10:0;
		Seg_Buf[6]=distance>10?distance/10%10:0;
		Seg_Buf[7]=distance%10;
		}
		break;
		case 3://参数界面
		Seg_Buf[0]=15;//P
		Seg_Buf[1]=Set_Mode+1;
		Seg_Buf[2]=Seg_Buf[3]=Seg_Buf[4]=10;
			switch(Set_Mode)
			{
				case 0://频率参数
				Seg_Buf[5]=freq_set_10x>99?freq_set_10x/100%10:10;
				Seg_Buf[6]=freq_set_10x/10%10;
				Seg_Buf[7]=freq_set_10x%10;	
				Seg_Point[6]=1;
				break;
				case 1://湿度参数
				Seg_Buf[5]=10;
				Seg_Buf[6]=humid_set/10%10;
				Seg_Buf[7]=humid_set%10;		
				Seg_Point[6]=0;
				break;
				case 2://距离参数
				Seg_Buf[5]=10;
				Seg_Buf[6]=distance_set_10x/10%10;
				Seg_Buf[7]=distance_set_10x%10;		
				Seg_Point[6]=1;
				break;
			}
		break;
	}
}
/*其他处理*/
void Led_Proc()
{
	unsigned char i;
	//LED
	for(i=0;i<3;i++)ucLed[i] = Disp_Mode < 3?(i==Disp_Mode):(i == Set_Mode)&Star_Flag;
	ucLed[3]=(freq>freq_set_10x*100);
	ucLed[4]=(humid>humid_set);
	ucLed[5]=(distance>distance_set_10x*10);
	
	//继电器
	if(ucLed[5])
	{
		if(Relay_Flag==0)
		{
			Relay(1);
			Relay_Flag=1;
			Relay_Count++;
			EEPROM_Write(&Relay_Count,0,1);Delay10ms();
		}
	}
	else
	{
		if(Relay_Flag==1)
		{
			Relay(0);
			Relay_Flag=0;
			Relay_Count++;
			EEPROM_Write(&Relay_Count,0,1);Delay10ms();
		}
	}
	//DA输出
	if (humid <= humid_set) dac_dat = 1.0f;
	else if (humid >= 80.0f) dac_dat = 5.0f;
	else dac_dat = 1.0f + (humid - humid_set) * 4.0f / (80.0f - humid_set);
	da_val = (unsigned char)(dac_dat * 255.0f / 5.0f);
	DA_Write(da_val);	
	//MOTOR 产生方波
	PWM_Level=ucLed[3]?8:2;
}

/*定时器0初始化   ---NE555频率*/
void Timer0Init(void)		//0毫秒@12.000MHz
{
	AUXR |= 0x80;		//定时器时钟1T模式
	TMOD &= 0xF0;		//设置定时器模式
	TMOD|=0x05;
	TL0 = 0x00;		//设置定时初值
	TH0 = 0x00;		//设置定时初值
	TF0 = 0;		//清除TF0标志
}
/*定时器1初始化  --数码管显示*/
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
	TH1 = 0xFC;
    TL1 = 0x18;
	if(++Key_Slow==10)Key_Slow=0;
	if(++Seg_Slow==20)Seg_Slow=0;
	if(++Seg_Pos==8)Seg_Pos=0;
	Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],Seg_Point[Seg_Pos]);
	Led_Disp(Seg_Pos,ucLed[Seg_Pos]);
	//NE555
	if(++Timer1s==1000)
	{
		TR0=0;
		Timer1s=0;
		freq=TH0<<8|TL0;
		TH0=TL0=0;
		TR0=1;
	}
	//长按
	if(Long_Flag)
	{
		Timer1000ms++;
		if(Timer1000ms>=1000)Timer1000ms=1001;		
	}
	//闪烁
	if(++Timer100ms==100)
	{
		Star_Flag^=1;
		Timer100ms=0;
	}
	
}
/* 定时器2中断服务函数 */
void Timer2Server() interrupt 12
{
	if(++PWM_Count == 10)PWM_Count = 0;//脉冲周期为1MS
	MOTOR(PWM_Count<PWM_Level);//PWM输出
}
void Delay10ms()		//@12.000MHz
{
	unsigned char i, j;

	i = 117;
	j = 184;
	do
	{
		while (--j);
	} while (--i);
}

void main()
{
	Init();
	Timer0Init();
	Timer1Init();
	Timer2Init();
	EEPROM_Read(&Relay_Count,0,1);
	AD_Read(0x03);
	humid = (unsigned char)(AD_Read(0x03) * 100.0 / 255.0 + 0.5);
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
	P2=0xe0;P0=0xff;P2=0x00;
	P2=0xc0;P0=seg_wei[wei];P2=0x00;
	P0=seg_table[table];
	if(point)P0&=~(1<<7);
	P2=0xe0;P2=0x00;
}
void Led_Disp(unsigned char addr,bit enable)
{
	static unsigned char temp=0x00;
	if(enable)temp|=(0x01<<addr);
		else temp&=~(0x01<<addr);
	P0=~temp;P2=0x80;P2=0x00;
}
void MOTOR(enable)
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