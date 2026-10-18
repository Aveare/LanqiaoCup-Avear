#include <STC15F2K60S2.H>
#include <onewire.h>
#include <iic.h>
#include <ultra.h>
#include <uart.h>
#include <intrins.h>
#include <string.h>
#include <stdio.h>
/*底层变量*/
unsigned char Key_Val,Key_Down,Key_Up,Key_Old;
unsigned char Key_Slow=0;
unsigned int Seg_Slow=0,Uart_Slow=0;
unsigned char code seg_wei[8]={0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
unsigned char code seg_table[]={0xC0,0xF9,0xA4,0xB0,0x99,0x92,0x82,0xF8,0x80,0x90,0xff,0xbf,0xc6,0xc7,0xc8,0x8c};
xdata unsigned char Seg_Buf[8]={10,10,10,10,10,10,10,10};
xdata unsigned char Seg_Point[8]={0,0,0,0,0,0,0,0};
unsigned char Seg_Pos=0;
unsigned char ucLed[8]={0,0,0,0,0,0,0,0};
/*底层函数声明*/
void Init();
unsigned char Key_Read();
void Seg_Disp(unsigned char wei,unsigned char table,bit point);
void Led_Disp(unsigned char addr,bit enable);
/*串口*/
unsigned char Uart_Rec[10];
unsigned char Uart_Rec_Index=0;
unsigned char Uart_Send[15];
bit Send1;//ST指令
bit Send2;//PARA指令
bit error_flag;//错误指令
/*变量*/
float temperature;//温度
unsigned char distance;//距离
bit Work_Mode=0;//0-数据界面 1-参数界面
unsigned char Disp_Mode=0;//0-温度数据 1-距离数据 2-变更参数
bit Set_Mode=0;//0-温度参数 1-距离参数
unsigned int Set_Count=0;//参数变动
unsigned char T_Set=30;//温度参数 0-99
unsigned char T_Set_Old;
unsigned char D_Set=35;//距离参数 0-99
unsigned char D_Set_Old;
bit L12_Flag;//清空计数值
unsigned int Timer1000ms;
bit L13_Flag;//启停DA
unsigned int Timer1000ms1;
bit DA_Mode=1;//0-关 1-开
bit Save_Flag;//保存至EEPROM
bit Read_Flag;//读取次数
unsigned char EEPROM[2];
unsigned char EEPROM_R[2];
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
		case 16://-
			if(Work_Mode==1&&Set_Mode==0)//参数界面的温度参数下-2
			{
				T_Set-=2;
				if(T_Set>100)T_Set=99;
			}
			else if(Work_Mode==1&&Set_Mode==1)//参数界面的距离参数下-5
			{
				D_Set-=5;
				if(D_Set>100)D_Set=99;
			}
		break;
		case 17://+
			if(Work_Mode==1&&Set_Mode==0)//参数界面的温度参数下+2
			{
				T_Set+=2;
				if(T_Set>=100)T_Set=0;
			}
			else if(Work_Mode==1&&Set_Mode==1)//参数界面的距离参数下+5
			{
				D_Set+=5;
				if(D_Set>=100)D_Set=0;
			}
		break;
	}
//S12长按短按
	if(Key_Down==12)L12_Flag=1;
	if(Key_Up==12)
	{
		if(Timer1000ms<1000)
		{
				if(Work_Mode==0)//数据界面下
				Disp_Mode=(Disp_Mode+1)%3;//0-2循环
				else //参数界面下
				Set_Mode^=1;
		}
		if(Timer1000ms>1000)Set_Count=0;
		Timer1000ms=L12_Flag=0;
	}
	
