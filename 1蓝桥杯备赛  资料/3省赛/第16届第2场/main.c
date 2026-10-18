/* 头文件声明区 */
#include <STC15F2K60S2.H>//15单片机寄存器专用头文件
#include <Init.h>//初始化底层驱动专用头文件
#include <Led.h>//Led底层驱动专用头文件
#include <Key.h>//按键底层驱动专用头文件
#include <Seg.h>//数码管底层驱动专用头文件
#include <ultrasonic.h>
#include "onewire.h"
#include "iic.h"
/* 变量声明区 */
unsigned char Key_Val,Key_Down,Key_Old,Key_Up;//按键专用变量
unsigned char Key_Slow_Down;//按键减速专用变量
unsigned char Seg_Buf[8] = {10,10,10,10,10,10,10,10};//数码管显示数据存放数组
unsigned char Seg_Point[8] = {0,0,0,0,0,0,0,0};//数码管小数点数据存放数组
unsigned char Seg_Pos;//数码管扫描专用变量
unsigned int Seg_Slow_Down;//数码管减速专用变量
unsigned char ucLed[8] = {0,0,0,0,0,0,0,0};//Led显示数据存放数组
unsigned char Disp_Mode;//显示模式 0-测距界面 1-时间参数 2-次数参数

unsigned char Dis_Mode=15;//测距模式切换索引
unsigned char Sec[4]={2,4,6,4};//时间参数
unsigned char Sec_Index=0;//时间参数索引
unsigned char Count[4]={3,5,7,5};//次数参数
unsigned char Count_Index=0;//时间参数索引
float Temp;//温度变量
bit One_Flag=0,Plus_Flag=0;//单次or多次 测量标志位
unsigned int Ultra;//超声波读取传播时间
unsigned int Distance;//超声波距离
bit S12_Flag=0;
bit L2_Flag=0;
bit Star_Flag=0;//闪烁标志位
unsigned char Timer100ms;//100ms闪烁标志位
unsigned char Done;//累加次数
unsigned char Done_Flag;//累加次数标志位
unsigned char Start_Plus=0;//累计标志位
unsigned int Count_Time_Para;//累加测量 单次时间
unsigned char First_Flag=0;//刚上电标志位 1-初上电 0-非初上电状态
bit Cancel_Flag;//单次测量打断累加测量标志位
/* 键盘处理函数 */
void Key_Proc()
{
	if(Key_Slow_Down) return;
	Key_Slow_Down = 1;//键盘减速程序

	Key_Val = Key_Read();//实时读取键码值
	Key_Down = Key_Val & (Key_Old ^ Key_Val);//捕捉按键下降沿
	Key_Up = ~Key_Val & (Key_Old ^ Key_Val);//捕捉按键上降沿
	Key_Old = Key_Val;//辅助扫描变量

	switch(Key_Down)
	{
		case 8://显示模式切换
			if(Plus_Flag==0)//非多次测量模式下
			Disp_Mode=(Disp_Mode+1)%3;
		break;
		case 4://单次测量按键
			if(Disp_Mode==0)
			{
				Plus_Flag=0;
				Done = 0;
				One_Flag=1;//开启单次测量
				Dis_Mode=13;
				S12_Flag=0;
				Distance=0;//清零
				if(Start_Plus)Cancel_Flag=1;//多次测量过程中置1开启打断
			}
		break;
		case 5://累加测量按键
			if(Disp_Mode==0)
			{
				Plus_Flag=1;//开启多次测量
				Dis_Mode=14;
				S12_Flag=0;
				Distance=0;//清零
			}
		break;
		case 9://参数设置
			if(Disp_Mode==1)//在时间参数界面
				Sec_Index=(Sec_Index+1)%4;//0-3循环
			else if(Disp_Mode==2)//在次数参数界面
				Count_Index=(Count_Index+1)%4;//0-3循环
		break;
		case 12://清除按钮
			if(Plus_Flag==0)//非多次测量模式下
			{
				Distance=0;//直接熄灭前三位了
				S12_Flag=1;//清除标志位，配合下面三元运算符
				Dis_Mode=15;
			}
		break;
	}
}

