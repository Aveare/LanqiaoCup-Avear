/* 头文件声明区 */
#include <STC15F2K60S2.H>//15单片机寄存器专用头文件
#include <Init.h>//初始化底层驱动专用头文件
#include <Led.h>//Led底层驱动专用头文件
#include <Key.h>//按键底层驱动专用头文件
#include <Seg.h>//数码管底层驱动专用头文件
#include "onewire.h"
#include "ds1302.h"  		
/* 变量声明区 */
unsigned char Key_Val,Key_Down,Key_Old,Key_Up;//按键专用变量
unsigned char Key_Slow_Down;//按键减速专用变量
unsigned char Seg_Buf[8] = {10,10,10,10,10,10,10,10};//数码管显示数据存放数组
unsigned char Seg_Point[8] = {0,0,0,0,0,0,0,0};//数码管小数点数据存放数组
unsigned char Seg_Pos;//数码管扫描专用变量
unsigned int Seg_Slow_Down;//数码管减速专用变量
unsigned char ucLed[8] = {0,0,0,0,0,0,0,0};//Led显示数据存放数组
unsigned char ucRtc[3]={0x12,0x00,0x02};  // 时、分、秒（BCD码）
unsigned char ucRtc_Set[6];               // 时间设置缓存（0:时十位,1:时个位,2:分十位,3:分个位,4:秒十位,5:秒个位）
unsigned char Disp_Mode;//显示模式 0-时间 1-温度
float Temp;//温度变量
unsigned int Timer3000ms;//定时器定时1s
bit Relay_Flag;

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
		case 4://显示模式切换
			if(++Disp_Mode==2) Disp_Mode=0;
			// 切换回时间模式时，初始化设置缓存为当前时间
			if(Disp_Mode == 0)
			{
				ucRtc_Set[0] = ucRtc[0] / 16; // 时十位
				ucRtc_Set[1] = ucRtc[0] % 16; // 时个位
				ucRtc_Set[2] = ucRtc[1] / 16; // 分十位
				ucRtc_Set[3] = ucRtc[1] % 16; // 分个位
				ucRtc_Set[4] = ucRtc[2] / 16; // 秒十位
				ucRtc_Set[5] = ucRtc[2] % 16; // 秒个位
			}
			break;
		case 5://时设置（S5）
			if(Disp_Mode==0)
			{
				ucRtc_Set[1]++;          // 时个位+1
				if(ucRtc_Set[1] >= 10)   // 个位满10进1
				{
					ucRtc_Set[1] = 0;
					ucRtc_Set[0]++;
				}
				if(ucRtc_Set[0]==2 && ucRtc_Set[1]== 4) // 小时最大23
				{
					ucRtc_Set[0] = 0;
					ucRtc_Set[1] = 0;
				}
				ucRtc[0]=ucRtc_Set[0] * 16 + ucRtc_Set[1]; // 转换为BCD码
				Set_Rtc(ucRtc); // 写入DS1302
			}
			break;
		case 6://分设置（S6）
			if(Disp_Mode==0)
			{
				ucRtc_Set[3]++;          // 分个位+1
				if(ucRtc_Set[3] >= 10)   // 个位满10进1
				{
					ucRtc_Set[3] = 0;
					ucRtc_Set[2]++;
				}
				if(ucRtc_Set[2]== 6)    // 分钟最大59
				{
					ucRtc_Set[2] = 0;
					ucRtc_Set[3] = 0;
				}
				ucRtc[1] = ucRtc_Set[2] * 16 + ucRtc_Set[3]; // 转换为BCD码
				Set_Rtc(ucRtc); // 写入DS1302
			}
			break;
		case 7://秒清零（S7）
			if(Disp_Mode==0)
			{
				ucRtc[2] = 0x00;         // 秒BCD码清零
				ucRtc_Set[4] = 0;        // 秒十位缓存清零
				ucRtc_Set[5] = 0;        // 秒个位缓存清零
				Set_Rtc(ucRtc);          // 写入DS1302
			}
			break;
	}
}

/* 信息处理函数 */
void Seg_Proc()
{
	if(Seg_Slow_Down) return;
	Seg_Slow_Down = 1;//数码管减速程序

	Temp=read_t();
	Temp=read_t();  // 读取两次保证数据稳定
	Read_Rtc(ucRtc); // 读取DS1302当前时间
	switch(Disp_Mode)
	{
		case 0://时间界面
			Seg_Buf[0]=ucRtc[0]/16;    // 时十位
			Seg_Buf[1]=ucRtc[0]%16;    // 时个位
			Seg_Buf[2]=11;             // 分隔符'-'
			Seg_Buf[3]=ucRtc[1]/16;    // 分十位
			Seg_Buf[4]=ucRtc[1]%16;    // 分个位
			Seg_Buf[5]=11;             // 分隔符'-'
			Seg_Buf[6]=ucRtc[2]/16;    // 秒十位
			Seg_Buf[7]=ucRtc[2]%16;    // 秒个位
			Seg_Point[6]=0;            // 关闭小数点
			break;
		case 1://温度界面
			Seg_Buf[0]=12;             // 符号'C'
			Seg_Buf[1]=Seg_Buf[2]=Seg_Buf[3]=Seg_Buf[4]=10; // 熄灭
			Seg_Buf[5]=(unsigned char)Temp/10%10; // 温度十位
			Seg_Buf[6]=(unsigned char)Temp%10;     // 温度个位
			Seg_Buf[7]=(unsigned int)(Temp*10)%10;// 温度小数位
			Seg_Point[6]=1;            // 打开小数点
			break;
	}
}

/* 其他显示函数 */
void Led_Proc()
{
	unsigned char i;
	if((ucRtc[0] == 0x12 || ucRtc[0] == 0x00) && ucRtc[1] == 0x00) Relay_Flag = 1;
	Relay(Relay_Flag);
	for(i=0;i<2;i++)
		ucLed[i]=(Disp_Mode==i);
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
	TL0 = 0x18;		// 重装初始值，避免定时误差
	TH0 = 0xFC;
	if(++Key_Slow_Down >= 10) Key_Slow_Down = 0;//键盘减速专用
	if(++Seg_Slow_Down >= 200) Seg_Slow_Down = 0;//数码管减速专用
	if(++Seg_Pos >= 8) Seg_Pos = 0;//数码管显示专用
	Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],Seg_Point[Seg_Pos]);
	Led_Disp(Seg_Pos,ucLed[Seg_Pos]);
	if(++Timer3000ms==3000&&Relay_Flag)
	{
		Timer3000ms=0;
		Relay_Flag=0;
	}
}

/* Main */
void main()
{
    System_Init();
    Timer0Init();

    Read_Rtc(ucRtc); // 先读取 DS1302 的真实时间

    // 初始化设置缓存
    ucRtc_Set[0] = ucRtc[0] / 16;
    ucRtc_Set[1] = ucRtc[0] % 16;
    ucRtc_Set[2] = ucRtc[1] / 16;
    ucRtc_Set[3] = ucRtc[1] % 16;
    ucRtc_Set[4] = ucRtc[2] / 16;
    ucRtc_Set[5] = ucRtc[2] % 16;

    read_t(); // 预热温度传感器

    while (1)
    {
        Key_Proc();
        Seg_Proc();
        Led_Proc();
    }
}