//S13长按短按	
	if(Key_Down==13)L13_Flag=1;
	if(Key_Up==13)
	{
		if(Timer1000ms1<1000)//短按 数据界面-参数界面切换
		{
			Work_Mode^=1;
			if(Work_Mode==0)//从参数界面切回数据界面
			{
				if((T_Set_Old!=T_Set)||(D_Set_Old!=D_Set))Set_Count++;
				Save_Flag=1;
			}
			if(Work_Mode==1)//数据到参数界面
			{
				T_Set_Old=T_Set;
				D_Set_Old=D_Set;
			}
			Disp_Mode=Set_Mode=0;
		}
		if(Timer1000ms1>1000)DA_Mode^=1;
		Timer1000ms1=L13_Flag=0;
	}
}
/*信息处理*/
void Seg_Proc()
{
	if(Seg_Slow)return;
	Seg_Slow=1;
	// 重要信息读取
	temperature=read_t();
	distance=Ut_Wave_Data();
	
//数码管显示
	if(Work_Mode==0)//数据界面
	{
		switch(Disp_Mode)
		{
			case 0://温度参数
			Seg_Buf[0]=12;//C
			Seg_Buf[1]=Seg_Buf[2]=Seg_Buf[3]=10;
			Seg_Buf[4]=(unsigned char)temperature/10%10;
			Seg_Buf[5]=(unsigned char)temperature%10;
			Seg_Buf[6]=(unsigned int)(temperature*10)%10;
			Seg_Buf[7]=(unsigned int)(temperature*100)%10;
			Seg_Point[5]=1;
			break;
			case 1://距离界面
			Seg_Buf[0]=13;//L
			Seg_Buf[1]=Seg_Buf[2]=Seg_Buf[3]=Seg_Buf[4]=Seg_Buf[5]=10;
			Seg_Buf[6]=distance/10%10;
			Seg_Buf[7]=distance%10;
			Seg_Point[5]=0;
			break;
			case 2://变更次数
			Seg_Buf[0]=14;//N
			Seg_Buf[1]=Seg_Buf[2]=10;
			Seg_Buf[3]=Set_Count>10000?Set_Count/10000%10:10;
			Seg_Buf[4]=Set_Count>1000?Set_Count/1000%10:10;
			Seg_Buf[5]=Set_Count>100?Set_Count/100%10:10;
			Seg_Buf[6]=Set_Count>10?Set_Count/10%10:10;
			Seg_Buf[7]=Set_Count%10;
			Seg_Point[5]=0;
			break;
		}
	}
	else //参数界面
	{
		if(Set_Mode==0)//温度参数
		{
			Seg_Buf[0]=15;//P
			Seg_Buf[1]=Seg_Buf[2]=Seg_Buf[4]=Seg_Buf[5]=10;
			Seg_Buf[3]=1;
			Seg_Buf[6]=T_Set/10%10;
			Seg_Buf[7]=T_Set%10;
			Seg_Point[5]=0;
		}
		else   //距离参数
		{
			Seg_Buf[0]=15;//P
			Seg_Buf[1]=Seg_Buf[2]=Seg_Buf[4]=Seg_Buf[5]=10;
			Seg_Buf[3]=2;
			Seg_Buf[6]=D_Set/10%10;
			Seg_Buf[7]=D_Set%10;
			Seg_Point[5]=0;
		}
	}
}
/*其他处理*/
void Led_Proc()
{
	//EEPROM保存
	if(Save_Flag)
	{
		EEPROM[0]=Set_Count/256;//高8位
		EEPROM[1]=Set_Count%256;//低8位
		EEPROM_Write(&EEPROM,0,2);
		Save_Flag=0;
	}
	if(Read_Flag)
	{
		Set_Count=(EEPROM_R[0]>>8)+EEPROM_R[1];
		Read_Flag=0;
	}
	//DA输出
	if(DA_Mode)
	{
			if(distance<=D_Set)DA_Write(2*51);
			else DA_Write(4*51);
	}
	else DA_Write(0.4*51);
	//LED
	ucLed[0]=(temperature>T_Set);
	ucLed[1]=(distance<D_Set);
	ucLed[2]=DA_Mode;
}
/*串口*/
void Uart_Proc()
{
	if(Uart_Slow)return;
	Uart_Slow=1;
	

	if(Uart_Rec_Index>0)
	{
		if(Uart_Rec_Index==4)//ST
		{
			if(Uart_Rec[0]=='S'&&Uart_Rec[1]=='T'&&Uart_Rec[2]=='\r'&&Uart_Rec[3]=='\n')Send1=1;
		}
		else if(Uart_Rec_Index==6)//PARA
		{
			if(Uart_Rec[0]=='P'&&Uart_Rec[1]=='A'&&Uart_Rec[2]=='R'&&Uart_Rec[3]=='A'&&Uart_Rec[4]=='\r'&&Uart_Rec[5]=='\n')Send2=1;
		}
		else error_flag=1;//ERROR
		Uart_Rec_Index=0;
	}
	//ST指令
		if(Send1)
		{
			sprintf(Uart_Send,"$%u,%.2f\r\n",(unsigned int)distance,(float)temperature);
			SendString(Uart_Send);
			Send1=0;
		}
	//PARA指令
		if(Send2)
		{
			sprintf(Uart_Send,"#%u,%u\r\n",(unsigned int)distance,(unsigned int)temperature);
			SendString(Uart_Send);
			Send2=0;
		}
	//错误指令
		if(error_flag)
		{
			SendString("ERROR\r\n\0");
			error_flag=0;
		}
}
/*定时器0初始化*/
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
void Timer0isr(void) interrupt 1
{
	if(++Key_Slow==10)Key_Slow=0;
	if(++Seg_Slow==100)Seg_Slow=0;
	if(++Uart_Slow==300)Uart_Slow=0;
	if(++Seg_Pos==8)Seg_Pos=0;
	Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],Seg_Point[Seg_Pos]);
	Led_Disp(Seg_Pos,ucLed[Seg_Pos]);

	if(L12_Flag)
	{
		Timer1000ms++;
		if(Timer1000ms>=1000)Timer1000ms=1001;
	}
	if(L13_Flag)
	{
		Timer1000ms1++;
		if(Timer1000ms1>=1000)Timer1000ms1=1001;
	}
	
}
void Uartisr(void) interrupt 4
{
	if(RI)
	{
		Uart_Rec[Uart_Rec_Index]=SBUF;
		Uart_Rec_Index++;
		RI=0;
	}
}
/*延迟750ms*/
void Delay750ms()		//@12.000MHz
{
	unsigned char i, j, k;

	_nop_();
	_nop_();
	i = 35;
	j = 51;
	k = 182;
	do
	{
		do
		{
			while (--k);
		} while (--j);
	} while (--i);
}

