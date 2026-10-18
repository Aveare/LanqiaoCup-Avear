/*头文件引用*/
#include <STC15F2K60S2.H>
#include <Seg.h>
#include <Key.h>
#include <Led.h>
#include <ds1302.h>
#include <iic.h>
#include <onewire.h>
#include <Init.h>
/*变量定义*/
unsigned char Key_Val,Key_Old,Key_Down,Key_Up;
unsigned char Key_Slow_Down;
unsigned int Seg_Slow_Down;
unsigned char Seg_Pos;
unsigned char Seg_Buf[8]={10,10,10,10,10,10,10,10};
unsigned char Seg_Point[8]={0,0,0,0,0,0,0,0};
unsigned char ucLed[8]={0,0,0,0,0,0,0,0};
unsigned int Timer1000ms;//1s定时
long Freq;//频率
long Max;//最大频率
unsigned char Disp_Mode=0;//0-频率界面 1-参数界面 2-时间界面 3-回显界面
bit S5_Flag;//切换超限参数和校准值  0-超限参数 1-校准值
unsigned int Chaoxian=2000;//超限参数1000-9000hz
int jiaozhun=0;//校准值-900~900hz

unsigned char ucRtc[3]={0x13,0x03,0x05};
unsigned char ucRtc_Set[6];
bit Huixian_Flag;//0-频率回显 1-时间回显
unsigned char Timer200ms;
bit Star_Flag;
bit Error_Flag;
/*按键处理函数*/
void Key_Proc()
{
	if(Key_Slow_Down)return;
	Key_Slow_Down=1;
	
	Key_Val=Key_Read();
	Key_Down=Key_Val&(Key_Old^Key_Val);
	Key_Up=~Key_Val&(Key_Old^Key_Val);
	Key_Old=Key_Val;
	
	switch(Key_Down)
	{
		case 4://显示界面切换
			Disp_Mode=(Disp_Mode+1)%4;
		if(Disp_Mode==1)S5_Flag=0;//从频率界面切换到参数界面 处于超限参数界面
		if(Disp_Mode==3)Huixian_Flag=0;//从时间界面切换到回显界面 处于频率回显子界面
		break;
		case 5://选择
			if(Disp_Mode==1)//在参数界面下 按下切换超限参数和校准值参数
				S5_Flag^=1;
			if(Disp_Mode==3)//在回显界面下 按下切换频率回显和时间回显
				Huixian_Flag^=1;
		break;
		case 8://+
			if(Disp_Mode==1&&S5_Flag==0)
			{
				Chaoxian+=1000;
				if(Chaoxian>9000)Chaoxian=9000;//设置上限
			}
			else if(Disp_Mode==1&&S5_Flag)
			{
				jiaozhun+=100;
				if(jiaozhun>900)jiaozhun=900;
			}
		break;
		case 9://-
			if(Disp_Mode==1&&S5_Flag==0)
			{
				Chaoxian-=1000;
				if(Chaoxian<1000)Chaoxian=1000;//设置上限
			}
			else if(Disp_Mode==1&&S5_Flag)
			{
				jiaozhun-=100;
				if(jiaozhun<-900)jiaozhun=-900;
			}
		break;
	}
}

