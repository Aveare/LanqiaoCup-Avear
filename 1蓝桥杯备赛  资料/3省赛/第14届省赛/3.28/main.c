#include <STC15F2K60S2.H>
#include <ds1302.h>
#include <iic.h>
#include <onewire.h>
/*底层变量*/
unsigned char Key_Val,Key_Old,Key_Down,Key_Up;
unsigned char Key_Slow;
unsigned int Seg_Slow;
unsigned char code seg_wei[8]={0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
unsigned char code seg_table[]={0xC0,0xF9,0xA4,0xB0,0x99,0x92,0x82,0xF8,0x80,0x90,0xff,0xbf,0xC6,0x89,0x8e,0x8c,0x86,0x88};
unsigned char Seg_Pos;
unsigned char Seg_Buf[8]={10,10,10,10,10,10,10,10};
unsigned char Seg_Point[8]={0,0,0,0,0,0,0,0};
unsigned char ucLed[8]={0,0,0,0,0,0,0,0};
/*底层函数声明*/
void Init();
unsigned char Key_Read();
void Seg_Disp(unsigned char wei,unsigned char table,bit point);
void Led_Disp(unsigned char addr,bit enable);
/*变量*/
unsigned int freq;
unsigned int Timer1s;
unsigned char Disp_Mode=0;//0-时间界面 1-回显界面 2-参数界面
unsigned char Huixian_Mode;//0-温度回显 1-湿度回显 2-时间回显
unsigned char ucRtc[3]={13,03,05};
unsigned char T_Set=30;//温度参数
bit Longpress_Flag;//S9长按标志位
unsigned int Timer2s;
unsigned char Trag_Count=0;//触发次数
unsigned char Trag_Time[3];
unsigned int temperature_10x;//温度10x
unsigned int temperature_10x_max;//最大温度10x
unsigned int temperature_10x_aver;
unsigned int humidity_10x;//湿度10x
unsigned int humidity_10x_max;//最大湿度10x
unsigned int humidity_10x_aver;
unsigned int temperature_10x_old,humidity_10x_old;
unsigned int temperature_count,humidity_count;
bit Invalid_Flag;//无效数据
unsigned char Lex;//光照
bit Dark_Flag;
bit Trag_Mode;
unsigned int Timer3s;
unsigned char Disp_Old;
bit Star_Flag;
unsigned char Timer100ms;
bit L6_Flag;
/*按键处理*/
void Key_Proc()
{
	if(Key_Slow)return;
	Key_Slow=1;
	
	Key_Val=Key_Read();
	Key_Down=Key_Val&(Key_Val^Key_Old);
	Key_Up=~Key_Val&(Key_Val^Key_Old);
	Key_Old=Key_Val;
	if(!Trag_Mode)//温湿度界面下 按键无效
	{
		switch(Key_Down)
		{
			case 4://显示界面切换
			Disp_Mode=(Disp_Mode+1)%3;
			Huixian_Mode=0;//每次从时间界面切换到回显界面时，处于温度回显子界面。
			break;
			case 5://回显字节码切换
			if(Disp_Mode==1)Huixian_Mode=(Huixian_Mode+1)%3;
			break;
			case 8://+
			if(Disp_Mode==2)
			{
				T_Set++;
				if(T_Set>100)T_Set=0;
			}
			break;
		}
		//S9长短按操作
		if(Key_Down==9)Longpress_Flag=1;
		if(Key_Up==9)
		{
			if(Timer2s<2000)//短按操作
			{
					if(Disp_Mode==2)//-
				{
					T_Set--;
					if(T_Set==255)T_Set=99;
				}
			}
			else //长按操作
			{
				if(Disp_Mode==1&&Huixian_Mode==2)
				{
					Trag_Count=0;
					temperature_10x_max=0;
					temperature_10x=0;
					temperature_10x_old=0;
					temperature_count=0;
					temperature_10x_aver=0;
					humidity_10x_max=0;
					humidity_10x=0;
					humidity_10x_old=0;
					humidity_count=0;
					humidity_10x_aver=0;
				}
			}
			Longpress_Flag=Timer2s=0;
		}
	}
}
//湿度处理
float Humid_Proc()
{
	float temp;
	if(freq<=200)
	{
		temp=10.0;
		Invalid_Flag=1;
	}
	else if(freq>=2000)
	{
		temp=90.0;
		Invalid_Flag=1;
	}
	else
	{
		temp=((float)freq-200.0)/1800.0*80.0+10.0;
		Invalid_Flag=0;
	}
	return temp;
}
/*信息处理*/
void Seg_Proc()
{
	if(Seg_Slow)return;
	Seg_Slow=1;
	//重要信息读取
	Read_Rtc(ucRtc);
	Lex=AD_Read(0x41);
	if(!Trag_Mode)
	{
		if(Lex>50)Dark_Flag=1;
		else if(Lex<50||Dark_Flag)//亮->暗
		{
			Dark_Flag=0;
			Trag_Mode=1;//进入温湿度界面
			temperature_10x=read_t()*10;
			Humid_Proc();
			if(!Invalid_Flag)//有效数据
			{
				humidity_10x=Humid_Proc()*10;//触发一次温度、湿度数据采集功能
				Trag_Time[0]=ucRtc[0];
				Trag_Time[1]=ucRtc[1];//记录触发时间
				Trag_Count++;//触发次数+1
				//最大值和平均值计算
				temperature_10x_max=temperature_10x>temperature_10x_old?temperature_10x:temperature_10x_old;
				humidity_10x_max=humidity_10x>humidity_10x_old?humidity_10x:humidity_10x_old;
					L6_Flag=(Trag_Count>=2&&temperature_10x>temperature_10x_old&&humidity_10x>humidity_10x_old);
				temperature_10x_old=temperature_10x;
				humidity_10x_old=humidity_10x;
				temperature_count+=temperature_10x_old;
				humidity_count+=humidity_10x_old;
				temperature_10x_aver=temperature_count/Trag_Count;
				humidity_10x_aver=humidity_count/Trag_Count;
			}
		}
	}
//显示
	if(Trag_Mode)//温湿度界面
	{
		Seg_Buf[0]=16;//E
		Seg_Buf[1]=Seg_Buf[2]=10;
		Seg_Buf[3]=temperature_10x/100%10;
		Seg_Buf[4]=temperature_10x/10%10;
		Seg_Buf[5]=11;//-
		//若采集到的湿度数据无效，以字符 AA 代替无效的湿度数据
		if(Invalid_Flag)Seg_Buf[6]=Seg_Buf[7]=17;//A
		else
		{
			Seg_Buf[6]=humidity_10x/100%10;
			Seg_Buf[7]=humidity_10x/10%10;
		}
		Seg_Point[6]=0;
	}
	else  //正常界面
	{
		switch(Disp_Mode)
		{
			case 0://时间界面
			Seg_Buf[0]=ucRtc[0]/10%10;
			Seg_Buf[1]=ucRtc[0]%10;
			Seg_Buf[3]=ucRtc[1]/10%10;
			Seg_Buf[4]=ucRtc[1]%10;
			Seg_Buf[6]=ucRtc[2]/10%10;
			Seg_Buf[7]=ucRtc[2]%10;
			Seg_Buf[2]=Seg_Buf[5]=11;
			Seg_Point[6]=0;
			break;
			case 1://回显界面
				switch(Huixian_Mode)
				{
					case 0://温度回显
					Seg_Buf[0]=12;//C
					Seg_Buf[1]=10;
					if(Trag_Count==0)
					{
							Seg_Buf[2]=Seg_Buf[3]=Seg_Buf[4]=Seg_Buf[5]=Seg_Buf[6]=Seg_Buf[7]=10;
					}
					else
					{
						Seg_Buf[2]=temperature_10x_max/100%10;
						Seg_Buf[3]=temperature_10x_max/10%10;
						Seg_Buf[4]=11;//-
						Seg_Buf[5]=temperature_10x_aver/100%10;
						Seg_Buf[6]=temperature_10x_aver/10%10;
						Seg_Buf[7]=temperature_10x_aver%10;
						Seg_Point[6]=1;
					}
					break;
					case 1://湿度回显
					Seg_Buf[0]=13;//H
					Seg_Buf[1]=10;
					if(Trag_Count==0)
					{
						Seg_Buf[2]=Seg_Buf[3]=Seg_Buf[4]=Seg_Buf[5]=Seg_Buf[6]=Seg_Buf[7]=10;
					}
					else
					{				
						Seg_Buf[2]=humidity_10x_max/100%10;
						Seg_Buf[3]=humidity_10x_max/10%10;
						Seg_Buf[4]=11;//-
						Seg_Buf[5]=humidity_10x_aver/100%10;
						Seg_Buf[6]=humidity_10x_aver/10%10;
						Seg_Buf[7]=humidity_10x_aver%10;
						Seg_Point[6]=1;
					}
					break;
					case 2://时间回显
					Seg_Buf[0]=14;//F	
					Seg_Buf[1]=Trag_Count/10%10;
					Seg_Buf[2]=Trag_Count%10;
					if(Trag_Count==0)
					{
						Seg_Buf[3]=Seg_Buf[4]=Seg_Buf[5]=Seg_Buf[6]=Seg_Buf[7]=10;
					}
					else
					{
						Seg_Buf[3]=Trag_Time[0]/10%10;
						Seg_Buf[4]=Trag_Time[0]%10;
						Seg_Buf[5]=11;//-
						Seg_Buf[6]=Trag_Time[1]/10%10;
						Seg_Buf[7]=Trag_Time[1]%10;
					}
					Seg_Point[6]=0;
					break;
				}
			break;
			case 2://参数界面
			Seg_Buf[0]=15;//P		
			Seg_Buf[1]=Seg_Buf[2]=Seg_Buf[3]=Seg_Buf[4]=Seg_Buf[5]=10;		
			Seg_Buf[6]=T_Set/10%10;
			Seg_Buf[7]=T_Set%10;
			Seg_Point[6]=0;
			break;
		}
	}
}

/*其他处理*/
void Led_Proc()
{
	ucLed[0]=Trag_Mode?0:(Disp_Mode==0);
	ucLed[1]=Trag_Mode?0:(Disp_Mode==1);
	ucLed[2]=Trag_Mode;
	ucLed[3]=(temperature_10x/10>T_Set)?Star_Flag:0;
	ucLed[4]=Invalid_Flag;
	ucLed[5]=L6_Flag;
	ucLed[6]=ucLed[7]=0;
}
/*定时器0初始化函数 NE555*/
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
/*定时器1初始化函数  数码管扫描*/
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
/*定时器1中断服务函数*/
void Timer1server(void) interrupt 3
{
	if(++Key_Slow==10)Key_Slow=0;
	if(++Seg_Slow==400)Seg_Slow=0;
	if(++Seg_Pos==8)Seg_Pos=0;
	Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],Seg_Point[Seg_Pos]);
	Led_Disp(Seg_Pos,ucLed[Seg_Pos]);
	//NE555
	if(++Timer1s==1000)
	{
		freq=TH0<<8|TL0;
		TH0=TL0=0;
		Timer1s=0;
	}
	//S9长按
	if(Longpress_Flag)
	{
		Timer2s++;
		if(Timer2s>=2000)Timer2s=2001;
	}
	//温湿度3s显示
	if(Trag_Mode)
	{
		Timer3s++;
		if(Timer3s>=3000)
		{
			Timer3s=0;
			Trag_Mode=0;
		}
	}
	//闪烁
		if(++Timer100ms==100)
		{
			Star_Flag^=1;
			Timer100ms=0;
		}
}
/*Main*/
void main()
{
	Init();
	Timer0Init();
	Timer1Init();
	Set_Rtc(ucRtc);
	while(read_t()==85);	
	while(1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
	}
	
}
void Init()

{
	P2=0xa0;P0=0x00;P2=0x00;
	P2=0x80;P0=0xff;P2=0x00;
}

unsigned char Key_Read()
{
	unsigned char temp;
	P44=0;P42=1;P35=1;
	if(P33==0)temp=4;
	if(P32==0)temp=5;
	P44=1;P42=0;P35=1;
	if(P33==0)temp=8;
	if(P32==0)temp=9;
	return temp;
}

void Seg_Disp(unsigned char wei,unsigned char table,bit point)
{
	P2=0xe0;P0=0x00;P0=0x00;
	P2=0xc0;P0=seg_wei[wei];P2=0x00;
	P2=0xe0;
	P0=seg_table[table];
	if(point)P0&=~(1<<7);
	P2=0x00;
}
void Led_Disp(unsigned char addr,bit enable)
{
	static unsigned char temp=0x00;
	if(enable)temp|=(0x01<<addr);
	else temp&=~(0x01<<addr);
	P0=~temp;P2=0x80;P2=0x00;
}