void main()
{
	Init();
	Timer0Init();
	UartInit();
	while(read_t()==85);
	Delay750ms();
	read_t();
	EEPROM_Read(&EEPROM_R,0,2);
	Read_Flag=1;
	T_Set_Old=T_Set;
	D_Set_Old=D_Set;
	while(1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
		Uart_Proc();
	}
	
}
void Init()
{
	P2=0x80;P0=0xff;P2=0x00;
	P2=0xA0;P0=0x00;P2=0x00;
}

unsigned char Key_Read()
{
	unsigned char temp=0;
	P42=0;P41=1;P35=1;P34=1;
	if(P33==0)temp=4;
	if(P32==0)temp=5;
	if(P31==0)temp=6;
	if(P30==0)temp=7;
	P42=1;P41=0;P35=1;P34=1;
	if(P33==0)temp=8;
	if(P32==0)temp=9;
	if(P31==0)temp=10;
	if(P30==0)temp=11;
	P42=1;P41=1;P35=0;P34=1;
	if(P33==0)temp=12;
	if(P32==0)temp=13;
	if(P31==0)temp=14;
	if(P30==0)temp=15;
	P42=1;P41=1;P35=1;P34=0;
	if(P33==0)temp=16;
	if(P32==0)temp=17;
	if(P31==0)temp=18;
	if(P30==0)temp=19;
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