/*信息处理函数*/
void Seg_Proc()
{
		if(Seg_Slow_Down)return;
		Seg_Slow_Down=1;
	
	Read_Rtc(ucRtc);//读取时间
	if(Freq<0)Error_Flag;
	switch(Disp_Mode)
	{
		case 0://频率界面
			Seg_Buf[0]=12;//F
			Seg_Buf[1]=Seg_Buf[2]=10;//熄灭
			Seg_Buf[3]=Freq>=10000?Freq/10000:10;
			Seg_Buf[4]=Freq>=1000?Freq/1000:10;
			Seg_Buf[5]=Freq>=100?Freq/100:10;
			Seg_Buf[6]=Freq>=10?Freq/10:10;
			Seg_Buf[7]=Freq/10;
		break;
		case 1://参数界面
			Seg_Buf[0]=13;//P
			Seg_Buf[1]=(unsigned char)S5_Flag+1;
			Seg_Buf[2]=Seg_Buf[3]=10;
		if(S5_Flag==0)//超限参数
		{
			Seg_Buf[4]=Chaoxian/1000%10;
			Seg_Buf[5]=Chaoxian/100%10;
			Seg_Buf[6]=Chaoxian/10%10;
			Seg_Buf[7]=Chaoxian%10;
		}
		else //校准值
		{
			if(jiaozhun>=0)//正数
				Seg_Buf[4]=10;
			else//负数
				Seg_Buf[4]=11;//-	
			
			Seg_Buf[5]=(unsigned int)jiaozhun>99?(unsigned int)jiaozhun/100%10:10;
			Seg_Buf[6]=(unsigned int)jiaozhun>9?(unsigned int)jiaozhun/10%10:10;
			Seg_Buf[7]=(unsigned int)jiaozhun%10;
		}
		break;
		case 2://时间界面
			Seg_Buf[0]=ucRtc[0]/16;
			Seg_Buf[1]=ucRtc[0]%16;
			Seg_Buf[3]=ucRtc[1]/16;
			Seg_Buf[4]=ucRtc[1]%16;
			Seg_Buf[6]=ucRtc[2]/16;
			Seg_Buf[7]=ucRtc[2]%16;
		Seg_Buf[2]=Seg_Buf[5]=11;//-
		break;
		case 3://回显界面
			Seg_Buf[0]=14;//H
			Seg_Buf[2]=10;//熄灭
			if(Huixian_Flag==0)//频率回显
			{
					Seg_Buf[1]=12;//F
					Seg_Buf[2]=10;//熄灭
					Seg_Buf[3]=Max>10000?Max/10000:10;
					Seg_Buf[4]=Max>1000?Max/1000:10;
					Seg_Buf[5]=Max>100?Max/100:10;
					Seg_Buf[6]=Max>10?Max/10:10;
					Seg_Buf[7]=Max>1?Max:10;
			}
			else //时间回显
			{
				Seg_Buf[1]=15;//A
				Seg_Buf[2]=ucRtc_Set[0];
				Seg_Buf[3]=ucRtc_Set[1];
				Seg_Buf[4]=ucRtc_Set[2];
				Seg_Buf[5]=ucRtc_Set[3];
				Seg_Buf[6]=ucRtc_Set[4];
				Seg_Buf[7]=ucRtc_Set[5];
			}
		break;
	}
}
/*其他处理函数*/
void Led_Proc()
{
	if(Disp_Mode==0)ucLed[0]=Star_Flag;
	if(Freq>Chaoxian)ucLed[1]=Star_Flag;
}	
/*定时器0初始化*/
void Timer0Init(void)		//0微秒@12.000MHz
{
	AUXR &= 0x7F;		//定时器时钟12T模式
	TMOD &= 0xF0;		//设置定时器模式
	TMOD|=0x05;
	TL0 = 0x00;		//设置定时初值
	TH0 = 0x00;		//设置定时初值
	TF0 = 0;		//清除TF0标志
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



/*定时器1中断服务函数*/
void Timer1Server() interrupt 3
{
	if(++Key_Slow_Down==10)Key_Slow_Down=0;
	if(++Seg_Slow_Down==500)Seg_Slow_Down=0;
	if(++Seg_Pos==8)Seg_Pos=0;
	Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],Seg_Point[Seg_Pos]);
	Led_Disp(Seg_Pos,ucLed[Seg_Pos]);
	
	if(++Timer1000ms==1000)
	{
		TR0=0;
		Timer1000ms=0;
		Freq=(TH0<<8|TL0)+jiaozhun;
		Max=Max>Freq?Max:Freq;//获取最大值
		if(Max>Freq)
		{
			ucRtc_Set[0]=ucRtc[0]/16;
			ucRtc_Set[1]=ucRtc[0]%16;
			ucRtc_Set[2]=ucRtc[1]/16;
			ucRtc_Set[3]=ucRtc[1]%16;
			ucRtc_Set[4]=ucRtc[2]/16;
			ucRtc_Set[5]=ucRtc[2]%16;//获取时间
		}
		TH0=TL0=0;
		TR0=1;
	}
	if(++Timer200ms==200)
	{
		Timer200ms=0;
		Star_Flag^=1;
	}
}

void main()
{
	System_Init();
	 Timer0Init();
	Timer1Init();
	Set_Rtc(ucRtc);
	while(1)
		{
			Key_Proc();
			Seg_Proc();
			Led_Proc();
		}
}