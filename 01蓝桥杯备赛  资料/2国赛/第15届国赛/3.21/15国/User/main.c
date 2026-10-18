#include <STC15F2K60S2.H>
#include <Init.h>
#include <Led.h>
#include <Key.h>
#include <Seg.h>
#include <Wave.h>
#include <Uart.h>
#include <iic.h>
#include <String.h>
#include <Stdio.h>
#include <math.h>

idata unsigned char Key_Val,Key_Down,Key_Up,Key_Old;
idata unsigned char Key_Slow_Down;
idata unsigned char Move_Slow_Down;
idata unsigned int  Seg_Slow_Down;
xdata unsigned char Seg_Buf[8]={10,10,10,10,10,10,10,10};
xdata unsigned char Seg_Point[8]={0,0,0,0,0,0,0,0};
xdata unsigned char Seg_Pos;
xdata unsigned char ucLed[8]={0,0,0,0,0,0,0,0};
unsigned char Uart_Recv[10]={0,0,0,0,0,0,0,0,0,0};
unsigned char Uart_Recv_Index=0;
unsigned char Uart_Tick;
bit Uart_Flag;
unsigned int Freq;
unsigned int Timer_1000ms;
unsigned char Seg_Disp_Mode;//0-坐标 1-速度 2-参数
unsigned int Coor[6];
unsigned int Temp_X;
unsigned int Temp_Y;
unsigned char Length;
unsigned char State;//状态 0-空闲 1-等待 2-运行
unsigned int Speed;//速度
unsigned char Distance_Obs;//障碍物距离
unsigned char Para_R_Disp=10;//R参数
unsigned char Para_R_Control=10;
char Para_B_Disp=0;//B参数
char Para_B_Control;
bit Para_Mode;//0-R参数 1-B参数
unsigned char result;
bit Date_Flag;//接收到有效目标
float Light_Voltage;//光敏电压
bit Envir;//0-夜间 1-日间
float Distance;//目标距离
float Across_Distance;//走过的距离
unsigned int time;//运行所需要时间
unsigned char Timer_100ms;
bit Led_Flag;
xdata unsigned int Time_SZ;//系统时钟
xdata unsigned int Time_Old;
xdata unsigned int Time_New;
bit Arrive_Flag;
idata unsigned int Timer_3000ms;
idata unsigned int dx;
idata unsigned int dy;

void Move_Proc()
{
	if(Move_Slow_Down)return;
	Move_Slow_Down=1;
	
	if(State==2)//运行状态、
	{
		Across_Distance+=Speed/100.0;
		if(Across_Distance<Distance)
		{
			Coor[2]=(Coor[4]-Coor[0]>=0)?Coor[0]+Across_Distance*(float)(dx/Distance):Coor[0]-Across_Distance*(float)(dx/Distance);
			Coor[3]=(Coor[5]-Coor[1]>=0)?Coor[1]+Across_Distance*(float)(dy/Distance):Coor[1]-Across_Distance*(float)(dy/Distance);
		}
		else//到达目的地
		{
			Arrive_Flag=1;
			State=0;
			Across_Distance=0;
			Coor[0]=Coor[2]=Coor[4];
			Coor[1]=Coor[3]=Coor[5];
		}
	}
}

void Key_Proc()
{
	if(Key_Slow_Down)return;
	Key_Slow_Down=1;
	
	Key_Val=Key_Read();
	Key_Down=Key_Val&(Key_Val^Key_Old);
	Key_Up=~Key_Val&(Key_Val^Key_Old);
	Key_Old=Key_Val;
	
	switch(Key_Down)
	{
		case 8:
			if(++Seg_Disp_Mode==3)
			{
				Seg_Disp_Mode=0;
				Para_Mode=0;
			}
		break;
		case 9:
			Para_Mode^=1;
		break;
		case 12:
			if(Para_Mode==0)
			{
				Para_R_Disp+=1;
				if(Para_R_Disp>20)
					Para_R_Disp=20;
				Para_R_Control=Para_R_Disp;
			}
			if(Para_Mode==1)
			{
				Para_B_Disp+=5;
				if(Para_B_Disp>90)
					Para_B_Disp=90;
				Para_B_Control=Para_B_Disp;
			}
		break;
		case 13:
			if(Para_Mode==0)
			{
				Para_R_Disp-=1;
				if(Para_R_Disp<10)
					Para_R_Disp=10;
				Para_R_Control=Para_R_Disp;
			}
			if(Para_Mode==1)
			{
				Para_B_Disp-=5;
				if(Para_B_Disp<-90)
					Para_B_Disp=-90;
				Para_B_Control=Para_B_Disp;
			}
		break;
		case 4:
			if(State==2)
			{
				State=1;
				break;
			}
			
			if(State==0&&Date_Flag==1)
			{
				State=2;
				dx=(Coor[4]-Coor[0]>0)?Coor[4]-Coor[0]:Coor[0]-Coor[4];
				dy=(Coor[5]-Coor[1]>0)?Coor[5]-Coor[1]:Coor[1]-Coor[5];
				Distance=sqrt(dx*dx+dy*dy);
				break;
			}
			
			if(State==1&&Distance_Obs>=30)
			{
				State=2;
				break;
			}
				
		break;
		case 5:
			if(State==0)
				Coor[0]=Coor[2]=Coor[1]=Coor[3]=0;
		break;
	}
}

