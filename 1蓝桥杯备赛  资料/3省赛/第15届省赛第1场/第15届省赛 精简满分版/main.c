/*头文件引用*/
#include <STC15F2K60S2.H>
#include <Key.h>
#include <Led.h>
#include <ds1302.h>
#include <iic.h>
#include <onewire.h>
#include <math.h>
/*变量定义*/
unsigned char Key_Val,Key_Old,Key_Down,Key_Up;
unsigned char Key_Slow_Down;
unsigned int Seg_Slow_Down;
unsigned char Seg_Pos;
unsigned char Seg_Buf[8]={10,10,10,10,10,10,10,10};
unsigned char Seg_Point[8]={0,0,0,0,0,0,0,0};

unsigned char seg_table[]={0xc0,0xf9,0xa4,0xb0,0x99,0x92,0x82,0xf8,0x80,0x90,0xff,0xbf,0x8e,0x8c,0x89,0x88,0xc7};
unsigned char seg_wei[] = {0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};


unsigned char ucLed[8]={0,0,0,0,0,0,0,0};
unsigned int Timer1000ms;//1s定时
long int Freq;//频率
long int Max;//最大频率

unsigned char Disp_Mode=0;//0-频率界面 1-参数界面 2-时间界面 3-回显界面
bit Set_Flag;//切换超限参数和校准值  0-超限参数 1-校准值
int Set_Arr[2]={2000,0}; //0-超限参数1000-9000hz 1-校准值-900~900hz

int code Step_Arr[2]={1000,100};//单次+/-
int code Max_Arr[2]={9000,900};//上限
int code Min_Arr[2]={1000,-900};//下限
//unsigned int Chaoxian=2000;//超限参数1000-9000hz
// int jiaozhun=0;//校准值-900~900hz

unsigned char ucRtc[3]={13,03,05};
unsigned char Max_Time[6];
bit Huixian_Flag;//0-频率回显 1-时间回显
unsigned char Timer200ms;
bit Star_Flag;
bit Error_Flag;

float Output;//DA输出

/*初始化*/
void Init()
{
	P2=0xa0;P0=0x00;P2=0x00;
	P2=0x80;P0=0xff;P2=0x00;
}
/*按键读取*/
unsigned char Key_Read()//矩阵按键
{
	unsigned char temp = 0;
	P44 = 0;P42 = 1;P35 = 1;P34 = 1;
	if(P33 == 0) temp = 4;
	if(P32 == 0) temp = 5;
	if(P31 == 0) temp = 6;
	if(P30 == 0) temp = 7;
	P44 = 1;P42 = 0;P35 = 1;P34 = 1;
	if(P33 == 0) temp = 8;
	if(P32 == 0) temp = 9;
	if(P31 == 0) temp = 10;
	if(P30 == 0) temp = 11;
	P44 = 1;P42 = 1;P35 = 0;P34 = 1;
	if(P33 == 0) temp = 12;
	if(P32 == 0) temp = 13;
	if(P31 == 0) temp = 14;
	if(P30 == 0) temp = 15;
	
	return temp;
}
/*数码管刷新*/
void Seg_Disp(unsigned char wei,table,point)
{
    P2=0xe0;P0=0x00;P2=0x00;          //消影
    P2=0xc0;P0=seg_wei[wei];P2=0x00;//位选
    P0=seg_table[table];              //段选码
	if(point) P0 &= ~(1 << 7);       //加小数点
    P2=0xe0;P2=0x00;               //开段码显示
}
/*Led显示*/
void Led_Disp(unsigned char addr,enable)
{
    static unsigned char temp = 0x00;
    if(enable) temp |= 0x01 << addr;
	else temp &=~(0x01 << addr);
    P0 = ~temp;P2 = 0x80;P2 = 0x00;
}
/*蜂鸣器*/
void Beep(unsigned char enable)
{
    static unsigned char temp = 0x00;
    if(enable)temp |= 0x40;
    else temp &= ~0x40;
    P0 = temp;P2 = 0xa0;P2 = 0x00;
}
/*继电器*/
void Relay(unsigned char enable)
{
    static unsigned char temp = 0x00;
    if(enable)temp |= 0x10;
    else temp &= ~0x10;
	P0 = temp;P2 = 0xa0;P2 = 0x00;
}
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
		if(Disp_Mode==1)Set_Flag=0;//从频率界面切换到参数界面 处于超限参数界面
		if(Disp_Mode==3)Huixian_Flag=0;//从时间界面切换到回显界面 处于频率回显子界面
		break;
		case 5://选择
			if(Disp_Mode==1)//在参数界面下 按下切换超限参数和校准值参数
				Set_Flag^=1;
			else if(Disp_Mode==3)//在回显界面下 按下切换频率回显和时间回显
				Huixian_Flag^=1;
		break;
		case 8://+
		if(Disp_Mode==1)
		{
			Set_Arr[Set_Flag]+=Step_Arr[Set_Flag];//+
			if(Set_Arr[Set_Flag]>=Max_Arr[Set_Flag])
				Set_Arr[Set_Flag]=Max_Arr[Set_Flag];//限制上限
		}
		break;
		case 9://-
			Set_Arr[Set_Flag]-=Step_Arr[Set_Flag];//-
			if(Set_Arr[Set_Flag]<=Min_Arr[Set_Flag])
				Set_Arr[Set_Flag]=Min_Arr[Set_Flag];//限制下限
		break;
	}
}

