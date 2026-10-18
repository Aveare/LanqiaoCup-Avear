#include <STC15F2K60S2.H>
#include <onewire.h>
#include <iic.h>
#include <ultra.h>
#include <math.h>

/*底层变量*/
unsigned char Key_Slow;
unsigned int Seg_Slow;
unsigned char Seg_Pos;
unsigned char Key_Val,Key_Down,Key_Up,Key_Old;
unsigned char code seg_wei[8]={0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
unsigned char code seg_table[]={0xC0,0xF9,0xA4,0xB0,0x99,0x92,0x82,0xF8,0x80,0x90,0xFF,0xBF,0xC6,0xC8,0xC7,0x8C};
unsigned char Seg_Buf[8]={10,10,10,10,10,10,10,10};
unsigned char Seg_Point[8]={0,0,0,0,0,0,0,0};
unsigned char ucLed[8]={0,0,0,0,0,0,0,0};
unsigned int Move_Slow;

/*底层函数声明*/
void Init();
unsigned char Key_Read();	
void Seg_Disp(unsigned char wei,unsigned char table,bit point);
void Led_Disp(unsigned char addr,bit enable);
void Relay(bit enable);

/*变量*/
unsigned char Disp_Mode=0;
bit Set_Mode;
unsigned char T_Set=30,D_Set=30;
float temperature;
bit High_Temperature_Flag;
unsigned char Lex;
unsigned char Lex_Level;
unsigned char distance[2];
unsigned char last_distance;
unsigned char prev_distance;
unsigned char delta;
bit Nearby_Flag;
unsigned char Work_Mode=0;          /* 0-静止 1-徘徊 2-跑动 */
unsigned char Work_Mode_Locked;     /* 锁定状态保存 */
unsigned int Relay_Count;
bit Count_Flag=0;
bit Long_Press_Flag=0;
unsigned int Timer2s;
bit Lock_Flag;
unsigned char Lock_Cnt=0;
bit Star_Flag;
unsigned char Timer100ms;
bit Relay_Flag;
unsigned char ucLed_Lock[8];
unsigned char Relay_State;
unsigned char L7_State;
bit L7_Star;

/*按键处理*/
void Key_Proc()
{
	if(Key_Slow)return;
	Key_Slow=1;
	Key_Val=Key_Read();
	Key_Down=Key_Val&(Key_Val^Key_Old);
	Key_Up=~Key_Val&(Key_Val^Key_Old);
	Key_Old=Key_Val;

	if(Key_Val==89)Long_Press_Flag=1;
	else Long_Press_Flag=Timer2s=0;

	switch(Key_Down)
	{
		case 4:
		Disp_Mode=(Disp_Mode+1)%4;
		Set_Mode=0;
		if(Disp_Mode==2)
		{
			if(Work_Mode==0) {L7_State=0; L7_Star=0;}
			else if(Work_Mode==1) {L7_State=1; L7_Star=0;}
			else {L7_Star=1;}
		}
		break;

		case 5:
		Set_Mode^=1;
		break;

		case 8:
		if(Disp_Mode==2)
		{
			if(!Set_Mode){T_Set++;if(T_Set>80)T_Set=20;}
			else{D_Set+=5;if(D_Set>80)D_Set=20;}
		}
		break;

		case 9:
		if(Disp_Mode==2)
		{
			if(!Set_Mode){T_Set--;if(T_Set<20)T_Set=80;}
			else{D_Set-=5;if(D_Set<20)D_Set=80;}
		}
		break;
	}
}

/*移动监测：只在1秒新采样到来时判一次运动状态*/
void Move_Proc()
{
	if(Move_Slow)return;
	Move_Slow=1;

	prev_distance = last_distance;
	last_distance = Ut_Wave_Data() + 3;
	distance[0] = prev_distance;
	distance[1] = last_distance;

	
}

/*信息处理*/
void Seg_Proc()
{
	unsigned char New_Work_Mode;
	if(Seg_Slow)return;
	Seg_Slow=1;

	temperature=read_t();
	High_Temperature_Flag=(temperature>T_Set);

	delta = abs((int)last_distance - (int)prev_distance);
	Nearby_Flag = (last_distance < D_Set);

	if(Lock_Flag==0)
	{
		if(delta < 5) New_Work_Mode = 0;
		else if(delta >= 10) New_Work_Mode = 2;
		else New_Work_Mode = 1;

		if(New_Work_Mode != Work_Mode)
		{
			Work_Mode = New_Work_Mode;
			Work_Mode_Locked = New_Work_Mode;
			Lock_Flag = 1;
			Lock_Cnt = 0;
		}
	}
	else
	{
		Work_Mode = Work_Mode_Locked;
		Lock_Cnt++;
		if(Lock_Cnt >= 3)
		{
			Lock_Flag = 0;
			Lock_Cnt = 0;
		}
	}
	Lex=AD_Read(0x41);
	if(Lex>=153)Lex_Level=1;
	else if(Lex>=102)Lex_Level=2;
	else if(Lex>=25)Lex_Level=3;
	else Lex_Level=4;

	switch(Disp_Mode)
	{
		case 0:
		Seg_Buf[0]=12;
		Seg_Buf[1]=(unsigned char)temperature/10%10;
		Seg_Buf[2]=(unsigned char)temperature%10;
		Seg_Buf[3]=Seg_Buf[4]=Seg_Buf[5]=10;
		Seg_Buf[6]=13;
		Seg_Buf[7]=Lex_Level;
		break;

		case 1:
		Seg_Buf[0]=14;
		Seg_Buf[1]=Work_Mode+1;
		Seg_Buf[2]=Seg_Buf[3]=Seg_Buf[4]=10;
		Seg_Buf[5]=last_distance/100%10;
		Seg_Buf[6]=last_distance/10%10;
		Seg_Buf[7]=last_distance%10;
		break;

		case 2:
		Seg_Buf[0]=15;
		Seg_Buf[2]=Seg_Buf[3]=Seg_Buf[4]=Seg_Buf[5]=10;
		if(Set_Mode==0){Seg_Buf[1]=12;Seg_Buf[6]=T_Set/10%10;Seg_Buf[7]=T_Set%10;}
		else{Seg_Buf[1]=14;Seg_Buf[6]=D_Set/10%10;Seg_Buf[7]=D_Set%10;}
		break;

		case 3:
		Seg_Buf[0]=13;
		Seg_Buf[1]=12;
		Seg_Buf[2]=Seg_Buf[3]=10;
		Seg_Buf[4]=Relay_Count>=1000?Relay_Count/1000%10:10;
		Seg_Buf[5]=Relay_Count>=100?Relay_Count/100%10:10;
		Seg_Buf[6]=Relay_Count>=10?Relay_Count/10%10:10;
		Seg_Buf[7]=Relay_Count%10;
		break;
	}
}

/*其他处理*/
void Led_Proc()
{
	unsigned char i;

	if(Disp_Mode==2)
	{
		for(i=0;i<7;i++)ucLed_Lock[i]=ucLed[i];
		ucLed_Lock[7]=(L7_Star==1)?Star_Flag:L7_State;
		Relay_State=Relay_Flag;
		Relay(Relay_State);
	}
	else
	{
		if(Nearby_Flag)
		{
			switch(Lex_Level)
			{
				case 1:ucLed[0]=1;ucLed[1]=ucLed[2]=ucLed[3]=0;break;
				case 2:ucLed[0]=ucLed[1]=1;ucLed[2]=ucLed[3]=0;break;
				case 3:ucLed[0]=ucLed[1]=ucLed[2]=1;ucLed[3]=0;break;
				case 4:ucLed[0]=ucLed[1]=ucLed[2]=ucLed[3]=1;break;
			}
		}
		else
		{
			ucLed[0]=0;ucLed[1]=0;ucLed[2]=0;ucLed[3]=0;
		}

		if(Work_Mode==0)ucLed[7]=0;
		else if(Work_Mode==1)ucLed[7]=1;
		else ucLed[7]=Star_Flag;

		Relay_Flag=(Nearby_Flag&&High_Temperature_Flag);
		Relay(Relay_Flag);

		if(Relay_Flag==0)Count_Flag=1;
		if(Relay_Flag&&Count_Flag)
		{
			Relay_Count++;
			Count_Flag=0;
		}
	}
}

/*定时器0初始化*/
void Timer0Init(void)
{
	AUXR &= 0x7F;
	TMOD &= 0xF0;
	TL0 = 0x18;
	TH0 = 0xFC;
	TF0 = 0;
	TR0 = 1;
	ET0 = 1;
	EA = 1;
}

/*定时器0中断*/
void Timer0server(void) interrupt 1
{
	if(++Key_Slow==10)Key_Slow=0;
	if(++Seg_Slow==100)Seg_Slow=0;      /* <=0.1s 刷新 */
	if(++Seg_Pos==8)Seg_Pos=0;
	if(++Move_Slow==1000)Move_Slow=0;   /* 严格1秒采样 */

	Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],Seg_Point[Seg_Pos]);
	if(Disp_Mode==2)Led_Disp(Seg_Pos,ucLed_Lock[Seg_Pos]);
	else Led_Disp(Seg_Pos,ucLed[Seg_Pos]);

	if(Long_Press_Flag)
	{
		Timer2s++;
		if(Timer2s>=2000)Relay_Count=0;
	}
	else Timer2s=0;

	if(++Timer100ms>=100)
	{
		Timer100ms=0;
		Star_Flag^=1;
	}
}

