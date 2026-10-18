#include <STC15F2K60S2.H>
#include <ds1302.h>
#include <iic.h>
#include <onewire.h>

unsigned char Key_Val,Key_Old,Key_Down,Key_Up;
unsigned char Key_Slow_Down;
unsigned int Seg_Slow_Down;
unsigned char Seg_Buf[8]={10,10,10,10,10,10,10,10};
unsigned char Seg_Pos;
unsigned char Seg_Point[8]={0,0,0,0,0,0,0,0};
unsigned char seg_wei[8]={0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
unsigned char code seg_table[]={
0xc0, //0
0xf9, //1
0xa4, //2
0xb0, //3
0x99, //4
0x92, //5
0x82, //6
0xf8, //7
0x80, //8
0x90, //9
0xff, //熄灭
0x88, //A
0x83, //b
0xc6, //C
0xa1, //d
0x86, //E
0x8e, //F
0xbf,//-
0x89, //H
0x8C //P	
	};
unsigned char ucLed[8]={0,0,0,0,0,0,0,0};
unsigned int Freq;//测量频率
unsigned int Timer1000ms;//定时器1s测量频率
unsigned char Disp_Mode=0;//0-时间界面 1-回显界面 2-参数界面 3-温湿度界面
unsigned char ucRtc[3]={13,03,05};
unsigned char Huixian_Mode;//回显界面子界面 0-温度回显 1-湿度回显 2-时间回显

unsigned int Max_T;//最大温度
unsigned int Old_Temp_10x;//上一次温度
unsigned int Max_H;//最大湿度
unsigned int Old_Humid_1x;//上一次湿度
unsigned int hum_all;//总湿度
unsigned int temperature_all;//总温度

unsigned char T_Set=30;//温度参数
unsigned char Trigger_Count=0;//采集次数
unsigned char Time[2];//触发时间
//判断标志
bit Caiji_Flag;//采集标志位
bit useless=1;  //无效状态 1有效 0无效
unsigned int Timer3000ms;//定时3s 用于锁定
unsigned char show_old;//原来的显示模式
bit flag;//亮暗状态标志  1亮 0暗
unsigned int S9_Press_Time = 0;    // S9按键按下持续时间（毫秒）
bit S9_LongPress_Flag = 0;          // S9长按释放标志
bit S9_IsPressing = 0;               // S9是否正在被按下
bit Star_Flag;//闪烁
unsigned char Timer100ms;

//临时变量用于判断
unsigned int temp_T_10x,temp_H_1x;
unsigned char temp_light;
void Init()
{
	P2=0xa0;P0=0x00;P2=0x00;
	P2=0x80;P0=0xff;P2=0x00;
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
	if(P31==0)temp=10;
	if(P30==0)temp=11;
	P44=1;P42=1;P35=0;//P34=1;
	if(P33==0)temp=12;
	if(P32==0)temp=13;
	if(P31==0)temp=14;
	if(P30==0)temp=15;
	return temp;
}

void Seg_Disp(unsigned char wei,table,point)
{
	P2=0xe0;P0=0x00;P2=0x00;
	P2=0xc0;P0=seg_wei[wei];P2=0x00;
	P0=seg_table[table];
	if(point)P0&=0x7f;
	P2=0xe0;P2=0x00;
}

void Led_Disp(unsigned char addr,enable)
{
	static unsigned char temp=0x00;
	if(enable)temp|=0x01<<addr;
		else temp&=~(0x01<<addr);
    P0=~temp;P2 =0x80;P2 =0x00;
}

/*温湿度计算函数*/
float rd_humid(unsigned int Freq)
{
	float Humid_Val;
	Humid_Val=(float)Freq/22.5+1.12;
	if(Humid_Val<10.0||Humid_Val>90.0)return 0;
	else return Humid_Val;
}

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
			if(Caiji_Flag==0)
		{
			Disp_Mode=(Disp_Mode+1)%3;
			Huixian_Mode=0;//每次从时间界面切换到回显界面 默认处于温度回显子界面
		}
		break;
		case 5://回显界面切换
			if(Disp_Mode==1)Huixian_Mode=(Huixian_Mode+1)%3;	
		break;
		case 8://+
			if(Disp_Mode==2)
			{
				T_Set++;
				if(T_Set==100)T_Set=99;
			}
		break;
		case 9://-
			if(Disp_Mode==2)
			{
				T_Set--;
				if(T_Set==255)T_Set=0;
			}
			
		break;
	}
	
    if(Key_Up==9)
    {
       if((Disp_Mode == 1) && (Huixian_Mode == 2) && (S9_Press_Time >= 2000)) 
		S9_LongPress_Flag = 1;  // 设置长按释放标志 
    }
    

    if(S9_LongPress_Flag)
    {
        S9_LongPress_Flag=0;  
        Max_T=0;              
        Old_Temp_10x=0;       
        Max_H=0;              
        Old_Humid_1x=0;       
        hum_all=0;            
        temperature_all =0;    
		
        Time[0]=0;            
        Time[1]=0;            
        
        Trigger_Count=0;      
        
        S9_Press_Time=0;      
    }	
}

