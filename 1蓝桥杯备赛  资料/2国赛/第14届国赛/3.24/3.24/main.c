/*头文件引用*/
#include <STC15F2K60S2.H>
#include <iic.h>
#include <onewire.h>
#include <Ultra.h>
#include <math.h>
/*底层变量声明*/
unsigned char Key_Val, Key_Down, Key_Up, Key_Old;
unsigned char Key_Slow;
unsigned int Seg_Slow;
unsigned char Seg_Pos;
unsigned char code seg_wei[8] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
unsigned char code seg_table[] = {0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8, 0x80, 0x90, 0xff, 0xbf, 0x8c, 0x8e};
unsigned char Seg_Buf[8] = {10, 10, 10, 10, 10, 10, 10, 10};
unsigned char Seg_Point[8] = {0, 0, 0, 0, 0, 0, 0, 0};
unsigned char ucLed[8] = {0, 0, 0, 0, 0, 0, 0, 0};
/*底层函数声明*/
void Init();
unsigned char Key_Read();
void Seg_Disp(unsigned char wei, unsigned char table, bit point);
void Led_Disp(unsigned char addr, bit enable);
void Relay(bit enable);
void Reset_Default();
/*变量*/
unsigned char Disp_Mode = 0;		// 0-测距界面 1-参数界面 2-工厂界面
unsigned int distance;				// 实时距离 cm
bit CM_Mode = 0;					// 0-cm 1-m
unsigned char distance_set = 40;	// 距离参数 10-90cm
float temperature;					// 实时温度
unsigned char temperature_set = 30; // 温度参数 0-80℃
bit Set_Mode;						// 参数切换 0-距离参数 1-温度参数
unsigned char Factory_Mode;			// 工厂模式切换 0-校准值 1-超声波介质速度 2-Dac输出下限
char calibration = 0;				// 校准值 -90-90
unsigned int Ut_Speed = 340;		// 超声波传播速度  m/s
unsigned char Dac_Least_10x = 10;	// DAC输出下限值
bit Reset_Flag = 0;
unsigned int Timer2s;
bit Key_Enable = 1;				   // 按键使能 0-未使能 1-使能
unsigned char Distance_Record[12]; // 测距记录数组
unsigned char Distance_Record_Index;
unsigned int Ultra_Slow; // 超声波延迟
bit Vol_Output_Flag;	 // 电压输出判断
bit Record_Judge;		 // 测距记录判断
unsigned int Record_Time;
unsigned char DAC_Output_Index; // dac输出索引
unsigned int Dac_Slow;			// dac输出延迟
unsigned char Timer100ms;
bit Star_Flag;
bit Relay_Flag;
/*恢复出厂设置*/
void Reset_Default()
{
	unsigned char i;

	Disp_Mode = 0;
	CM_Mode = 0;
	Set_Mode = 0;
	Factory_Mode = 0;

	distance_set = 40;
	temperature_set = 30;
	calibration = 0;
	Ut_Speed = 340;
	Dac_Least_10x = 10;

	Reset_Flag = 0;
	Timer2s = 0;

	Key_Enable = 1;
	Record_Judge = 0;
	Vol_Output_Flag = 0;
	Distance_Record_Index = 0;
	DAC_Output_Index = 0;
	Record_Time = 0;
	Dac_Slow = 0;

	Relay_Flag = 0;
	Relay(0);
	DA_Write(0);

	for (i = 0; i < 12; i++)
		Distance_Record[i] = 0;
}
/*按键处理*/
void Key_Proc()
{
	if (Key_Slow)
		return;
	Key_Slow = 1;

	Key_Val = Key_Read();
	Key_Down = Key_Val & (Key_Val ^ Key_Old);
	Key_Up = ~Key_Val & (Key_Val ^ Key_Old);
	Key_Old = Key_Val;

	/* 同时按下 S8、S9：启动长按计时 */
	if (Key_Val == 89)
		Reset_Flag = 1;
	else
	{
		/* 未达到 2s 就松手，清除长按状态 */
		Reset_Flag = 0;
		Timer2s = 0;
	}

	/* 记录过程中，所有按键无效 */
	if (Key_Enable == 0)
		return;
	switch (Key_Down)
	{
	case 4: // 显示界面切换
		Disp_Mode = (Disp_Mode + 1) % 3;
		CM_Mode = Set_Mode = Factory_Mode = 0; // 进入默认界面
		break;
	case 5: // 模式切换
		if (Disp_Mode == 0)
			CM_Mode ^= 1; // 测距界面下，切换距离显示单位
		if (Disp_Mode == 1)
			Set_Mode ^= 1; // 在参数界面下，切换距离参数和温度参数两个界面
		if (Disp_Mode == 2)
			Factory_Mode = (Factory_Mode + 1) % 3; // 在工厂模式下，切换校准值设置、介质设置、DAC 输出设置三个界面
		break;
	case 8: //+
		// 参数界面
		if (Disp_Mode == 1)
		{
			if (Set_Mode == 0) // 距离参数界面下，按下后，距离参数值加 10cm
			{
				distance_set += 10;
				if (distance_set > 90)
					distance_set = 10;
			}
			else // 温度参数界面下，按下后，温度参数值加 1℃。
			{
				temperature_set += 1;
				if (temperature_set > 80)
					temperature_set = 0;
			}
		}
		// 工厂模式
		else if (Disp_Mode == 2)
		{
			if (Factory_Mode == 0) // 校准值设置界面下，按下后，校准值加 5cm
			{
				calibration += 5;
				if (calibration > 90)
					calibration = -90;
			}
			else if (Factory_Mode == 1) // 介质设置界面下，按下后，传输速度值加 10m/s。
			{
				Ut_Speed += 10;
				if (Ut_Speed > 9990)
					Ut_Speed = 10;
			}
			else if (Factory_Mode == 2) // DAC 输出设置界面下，按下后，DAC 输出下限值加 0.1V。
			{
				Dac_Least_10x += 1;
				if (Dac_Least_10x > 20)
					Dac_Least_10x = 1;
			}
		}
		// 测距界面下
		else if (Disp_Mode == 0)
		{
			Key_Enable = 0;
			Distance_Record_Index = 0;
			Record_Time = 0;
			Record_Judge = 0;
			DAC_Output_Index = 0;
			Vol_Output_Flag = 0;
		}
		break;
	case 9: //-
		// 参数界面
		if (Disp_Mode == 1)
		{
			if (Set_Mode == 0) // 距离参数界面下，按下后，距离参数值减 10cm
			{
				distance_set -= 10;
				if (distance_set < 10)
					distance_set = 90;
			}
			else // 温度参数界面下，按下后，温度参数值减 1℃。
			{
				temperature_set -= 1;
				if (temperature_set == 255)
					temperature_set = 80;
			}
		}
		// 工厂模式
		if (Disp_Mode == 2)
		{
			if (Factory_Mode == 0) // 校准值设置界面下，按下后，校准值减 5cm
			{
				calibration -= 5;
				if (calibration < -90)
					calibration = 90;
			}
			else if (Factory_Mode == 1) // 介质设置界面下，按下后，传输速度值减 10m/s。
			{
				Ut_Speed -= 10;
				if (Ut_Speed < 10)
					Ut_Speed = 9990;
			}
			else if (Factory_Mode == 2) // DAC 输出设置界面下，按下后，DAC 输出下限值减 0.1V。
			{
				Dac_Least_10x -= 1;
				if (Dac_Least_10x < 1)
					Dac_Least_10x = 20;
			}
		}
		// 测距界面下
		if (Disp_Mode == 0)
		{
			/* 只有完成过记录后才允许输出 */
			if (Record_Judge == 1)
			{
				Vol_Output_Flag = 1;
				DAC_Output_Index = 0;
				Dac_Slow = 0;
			}
		}
		break;
	}
}
/*信息处理*/
void Seg_Proc()
{
	if (Seg_Slow)
		return;
	Seg_Slow = 1;
	// 重要信息读取
	temperature = read_t();

	Seg_Buf[0] = Seg_Buf[1] = Seg_Buf[2] = Seg_Buf[3] = 10;
	Seg_Buf[4] = Seg_Buf[5] = Seg_Buf[6] = Seg_Buf[7] = 10;
	Seg_Point[0] = Seg_Point[1] = Seg_Point[2] = Seg_Point[3] = 0;
	Seg_Point[4] = Seg_Point[5] = Seg_Point[6] = Seg_Point[7] = 0;

	switch (Disp_Mode)
	{
	case 0: // 测距界面
		Seg_Buf[0] = (unsigned int)temperature / 10 % 10;
		Seg_Buf[1] = (unsigned int)temperature % 10;
		Seg_Buf[2] = (unsigned int)(temperature * 10) % 10;
		Seg_Point[1] = 1;
		Seg_Buf[3] = 11; //-
		if (!CM_Mode)	 // cm模式
		{
			Seg_Buf[4] = distance >= 1000 ? distance / 1000 % 10 : 10;
			Seg_Buf[5] = distance >= 100 ? distance / 100 % 10 : 10;
			Seg_Buf[6] = distance >= 10 ? distance / 10 % 10 : 10;
			Seg_Buf[7] = distance % 10;
			Seg_Point[5] = 0;
		}
		else // m模式
		{
			if (distance >= 1000) // 10.00 ~ 99.99
			{
				Seg_Buf[4] = distance / 1000 % 10;
				Seg_Buf[5] = distance / 100 % 10;
				Seg_Point[5] = 1;
				Seg_Buf[6] = distance / 10 % 10;
				Seg_Buf[7] = distance % 10;
			}
			else // 0.10 ~ 9.99，左侧熄灭
			{
				Seg_Buf[4] = 10;
				Seg_Buf[5] = distance / 100 % 10;
				Seg_Point[5] = 1;
				Seg_Buf[6] = distance / 10 % 10;
				Seg_Buf[7] = distance % 10;
			}
		}
		Seg_Point[6] = 0;
		break;
	case 1:				 // 参数界面
		Seg_Buf[0] = 12; // P
		Seg_Buf[1] = (unsigned char)Set_Mode + 1;
		Seg_Buf[2] = Seg_Buf[3] = Seg_Buf[4] = Seg_Buf[5] = 10;
		if (Set_Mode == 0) // 距离参数
		{
			Seg_Buf[6] = distance_set / 10 % 10;
			Seg_Buf[7] = distance_set % 10;
		}
		else
		{
			Seg_Buf[6] = temperature_set / 10 % 10;
			Seg_Buf[7] = temperature_set % 10;
		}
		Seg_Point[1] = 0;
		Seg_Point[6] = 0;
		Seg_Point[5] = 0;
		break;
	case 2:				 // 工厂界面
		Seg_Buf[0] = 13; // F
		Seg_Buf[1] = Factory_Mode + 1;
		switch (Factory_Mode)
		{
		case 0: // 校准值
			Seg_Buf[2] = Seg_Buf[3] = Seg_Buf[4] = 10;
			if (calibration >= 0) // 正数正常显示
			{
				Seg_Buf[5] = calibration > 100 ? calibration / 100 % 10 : 10;
				Seg_Buf[6] = calibration >= 10 ? calibration / 10 % 10 : 10;
				Seg_Buf[7] = calibration % 10;
			}
			else // 负数加符号
			{
				if (abs(calibration) >= 10)
				{
					Seg_Buf[5] = 11;
					Seg_Buf[6] = abs(calibration) / 10 % 10;
					Seg_Buf[7] = abs(calibration) % 10;
				}
				else
				{
					Seg_Buf[5] = 10;
					Seg_Buf[6] = 11;
					Seg_Buf[7] = abs(calibration) % 10;
				}
			}
			Seg_Point[6] = 0;
			break;
		case 1: // 超声波介质速度
			Seg_Buf[2] = Seg_Buf[3] = 10;
			Seg_Buf[4] = Ut_Speed >= 1000 ? Ut_Speed / 1000 % 10 : 10;
			Seg_Buf[5] = Ut_Speed >= 100 ? Ut_Speed / 100 % 10 : 10;
			Seg_Buf[6] = Ut_Speed >= 10 ? Ut_Speed / 10 % 10 : 10;
			Seg_Buf[7] = Ut_Speed % 10;
			break;
		case 2: // Dac输出下限
			Seg_Buf[2] = Seg_Buf[3] = Seg_Buf[4] = Seg_Buf[5] = 10;
			Seg_Buf[6] = Dac_Least_10x / 10 % 10;
			Seg_Buf[7] = Dac_Least_10x % 10;
			Seg_Point[6] = 1;
			break;
		}
		Seg_Point[1] = 0;
		break;
	}
}
/*其他处理*/
void Led_Proc()
{
	unsigned char i;
	unsigned char dis_temp;

	switch (Disp_Mode)
	{
	case 0:
		if (distance >= 255)
		{
			for (i = 0; i < 8; i++)
				ucLed[i] = 1;
		}
		else
		{
			dis_temp = (unsigned char)distance;
			for (i = 0; i < 8; i++)
			{
				ucLed[i] = (dis_temp >> i) & 0x01; // L1 对应最低位
			}
		}
		break;
	case 1:
		ucLed[7] = 1;
		for (i = 0; i < 7; i++)
			ucLed[i] = 0;
		break;
	case 2:
		ucLed[0] = Star_Flag;
		for (i = 1; i < 8; i++)
			ucLed[i] = 0;
		break;
	}

	if (distance >= distance_set - 5 && distance <= distance_set + 5 && temperature <= temperature_set)
		Relay_Flag = 1;
	else
		Relay_Flag = 0;
	Relay(Relay_Flag);
}
/*超声波处理*/
void Ultra_Proc()
{
	unsigned int distance_temp;
	if (Ultra_Slow)
		return;
	Ultra_Slow = 1;
	// 距离读取
	distance_temp = Ut_Wave_Data(Ut_Speed);
	distance = (distance_temp + calibration >= 0) ? distance_temp + calibration : 0;
	// 记录模式开启
	if (Key_Enable == 0)
	{
		if (Distance_Record_Index > 0)
			Record_Judge = 1;
		if (Distance_Record_Index < 12)
		{
			Distance_Record[Distance_Record_Index] = distance;
			Distance_Record_Index++;
		}
	}
}
/*DA输出*/
void Dac_Proc()
{
	float Vol_Output;
	unsigned char dac_val;
	unsigned char dis;

	if (Vol_Output_Flag == 0)
		return;
	if (Record_Judge == 0)
		return;

	if (DAC_Output_Index >= Distance_Record_Index)
	{
		Vol_Output_Flag = 0;
		DAC_Output_Index = 0;
		return; // 不再清零，保持最后输出
	}

	dis = Distance_Record[DAC_Output_Index];

	if (dis < 10)
	{
		Vol_Output = Dac_Least_10x / 10.0f;
	}
	else if (dis <= 90)
	{
		Vol_Output = (5.0f - Dac_Least_10x / 10.0f) * (dis - 10) / 80.0f + Dac_Least_10x / 10.0f;
	}
	else
	{
		Vol_Output = 5.0f;
	}

	dac_val = (unsigned char)(Vol_Output * 51.0f + 0.5f);
	DA_Write(dac_val);
}
/*定时器0初始化函数*/
void Timer0Init(void) // 1毫秒@12.000MHz
{
	AUXR &= 0x7F; // 定时器时钟12T模式
	TMOD &= 0xF0; // 设置定时器模式
	TL0 = 0x18;	  // 设置定时初值
	TH0 = 0xFC;	  // 设置定时初值
	TF0 = 0;	  // 清除TF0标志
	TR0 = 1;	  // 定时器0开始计时
	ET0 = 1;
	EA = 1;
}
/*定时0中断服务函数*/
void Timer0server(void) interrupt 1
{
	if (++Key_Slow == 20)
		Key_Slow = 0;
	if (++Seg_Slow == 200)
		Seg_Slow = 0;
	if (++Seg_Pos == 8)
		Seg_Pos = 0;
	if (++Ultra_Slow == 480)
		Ultra_Slow = 0;
	Seg_Disp(Seg_Pos, Seg_Buf[Seg_Pos], Seg_Point[Seg_Pos]);
	Led_Disp(Seg_Pos, ucLed[Seg_Pos]);
	// 长按2s恢复出厂设置
	if (Reset_Flag)
	{
		Timer2s++;
		if (Timer2s >= 2000)
			Reset_Default(); // 超过2s直接恢复出厂设置
	}
	// 距离记录
	if (Key_Enable == 0)
	{
		if (++Record_Time >= 6000)
		{
			Key_Enable = 1;
			Record_Time = 0;
		}
	}
	// 电压输出  500ms一次
	if (Vol_Output_Flag)
	{
		if (++Dac_Slow >= 500)
		{
			Dac_Slow = 0;
			DAC_Output_Index++;
			if (DAC_Output_Index >= Distance_Record_Index)
			{
				Vol_Output_Flag = 0;
				DAC_Output_Index = 0;
			}
		}
	}
	// 闪烁
	if (++Timer100ms == 100)
	{
		Star_Flag ^= 1;
		Timer100ms = 0;
	}
}
/*Main*/
void main()
{
	Init();
	Reset_Default();
	Timer0Init();
	read_t();
	while (1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
		Ultra_Proc();
		Dac_Proc();
	}
}
void Init()
{
	P2 = 0x80;
	P0 = 0xff;
	P2 = 0x00;
	P2 = 0xa0;
	P0 = 0x00;
	P2 = 0x00;
}