void Seg_Proc()
{
	unsigned char i;
	if(Seg_Slow_Down)return;
	Seg_Slow_Down=1;
	
	Distance_Obs=Wave_Read();
	
	if(State==2)
	{
		if(Distance_Obs<30)
			State=1;
	}
	
	Light_Voltage=AD_Read(0x41)/51.0;
	if(Light_Voltage>1.2)Envir=1;//日间
	else Envir=0;
	
	if(Seg_Disp_Mode==0)
	{
		if(State==1||State==2)
		{
			Seg_Buf[0]=12;//L
			Seg_Buf[1]=Coor[4]/100%10;
			Seg_Buf[2]=Coor[4]/10%10;
			Seg_Buf[3]=Coor[4]%10;
			Seg_Buf[4]=11;//-
			Seg_Buf[5]=Coor[5]/100%10;
			Seg_Buf[6]=Coor[5]/10%10;
			Seg_Buf[7]=Coor[5]%10;
			Seg_Point[2]=Seg_Point[6]=0;
			for(i=1;i<3;i++)
			{
				if(Seg_Buf[i]==0)Seg_Buf[i]=10;
				else break;
			}
			for(i=5;i<7;i++)
			{
				if(Seg_Buf[i]==0)Seg_Buf[i]=10;
				else break;
			}
		}
		else
		{
			Seg_Buf[0]=12;//L
			Seg_Buf[1]=Coor[0]/100%10;
			Seg_Buf[2]=Coor[0]/10%10;
			Seg_Buf[3]=Coor[0]%10;
			Seg_Buf[4]=11;//-
			Seg_Buf[5]=Coor[1]/100%10;
			Seg_Buf[6]=Coor[1]/10%10;
			Seg_Buf[7]=Coor[1]%10;
			Seg_Point[2]=Seg_Point[6]=0;
			for(i=1;i<3;i++)
			{
				if(Seg_Buf[i]==0)Seg_Buf[i]=10;
				else break;
			}
			for(i=5;i<7;i++)
			{
				if(Seg_Buf[i]==0)Seg_Buf[i]=10;
				else break;
			}
		}
	}
	else if(Seg_Disp_Mode==1)//速度界面
	{
		if(State==2)//运行
		{
			Seg_Buf[0]=13;//E
			Seg_Buf[1]=1;
			Seg_Buf[2]=10;
			Seg_Buf[3]=Speed/10000%10;
			Seg_Buf[4]=Speed/1000%10;
			Seg_Buf[5]=Speed/100%10;
			Seg_Buf[6]=Speed/10%10;
			Seg_Buf[7]=Speed%10;
			Seg_Point[6]=1;
			Seg_Point[2]=0;
			for(i=3;i<6;i++)
			{
				if(Seg_Buf[i]==0)Seg_Buf[i]=10;
				else break;
			}
		}
		else if(State==0)//空闲
		{
			Seg_Buf[0]=13;//E
			Seg_Buf[1]=2;
			Seg_Buf[2]=10;
			Seg_Buf[3]=11;//-
			Seg_Buf[4]=11;
			Seg_Buf[5]=11;
			Seg_Buf[6]=11;
			Seg_Buf[7]=11;
			Seg_Point[2]=Seg_Point[6]=0;
		}
		else//等待
		{
			Seg_Buf[0]=13;//E
			Seg_Buf[1]=3;
			Seg_Buf[2]=10;
			Seg_Buf[3]=10;
			Seg_Buf[4]=10;
			Seg_Buf[5]=(unsigned int)Distance_Obs/100%10;
			Seg_Buf[6]=(unsigned int)Distance_Obs/10%10;
			Seg_Buf[7]=(unsigned int)Distance_Obs%10;
			Seg_Point[2]=Seg_Point[6]=0;
			for(i=5;i<7;i++)
			{
				if(Seg_Buf[i]==0)Seg_Buf[i]=10;
				else break;
			}
		}
	}
	else
	{
		Seg_Buf[0]=14;//P
		Seg_Buf[1]=10;
		Seg_Buf[2]=Para_R_Disp/10%10;
		Seg_Buf[3]=Para_R_Disp%10;
		Seg_Buf[4]=10;
		Seg_Point[2]=1;
		Seg_Point[6]=0;
		if(Para_B_Disp<0)
		{
			Seg_Buf[5]=abs(Para_B_Disp)/10%10?11:10;
			Seg_Buf[6]=abs(Para_B_Disp)/10%10?abs(Para_B_Disp)/10%10:11;
			Seg_Buf[7]=abs(Para_B_Disp)%10;
		}
		else
		{
			Seg_Buf[5]=Para_B_Disp/100%10;
			Seg_Buf[6]=Para_B_Disp/10%10;
			Seg_Buf[7]=Para_B_Disp%10;
		}
		for(i=5;i<7;i++)
		{
			if(Seg_Buf[i]==0)Seg_Buf[i]=10;
			else break;
		}
	}
}

