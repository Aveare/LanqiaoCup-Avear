#include <STC15F2K60S2.H>
#include <ds1302.h>
#include <iic.h>
#include <ultra.h>
/*底层变量*/
unsigned char Key_Slow;
unsigned int Seg_Slow;
unsigned char Key_Val,Key_Down,Key_Up,Key_Old;
unsigned char Seg_Pos;
unsigned char code seg_wei[8]={0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
unsigned char code seg_table[]={0xC0,0xF9,0xA4,0xB0,0x99,0x92,0x82,0xF8,0x80,0x90,0xff,0xbf,0xc7,0x8e,0xc6,0x89,0xFE,0xBF,0xF7,0x8c};
unsigned char Seg_Buf[8]={10,10,10,10,10,10,10,10};
unsigned char Seg_Point[8]={0,0,0,0,0,0,0,0};
unsigned char ucLed[8]={0,0,0,0,0,0,0,0};
/*底层函数声明*/
void Init();
unsigned char Key_Read();
void Seg_Disp(unsigned char wei,unsigned char table,bit point);
void Led_Disp(unsigned char addr,bit enable);
/*变量*/
unsigned char Disp_Mode=0;//0-时间数据 1-测距数据 2-数据记录
bit Sys_Mode=0;//0-数据显示 1-参数设置
bit Set_Mode=0;//0-采集时间参数 1-距离参数
unsigned char ucRtc[3]={00,00,01};
bit Ut_Mode=1;//超声波模式 0-定时F 1-触发C
unsigned int distance;
unsigned char distance_mode[3]={16,17,18};//最大值，平均值，最小值
unsigned char distance_mode_index=0;
unsigned int distance_old;
unsigned int distance_max,distance_min;//最大最小值
float distance_count;//总值
float distance_average;//平均值
unsigned char Time_Set=2;//采集时间参数
unsigned char Time_Set_Control[5]={2,3,5,7,9};
unsigned char Time_Set_Control_Index=0;
unsigned char Distace_Set=20;//距离参数
unsigned char Distace_Set_Control=20;
float Vol_Output;//电压输出
bit Gather_Flag;//采集标志位
unsigned char Gather_Count=0;
unsigned char Nearby_Count=0;
bit Nearby_Flag;
unsigned char Lex;//光敏电阻采集值
bit Light_Flag;
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
		case 4://界面切换
		Sys_Mode^=1;
		Set_Mode=Disp_Mode=0;
		if(!Sys_Mode)//退出参数界面  参数保存生效
		{
			Time_Set=Time_Set_Control[Time_Set_Control_Index];
			Distace_Set=Distace_Set_Control;
		}
		break;
		case 5://显示界面切换
		if(Sys_Mode==0)Disp_Mode=(Disp_Mode+1)%3;
		else Set_Mode^=1;
		distance_mode_index=0;
		break;
		case 8:
		if(Sys_Mode==0&&Disp_Mode==1)
			Ut_Mode^=1;	//测距数据显示界面下 超声波触发模式切换
	  if(Sys_Mode==0&&Disp_Mode==2)
			distance_mode_index=(distance_mode_index+1)%3;//数据记录界面下。切换最大最小平均值
		break;
		case 9://参数调整
			if(Sys_Mode==1&&Set_Mode==0)//时间参数
				if(++Time_Set_Control_Index==5)Time_Set_Control_Index=0;
		  if(Sys_Mode==1&&Set_Mode==1)//距离参数
			{
				Distace_Set_Control+=10;
				if(Distace_Set_Control>80)Distace_Set_Control=10;
			}
		break;
	}
}

