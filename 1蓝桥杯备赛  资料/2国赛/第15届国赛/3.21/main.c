#include <STC15F2K60S2.H>
#include <iic.h>
#include <ultra.h>
#include <uart.h>
#include <stdio.H>
#include <string.H>
#include <intrins.H>
#include <math.H>

unsigned char Key_Down,Key_Val,Key_Up,Key_Old;
unsigned char code seg_wei[8]={0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
unsigned char code seg_table[]={0xC0,0xF9,0xA4,0xB0,0x99,0x92,0x82,0xF8,0x80,0x90,0xff,0xbf,0xc7,0x86,0x8c};
unsigned char Seg_Buf[8]={10,10,10,10,10,10,10,10};
unsigned char Seg_Point[8]={0,0,0,0,0,0,0,0};
unsigned char Seg_Pos;
unsigned char ucLed[8]={0,0,0,0,0,0,0,0};
unsigned char Key_Slow;
unsigned char Data_Down;
unsigned int Seg_Slow;
/*底层函数声明*/
void Init();
unsigned char Key_Read();
void Seg_Disp(unsigned char wei,unsigned char table,bit point);
void Led_Disp(unsigned char addr,bit enable);
void Relay(bit enable);
/*变量*/

bit Rec_Flag;
unsigned int Timer1s;
unsigned int freq;
unsigned char Disp_Mode=0;//0-坐标界面 1-速度界面 2-参数界面
unsigned char Work_Mode=0;//0空闲，1运行，2等待
int Place_X_Now=0;
int Place_Y_Now=0;
int Place_X_Destination=0;
int Place_Y_Destination=0;
unsigned char distance;//障碍物距离
float speed;
unsigned char R_Set_10x=10;//1.0-2.0  10-20存
unsigned char R_Set_10x_Control=10;
char B_Set=0;//-90-90
char B_Set_Control=0;
bit Set_Mode=0;//0-R参数 1-B参数
bit flat_zhangai=0;
bit Get_Place_Flag=0;//接收到目的坐标
bit Wait_Flag=0;//0-运行 1-等待
long int distance_count;//总路程
unsigned int Timer500ms;
unsigned char Lex;
bit Star_Flag=0;// 闪烁
unsigned char Timer100ms;
bit Destination_Flag=0;//到达目的地
unsigned int Timer3s;
bit L3_Onflag;
float temp_x;
float temp_y;
/*串口*/
unsigned int Sys_Tick;
unsigned char Uart_Rec[10];
unsigned char Uart_Rec_Index;
bit Send1;//发送目的地坐标
bit Send2;//查询设备状态
bit Send3;//查询设备位置
bit Error_Flag=0;
/*按键*/
void Key_Proc()
{
	if(Key_Slow)return;
	Key_Slow=1;
	Key_Val=Key_Read();
	Key_Down=Key_Val&(Key_Old^Key_Val);
	Key_Up=~Key_Val&(Key_Old^Key_Val);
	Key_Old=Key_Val;
	switch(Key_Down)
	{
		case 4://启动
		if(Get_Place_Flag==1&&Work_Mode==0)//“空闲”状态下，若设备当前已接收目的地坐标，按下 S4 按键切 换为“运行”状态
		{
			Get_Place_Flag=0;
			if(Work_Mode==0)
			{
				Wait_Flag=1;//运行变等待
				Work_Mode=1;
			}
		}
		else if(Work_Mode==1)Work_Mode=2;//s4按下，运行变等待
		else if(Work_Mode==2)
		{
			if(distance>30)Work_Mode=1;
		}
		break;
		case 5://重置按钮
		if(Work_Mode==0)Place_X_Now=Place_Y_Now=0;
		break;
		case 8://数码管显示界面切换
		Disp_Mode=(Disp_Mode+1)%3;
		if(Disp_Mode==0)//退出参数界面生效
		{
			Set_Mode=0;
			R_Set_10x=R_Set_10x_Control;
			B_Set=B_Set_Control;
		}
		break;
		case 9://选择按钮（参数界面）
		if(Disp_Mode==2)Set_Mode^=1;
		break;
		case 12://+
			if(Set_Mode==0)
			{
				R_Set_10x_Control+=1;
				if(R_Set_10x_Control>20)R_Set_10x_Control=10;
			}
			else 
			{
				B_Set_Control+=5;
				if(B_Set_Control>90)B_Set_Control=-90;
			}
		break;
		case 13:
			if(Set_Mode==0)
			{
				R_Set_10x_Control-=1;
				if(R_Set_10x_Control<10)R_Set_10x_Control=20;
			}
			else 
			{
				B_Set_Control-=5;
				if(B_Set_Control<-90)B_Set_Control=90;
			}
		break;
	}
}
/*信息处理*/
void Seg_Proc()
{
	if(Seg_Slow)return;
	Seg_Slow=1;
	switch(Disp_Mode)
	{
		case 0://坐标界面
		if(Work_Mode==0)//空闲状态
		{
			Seg_Buf[0]=12;//L
			Seg_Buf[1]=Place_X_Now>100?Place_X_Now/100%10:10;
			Seg_Buf[2]=Place_X_Now>10?Place_X_Now/10%10:10;
			Seg_Buf[3]=Place_X_Now%10;
			Seg_Buf[4]=11;//-
			Seg_Buf[5]=Place_Y_Now>100?Place_Y_Now/100%10:10;
			Seg_Buf[6]=Place_Y_Now>10?Place_Y_Now/10%10:10;
			Seg_Buf[7]=Place_Y_Now%10;
		}
		else //等待or运行状态
		{
			Seg_Buf[0]=12;
			Seg_Buf[1]=Place_X_Destination>100?Place_X_Destination/100%10:10;
			Seg_Buf[2]=Place_X_Destination>10?Place_X_Destination/10%10:10;
			Seg_Buf[3]=Place_X_Destination%10;
			Seg_Buf[4]=11;
			Seg_Buf[5]=Place_Y_Destination>100?Place_Y_Destination/100%10:10;
			Seg_Buf[6]=Place_Y_Destination>10?Place_Y_Destination/10%10:10;
			Seg_Buf[7]=Place_Y_Destination%10;
		}
		Seg_Point[2]=Seg_Point[6]=0;
		break;
		case 1://速度界面  Work_Mode=0;//0空闲，1运行，2等待
			switch(Work_Mode)
			{
				case 1://运行模式
					Seg_Buf[0]=13; // E
					Seg_Buf[1]=1;
					Seg_Buf[2]=10;
					Seg_Buf[3]=(speed>1000)?(unsigned int)speed/1000%10:10;
					Seg_Buf[4]=(speed>100)?(unsigned int)speed/100%10:10;
					Seg_Buf[5]=(speed>10)?(unsigned int)speed/10%10:10;
					Seg_Buf[6]=(unsigned int)speed%10;
					Seg_Buf[7]=(unsigned int)(speed*10)%10;
					Seg_Point[6]=1;
					break;
				case 0://空闲模式
					Seg_Buf[0]=13; // E
					Seg_Buf[1]=2;
					Seg_Buf[2]=10;
					Seg_Buf[3]=Seg_Buf[4]=Seg_Buf[5]=Seg_Buf[6]=Seg_Buf[7]=11; //-
					Seg_Point[6]=0;
					break;
				case 2://等待模式
					Seg_Buf[0]=13;
					Seg_Buf[1]=3;
					Seg_Buf[2]=10;
					Seg_Buf[3]=distance>10000?distance/10000%10:10;
					Seg_Buf[4]=distance>1000?distance/1000%10:10;
					Seg_Buf[5]=distance>100?distance/100%10:10;
					Seg_Buf[6]=distance>10?distance/10%10:10;
					Seg_Buf[7]=distance%10;
					Seg_Point[6]=0;
					break;
			}	
				Seg_Point[2]=0;		
		break;
		case 2://参数界面
			Seg_Buf[0]=14;//P
			Seg_Buf[1]=Seg_Buf[4]=10;
			Seg_Buf[2]=R_Set_10x_Control/10%10;
			Seg_Buf[3]=R_Set_10x_Control%10;
			Seg_Point[2]=1;
			if(B_Set_Control<0)
			{
				Seg_Buf[5]=(-B_Set_Control>=10)?11:10;
				Seg_Buf[6]=(-B_Set_Control<10)?11:(-B_Set_Control)/10%10;
				Seg_Buf[7]=(-B_Set_Control)%10;
				Seg_Point[6]=0;
			}
			else
			{
				Seg_Buf[5]=(B_Set_Control<100)?10:B_Set_Control/100%10;
				Seg_Buf[6]=(B_Set_Control<10)?10:B_Set_Control/10%10;
				Seg_Buf[7]=B_Set_Control%10;
				Seg_Point[6]=0;
			}
		break;
	}
}
/*其他*/
void Led_Proc()
{
	//Work_Mode=0;//0空闲，1运行，2等待
	if(Work_Mode==0)ucLed[0]=0;
	else if(Work_Mode==1)ucLed[0]=1;
	else if(Work_Mode==2)ucLed[0]=Star_Flag;
	
	if(Work_Mode&&Lex<61)ucLed[1]=1;
	else ucLed[1]=0;
	
	if(Destination_Flag==0) L3_Onflag=0;
	ucLed[2]=L3_Onflag;
	
	Relay(Work_Mode==1);
}

void Data_Proc()
{
	if(Data_Down)return;
	Data_Down=1;
	//计算速度
	speed=(3.14*R_Set_10x/10*freq/100)+B_Set;
	//障碍物触发等待模式
	distance=Ut_Wave_Data()+3;
	if(distance<30&&Work_Mode==1)Work_Mode=2;
	//日间or夜间场景
	Lex=AD_Read(0x01);
	//路径规划
	if(Work_Mode==1&&Wait_Flag==1)
	{
		distance_count=sqrt((float)(Place_X_Destination-Place_X_Now)*(Place_X_Destination-Place_X_Now)+(float)(Place_Y_Destination-Place_Y_Now)*(Place_Y_Destination-Place_Y_Now));
		temp_x=(float)(Place_X_Destination-Place_X_Now)/distance_count;
		temp_y=(float)(Place_Y_Destination-Place_Y_Now)/distance_count;
		Wait_Flag=0;
	}
	if(Work_Mode==1)//运行状态
	{
		if(distance_count<=0)//到达目的地
		{
			distance_count=0;
			Place_X_Now=Place_X_Destination;
			Place_Y_Now=Place_Y_Destination;
			Work_Mode=0;//变空闲
			Destination_Flag=1;
			Timer3s=0;
		}
	}
}

void Uart_Proc()
{
	unsigned char i;
	//接受指令判断
	if(Uart_Rec_Index==0)return;
	if(Uart_Rec_Index>=10)
	{
		Uart_Rec_Index=0;
		Error_Flag=1;
	}
	if(Sys_Tick>10)
	{
		Rec_Flag=Sys_Tick=0;
		if(Uart_Rec_Index>=5&&Uart_Rec[0]=='('&&Uart_Rec[Uart_Rec_Index-1]==')'&&Uart_Rec_Index<10)
		{
			Send1=1;
			if(Work_Mode==0)
			{
				Place_X_Destination=0;
				Place_Y_Destination=0;
				Get_Place_Flag=1;//得到目的地位置
				//数据解析
				for(i=1;i<Uart_Rec_Index;i++)
				{
					if(Uart_Rec[i]!=',')
					{
						Place_X_Destination*=10;Place_X_Destination+=(Uart_Rec[i]-'0');
					}
					else
					break;
				}
				i++;
				for(;i<Uart_Rec_Index;i++)
				{
					if(Uart_Rec[i]!=')')
					{
						Place_Y_Destination*=10;Place_Y_Destination+=(Uart_Rec[i]-'0');
					}
					else
					break;
				}
			}
		}
		else if(Uart_Rec_Index==1&&Uart_Rec[0]=='?')Send2=1;
		else if(Uart_Rec_Index==1&&Uart_Rec[0]=='#')Send3=1;
		else Error_Flag=1;
		memset(Uart_Rec,0,Uart_Rec_Index);
		Uart_Rec_Index=0;
	}

	//发送
	if(Send1)
	{
			if(Work_Mode==0)printf("Got it");
			if(Work_Mode==1)printf("Busy");
			Send1=0;
	}
	if(Send2)
	{
		if(Work_Mode==0)printf("Idle");
		else if(Work_Mode==1)printf("Busy");
		else if(Work_Mode==2)printf("Wait");
		Send2=0;
	}
	if(Send3)
	{
		printf("(%d,%d)",(unsigned int)Place_X_Now,(unsigned int)Place_Y_Now);
		Send3=0;
	}
	if(Error_Flag)
	{
		printf("Error");
		Error_Flag=0;
	}
}

void Timer1Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0xBF;		//定时器时钟12T模式
	TMOD &= 0x0F;		//设置定时器模式
	TL1 = 0x18;		//设置定时初值
	TH1 = 0xFC;		//设置定时初值
	TF1 = 0;		//清除TF1标志
	TR1 = 1;		//定时器1开始计时
	ET1 = 1;
	EA = 1;
}

