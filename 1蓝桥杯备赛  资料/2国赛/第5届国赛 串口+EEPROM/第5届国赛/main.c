/*头文件*/
#include <STC15F2K60S2.H>
#include <ds1302.h>
#include <iic.h>
#include <onewire.h>
#include <intrins.h>
#include <Uart.h>
#include <string.h>
#include <intrins.h>
#include <stdio.h>
/*底层*/
unsigned char code seg_wei[]={0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
unsigned char code seg_table[]={0xC0,0xF9,0xA4,0xB0,0x99,0x92,0x82,0xF8,0x80,0x90,0xff,0xbf,0xc6,0x89};
unsigned char Seg_Pos=0;
unsigned char Seg_Buf[8]={10,10,10,10,10,10,10,10};
unsigned char Seg_Point[8]={0,0,0,0,0,0,0,0};
unsigned char Key_Slow=0;
unsigned int Seg_Slow=0;
unsigned char ucLed[8]={0,0,0,0,0,0,0,0};
/*底层声明*/
void Init();
unsigned char Key_Read();
void Led_Disp(unsigned char addr,enbale);
void Seg_Disp(unsigned char wei,table,point);
/*变量*/
unsigned char Disp_Mode=0;//显示模式 0-温度湿度 1-时间
float temperature;//温度
float humid;//湿度
unsigned char ucRtc[3]={23,59,55};
unsigned int Timer1s=0;//闪烁
bit Star_Flag;//闪烁标志位
unsigned char Lex;
unsigned char Lex_Flag=0;
long Time;//停留时间 ms
unsigned int Dwell_Tm=0;//s
bit Mode;//0-温湿度 1停留时间
/*串口*/
char Uart_Send[30];
char Uart_Rec[7];
char Uart_Rec_Index;
unsigned int Uart_Slow_Down;
bit Send_Flag=0;//收到AAASSS自动发送标志位
bit Work_Mode;//0-自动传输 1-自动记录
bit Send_1s_Flag;//1s发送间隔
bit cmd_ready;
/*EEPROM存储*/
unsigned int near_hour;
unsigned int near_min;
unsigned int near_sec;
unsigned int near_temperature;
unsigned int near_humid;
unsigned int Stay_Time;
unsigned char EEPROM_String[7];  // 临时存储数组（类似图片写法）
unsigned char current_index = 0;           // 当前写入位置(0-4)
bit record_full = 0;    
unsigned char read_buf[7];
unsigned char dwell_time;
/*按键处理*/
void Key_Proc()
{
	unsigned char Key_Val,Key_Down,Key_Old;
	if(Key_Slow)return;
	Key_Slow=1;
	
	Key_Val=Key_Read();
	Key_Down=Key_Val&(Key_Val^Key_Old);
	Key_Old=Key_Val;

	switch(Key_Down)
	{
		case 4://数码管显示状态切换
			Work_Mode^=1;
		break;
		case 5://显示模式切换
		Disp_Mode=(Disp_Mode+1)%3;//01循环
		break;	
	}
}
/*信息处理*/
void Seg_Proc()
{
	if(Seg_Slow)return;
	Seg_Slow=1;
	//重要信息读取
	temperature=read_t();
	EA=0;humid=AD_Read(1)/51.0*20.0;EA=1;
	EA=0;Lex=AD_Read(3);EA=1;
	Read_Rtc(ucRtc);
	
	if(Lex<50&&Lex_Flag==0)//第一次有物体停留
	{
		
		Lex_Flag=1;//靠近
		if(Work_Mode==1)//自动记录模式并且自动保存
		{
			/*EEPROM存储*/
			near_hour = ucRtc[0];
			near_min = ucRtc[1];
			near_sec = ucRtc[2];
			near_temperature=temperature;
			near_humid=humid;
		}
		Dwell_Tm=0;
	}
	else if(Lex>=50&&Lex_Flag==1)//物体离开
	{
		Lex_Flag=0;
		Dwell_Tm=Time/1000;
		if(Work_Mode==1)//自动记录模式
		{
			/*EEPROM存储*/
			EEPROM_String[0] = near_temperature;  // 温度
            EEPROM_String[1] = near_humid;        // 湿度
			EEPROM_String[2] =	near_hour;  // 时
			EEPROM_String[3] =	near_min;  // 分
			EEPROM_String[4] =	near_sec;  // 秒
            EEPROM_String[5] = (unsigned char)(Dwell_Tm >> 8);   
			EEPROM_String[6] = (unsigned char)(Dwell_Tm & 0xFF); 
			EEPROM_Write(EEPROM_String,0x00+current_index*7,7); 
            //更新索引超过5组覆盖
            current_index++;
            if(current_index >= 5)
            {
                current_index = 0;
                record_full = 1;//计满了
            }
		}
		Time=0;
	}
		

	switch(Disp_Mode)
	{
		case 0:
			Seg_Buf[0]=(unsigned int)temperature/10%10;
			Seg_Buf[1]=(unsigned int)temperature%10;
			Seg_Buf[2]=12;//C
			Seg_Buf[3]=Seg_Buf[4]=10;//熄灭
			Seg_Buf[5]=(unsigned int)humid/10%10;
			Seg_Buf[6]=(unsigned int)humid%10;
			Seg_Buf[7]=13;//H	
			
		break;
		case 1://实时时钟
			Seg_Buf[0]=ucRtc[0]/10;
			Seg_Buf[1]=ucRtc[0]%10;
			Seg_Buf[2]=Star_Flag?11:10;//-
			Seg_Buf[3]=ucRtc[1]/10;
			Seg_Buf[4]=ucRtc[1]%10;
			Seg_Buf[5]=Star_Flag?11:10;//-
			Seg_Buf[6]=ucRtc[2]/10;
			Seg_Buf[7]=ucRtc[2]%10;
		break;
		case 2://停留时间
			Seg_Buf[0]=Seg_Buf[1]=Seg_Buf[2]=10;
			Seg_Buf[3]=11;//-
			Seg_Buf[4]=Dwell_Tm/1000%10;
			Seg_Buf[5]=Dwell_Tm/100%10;
			Seg_Buf[6]=Dwell_Tm/10%10;
			Seg_Buf[7]=Dwell_Tm%10;
		break;
	}
}

/*Led*/
void Led_Proc()
{
	ucLed[0]=!Work_Mode;
	ucLed[1]=Work_Mode;
	ucLed[2]=Lex_Flag;
	
}
/*串口*/
void Uart_Proc()
{
	unsigned char record_count;//记录时间先后顺序
    unsigned char idx;//现在的顺序
	unsigned char i;
	if(Uart_Slow_Down)return;
	Uart_Slow_Down=1;

	if (Uart_Rec_Index > 0)
    {
		//接收判断
        if(strcmp(Uart_Rec,"AAASSS")==0) Send_Flag=cmd_ready=1;
        Uart_Rec_Index = 0;
		memset(Uart_Rec, 0, sizeof(Uart_Rec));
	}
	
		//进行发送
		if(Send_Flag)
		{
			if(Work_Mode==0)//自动传输模式 每秒发1次
			{
				if(Send_1s_Flag)
				{
					Send_1s_Flag=0;
					sprintf(Uart_Send,"{%02d-%02d%}{%02d-%02d-%02d}{%d}\r\n",
					(unsigned int)temperature,
					(unsigned int)humid,
					(unsigned int)ucRtc[0],
					(unsigned int)ucRtc[1],
					(unsigned int)ucRtc[2],
					(unsigned int)Lex_Flag);
					Uart_Send_String(Uart_Send);
				}
			}	
			else if(Work_Mode==1&&cmd_ready)//自动记录模式
			{
				cmd_ready=0;
				record_count=record_full?5:current_index;
				
				 // 循环读取并发送每条记录
					for(i = 0; i < record_count; i++)
					{
					// 计算实际存储位置
					if(record_full)idx=(current_index+i)%5;  // 从最早的开始读
					else idx = i;
					//再读取到临时数组里
					EEPROM_Read(read_buf,0x00+idx*7,7);
					dwell_time = ((unsigned int)read_buf[5] << 8) | read_buf[6];
						sprintf(Uart_Send, "{%02d-%02d%%}{%02d-%02d-%02d}{%d}\r\n",
						(unsigned int)read_buf[0],//温度
						(unsigned int)read_buf[1],//湿度
						(unsigned int)read_buf[2],//时
						(unsigned int)read_buf[3],//分
						(unsigned int)read_buf[4],//秒
						(unsigned int)dwell_time);//停留时间
						Uart_Send_String(Uart_Send);	
					}					
				Send_Flag = 0;
			}
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
	if(++Key_Slow==100)Key_Slow=0;
	if(++Seg_Slow==1000)Seg_Slow=0;
	if(++Uart_Slow_Down==300) Uart_Slow_Down=0;
	if(++Seg_Pos==8)Seg_Pos=0;
	Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],Seg_Point[Seg_Pos]);
	Led_Disp(Seg_Pos,ucLed[Seg_Pos]);
	
	if(++Timer1s==1000)
	{
		Timer1s=0;
		Star_Flag^=1;
		Send_1s_Flag=1;
	}
	//停留时间
	if(Lex_Flag)Time++;
}