/* 信息处理函数 */
void Seg_Proc()
{
	if(Seg_Slow_Down) return;
	Seg_Slow_Down = 1;//数码管减速程序

	Temp=read_t();//读取温度
	
	//超声波单次测距
	if(One_Flag)
	{
		First_Flag=0;//结束初上电状态
		Ultra=Ut_Wave_Data()*(330.0f+0.6f*Temp)/20000.0f;
		Distance=Ultra;
		One_Flag=0;
	}
	
	//超声波多次测量
	if(Plus_Flag)
	{
		First_Flag=0;//结束初上电状态
		Start_Plus=1;//开启累计标志位
		if(Start_Plus)
		{
			if(Done==0)
			{
				Ultra=Ut_Wave_Data()*(330.0f+0.6f*Temp)/20000.0f;//那么读取距离加在Distance上
				Distance += Ultra;
				Done++;//已完成次数+1
			}
			if(Done!=0)
			{
				if(Done_Flag==1)//完成单次间隔时间计时
				{
					Ultra=Ut_Wave_Data()*(330.0f+0.6f*Temp)/20000.0f;//那么读取距离加在Distance上
					Distance += Ultra;
					Done++;//已完成次数+1
					Done_Flag=0;
				}
			}
		}
	}
	if(Done==Count[Count_Index]) 
	{
		Plus_Flag=Start_Plus=0;//到达次数了，结束累加
		Cancel_Flag=0;//置0再允许打断
		Star_Flag=0;//关闪烁
	}
	
	switch(Disp_Mode)
	{
		case 0://测距界面
			Seg_Buf[0]=Dis_Mode;
			Seg_Buf[1]=15;//C
			Seg_Buf[2]=(unsigned char)Temp/10%10;
			Seg_Buf[3]=(unsigned char)Temp%10;
			//显示距离
			Seg_Buf[4]=Distance>=1000?Distance/1000%10:10;
			Seg_Buf[5]=Distance>=100?Distance/100%10:10;
			Seg_Buf[6]=Distance>=10?Distance/10%10:10;
			Seg_Buf[7]=(S12_Flag||First_Flag)?10:Distance%10;  //是否清除or刚上电 1是—>熄灭 0否->显示个位
		break;
		case 1://时间参数设置
			Seg_Buf[0]=12;
			Seg_Buf[1]=1;
			Seg_Buf[2]=Seg_Buf[3]=Seg_Buf[4]=Seg_Buf[5]=10;
			Seg_Buf[6]=0;
			Seg_Buf[7]=Sec[Sec_Index]%10;
		break;
		case 2://次数参数设置
			Seg_Buf[0]=12;
			Seg_Buf[1]=2;
			Seg_Buf[2]=Seg_Buf[3]=Seg_Buf[4]=Seg_Buf[5]=10;
			Seg_Buf[6]=0;
			Seg_Buf[7]=Count[Count_Index]%10;
		break;
	}

	//Led判断功能
	if((Distance>0&&Distance<20)||Distance>100)L2_Flag=1;//小于20cm或大于100cm  L2亮
	else L2_Flag=0;
}

/* 其他显示函数 */
void Led_Proc()
{
	
	//DAC功能
	if(Ut_Wave_Data()==0&&First_Flag)//返回值为0 无效测量
		DA_Write(0);
	else //有效测量
	{
		if((First_Flag==0)&&(One_Flag==0||Plus_Flag==0))// 并非初上电and（单次测量完成or累加测量完成）
		{
			if((Distance>0&&Distance<20))DA_Write(51);
			else if(Distance>100)DA_Write(255);
			else//一次函数输出转换
				DA_Write(51.0f+((float)Distance-20.0f)*0.05f*51.0f);
		}
	}
	ucLed[0]=(Disp_Mode>0)?0:1;
	ucLed[1]=L2_Flag;
	ucLed[2]=Star_Flag;
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
	if(++Seg_Slow_Down == 80) Seg_Slow_Down = 0;//数码管减速专用
	if(++Seg_Pos == 8) Seg_Pos = 0;//数码管显示专用
	Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],Seg_Point[Seg_Pos]);
	Led_Disp(Seg_Pos,ucLed[Seg_Pos]);
	
//累加模块
	if(Start_Plus)
	{
		if(++Timer100ms==100)Star_Flag^=1;
		Count_Time_Para++;
		if(Count_Time_Para==Sec[Sec_Index]*1000)
		{
			Count_Time_Para=0;
			Done_Flag=1;  //读完间隔时间
		}
		
		//单次测量打断
		if(Cancel_Flag==1)
		{
			Plus_Flag=Start_Plus=0;//直接标志位置0打断
			Cancel_Flag=0;//置0再允许打断
			Star_Flag=0;//关闪烁
		}
	}
	else Timer100ms=0;
}

/*上电延迟函数*/
void Delay750ms(void)	//@12.000MHz
{
	unsigned char data i, j, k;
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

/* Main */
void main()
{
	System_Init();
	Delay750ms();//上电延迟750ms 防止85C显示
	Timer0Init();
	First_Flag=1;//刚上电标志位
	while (1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
	}
}