void Led_Proc()
{
	//Relay
	Relay(!(State-2));
	//Led
	if(State==2)
		ucLed[0]=1;
	else if(State==0)
		ucLed[0]=0;
	else
		ucLed[0]=Led_Flag;
	
	if(State==2)
		ucLed[1]=!Envir;
	else
		ucLed[1]=0;
	
	
		ucLed[2]=Arrive_Flag;
}

void Uart_Proc()
{
	if(Uart_Recv_Index==0)return;
	if(Uart_Tick>=10)
	{
		Uart_Flag=0;
		Uart_Tick=0;
		if(strcmp(Uart_Recv,"#")==0)
			printf("(%u,%u)",Coor[2],Coor[3]);
		else if(strcmp(Uart_Recv,"?")==0)
		{
			if(State==0)
				printf("Idle");
			else if(State==1)
				printf("Wait");
			else if(State==2)
				printf("Busy");
		}
		else if(Uart_Recv[0]=='('&&Uart_Recv[Uart_Recv_Index-1]==')')
		{
			idata unsigned int x=0,y=0;
			idata bit parse=0; //解析标志位 0-解析x 1-解析y
			idata bit useful=0;//收到的坐标是否有效 0-有效 1-无效
			idata unsigned char i;
			for(i=1;i<Uart_Recv_Index-1;i++)
			{
				unsigned char ch=Uart_Recv[i];
				if(ch>='0'&&ch<='9')
					!parse?(x=x*10+ch-48):(y=y*10+ch-'0');
				else if(ch==',')
					parse=1;
				else//接收到无效数据
				{
					useful=1;//收到的坐标无效
					break;//直接退出循环
				}
			}
			if(!useful&&parse)//数据有效时才保存坐标
			{
				if(!State)
				{
					printf("Got it");
					Coor[4]=x;
					Coor[5]=y;
					Date_Flag=1; //收到目的地坐标
				}
				else
					printf("Busy");
			}
			else
				printf("Error");
		}
		else
			printf("Error");
		memset(Uart_Recv,0,Uart_Recv_Index);
		Uart_Recv_Index=0;
	}
}

void Timer0Init(void)		//0微秒@12.000MHz
{
	AUXR &= 0x7F;		//定时器时钟12T模式
	TMOD &= 0xF0;		//设置定时器模式
	TMOD|=0x05;
	TL0 = 0x00;		//设置定时初值
	TH0 = 0x00;		//设置定时初值
	TF0 = 0;		//清除TF0标志
	TR0 = 1;		//定时器0开始计时
}

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

void Timer1_Server() interrupt 3
{
	if(++Move_Slow_Down==100)Move_Slow_Down=0;
	if(++Key_Slow_Down==10)Key_Slow_Down=0;
	if(++Seg_Slow_Down==300)Seg_Slow_Down=0;
	if(++Seg_Pos==8)Seg_Pos=0;
	Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],Seg_Point[Seg_Pos]);
	Led_Disp(Seg_Pos,ucLed[Seg_Pos]);
	if(Uart_Flag)
		Uart_Tick++;
	
	if(++Timer_1000ms==1000)
	{
		Timer_1000ms=0;
		Freq=TH0<<8|TL0;
		TH0=TL0=0;
		Speed=(3.14*Para_R_Control*Freq/1000.0+Para_B_Control)*10;
	}
	
	if(++Timer_100ms==100)
	{
		Timer_100ms=0;
		Led_Flag^=1;
	}
	if(Arrive_Flag)
	{
		if(++Timer_3000ms>=3000)
		{
			Timer_3000ms=0;
			Arrive_Flag=0;
		}
	}
}

void Uart_Server() interrupt 4
{
	if(RI==1)
	{
		Uart_Flag=1;
		Uart_Tick=0;
		Uart_Recv[Uart_Recv_Index]=SBUF;
		Uart_Recv_Index++;
		RI=0;
	}
	if(Uart_Recv_Index>10)
	{
		Uart_Tick=Uart_Flag=0;
		Uart_Recv_Index=0;
		memset(Uart_Recv,0,10);
	}
}

void main()
{
	Sys_Init();
	UartInit();
	Timer0Init();
	Timer1Init();
	while(1)
	{
		Move_Proc();
		Key_Proc();
		Seg_Proc();
		Led_Proc();
		Uart_Proc();
	}
}