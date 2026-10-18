/* 头文件声明区 */
#include <STC15F2K60S2.H>
#include <Init.h>
#include <Led.h>
#include <Key.h>
#include <Seg.h>
#include <iic.h>
/* 变量声明区 */
unsigned char Key_Val,Key_Down,Key_Old,Key_Up;//按键专用变量
unsigned char Key_Slow_Down;//按键减速专用变量
unsigned char Seg_Buf[8] = {10,10,10,10,10,10,10,10};//数码管显示数据存放数组
unsigned char Seg_Point[8] = {0,0,0,0,0,0,0,0};//数码管小数点数据存放数组
unsigned char Seg_Pos;//数码管扫描专用变量
unsigned int Seg_Slow_Down;//数码管减速专用变量
unsigned char ucLed[8] = {0,0,0,0,0,0,0,0};//Led显示数据存放数组
bit Seg_Disp_Mode;//数码管显示模式 0-参数界面 1-亮度界面
unsigned int Led_Time_Disp[4]={400,400,400,400};//流转时间间隔数组
unsigned int Led_Time_Ctrol[4]={400,400,400,400};//流转时间间隔控制数组
unsigned char Led_Time_Index;//流转时间间隔索引值 -运行模式编号
unsigned int Timer400ms;//400ms计时变量
bit Seg_Star_Flag;//数码管闪烁标志位
unsigned char Seg_Index;//数码管索引值 0-熄灭 1-模式变化 2-流转间隔闪烁
bit Led_Flag;
unsigned char EEPROM_Dat[4];
unsigned char Led_Mode;//Led流转模式
unsigned int Ms_Tick;
unsigned char Led_Pos;
unsigned char Led_Level;//亮度等级
unsigned char Led_Count;



/* 键盘处理函数 */
void Key_Proc()
{
	unsigned char i;//循环变量
	if(Key_Slow_Down) return;
	Key_Slow_Down = 1;//键盘减速程序

	Key_Val = Key_Read();//实时读取键码值
	Key_Down = Key_Val & (Key_Old ^ Key_Val);//捕捉按键下降沿
	Key_Up = ~Key_Val & (Key_Old ^ Key_Val);//捕捉按键上降沿
	Key_Old = Key_Val;//辅助扫描变量

	if(Seg_Index==0)//处于0-熄灭状态有效
	{
		if(Key_Old==4)Seg_Disp_Mode=1;
		else Seg_Disp_Mode=0;
	}
	switch(Key_Down)
	{
		case 6://流转间隔设置
			if(++Seg_Index==3)Seg_Index=0;
		if(Seg_Index==0)
		{
			Led_Time_Index=0;
			for(i=0;i<4;i++)//利用循环保存
			{
				Led_Time_Ctrol[i]=Led_Time_Disp[i];//保存
				EEPROM_Dat[i]=Led_Time_Ctrol[i]/100;//char类型只有0-255 /100保存可以进去
			}
			EEPROM_Write(EEPROM_Dat,0,4);//写入数据
		}
		break;
		case 7://Led流转启停
			Led_Flag^=1;
		break;
		case 5://加
			if(Seg_Index==1)
			{
				if(++Led_Time_Index==4)
					Led_Time_Index=0;
			}
			else if(Seg_Index==2)
			{
				Led_Time_Disp[Led_Time_Index]+=100;//流转时间+100
				if(Led_Time_Disp[Led_Time_Index]>=1200)Led_Time_Disp[Led_Time_Index]=1200;//限制上限
			}
		break;
		case 4://减
			if(Seg_Index==1)
			{
				if(--Led_Time_Index==255)
					Led_Time_Index=3;
			}
			else if(Seg_Index==2)
			{
				Led_Time_Disp[Led_Time_Index]-=100;//流转时间-100
				if(Led_Time_Disp[Led_Time_Index]<400)Led_Time_Disp[Led_Time_Index]=400;//限制下限
			}
		break;
	}
}