unsigned char Key_Read()
{
	unsigned char temp = 0;
	P44 = 0;
	P42 = 1;
	P35 = 1;
	P34 = 1;
	if (P33 == 0)
		temp = 4;
	if (P32 == 0)
		temp = 5;
	P44 = 1;
	P42 = 0;
	P35 = 1;
	P34 = 1;
	if (P33 == 0)
		temp = 8;
	if (P32 == 0)
		temp = 9;
	if (P33 == 0 && P32 == 0)
		temp = 89;
	return temp;
}

void Seg_Disp(unsigned char wei, unsigned char table, bit point)
{
	P2 = 0xe0;
	P0 = 0xff;
	P2 = 0x00;
	P2 = 0xc0;
	P0 = seg_wei[wei];
	P2 = 0x00;
	P0 = seg_table[table];
	if (point)
		P0 &= ~(1 << 7);
	P2 = 0xe0;
	P2 = 0x00;
}
void Led_Disp(unsigned char addr, bit enable)
{
	static unsigned char temp = 0x00;
	if (enable)
		temp |= 0x01 << addr;
	else
		temp &= ~(0x01 << addr);
	P0 = ~temp;
	P2 = 0x80;
	P2 = 0x00;
}
void Relay(bit enable)
{
	static unsigned char temp1 = 0x00;

	if (enable)
		temp1 |= 0x10;
	else
		temp1 &= ~0x10;

	P0 = temp1;
	P2 = 0xa0;
	P2 = 0x00;
}