/*主函数*/
void main()
{
	Init();
	Timer0Init();

	while(read_t()==85);

	last_distance=Ut_Wave_Data()+3;
	prev_distance=last_distance;
	delta=0;
	Nearby_Flag=(last_distance<D_Set);

	while(1)
	{
		Key_Proc();
		Move_Proc();
		Seg_Proc();
		Led_Proc();
	}
}

void Init()
{
	P2=0x80;P0=0xFF;P2=0x00;
	P2=0xA0;P0=0x00;P2=0x00;
}

unsigned char Key_Read()
{
	unsigned char temp=0;
	P44=0;P42=1;P35=1;P34=1;
	if(P33==0)temp=4;
	if(P32==0)temp=5;
	P44=1;P42=0;P35=1;P34=1;
	if(P33==0)temp=8;
	if(P32==0)temp=9;
	if(P33==0&&P32==0)temp=89;
	return temp;
}

void Seg_Disp(unsigned char wei,unsigned char table,bit point)
{
	P2=0xE0;P0=0x00;P2=0x00;
	P2=0xC0;P0=seg_wei[wei];P2=0x00;
	P2=0xE0;
	P0=seg_table[table];
	if(point)P0&=~(1<<7);
	P2=0x00;
}

void Led_Disp(unsigned char addr,bit enable)
{
	static unsigned char temp;
	if(enable)temp|=(0x01<<addr);
	else temp&=~(0x01<<addr);
	P0=~temp;P2=0x80;P2=0x00;
}

void Relay(bit enable)
{
	static unsigned char temp1;
	if(enable)temp1|=0x10;
	else temp1&=~0x10;
	P2=0xA0;P0=temp1;P2=0x00;
}