/* 信息处理函数 */
void Seg_Proc()
{
	unsigned char i;
	if(Seg_Slow_Down) return;
	Seg_Slow_Down = 1;//数码管减速程序

	Led_Level=AD_Read(0x03)/64;       //255/64不到4 只有0-3符合char
	
	if(Seg_Disp_Mode==0)//处于参数界面
	{
		Seg_Buf[0]=Seg_Buf[2]=11;//-
		Seg_Buf[1]=Led_Time_Index+1;
		Seg_Buf[4]=Led_Time_Disp[Led_Time_Index]/1000%10;
		Seg_Buf[5]=Led_Time_Disp[Led_Time_Index]/100%10;
		Seg_Buf[6]=Led_Time_Disp[Led_Time_Index]/10%10;
		Seg_Buf[7]=Led_Time_Disp[Led_Time_Index]%10;
		
		if(Seg_Index==1)//模式
		{
		Seg_Buf[0]=Seg_Buf[2]=Seg_Star_Flag?10:11;//-
		Seg_Buf[1]=Seg_Star_Flag?10:Led_Time_Index+1;
		}
		else if(Seg_Index==2)//流转时间
		{
		Seg_Buf[4]=Seg_Star_Flag?10:Led_Time_Disp[Led_Time_Index]/1000%10;
		Seg_Buf[5]=Seg_Star_Flag?10:Led_Time_Disp[Led_Time_Index]/100%10;
		Seg_Buf[6]=Seg_Star_Flag?10:Led_Time_Disp[Led_Time_Index]/10%10;
		Seg_Buf[7]=Seg_Star_Flag?10:Led_Time_Disp[Led_Time_Index]%10;
		}
		if(Seg_Buf[4]==0)Seg_Buf[4]=10;//熄灭
	}
	else//处于亮度界面
	{
		for(i=0;i<6;i++)Seg_Buf[i]=10;//1-5数码管全熄灭
		Seg_Buf[6]=11;//-
		Seg_Buf[7]=Led_Level+1;
	}
}

/* 其他显示函数 */
void Led_Proc()
{
	unsigned char i;
	if(Led_Flag==1)
	{
		if(Ms_Tick==Led_Time_Ctrol[Led_Mode])
		{
			Ms_Tick=0;
			switch(Led_Mode)
			{
				case 0://模式1 0-7
					if(++Led_Pos==8)
					{
						Led_Pos=7;
						Led_Mode=1;
					}
				break;
					
				case 1://模式2 7-0
					if(--Led_Pos==255)Led_Mode=2;
				break;
				case 2:
					Led_Pos+=9;
				if(Led_Pos>34)
				{
					Led_Pos=34;
					Led_Mode=3;
				}
				break;
				case 3:
					Led_Pos-=9;
				if(Led_Pos>200)
				{
					Led_Pos=0;
					Led_Mode=0;
				}
				break;
			}
		}
	}
	if(Led_Mode<2)//前两种互斥点亮
	{
		for(i=0;i<8;i++)ucLed[i]=(i==Led_Pos);
	}
	else         //后两种
	{
		for(i=0;i<8;i++) ucLed[i]=((i==Led_Pos/10)||(i==Led_Pos%10));//个位或十位有一个满足就点亮
		
	}
}

/* 定时器0中断初始化函数 */
void Timer0Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0x7F;		//定时器时钟12T模式
	TMOD &= 0xF0;		//设置定时器模式
	TL0 = 0x18;		//设置定时初始值
	TH0 = 0xFC;		//设置定时初始值
	TF0 = 0;		//清除TF0标志
	TR0 = 1;		//定时器0开始计时
	ET0 = 1;    //定时器中断0打开
	EA = 1;     //总中断打开
}

/* 定时器0中断服务函数 */
void Timer0Server() interrupt 1
{  
	if(++Key_Slow_Down == 10) Key_Slow_Down = 0;//键盘减速专用
	if(++Seg_Slow_Down == 10) Seg_Slow_Down = 0;//数码管减速专用
	if(++Seg_Pos == 8) Seg_Pos = 0;//数码管显示专用
	if(++Led_Count==12)Led_Count=0;//亮度分级计数
	
	if(Seg_Index!=0||Seg_Disp_Mode==1)//亮或灭判断
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],Seg_Point[Seg_Pos]);
	else
		Seg_Disp(Seg_Pos,10,0);
	
	if(Led_Count<=(Led_Level+1)*3)//长按S4实现亮度旋钮
		Led_Disp(Seg_Pos,ucLed[Seg_Pos]);
	else Led_Disp(Seg_Pos,0);
	
	if(++Timer400ms>=400)//闪烁
	{
		Timer400ms=0;
		Seg_Star_Flag^=1;
	}
	if(Led_Flag==1)Ms_Tick++;
}

/* Main */
void main()
{
	unsigned char i;
	System_Init();
	EEPROM_Read(EEPROM_Dat,0,4);
		for(i=0;i<4;i++) Led_Time_Disp[i]=Led_Time_Ctrol[i]=EEPROM_Dat[i]*100;//断电再上电读取
	Timer0Init();
	while (1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
	}
}