/*信息处理函数*/
void Seg_Proc()
{
	if(Seg_Slow_Down)return;
	Seg_Slow_Down=1;
	
	Read_Rtc(ucRtc);//读取时间
	if(Max<Freq)
	{
		Max=Freq;               //获取回显频率
		Max_Time[0]=ucRtc[0]/10;
		Max_Time[1]=ucRtc[0]%10;
		Max_Time[2]=ucRtc[1]/10;
		Max_Time[3]=ucRtc[1]%10;
		Max_Time[4]=ucRtc[2]/10;
		Max_Time[5]=ucRtc[2]%10;//获取回显时间
	}
	switch(Disp_Mode)
	{
		case 0://频率界面
			Seg_Buf[0]=12;//F
			Seg_Buf[1]=Seg_Buf[2]=10;//熄灭
		if(Freq>=0)
		{
			Seg_Buf[3]=Freq>=10000?Freq/10000%10:10;
			Seg_Buf[4]=Freq>=1000?Freq/1000%10:10;
			Seg_Buf[5]=Freq>=100?Freq/100%10:10;
			Seg_Buf[6]=Freq>=10?Freq/10%10:10;
			Seg_Buf[7]=Freq%10;
		}
		else
		{
			Seg_Buf[5]=Seg_Buf[4]=Seg_Buf[3]=10;//熄灭
			Seg_Buf[6]=Seg_Buf[7]=16;
		}
		/* 高位熄灭另一位写法
		unsigned char i=3;
		Seg_Buf[3]=Freq/10000%10;
		Seg_Buf[4]=Freq/1000%10;
		Seg_Buf[5]=Freq/100%10;
		Seg_Buf[6]=Freq/10%10;
		Seg_Buf[7]=Freq%10;
		while(Seg_Buf[i]==0)
		{
			Seg_Buf[i]=10;
			if(++i==7)break;
		}
		*/
		break;
		case 1://参数界面
			Seg_Buf[0]=13;//P
			Seg_Buf[1]=(unsigned char)Set_Flag+1;
			Seg_Buf[2]=Seg_Buf[3]=10;
		Seg_Buf[4]=Set_Flag?(10+(Set_Arr[Set_Flag]<0)):abs(Set_Arr[Set_Flag])/1000%10;//正数10熄灭 负数11-
			Seg_Buf[5]=abs(Set_Arr[Set_Flag])/100%10;
			Seg_Buf[6]=abs(Set_Arr[Set_Flag])/10%10;
			Seg_Buf[7]=abs(Set_Arr[Set_Flag])%10;
		if(Set_Arr[Set_Flag]==0)Seg_Buf[5]=Seg_Buf[6]=10;//校准值为0，只显示0
		
		/*if(Set_Flag==0)//超限参数
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
		}*/
		break;
		case 2://时间界面
			Seg_Buf[0]=ucRtc[0]/10;
			Seg_Buf[1]=ucRtc[0]%10;
			Seg_Buf[3]=ucRtc[1]/10;
			Seg_Buf[4]=ucRtc[1]%10;
			Seg_Buf[6]=ucRtc[2]/10;
			Seg_Buf[7]=ucRtc[2]%10;
		Seg_Buf[2]=Seg_Buf[5]=11;//-
		break;
		case 3://回显界面
			Seg_Buf[0]=14;//H
			Seg_Buf[2]=10;//熄灭
			if(Huixian_Flag==0)//频率回显
			{
				Seg_Buf[1]=12;//F
				Seg_Buf[2]=10;//熄灭
				Seg_Buf[3]=Max>10000?Max/10000%10:10;
				Seg_Buf[4]=Max>1000?Max/1000%10:10;
				Seg_Buf[5]=Max>100?Max/100%10:10;
				Seg_Buf[6]=Max>10?Max/10%10:10;
				Seg_Buf[7]=Max>1?Max%10:10;
			}
			else //时间回显
			{
				Seg_Buf[1]=15;//A
				Seg_Buf[2]=Max_Time[0];
				Seg_Buf[3]=Max_Time[1];
				Seg_Buf[4]=Max_Time[2];
				Seg_Buf[5]=Max_Time[3];
				Seg_Buf[6]=Max_Time[4];
				Seg_Buf[7]=Max_Time[5];
			}
		break;
	}
}
/*其他处理函数*/
void Led_Proc()
{
	float k;
	float Vout;
	if(Freq<0)DA_Write(0);
	else if(Freq>=0&&Freq<=500)DA_Write(1*51);
	else if(Freq>=Set_Arr[0])DA_Write(5*51);
	else
    {
        k = (float)(Freq - 500) / (Set_Arr[0] - 500);//计算频率在500Hz~Pf区间内的占比（0~1）
        Vout = 1.0f + 4.0f * k;                      //根据占比计算输出电压（1V~5V线性变化）
        DA_Write((unsigned char)(Vout * 51));        //转换为DA的数字量（51 = 255/5，对应1V对应51，5V对应255）
    }
	ucLed[0]=Star_Flag&(!Disp_Mode);
	ucLed[1]=(Star_Flag&(Freq>Set_Arr[0]))||(Freq<0);
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
	if(++Seg_Slow_Down==80)Seg_Slow_Down=0;
	

	if(++Seg_Pos==8)Seg_Pos=0;
	Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],Seg_Point[Seg_Pos]);
	Led_Disp(Seg_Pos,ucLed[Seg_Pos]);
	
	if(++Timer1000ms==1000)
	{
		TR0=0;
		Timer1000ms=0;
		Freq=(TH0<<8|TL0);
		Freq+=Set_Arr[1];//加校准值
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
	Init();
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