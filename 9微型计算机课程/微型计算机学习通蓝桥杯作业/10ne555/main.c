/*头文件引用*/
#include <STC15F2K60S2.H>
#include <Init.H>
#include <Seg.h>
#define S4 0x08
#define S5 0x04

/* 全局变量定义 */
unsigned char Mode = 0;           // 0-频率 1-周期
bit flag_key_read = 0;  // 按键读取标志
bit flag_fre_read = 0;  // 频率读取标志
unsigned char Trg = 0;  // 按键触发值
unsigned char Cont = 0; // 按键保持值
unsigned long Fre = 0;  // 频率计数器
unsigned int Disp_Fre = 0; // 显示的频率值
unsigned int Timer1000ms = 0; // 1秒定时器
unsigned long Tick = 0; // 周期值
unsigned char Seg_Buf[8] = {0}; // 数码管显示缓冲区
unsigned char Seg_Point[8] = {0}; // 数码管小数点缓冲区
unsigned char Seg_Pos = 0; // 数码管显示位置
unsigned int Count1ms = 0; // 1ms计数器
unsigned char Key_Flag = 0; // 10ms按键标志
unsigned char Timer10ms = 0; // 10ms计时器
unsigned char Timer1ms=0;
unsigned char fre;
/*三行代码*/
void ReadKey()
{
	unsigned char ReadData;
	ReadData = (P3 | 0x10) ^ 0xff;  // 读取P3端口，屏蔽P3.4
	Trg = ReadData & (ReadData ^ Cont);
	Cont = ReadData;
}

/*按键处理函数*/
void Key_Proc()
{
	if(flag_key_read)//消抖
	{
		flag_key_read = 0;
		ReadKey();

		if(Trg == S4)//切换周期和频率
			Mode = (Mode + 1) % 2;

		if(Trg == S5) // S5按下, 开启频率测量功能 (使用宏定义)
		{
			flag_fre_read = 1; //频率读取标志位置1
			Mode = 1;          //切换到频率显示页面
			TR0 = 0;		       //先停止计数，准备重装
			TH0 = 0xFF;        //重装初值
			TL0 = 0xFF;
			Fre = 0;           //清零计数器
			TR0 = 1;		       //开始计数
		}
	}
}

/*信息处理函数*/
void Seg_Proc()
{
	Seg_Buf[0] = Seg_Buf[2] = 11; // "-"
	Seg_Buf[1] = Mode + 1;

	if(Mode == 0) // 处于频率测量
	{
		Seg_Buf[3] = Disp_Fre >= 10000 ? Disp_Fre / 10000 % 10 : 10; // 高位熄灭
		Seg_Buf[4] = Disp_Fre >= 1000 ? Disp_Fre / 1000 % 10 : 10;
		Seg_Buf[5] = Disp_Fre >= 100 ? Disp_Fre / 100 % 10 : 10;
		Seg_Buf[6] = Disp_Fre >= 10 ? Disp_Fre / 10 % 10 : 10;
		Seg_Buf[7] = Disp_Fre % 10;
	}
	else // 处于周期测量界面
	{
		Seg_Buf[3] = Tick >= 10000 ? Tick / 10000 % 10 : 10; // 高位熄灭
		Seg_Buf[4] = Tick >= 1000 ? Tick / 1000 % 10 : 10;
		Seg_Buf[5] = Tick >= 100 ? Tick / 100 % 10 : 10;
		Seg_Buf[6] = Tick >= 10 ? Tick / 10 % 10 : 10;
		Seg_Buf[7] = Tick % 10;
	}
}

/*定时器初始化*/
void TimerInit()
{
	TH0 = 0xFF; // 定时器0计数
	TL0 = 0xFF;
	TH1 = (65536 - 10000) / 256; // 定时器1 定时50ms
	TL1 = (65536 - 10000) % 256;
	TMOD = 0x16; // 0001 0110 高四位控制T1，低四位控制T0
	ET0 = 1;
	ET1 = 1;
	EA = 1;
	TR0 = 0; // 初始关闭，等待按键开启
	TR1 = 1;
}

/*计数器1中断服务函数*/
void service_T0() interrupt 1
{
	Fre++; // 累加频率
}

/*定时器1中断服务函数*/
void service_T1() interrupt 3
{
	TH1 = (65536 - 10000) / 256;
	TL1 = (65536 - 10000) % 256; // 重载初值

	if(++Seg_Pos == 8) Seg_Pos = 0; // 数码管显示专用
	Seg_Disp(Seg_Pos, Seg_Buf[Seg_Pos], Seg_Point[Seg_Pos]);

	// 10ms按键扫描标志
	if(++Timer1ms >= 10)  // 10ms触发一次按键读取
	{
		Timer1ms = 0;
		flag_key_read = 1;
	}

	// 1秒频率显示更新
	if(++Timer1000ms >= 1000)
	{
		Disp_Fre = Fre;
		Fre = 0;
		Timer1000ms = 0;
	}

	// 周期测量（1秒计数）
	if(flag_fre_read)
	{
		Count1ms++;
		if(Count1ms >= 1000) // 1秒到
		{
			Count1ms = 0;
			fre = TH0 * 256 + TL0;
			if(fre != 0) {
				Tick = 1000000 / fre; // 计算周期
			}
			TH0 = 0xFF; // 重装初值
			TL0 = 0xFF;
		}
	}
}

/*Main*/
void main()
{
	System_Init();
	TimerInit();
	while(1)
	{
		if(Key_Flag)
		{
			Key_Proc();
			Key_Flag = 0;
		}
		Seg_Proc();
	}
}
