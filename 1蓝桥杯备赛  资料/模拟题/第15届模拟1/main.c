/* 头文件声明区 */
#include <STC15F2K60S2.H>//15单片机寄存器专用头文件
#include <Init.h>//初始化底层驱动专用头文件
#include <Led.h>//Led底层驱动专用头文件
#include <Key.h>//按键底层驱动专用头文件
#include <Seg.h>//数码管底层驱动专用头文件
#include <DS1302.H>
#include <IIC.H>
/* 变量声明区 */
unsigned char Key_Val,Key_Down,Key_Old,Key_Up;//按键专用变量
unsigned char Key_Slow_Down;//按键减速专用变量
unsigned char Seg_Buf[8] = {10,10,10,10,10,10,10,10};//数码管显示数据存放数组
unsigned char Seg_Point[8] = {0,0,0,0,0,0,0,0};//数码管小数点数据存放数组
unsigned char Seg_Pos;//数码管扫描专用变量
unsigned int Seg_Slow_Down;//数码管减速专用变量
unsigned char ucLed[8] = {0,0,0,0,0,0,0,0};//Led显示数据存放数组

unsigned char Seg_Mode=0;//0-时间界面 1-输入界面 2-记录界面
unsigned char ucRtc[3]={0x23,0x09,0x59};//写09会以为时是八进制,所以要写9
unsigned char Input_Data[4]={10,10,10,10};//密码门输入
unsigned char Input_Index=0;//密码门索引值
unsigned char JiLu_Time[3]={0x23,0x9,0x59};//触发时间
 
unsigned int Data;//储存输入数据到eeprom
unsigned int Data_Old;//输入数据老的值
bit LL_Flag;//新的比老的大标志位
 
unsigned char EEPROM_String_New[4];//eeprom存放数组
unsigned char EEPROM_String_Old[4];//eeprom读取数组
/* 键盘处理函数 */
void Key_Proc()
{
	unsigned char i;
	if(Key_Slow_Down) return;
	Key_Slow_Down = 1;//键盘减速程序

	Key_Val = Key_Read();//实时读取键码值
	Key_Down = Key_Val & (Key_Old ^ Key_Val);//捕捉按键下降沿
	Key_Up = ~Key_Val & (Key_Old ^ Key_Val);//捕捉按键上升沿
	Key_Old = Key_Val;//辅助扫描变量

	/*--------------------------清0置位--------------------------*/
	if(Seg_Mode==1)//位于输入界面
	{
		if(Key_Down==15)//按下按键5
		{
			for(i=0;i<=4;i++)
				Input_Data[i]=10;//把数据清0
			Input_Index=0;//标志位置位
		}
	}

	/*--------------------------密码门输入--------------------------*/
	if(Seg_Mode==1)
	{
		if(Input_Index==1)Read_Rtc(JiLu_Time);//!!!要放在外面，判断到有数据输入了就读取当前的记录时间
		if(Input_Index<4)//密码门：要限制住输入的位数
		{
			if((Key_Down-1)>=0 && (Key_Down-1)<=9)//对应按键按下
			{
				Input_Data[Input_Index]=Key_Down-1;//把键码值赋值给输入数据
				if(++Input_Index==4)//赋值完后再++
				{
					Input_Index=4;//卡死在4后面跳到界面0再置0
				}
			}
		}
	}
	/*--------------------------按键切换--------------------------*/
	if(Key_Down==14)//切换按键
{
    // 先保存数据（当前在输入界面，准备切换到记录界面）
    if(Seg_Mode==1)
    {
        Data=Input_Data[0]*1000+Input_Data[1]*100+Input_Data[2]*10+Input_Data[3];
        EEPROM_String_New[0]=((JiLu_Time[0] >> 4) * 10) + (JiLu_Time[0] & 0x0F);
        EEPROM_String_New[1]=((JiLu_Time[1] >> 4) * 10) + (JiLu_Time[1] & 0x0F);
        EEPROM_String_New[2]=Data>>8;
        EEPROM_String_New[3]=Data&0x00ff;
        EEPROM_Write(EEPROM_String_New,0,4);
        LL_Flag=(Data>Data_Old)?1:0;
        Data_Old = Data;
    }

    // 再切换界面
    if(++Seg_Mode==3)Seg_Mode=0;

    // 最后清空输入数据
    for(i=0;i<4;i++)
        Input_Data[i]=10;
    Input_Index=0;
}
}

/* 信息处理函数 */
void Seg_Proc()
{
	unsigned char i;
	if(Seg_Slow_Down) return;
	Seg_Slow_Down = 1;//数码管减速程序

	switch(Seg_Mode)
	{
		case 0://时间界面
			Read_Rtc(ucRtc);//可以把时钟放进来，这样只有在时钟界面时才读取时钟，就不会浪费资源
			Seg_Buf[0]=ucRtc[0]/16;
			Seg_Buf[1]=ucRtc[0]%16;
			Seg_Buf[2]=11;
			Seg_Buf[3]=ucRtc[1]/16;
			Seg_Buf[4]=ucRtc[1]%16;
			Seg_Buf[5]=11;
			Seg_Buf[6]=ucRtc[2]/16;
			Seg_Buf[7]=ucRtc[2]%16;
			break;

		case 1://1-输入界面	
    Seg_Buf[0]=12;//显示C
    Seg_Buf[1]=Seg_Buf[2]=Seg_Buf[3]=10;//熄灭前3位
    for(i=4;i<8;i++)Seg_Buf[i] = 10;
			for(i=0;i<Input_Index;i++)Seg_Buf[7-i] = Input_Data[Input_Index-i-1];
    break;

		case 2://记录界面
			Seg_Buf[0]=13;//E
			for(i=1;i<=2;i++)Seg_Buf[i]=10;
			Seg_Buf[3]=JiLu_Time[0]/16;
			Seg_Buf[4]=JiLu_Time[0]%16;
			Seg_Buf[5]=11;
			Seg_Buf[6]=JiLu_Time[1]/16;
			Seg_Buf[7]=JiLu_Time[1]%16;
			break;
	}
}

/* 其他显示函数 */
void Led_Proc()
{
	unsigned char i;
	for(i=0;i<3;i++)
		ucLed[i] = (i==Seg_Mode);
    ucLed[3]=LL_Flag;
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
	if(++Seg_Slow_Down == 20) Seg_Slow_Down = 0;//数码管减速专用
	if(++Seg_Pos == 8) Seg_Pos = 0;//数码管显示专用
	Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],Seg_Point[Seg_Pos]);
	Led_Disp(Seg_Pos,ucLed[Seg_Pos]);
}


/* Main */
void main()
{
	Set_Rtc(ucRtc);
	System_Init();
	Timer0Init();
	EEPROM_Read(EEPROM_String_Old,0,4);
	Data_Old = EEPROM_String_Old[2]<<8 | EEPROM_String_Old[3];
	//把eeprom里的值读出来，十六进制->十进制
	while (1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
	}
}