void Timer0Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0x7F;		//定时器时钟12T模式
	TMOD &= 0xF0;		//设置定时器模式
	TMOD |= 0x05;
	TL0 = 0;		//设置定时初值
	TH0 = 0;		//设置定时初值
	TF0 = 0;		//清除TF0标志
	TR0 = 1;		//定时器0开始计时
}


void Timer1server() interrupt 3
{
	if(++Key_Slow==10)Key_Slow=0;
	if(++Seg_Slow==300)Seg_Slow=0;
	if(++Data_Down==300)Data_Down=0;
	if(++Seg_Pos==8)Seg_Pos=0;
	Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],Seg_Point[Seg_Pos]);
	Led_Disp(Seg_Pos,ucLed[Seg_Pos]);
	// 串口超时解析
	if(Rec_Flag)Sys_Tick++;
	//频率
	if(++Timer1s==1000)
	{
		TR0=0;
		Timer1s=0;
		freq=(TH0<<8)|TL0;
		TH0=TL0=0;
		TR0=1;
	}
	//路径规划
	if(Work_Mode==1&&distance_count>=0&&Timer500ms++==500)
	{
		Timer500ms=0;
		distance_count-=(speed*0.5);
		Place_X_Now+=(speed*0.5)*temp_x;
		Place_Y_Now+=(speed*0.5)*temp_y;
	}
	//闪烁
	if(++Timer100ms==100)
	{
		Timer100ms=0;
		Star_Flag^=1;
	}
	//到达目的地
	if(Destination_Flag==1)
	{
		L3_Onflag=1;
		Timer3s++;
		if(Timer3s>=3000)
		{
			L3_Onflag=0;
			Timer3s=0;
			Destination_Flag=0;
		}
	}
}