/* 串口初始化函数 */
void UartInit(void)		//9600bps@12.000MHz
{
	SCON = 0x50;		//8位数据,可变波特率
	AUXR |= 0x01;		//串口1选择定时器2为波特率发生器
	AUXR &= 0xFB;		//定时器2时钟为Fosc/12,即12T
	T2L = 0xE6;		//设定定时初值  65526-256=65270 (9600波特率)
	T2H = 0xFF;		//设定定时初值
	AUXR |= 0x10;		//启动定时器2
	ES = 1;             //使能串口中断
	EA = 1;             //开启总中断
}

void Uart1Server(void) interrupt 4
{
	if(RI == 1)
	{
		Uart_Rec[Uart_Rec_Index]=SBUF;
		Uart_Rec_Index++;
		if(Uart_Rec_Index < 10)
			Uart_Rec[Uart_Rec_Index] = '\0';
		RI=0;
	}
}

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
	Set_Rtc(ucRtc);
	Delay750ms();
	read_t();
	Uart_Send_String("Init ok");
	while(1)
	{
		Seg_Point[0]=Seg_Point[1]=0;
		Key_Proc();
		Seg_Point[0]=Seg_Point[1]=0;
		Seg_Proc();
		Seg_Point[0]=Seg_Point[1]=0;
		Led_Proc();
		Seg_Point[0]=Seg_Point[1]=0;
		Uart_Proc();
		Seg_Point[0]=Seg_Point[1]=0;
	}
}
void Init()
{
	P2=0x80;P0=0xff;P2=0x00;
	P2=0xA0;P0=0x00;P2=0x00;
}
unsigned char Key_Read()
{
	unsigned char temp;
	if(P33==0)temp=4;
	if(P32==0)temp=5;
	return temp;
}

void Led_Disp(unsigned char addr,enable)
{
	static unsigned char temp=0x00;
	if(enable) temp|=(0x01<<addr);
		else temp&=~(0x01<<addr);
	P0=~temp;P2=0x80;P2=0x00;
}

void Seg_Disp(unsigned char wei,table,point)
{
	P2=0xe0;P0=0xff;P2=0x00;
	P2=0xc0;P0=seg_wei[wei];P2=0x00;
	P0=seg_table[table];
	if(point)P0&=~(1<<7);
	P2=0xe0;P2=0x00;
}