/*信息处理*/
void Seg_Proc()
{
	if(Seg_Slow)return;
	Seg_Slow=1;
	//重要信息读取
	//定时判断采集
	Read_Rtc(ucRtc);
	if(Ut_Mode==0&&ucRtc[2]%Time_Set==0)Gather_Flag=1;
	//光照判断亮暗
	Lex=AD_Read(1);
	if(Lex>=50)Light_Flag=1;
	else if(Lex<50&&Light_Flag)//亮->暗
	{
		Light_Flag=0;
		if(Ut_Mode)Gather_Flag=1;//触发采集
	}
	//距离数据采集
	if(Gather_Flag)
	{
		Gather_Count++;
		distance_old=distance;//旧值读取，方便比较大小
		distance=Ut_Wave_Data();//读取值
		if(distance>distance_old)distance_max=distance;//Max
		if(distance<distance_old)distance_min=distance;//Min
		distance_count+=distance;
		distance_average=(float)distance_count/(float)Gather_Count;//平均值
		//附近判断
		if(distance>=Distace_Set-5&&distance<Distace_Set+5)Nearby_Count++;
		else Nearby_Count=Nearby_Flag=0;
		Nearby_Flag=(Nearby_Count>=3);
		Gather_Flag=0;
	}
// 数码管显示
	if(Sys_Mode==0)//0-数据显示
	{
		switch(Disp_Mode)
		{
			case 0://时间显示界面
			Seg_Buf[0]=ucRtc[0]/10%10;
			Seg_Buf[1]=ucRtc[0]%10;
			Seg_Buf[3]=ucRtc[1]/10%10;
			Seg_Buf[4]=ucRtc[1]%10;
			Seg_Buf[6]=ucRtc[2]/10%10;
			Seg_Buf[7]=ucRtc[2]%10;
			Seg_Buf[2]=Seg_Buf[5]=11;//-
			Seg_Point[6]=0;
			break;
			case 1://距离数据显示
			Seg_Buf[0]=12;//L
			Seg_Buf[1]=(Ut_Mode==0)?13:14;
			Seg_Buf[2]=Seg_Buf[3]=Seg_Buf[4]=10;
			Seg_Buf[5]=distance>=100?distance/100%10:10;
			Seg_Buf[6]=distance>=10?distance/10%10:10;
			Seg_Buf[7]=distance%10;
			Seg_Point[6]=0;
			break;
			case 2://数据记录
			Seg_Buf[0]=15;//H	
			Seg_Buf[1]=distance_mode[distance_mode_index];
			Seg_Buf[2]=Seg_Buf[3]=10;
			switch(distance_mode_index)
			{
				case 0://最大值
				Seg_Buf[4]=distance_max>=1000?distance_max/1000%10:10;
				Seg_Buf[5]=distance_max>=100?distance_max/100%10:10;
				Seg_Buf[6]=distance_max>=10?distance_max/10%10:10;
				Seg_Buf[7]=distance_max%10;
				break;
				case 1://平均值
				Seg_Buf[4]=(unsigned int)distance_average>=100?(unsigned int)distance_average/100%10:10;
				Seg_Buf[5]=(unsigned int)distance_average>=10?(unsigned int)distance_average/10%10:10;
				Seg_Buf[6]=(unsigned int)distance_average%10;
				Seg_Buf[7]=(unsigned int)(distance_average*10)/10%10;
				Seg_Point[6]=1;
				break;
				case 2://最小值
				Seg_Buf[4]=distance_min>=1000?distance_min/1000%10:10;
				Seg_Buf[5]=distance_min>=100?distance_min/100%10:10;
				Seg_Buf[6]=distance_min>=10?distance_min/10%10:10;
				Seg_Buf[7]=distance_min%10;	
				Seg_Point[6]=0;
				break;
			}
			break;
		}
	}
	else  //1-参数设置
	{
		Seg_Buf[0]=19;//P
		Seg_Buf[1]=(unsigned char)Set_Mode+1;
		Seg_Buf[2]=Seg_Buf[3]=Seg_Buf[4]=Seg_Buf[5]=10;
		if(Set_Mode==0)
		{
			Seg_Buf[6]=Time_Set_Control[Time_Set_Control_Index]/10%10;
			Seg_Buf[7]=Time_Set_Control[Time_Set_Control_Index]%10;
		}
		else
		{
			Seg_Buf[6]=Distace_Set_Control/10%10;
			Seg_Buf[7]=Distace_Set_Control%10;
		}
		Seg_Point[6]=0;
	}
}

/*其他处理*/
void Led_Proc()
{
	unsigned char i;
	//DA输出
	if(distance<=10)Vol_Output=1.0;
	else if(distance>=80)Vol_Output=5.0;
	else Vol_Output=1.0f+(distance-10)/70.0*4.0;
	DA_Write(Vol_Output*51);
	//Led
	for(i=0;i<3;i++)ucLed[i]=(Sys_Mode==0&&Disp_Mode==i);//互斥点亮
	ucLed[3]=Ut_Mode;
	ucLed[4]=(Ut_Mode==0&&Nearby_Flag);
	ucLed[5]=(Lex>=50);
}

/*定时器0初始化函数*/
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

void Timer0server(void) interrupt 1
{
	if(++Key_Slow==10)Key_Slow=0;
	if(++Seg_Slow==199)Seg_Slow=0;
	if(++Seg_Pos==8)Seg_Pos=0;
	Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],Seg_Point[Seg_Pos]);
	Led_Disp(Seg_Pos,ucLed[Seg_Pos]);
}

/*Main*/
void main()
{
	Init();
	Timer0Init();
	Set_Rtc(ucRtc);
	Lex=AD_Read(0x41);//上电初始化亮度，防止上电读暗
	Light_Flag=(Lex>=50);
	while(1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
	}
}
/*初始化关闭led和外设*/
void Init()
{
	P2=0x80;P0=0x00;P2=0x00;
	P2=0xa0;P0=0xff;P0=0x00;
}
/*按键读取*/
unsigned char Key_Read()
{
	unsigned char temp=0;
	ET0=0;
	P44=0;P42=1;P35=1;P34=1;
	if(P33==0)temp=4;
	if(P32==0)temp=5;
	P44=1;P42=0;P35=1;P34=1;
	if(P33==0)temp=8;
	if(P32==0)temp=9;
	ET0=1;
	return temp;
}
void Seg_Disp(unsigned char wei,unsigned char table,bit point)
{
	P2=0xe0;P0=0x00;P2=0x00;
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
	P2=0x80;P0=~temp;P2=0x00;
}