void uart1proc() interrupt 4
{
	if(RI==1)
	{
		Rec_Flag=1;
		Sys_Tick=0;
		Uart_Rec[Uart_Rec_Index]=SBUF;
		Uart_Rec_Index++;
		RI=0;
	}
	if(Uart_Rec_Index>=10)Uart_Rec_Index=0;
}

void main()
{
	Init();
	Timer1Init();
	Timer0Init();
	UartInit();
	while(1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
		Uart_Proc();
		Data_Proc();
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
	P44=0;P42=1;P35=1;//P34=1;
	if(P33==0)temp=4;
	if(P32==0)temp=5;
	if(P31==0)temp=6;
	if(P30==0)temp=7;
	P44=1;P42=0;P35=1;//P34=1;
	if(P33==0)temp=8;
	if(P32==0)temp=9;
	if(P30==0)temp=11;
	P44=1;P42=1;P35=0;//P34=1;
	if(P33==0)temp=12;
	if(P32==0)temp=13;
	if(P31==0)temp=14;
	if(P30==0)temp=15;
	P3=0xff;
	return temp;
}
void Seg_Disp(unsigned char wei,unsigned char table,bit point)
{
	P2=0xe0;P0=0xff;P2=0x00;
	P2=0xc0;P0=seg_wei[wei];P2=0x00;
	P2=0xe0;
	P0=seg_table[table];
	if(point)P0&=~(1<<7);
	P2=0x00;
}
void Led_Disp(unsigned char addr,bit enable)
{
	static unsigned char temp=0x00;
	if(enable)temp|=0x01<<addr;
		else temp&=~(0x01<<addr);
	P0=~temp;P2=0x80;P2=0x00;
}
void Relay(bit enable)
{
	unsigned char temp0=0x00;
	if(enable)temp0|=0x10;
		 else temp0&=~(0x10);
	P0=temp0;P2=0xa0;P2=0x00;
}