void Seg_Proc()
{
	
	
	if(Seg_Slow_Down)return;
	Seg_Slow_Down=1;
	read_t();
	Read_Rtc(ucRtc);
	//获取光照值
	temp_light=AD_Read(1);
	//亮->暗以及未触发时触发
	if(temp_light<50&&flag==1&&Caiji_Flag==0)
	{
		flag=0;
		Caiji_Flag=1;
		useless=1;
		show_old=Disp_Mode;
		
		//新数据覆盖老数据
		Old_Humid_1x=temp_H_1x;
		Old_Temp_10x=temp_T_10x;
		//采集新数据
		temp_T_10x=read_t()*10;
		temp_H_1x=Freq/22.5+1.12;
		if(temp_H_1x>=10&&temp_H_1x<=90)//有效触发
		{
			//置触发状态
			Caiji_Flag=1;
			//触发次数+1
			if(++Trigger_Count==100)Trigger_Count=99;
			//记录触发时间
			Time[0]=ucRtc[0];
			Time[1]=ucRtc[1];
			//最大值判断
			Max_T=(Max_T>temp_T_10x)?Max_T:temp_T_10x;
			Max_H=(Max_H>temp_H_1x)?Max_H:temp_H_1x; 
			//总值计算
			hum_all+=temp_H_1x*10;
			temperature_all+=temp_T_10x;
		}
		else useless=0;
		Disp_Mode=3;
	}
	else if(temp_light>50) flag=1;
	if(Timer3000ms>3000)
	{
		Caiji_Flag=0;
		Timer3000ms=0;
		Disp_Mode=show_old;
	}

		switch(Disp_Mode)
	{
		case 0://时间界面
			Seg_Buf[0]=ucRtc[0]/10;
			Seg_Buf[1]=ucRtc[0]%10;
			Seg_Buf[3]=ucRtc[1]/10;
			Seg_Buf[4]=ucRtc[1]%10;
			Seg_Buf[6]=ucRtc[2]/10;
			Seg_Buf[7]=ucRtc[2]%10;
			Seg_Buf[2]=Seg_Buf[5]=17;//-
			Seg_Point[6]=0;
		break;
		case 1://回显界面
			switch(Huixian_Mode)//回显界面 子界面
			{
				case 0://温度回显
					Seg_Buf[0]=13;//C
						Seg_Buf[1]=10;//熄灭
					Seg_Buf[2]=(Trigger_Count>0)?Max_T/100%10:10;
					Seg_Buf[3]=(Trigger_Count>0)?Max_T/10%10:10;
					Seg_Buf[4]=(Trigger_Count>0)?17:10;//-
					Seg_Buf[5]=(Trigger_Count>0)?(temperature_all/Trigger_Count)/100:10;
					Seg_Buf[6]=(Trigger_Count>0)?(temperature_all/Trigger_Count)/10%10:10;
					Seg_Buf[7]=(Trigger_Count>0)?(temperature_all/Trigger_Count)%10:10;
					Seg_Point[6]=(Trigger_Count>0)?1:0;
		
				break;
				case 1://湿度回显
					Seg_Buf[0]=18;//H
				
					Seg_Buf[1]=10;//熄灭
					Seg_Buf[2]=(Trigger_Count>0)?Max_H/10%10:10;
					Seg_Buf[3]=(Trigger_Count>0)?Max_H%10:10;
					Seg_Buf[4]=(Trigger_Count>0)?17:10;//-
					Seg_Buf[5]=(Trigger_Count>0)?(hum_all/Trigger_Count)/100%10:10;
					Seg_Buf[6]=(Trigger_Count>0)?(hum_all/Trigger_Count)/10%10:10;
					Seg_Buf[7]=(Trigger_Count>0)?(hum_all/Trigger_Count)%10:10;
					Seg_Point[6]=(Trigger_Count>0)?1:0;
				break;
				case 2://时间回显
					Seg_Point[6]=0;
					Seg_Buf[0]=16;//F
					Seg_Buf[1]=Trigger_Count/10%10;
					Seg_Buf[2]=Trigger_Count%10;
				
					Seg_Buf[3]=(Trigger_Count>0)?Time[0]/10:10;
					Seg_Buf[4]=(Trigger_Count>0)?Time[0]%10:10;
					Seg_Buf[5]=(Trigger_Count>0)?17:10;//-
					Seg_Buf[6]=(Trigger_Count>0)?Time[1]/10:10;
					Seg_Buf[7]=(Trigger_Count>0)?Time[1]%10:10;
					
			
				break;
			}
		break;
		case 2://参数界面
		Seg_Point[6]=0;
			Seg_Buf[0]=19;//P	
			Seg_Buf[1]=Seg_Buf[2]=Seg_Buf[3]=Seg_Buf[4]=Seg_Buf[5]=10;
			Seg_Buf[6]=T_Set/10%10;
			Seg_Buf[7]=T_Set%10;
			break;
		case 3://温湿度界面
			Seg_Point[6]=0;
			Seg_Buf[0]=15;//E
			Seg_Buf[1]=Seg_Buf[2]=10;
			Seg_Buf[3]=temp_T_10x/100%10;
			Seg_Buf[4]=temp_T_10x/10%10;
			Seg_Buf[5]=17;//-
			Seg_Buf[6]=(temp_H_1x>=10&&temp_H_1x<=90)?temp_H_1x/10:11;
			Seg_Buf[7]=(temp_H_1x>=10&&temp_H_1x<=90)?temp_H_1x%10:11;
		break;
	}
	
}
void Led_Proc()
{
	ucLed[0]=(Disp_Mode==0)?1:0;
	ucLed[1]=(Disp_Mode==1)?1:0;
	ucLed[2]=(Disp_Mode==3)?1:0;
	ucLed[3]=(temp_T_10x/10.0>T_Set)?Star_Flag:0;
	ucLed[4]=(useless==0)?1:0;
	ucLed[5]=(Trigger_Count>1&&(temp_H_1x>Old_Humid_1x&&temp_T_10x>Old_Temp_10x))?1:0;
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
	TR0=1;
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

void Timer1server(void) interrupt 3
{
	//NE555采集频率
	if(++Timer1000ms==1000)
	{
		TR0=0;
		Timer1000ms=0;
		Freq=(TH0<<8|TL0);
		TH0=TL0=0;
		TR0=1;
	}
	if(++Key_Slow_Down==10)Key_Slow_Down=0;
	if(++Seg_Slow_Down==100)Seg_Slow_Down=0;
	if(++Seg_Pos==8)Seg_Pos=0;
	Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],Seg_Point[Seg_Pos]);
	Led_Disp(Seg_Pos,ucLed[Seg_Pos]);
	//采集标志位
	if(Caiji_Flag)Timer3000ms++;
	
	//长按2s
	// 判断条件：在回显界面的时间子界面，且S9被按下
    if((Disp_Mode == 1) && (Huixian_Mode == 2) && (Key_Val == 9))
    {
        S9_IsPressing = 1;  // 标记S9正在被按下
        
        // 计时增加，但不超过2000ms
        if(S9_Press_Time < 2000)
        {
            S9_Press_Time++;
        }
    }
    else
    {
        S9_IsPressing = 0;  // S9没有被按下
        
        // 如果不是长按释放的情况，才清零计时器
        if(!S9_LongPress_Flag)
        {
            S9_Press_Time = 0;
        }
    }
	//闪烁
	if(++Timer100ms==100)
	{
		Star_Flag^=1;
		Timer100ms=0;
	}
}

void Delay750ms()		//@12.000MHz
{
	unsigned char i, j, k;
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
	Delay750ms();
	read_t();
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