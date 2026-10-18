/*头文件引入*/
#include <STC15F2K60S2.H>
#include <onewire.h>
#include <ds1302.h>
#include <iic.H>
#include <ut.h>
#include <uart.h>
#include <stdio.h>
#include <math.h>
/* 底层变量 */
unsigned char Key_Slow = 0, Key_Val = 0, Key_Down = 0, Key_Up, Key_Old = 0;
unsigned int Seg_Slow = 0;
unsigned char Seg_Pos = 0;
idata unsigned char Seg_Buf[8] = {10, 10, 10, 10, 10, 10, 10, 10};
idata unsigned char Seg_Point[8] = {0, 0, 0, 0, 0, 0, 0, 0};
idata unsigned char ucLed[8] = {0, 0, 0, 0, 0, 0, 0, 0};

unsigned char code seg_table[] = {0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8, 0x80, 0x90, 0xff, 0xbf, 0x8c, 0x86};
unsigned char code seg_wei[] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};

/* 底层函数声明 */
void Sys_Init();
unsigned char Key_Read();
void Seg_Disp(unsigned char wei, unsigned char addr, bit point);
void Led_Disp(unsigned char addr, bit enable);
void Relay(bit enable);

/* 计时变量 */
xdata unsigned int freq = 0; // 频率
idata unsigned char Freq_Timer200ms = 0;
idata unsigned char Freq_Count = 0;
bit Freq_New = 0; // 全局变量，表示频率已更新
xdata float temperature = 0;
idata unsigned char ucRtc[3] = {23, 59, 50};
idata unsigned int Speed_10x = 0;	  // 超声波声速（温度补偿)
bit Speed_10x_EN = 0;				  // 声速计算使能
idata int Voltage_Calibration_Mv = 0; // 电压校准值
/*容器类型/尺寸参数 */
unsigned char Container_Type = 0; // 0-球 1-圆柱 2-长方体
xdata float R_Para_Cm = 0;		  // 半径
xdata float Cylinder_H_Cm = 0;
xdata float Cuboid_L_Cm = 0, Cuboid_W_Cm = 0; // 长 宽
xdata float Cuboid_H_Cm = 0;				  // 高
xdata float Liquid_Height = 0;				  // 液位高度
idata unsigned int High_Val_100x = 0;		  // 液位高度放大100倍
xdata float Volume_10x;						  // 液体体积
xdata float Liquid_Volume = 0.0f;			  // 最终液体体积
unsigned char Remaining_Space = 0;			  // 剩余空间
xdata float Weight = 0;						  // 重量
/*显示相关*/
unsigned char Disp_Mode = 0;											// 0-数据界面 1-参数界面 2-标定界面
unsigned char Data_Mode = 0;											// 0-时间 1-液位 2-体积 3-重量
xdata unsigned int Parameter_Set[8] = {10, 1, 2000, 2, 10, 10, 10, 10}; // 0-H1 1-H2 2-F 3-S 4-r 5-L 6-W 7-H
unsigned char Set_Mode_Index = 0;
/*重量标定参数 */
xdata unsigned char Mark[5] = {0, 5, 10, 15, 20};				  // 标定重量
xdata unsigned char Vol_Calibration_10x[5] = {0, 10, 20, 40, 50}; // 标定电压值
xdata unsigned char Vol_Calibration_10x_Con[5] = {0, 10, 20, 40, 50};
unsigned char Mark_Index = 0; // 索引
/*频率记录相关*/
idata unsigned int Freq_History[5] = {0, 0, 0, 0, 0}; // 频率历史值
unsigned char Freq_History_Count = 0;				  // 频率值改变 索引
unsigned char Freq_Jump_Led_Tick = 0;
unsigned char Timer100ms = 0;
/* 串口 */
xdata unsigned char Uart_Rec[64];
unsigned char Uart_Rec_Index = 0;
bit Rec_Flag = 0;
unsigned char Sys_Tick = 0;
/* 异常与记录 */
bit Level_High_Alarm = 0; // 液位上限报警
bit Level_Low_Alarm = 0;  // 液位下限报警
bit Freq_Value_Alarm = 0; // 频率超限报警
bit Freq_Trend_Alarm = 0; // 频率趋势报警
bit StarL7_Flag = 0;	  // 闪烁L7
bit Freq_Trend_Dir = 0;	  // 频率趋势方向
bit Freq_Jump_Alarm = 0;  // 频率突变报警
unsigned char Timer200ms = 0;
bit StarL8_Flag = 0;
/*温度读取+速度校准值 */
void Get_Temperature()
{
	temperature = read_t();
	Speed_10x_EN = 1;									// 使能速度 先读温度再算校准值 再计算距离
	Speed_10x = (unsigned int)(3300 + 6 * temperature); // 声速=330m/s+温度×0.6
	Voltage_Calibration_Mv = (int)(4 * temperature * temperature / 100.0f - 10.0f * temperature + 225.0f);
}
/*重量计算 AD 采集 +五点标定*/
void Get_Calc_Weight()
{
	int i = 0;
	unsigned int ad_val; // AD值
	int voltage_mv;		 // 采集mv值
	int cal_mv;			 // 计算的mv值=（采集+校准）
	float lower_mv;		 // 区间下限电压 mv
	float upper_mv;		 // 区间上限电压 mv
	float k;
	i = 0;
	EA = 0;
	ad_val = AD_Read(0x43);
	EA = 1;
	voltage_mv = (int)((float)ad_val * 5000.0f / 255.0f);
	cal_mv = voltage_mv + (int)Voltage_Calibration_Mv;
	// 区间判断
	if (cal_mv <= (int)Vol_Calibration_10x[0] * 100)
	{
		Weight = 0.0f;
		return;
	}
	if (cal_mv >= (int)Vol_Calibration_10x[4] * 100)
	{
		Weight = 20.0f;
		return;
	}
	// 找到RB2电压值对应的区间
	for (i = 0; i < 4; i++)
	{
		if (cal_mv < (int)Vol_Calibration_10x[i + 1] * 100)
			break;
	}

	// 获取区间上下限电压 mv
	lower_mv = (float)Vol_Calibration_10x[i] * 100.0f;
	upper_mv = (float)Vol_Calibration_10x[i + 1] * 100.0f;
	// 计算所占比例
	k = ((float)cal_mv - lower_mv) / (upper_mv - lower_mv);
	// 计算出重量
	Weight = (float)Mark[i] + ((float)Mark[i + 1] - (float)Mark[i]) * k;
	// 防止溢出
	if (Weight < 0.0f)
		Weight = 0.0f;
	if (Weight > 20.0f)
		Weight = 20.0f;
	// printf("AD:%d, V_mv:%d, Cal_mv:%d, idx:%d, Weight:%.2f kg\r\n",
	//	   ad_val, voltage_mv, cal_mv, i, Weight);
}
/*超声波读距+计算*/
void Get_Distance()
{
	unsigned int temp_distance;
	float temp1;			// 到达的高度
	float total_volume_10x; // 总液面高度10x
	if (!Speed_10x_EN)
		return; // 若未读取t，直接返回（校准值未计算，速度值未校准）
	// 获取计算参数
	Container_Type = (unsigned char)Parameter_Set[3]; // 容器类型
	R_Para_Cm = Parameter_Set[4] * 10.0f;			  // 半径
	Cylinder_H_Cm = Parameter_Set[7] * 10.0f;		  // 圆柱体的高度
	Cuboid_L_Cm = Parameter_Set[5] * 10.0f;			  // 长
	Cuboid_W_Cm = Parameter_Set[6] * 10.0f;			  // 宽
	Cuboid_H_Cm = Parameter_Set[7] * 10.0f;			  // 高
	// 读取速度值
	temp_distance = Ut_Wave_Data((float)(Speed_10x / 10.0f));
	// 根据容器类型计算
	switch (Container_Type)
	{
	case 0: // 球
		if (temp_distance >= 2 * R_Para_Cm)
			Liquid_Height = 0.0f;
		else
			Liquid_Height = 2 * R_Para_Cm - temp_distance;

		temp1 = 3.14f * Liquid_Height * Liquid_Height * (3 * R_Para_Cm - Liquid_Height) / 3.0f;
		Volume_10x = temp1 / 100000.0f; // cm^3->10x m^3   /10^5

		total_volume_10x = 4.0f * 3.14f * R_Para_Cm * R_Para_Cm * R_Para_Cm / 3.0f / 100000.0f;

		// 剩余空间百分比
		if (total_volume_10x > 0)
			Remaining_Space = (unsigned char)((1.0f - (Volume_10x / total_volume_10x)) * 100.0f + 0.5f);
		else
			Remaining_Space = 0;
		break;
	case 1: // 圆柱
		if (temp_distance >= Cylinder_H_Cm)
			Liquid_Height = 0;
		else
			Liquid_Height = Cylinder_H_Cm - temp_distance;
		Volume_10x = 3.14f * R_Para_Cm * R_Para_Cm * Liquid_Height / 100000.0f;
		if (Cylinder_H_Cm > 0)
			Remaining_Space = (unsigned char)((1.0f - Liquid_Height / Cylinder_H_Cm) * 100.0f + 0.5f);
		else
			Remaining_Space = 0;
		break;
	case 2: // 长方体
		if (temp_distance >= Cuboid_H_Cm)
			Liquid_Height = 0;
		else
			Liquid_Height = Cuboid_H_Cm - temp_distance;
		Volume_10x = Cuboid_L_Cm * Cuboid_W_Cm * Liquid_Height / 100000.0f;
		if (Cuboid_H_Cm > 0)
			Remaining_Space = (unsigned char)((1.0f - Liquid_Height / Cuboid_H_Cm) * 100.0f + 0.5f);
		else
			Remaining_Space = 0;
		break;
	}

	if (Remaining_Space > 100)
		Remaining_Space = 100;
	if (Liquid_Height < 0)
		Liquid_Height = 0;
	High_Val_100x = (unsigned int)Liquid_Height;
	Liquid_Volume = Volume_10x / 10.0f;
}
/*故障上报 */
void Report_Fault(unsigned int fault_id, unsigned int value_100x)
{
	Read_Rtc(ucRtc);		  // 读取当前时间
	printf("[%u,", fault_id); // 打印故障ID
	printf("%02bu%02bu%02bu,", ucRtc[0], ucRtc[1], ucRtc[2]);

	if (fault_id == 500 || fault_id == 501)
		printf("%d.%02d]\r\n", (int)value_100x / 100 % 10, (int)value_100x % 100);
	else
		printf("%u]\r\n", value_100x);
}
/*更新报警状态*/
void Update_Alarm_State()
{
	unsigned char i;
	unsigned int cur_level_cm = 0;	// 当前液位高度
	unsigned int diff = 0;			// 频率差值
	bit up_flag = 0, down_flag = 0; // 默认上升 下降趋势都是1

	bit cur_high = 0, cur_low = 0; // 当前液面是否过高 过低
	bit cur_freq = 0;			   // 频率是否超限
	bit cur_trend = 0;			   // 频率趋势异常
	bit cur_dir = 0;			   // 趋势方向	0-下降 1-上升
	bit Freq_Enable = 0;		   // 使能判断标志位
	if (Freq_New)
		Freq_New = 0;
	else
		return;

	// 获取当前液位置高度
	cur_level_cm = High_Val_100x;
	// 液面过高过低判断+频率超限判断
	cur_high = (cur_level_cm > Parameter_Set[0] * 10);
	cur_low = (cur_level_cm < Parameter_Set[1] * 10);
	cur_freq = (freq > Parameter_Set[2]);
	// 报警判断
	if (cur_high && !Level_High_Alarm)
		Report_Fault(500, cur_level_cm);
	if (cur_low && !Level_Low_Alarm)
		Report_Fault(501, cur_level_cm);
	if (cur_freq && !Freq_Value_Alarm)
		Report_Fault(502, freq);
	// 复制给Alarm进行报警
	Level_High_Alarm = cur_high;
	Level_Low_Alarm = cur_low;
	Freq_Value_Alarm = cur_freq;
	// 记录最近5次频率
	if (Freq_Count == 4)
	{
		if (Freq_History_Count < 5)
		{
			Freq_History[Freq_History_Count] = freq;
			Freq_History_Count++;
		}
		else
		{
			for (i = 0; i < 4; i++)
				Freq_History[i] = Freq_History[i + 1];
			Freq_History[4] = freq;
		}
		Freq_Enable = 1;
	}
	if (Freq_Enable)
	{
		// 先重置状态
		Freq_Enable = 0;
		cur_trend = cur_dir = 0;
		// 判断频率是否连续上升 / 下降
		if (Freq_History_Count >= 5)
		{
			up_flag = down_flag = 1;
			for (i = 0; i < 4; i++) // 0↔1、1↔2、2↔3、3↔4
			{
				if (Freq_History[i + 1] <= Freq_History[i])
					up_flag = 0; // 上升标志位置0
				if (Freq_History[i + 1] >= Freq_History[i])
					down_flag = 0; // 下降标志位置0
			}
			if (up_flag) // 如果连续 5 次上升 → 趋势报警，方向 = 上升 (1)
			{
				cur_trend = 1; // 频率变化异常了
				cur_dir = 1;   // 1-上升
			}
			else if (down_flag)
			{
				cur_trend = 1; // 频率变化异常了
				cur_dir = 0;   // 0-下降
			}
		}
		// 只要出现趋势 或 趋势方向变了 → 报警 503
		if (cur_trend)
		{
			if ((!Freq_Trend_Alarm) || (Freq_Trend_Dir != cur_dir))
			{
				Report_Fault(503, cur_dir);
			}
		}
		else
		{
			// 恢复正常，清除趋势报警标志
			Freq_Trend_Alarm = 0;
		}
		// 更新变量
		Freq_Trend_Alarm = cur_trend;
		Freq_Trend_Dir = cur_dir;

		// 判断频率突变：3 次内变化超 1000
		if (Freq_History_Count >= 3)
		{
			diff = abs(Freq_History[Freq_History_Count - 3] - Freq_History[Freq_History_Count - 1]);
			if (diff > 999) // 突变报警
			{
				Report_Fault(504, diff);
				Freq_Jump_Alarm = 1;
				Freq_Jump_Led_Tick = 15; // 闪烁3s开始倒计时 200msX15 =3s
			}
		}
	}
}

void Led_Proc()
{
	unsigned char i;
	if (Disp_Mode == 0)
	{
		for (i = 0; i < 4; i++)
			ucLed[i] = (Data_Mode == i);
	}
	else
	{
		for (i = 0; i < 4; i++)
			ucLed[i] = 0;
	}
	ucLed[4] = (Level_High_Alarm || Level_Low_Alarm); // 到达上限or下限
	ucLed[5] = Freq_Value_Alarm;					  // 频率超限
	ucLed[6] = StarL7_Flag;
	ucLed[7] = StarL8_Flag;

	// 继电器
	Relay(Level_High_Alarm || Level_Low_Alarm || Freq_Value_Alarm || Freq_Trend_Alarm || Freq_Jump_Alarm);
}

/* 按键处理 */
void Key_Proc()
{
	unsigned char i;
	if (Key_Slow)
		return;
	Key_Slow = 1;
	Key_Val = Key_Read();
	Key_Down = Key_Val & (Key_Val ^ Key_Old);
	Key_Up = ~Key_Val & (Key_Val ^ Key_Old);
	Key_Old = Key_Val;

	switch (Key_Down)
	{
	case 4: // 主界面切换

		Disp_Mode = (Disp_Mode + 1) % 3;
		Data_Mode = Set_Mode_Index = Mark_Index = 0;
		// 标定电压值合理判断
		if (Disp_Mode == 0)
		{
			if (Vol_Calibration_10x_Con[0] < Vol_Calibration_10x_Con[1] && Vol_Calibration_10x_Con[1] < Vol_Calibration_10x_Con[2] && Vol_Calibration_10x_Con[2] < Vol_Calibration_10x_Con[3] && Vol_Calibration_10x_Con[3] < Vol_Calibration_10x_Con[4])
			{
				for (i = 0; i < 5; i++)
					Vol_Calibration_10x[i] = Vol_Calibration_10x_Con[i]; // 合理保存电压值
			}
			else // 否则恢复初始值
			{
				for (i = 0; i < 5; i++)
					Vol_Calibration_10x_Con[i] = Vol_Calibration_10x[i];
			}
		}
		break;
	case 5: // 子界面切换
		if (Disp_Mode == 0)
			Data_Mode = (Data_Mode + 1) % 4;
		else if (Disp_Mode == 1)
			Set_Mode_Index = (Set_Mode_Index + 1) % 8;
		else
			Mark_Index = (Mark_Index + 1) % 5;
		break;
	case 8: //+
		if (Disp_Mode == 2 && ++Vol_Calibration_10x_Con[Mark_Index] > 99)
			Vol_Calibration_10x_Con[Mark_Index] = 99;
		break;
	case 9: //-
		if (Disp_Mode == 2 && --Vol_Calibration_10x_Con[Mark_Index] > 99)
			Vol_Calibration_10x_Con[Mark_Index] = 0;
		break;
	}
}

/* 信息处理函数 */
void Seg_Proc()
{
	if (Seg_Slow)
		return;
	Seg_Slow = 1;
	Get_Temperature();
	Get_Calc_Weight();
	Get_Distance();
	Read_Rtc(ucRtc);
	switch (Disp_Mode)
	{
	case 0:					// 数据界面
		if (Data_Mode == 0) // 时间
		{
			Seg_Buf[0] = ucRtc[0] / 10 % 10;
			Seg_Buf[1] = ucRtc[0] % 10;
			Seg_Buf[2] = 11;
			Seg_Buf[3] = ucRtc[1] / 10 % 10;
			Seg_Buf[4] = ucRtc[1] % 10;
			Seg_Buf[5] = 11;
			Seg_Buf[6] = ucRtc[2] / 10 % 10;
			Seg_Buf[7] = ucRtc[2] % 10;
			Seg_Point[6] = Seg_Point[6] = 0;
		}
		else if (Data_Mode == 1) // 液位
		{
			Seg_Buf[0] = 10;
			if (temperature >= 0)
			{
				Seg_Buf[1] = (int)temperature >= 100 ? (int)temperature / 100 % 10 : 10;
				Seg_Buf[2] = (int)temperature >= 10 ? (int)temperature / 10 % 10 : 10;
				Seg_Buf[3] = (int)temperature % 10;
			}
			else
			{
				Seg_Buf[1] = 11; //-
				Seg_Buf[2] = (int)temperature * (-1) >= 10 ? (int)temperature * (-1) / 10 % 10 : 10;
				Seg_Buf[3] = (int)temperature * (-1) % 10;
			}
			Seg_Buf[4] = 11;
			Seg_Buf[5] = High_Val_100x >= 100 ? High_Val_100x / 100 % 10 : 0;
			Seg_Buf[6] = High_Val_100x >= 10 ? High_Val_100x / 10 % 10 : 0;
			Seg_Buf[7] = High_Val_100x % 10;
			Seg_Point[5] = 1;
			Seg_Point[6] = 0;
		}
		else if (Data_Mode == 2) // 体积
		{
			Seg_Buf[0] = Remaining_Space >= 100 ? Remaining_Space / 100 % 10 : 10;
			Seg_Buf[1] = Remaining_Space >= 10 ? Remaining_Space / 10 % 10 : 10;
			Seg_Buf[2] = Remaining_Space % 10;
			Seg_Buf[3] = 11; //-
			Seg_Buf[4] = (unsigned int)Liquid_Volume >= 100 ? (unsigned int)Liquid_Volume / 100 % 10 : 10;
			Seg_Buf[5] = (unsigned int)Liquid_Volume >= 10 ? (unsigned int)Liquid_Volume / 10 % 10 : 10;
			Seg_Buf[6] = (unsigned int)Liquid_Volume % 10;
			Seg_Buf[7] = (unsigned int)(Liquid_Volume * 10) % 10;
			Seg_Point[5] = 0;
			Seg_Point[6] = 1;
		}
		else // 重量
		{
			if (temperature >= 0)
			{
				Seg_Buf[0] = (int)temperature >= 100 ? (int)temperature / 100 % 10 : 10;
				Seg_Buf[1] = (int)temperature >= 10 ? (int)temperature / 10 % 10 : 10;
				Seg_Buf[2] = (int)temperature % 10;
			}
			else
			{
				Seg_Buf[0] = 11; //-
				Seg_Buf[1] = (int)temperature * (-1) >= 10 ? (int)temperature * (-1) / 10 % 10 : 10;
				Seg_Buf[2] = (int)temperature * (-1) % 10;
			}
			Seg_Buf[3] = 11;
			Seg_Buf[4] = (unsigned int)Weight >= 100 ? (unsigned int)Weight / 100 % 10 : 10;
			Seg_Buf[5] = (unsigned int)Weight >= 10 ? (unsigned int)Weight / 10 % 10 : 10;
			Seg_Buf[6] = (unsigned int)Weight % 10;
			Seg_Buf[7] = (unsigned int)(Weight * 10) % 10;
			Seg_Point[5] = 0;
			Seg_Point[6] = 1;
		}
		break;
	case 1:				 // 参数界面
		Seg_Buf[0] = 12; // P
		Seg_Buf[1] = Set_Mode_Index + 1;
		Seg_Buf[2] = 11; //-
		switch (Set_Mode_Index)
		{
		case 2: // 频率参数F
			Seg_Buf[3] = Parameter_Set[Set_Mode_Index] >= 10000 ? Parameter_Set[Set_Mode_Index] / 10000 % 10 : 10;
			Seg_Buf[4] = Parameter_Set[Set_Mode_Index] >= 1000 ? Parameter_Set[Set_Mode_Index] / 1000 % 10 : 10;
			Seg_Buf[5] = Parameter_Set[Set_Mode_Index] >= 100 ? Parameter_Set[Set_Mode_Index] / 100 % 10 : 10;
			Seg_Buf[6] = Parameter_Set[Set_Mode_Index] >= 10 ? Parameter_Set[Set_Mode_Index] / 10 % 10 : 10;
			Seg_Buf[7] = Parameter_Set[Set_Mode_Index] % 10;
			Seg_Point[5] = Seg_Point[6] = 0;
			break;
		case 3: // 容器类型S
			Seg_Buf[3] = Seg_Buf[4] = Seg_Buf[5] = Seg_Buf[6] = 10;
			Seg_Buf[7] = Parameter_Set[Set_Mode_Index];
			Seg_Point[5] = Seg_Point[6] = 0;
			break;
		case 0: // 液体上限H1
		case 1: // 液体下限H2
		case 4: // 半径r
		case 5: // 长度L
		case 6: // 宽度W
		case 7: // 高度H
			Seg_Buf[3] = Parameter_Set[Set_Mode_Index] >= 10000 ? Parameter_Set[Set_Mode_Index] / 10000 % 10 : 10;
			Seg_Buf[4] = Parameter_Set[Set_Mode_Index] >= 1000 ? Parameter_Set[Set_Mode_Index] / 1000 % 10 : 10;
			Seg_Buf[5] = Parameter_Set[Set_Mode_Index] >= 100 ? Parameter_Set[Set_Mode_Index] / 100 % 10 : 10;
			Seg_Buf[6] = Parameter_Set[Set_Mode_Index] >= 10 ? Parameter_Set[Set_Mode_Index] / 10 % 10 : 0;
			Seg_Buf[7] = Parameter_Set[Set_Mode_Index] % 10;
			Seg_Point[5] = 0;
			Seg_Point[6] = 1;
			break;
		}
		break;
	case 2:
		Seg_Buf[0] = 13; // E
		Seg_Buf[1] = Mark[Mark_Index] / 10 % 10;
		Seg_Buf[2] = Mark[Mark_Index] % 10;
		Seg_Buf[3] = Seg_Buf[4] = Seg_Buf[5] = 10;
		Seg_Buf[6] = Vol_Calibration_10x_Con[Mark_Index] / 10 % 10;
		Seg_Buf[7] = Vol_Calibration_10x_Con[Mark_Index] % 10;
		Seg_Point[5] = 0;
		Seg_Point[6] = 1;
		break;
	}
}
/*字符串对比*/
bit str_equal(unsigned char *str1, unsigned char *str2)
{
	unsigned char i = 0;
	while (str1[i] != '\0' && str2[i] != '\0')
	{
		if (str1[i] != str2[i])
			return 0; // 字符不同→不相等
		i++;
	}
	return (str1[i] == str2[i]); // 全部相同→相等
}
/*字符串转整数*/
unsigned int parse_integer(unsigned char *str)
{
	unsigned int val = 0;
	unsigned char i = 0;
	if (str[0] == '\0')
		return 0xFFFF; // 空字符串→错误
	while (str[i] != '\0')
	{
		if (str[i] < '0' || str[i] > '9')
			return 0xFFFF;			   // 非数字→错误
		val = val * 10 + str[i] - '0'; // ASCII码转数字
		i++;
	}
	return val;
}
/*字符串转小数（放大 10 倍）*/
unsigned int parse_m_to_10x(unsigned char *str)
{
	unsigned int integer = 0;  // 整数部分
	unsigned char decimal = 0; // 小数部分
	unsigned char i = 0;
	bit has_dot = 0; // 是否有小数点

	if (str[0] == '\0')
		return 0xFFFF;
	while (str[i] != '\0')
	{
		if (str[i] == '.')
		{
			if (has_dot)
				return 0xFFFF; // 多个小数点→错误
			has_dot = 1;
		}
		else if (str[i] >= '0' && str[i] <= '9')
		{
			if (!has_dot)
				integer = integer * 10 + str[i] - '0'; // 获取整数部分
			else
			{
				decimal = str[i] - '0'; // 获取 小数
				i++;
				if (str[i] != '\0')
					return 0xFFFF;
				return integer * 10 + decimal; // 返回整个数
			}
		}
		else
			return 0xFFFF;
		i++;
	} // 跑完循环返回即可
	return integer * 10;
}
// 功能：从串口接收缓冲区中，解析出一条 (参数名,参数值) 格式的数据
// 输入：start=开始解析的位置 id_buf=存放参数名 val_buf=存放参数值
// 返回：解析完成后的位置，失败返回0xFF
unsigned char extract_one_param(unsigned char start, unsigned char *id_buf, unsigned char *val_buf)
{
	unsigned char i = start; // i从指定起始位置开始遍历
	unsigned char j;		 // 临时索引，用于填充id/val缓存

	// 1. 查找字符 '('，找到才开始解析
	while (i < Uart_Rec_Index && Uart_Rec[i] != '(')
		i++;
	// 如果遍历完都没找到'('，说明格式错误，返回0xFF
	if (i >= Uart_Rec_Index)
		return 0xFF;

	i++;   // 跳过 '(' 这个字符
	j = 0; // 重置参数名缓存索引
	// 2. 读取参数名，直到遇到 ',' 或 ':' 停止
	while (i < Uart_Rec_Index && Uart_Rec[i] != ',' && Uart_Rec[i] != ':')
	{
		id_buf[j++] = Uart_Rec[i++]; // 把字符存入参数名缓存
	}
	id_buf[j] = '\0'; // 参数名末尾加结束符，变成标准字符串
	// 如果没找到','或':'，格式错误
	if (i >= Uart_Rec_Index)
		return 0xFF;

	i++;   // 跳过 ',' 或 ':'
	j = 0; // 重置参数值缓存索引
	// 3. 读取参数值，直到遇到 ')' 停止
	while (i < Uart_Rec_Index && Uart_Rec[i] != ')')
	{
		val_buf[j++] = Uart_Rec[i++]; // 把字符存入参数值缓存
	}
	val_buf[j] = '\0'; // 参数值末尾加结束符
	// 如果没找到')'，格式错误
	if (i >= Uart_Rec_Index)
		return 0xFF;

	return i; // 返回当前解析到的位置，供下一次继续解析
}

// 功能：根据解析出的参数名和参数值，执行对应的操作（查询/设置）
// 输入：id=参数名 val=参数值 has_ok=是否设置成功 has_err=是否出错 has_set=是否是设置指令
void handle_param(unsigned char *id, unsigned char *val, unsigned char *has_ok, unsigned char *has_err, unsigned char *has_set)
{
	unsigned int temp;	   // 临时变量，存放转换后的参数值
	unsigned char h, m, s; // 时、分、秒

	// 如果参数值是 '?' → 表示查询参数，返回当前值
	if (val[0] == '?')
	{
		if (str_equal(id, "H1"))
			printf("(H1,%u.%u)\r\n", Parameter_Set[0] / 10, Parameter_Set[0] % 10);
		else if (str_equal(id, "H2"))
			printf("(H2,%u.%u)\r\n", Parameter_Set[1] / 10, Parameter_Set[1] % 10);
		else if (str_equal(id, "F"))
			printf("(F,%u)\r\n", Parameter_Set[2]);
		else if (str_equal(id, "S"))
			printf("(S,%u)\r\n", Parameter_Set[3]);
		else if (str_equal(id, "r"))
			printf("(r,%u.%u)\r\n", Parameter_Set[4] / 10, Parameter_Set[4] % 10);
		else if (str_equal(id, "L"))
			printf("(L,%u.%u)\r\n", Parameter_Set[5] / 10, Parameter_Set[5] % 10);
		else if (str_equal(id, "W"))
			printf("(W,%u.%u)\r\n", Parameter_Set[6] / 10, Parameter_Set[6] % 10);
		else if (str_equal(id, "H"))
			printf("(H,%u.%u)\r\n", Parameter_Set[7] / 10, Parameter_Set[7] % 10);
		else
			*has_err = 1; // 不认识的参数名 → 错误
		return;			  // 查询完成，退出
	}

	*has_set = 1; // 标记：这是一条设置指令

	// ============== 设置时间 T ==============
	if (str_equal(id, "T"))
	{
		// 检查时间格式必须是6位数字：HHMMSS
		if (val[0] < '0' || val[0] > '9' ||
			val[1] < '0' || val[1] > '9' ||
			val[2] < '0' || val[2] > '9' ||
			val[3] < '0' || val[3] > '9' ||
			val[4] < '0' || val[4] > '9' ||
			val[5] < '0' || val[5] > '9' ||
			val[6] != '\0')
		{
			*has_err = 1; // 格式非法 → 错误
			return;
		}
		// 把字符串转成 时、分、秒
		h = (val[0] - '0') * 10 + (val[1] - '0');
		m = (val[2] - '0') * 10 + (val[3] - '0');
		s = (val[4] - '0') * 10 + (val[5] - '0');
		// 判断时间是否合法
		if (h > 23 || m > 59 || s > 59)
		{
			*has_err = 1;
			return;
		}
		// 设置时间到时钟芯片
		ucRtc[0] = h;
		ucRtc[1] = m;
		ucRtc[2] = s;
		Set_Rtc(ucRtc);
		*has_ok = 1; // 设置成功
		return;
	}

	// ============== 设置液位上限 H1 ==============
	if (str_equal(id, "H1"))
	{
		temp = parse_m_to_10x(val); // 把字符串转成小数×10
		// 格式错误 或 H1 小于 H2（下限）→ 非法
		if (temp == 0xFFFF || temp < Parameter_Set[1])
		{
			*has_err = 1;
			return;
		}
		Parameter_Set[0] = temp; // 保存参数
		*has_ok = 1;			 // 设置成功
		return;
	}
	// ============== 设置液位下限 H2 ==============
	if (str_equal(id, "H2"))
	{
		temp = parse_m_to_10x(val);
		// 格式错误 或 H2 大于 H1（上限）→ 非法
		if (temp == 0xFFFF || temp > Parameter_Set[0])
		{
			*has_err = 1;
			return;
		}
		Parameter_Set[1] = temp;
		*has_ok = 1;
		return;
	}
	// ============== 设置频率上限 F ==============
	if (str_equal(id, "F"))
	{
		temp = parse_integer(val); // 字符串转整数
		if (temp == 0xFFFF)		   // 格式错误
		{
			*has_err = 1;
			return;
		}
		Parameter_Set[2] = temp;
		*has_ok = 1;
		return;
	}
	// ============== 设置容器类型 S ==============
	if (str_equal(id, "S"))
	{
		temp = parse_integer(val);
		if (temp > 2) // 只能是0/1/2
		{
			*has_err = 1;
			return;
		}
		Parameter_Set[3] = temp;
		*has_ok = 1;
		return;
	}
	// ============== 设置半径 r ==============
	if (str_equal(id, "r"))
	{
		temp = parse_m_to_10x(val);
		if (temp == 0xFFFF)
		{
			*has_err = 1;
			return;
		}
		Parameter_Set[4] = temp;
		*has_ok = 1;
		return;
	}
	// ============== 设置长度 L ==============
	if (str_equal(id, "L"))
	{
		temp = parse_m_to_10x(val);
		if (temp == 0xFFFF)
		{
			*has_err = 1;
			return;
		}
		Parameter_Set[5] = temp;
		*has_ok = 1;
		return;
	}
	// ============== 设置宽度 W ==============
	if (str_equal(id, "W"))
	{
		temp = parse_m_to_10x(val);
		if (temp == 0xFFFF)
		{
			*has_err = 1;
			return;
		}
		Parameter_Set[6] = temp;
		*has_ok = 1;
		return;
	}
	// ============== 设置高度 H ==============
	if (str_equal(id, "H"))
	{
		temp = parse_m_to_10x(val);
		if (temp == 0xFFFF)
		{
			*has_err = 1;
			return;
		}
		Parameter_Set[7] = temp;
		*has_ok = 1;
		return;
	}

	*has_err = 1; // 以上参数都不匹配 → 未知指令（可能为设置指令）
}

/* 串口处理：接收完成后解析指令 */
void Uart_Proc()
{
	unsigned char pos = 0;	   // 解析位置索引
	unsigned char id[5];	   // 存放参数名
	unsigned char val[12];	   // 存放参数值
	unsigned char has_ok = 0;  // 是否有成功设置的参数
	unsigned char has_err = 0; // 是否有错误
	unsigned char has_set = 0; // 是否是设置指令

	if (Uart_Rec_Index == 0)
		return;			// 没有接收数据，直接退出
	if (Sys_Tick >= 10) // 接收超时（10ms），表示一帧数据接收完成
	{
		Rec_Flag = 0; // 清除接收标志
		Sys_Tick = 0; // 清空超时计时

		// 循环解析缓冲区里的所有指令
		while (pos < Uart_Rec_Index)
		{
			// 提取一条 (参数名,参数值)
			pos = extract_one_param(pos, id, val);
			if (pos == 0xFF) // 解析失败，格式错误
			{
				has_err = 1;
				break;
			}
			// 处理这条参数
			handle_param(id, val, &has_ok, &has_err, &has_set);
			pos++; // 位置+1，继续解析下一条
		}

		// 如果是设置指令，返回结果
		if (has_set)
		{
			if (has_err && has_ok)
				printf("WARN\r\n"); // 既有成功又有错误
			else if (has_err)
				printf("ERROR\r\n"); // 全部错误
			else if (has_ok)
				printf("OK\r\n"); // 全部成功
		}

		Uart_Rec_Index = 0; // 准备下一次接收
	}
}
/*定时器0初始化*/
void Timer0Init(void) // 0毫秒@12.000MHz
{
	AUXR &= 0x7F; // 定时器时钟12T模式
	TMOD &= 0xF0; // 设置定时器模式
	TMOD |= 0x05;
	TL0 = 0x00; // 设置定时初值
	TH0 = 0x00; // 设置定时初值
	TF0 = 0;	// 清除TF0标志
	TR0 = 1;	// 定时器0开始计时
}

/*定时器1初始化*/
void Timer1Init(void) // 1毫秒@12.000MHz
{
	AUXR &= 0xBF; // 定时器时钟12T模式
	TMOD &= 0x0F; // 设置定时器模式
	TL1 = 0x18;	  // 设置定时初值
	TH1 = 0xFC;	  // 设置定时初值
	TF1 = 0;	  // 清除TF1标志
	TR1 = 1;	  // 定时器1开始计时
	ET1 = 1;
	EA = 1;
}

/* 定时器1中断服务函数 */
void Timer1server(void) interrupt 3
{
	if (++Key_Slow == 10)
		Key_Slow = 0;
	if (++Seg_Slow == 400)
		Seg_Slow = 0;
	if (++Seg_Pos == 8)
		Seg_Pos = 0;
	Seg_Disp(Seg_Pos, Seg_Buf[Seg_Pos], Seg_Point[Seg_Pos]);
	Led_Disp(Seg_Pos, ucLed[Seg_Pos]);
	// 串口超时解析
	if (Rec_Flag)
		Sys_Tick++;
	// 1s读一次
	if (++Freq_Timer200ms == 200)
	{
		Freq_Timer200ms = 0;
		freq = TH0 << 8 | TL0;
		freq *= 5;
		Freq_Count = (Freq_Count + 1) % 5;
		TH0 = TL0 = 0;
		Freq_New = 1; // 通知主循环
	}
	// L7闪烁
	if (Freq_Trend_Alarm)
	{
		if (++Timer100ms == 100)
		{
			Timer100ms = 0;
			StarL7_Flag ^= 1;
		}
	}
	else
		StarL7_Flag = Timer100ms = 0;
	// 剧烈变化 点亮3s
	if (Freq_Jump_Alarm)
	{
		// 3秒计时（100ms 递减一次）
		if (++Timer200ms == 200)
		{
			Timer200ms = 0;
			StarL8_Flag ^= 1; // 闪烁
			if (Freq_Jump_Led_Tick > 0)
				Freq_Jump_Led_Tick--;
		}
		if (Freq_Jump_Led_Tick == 0)
			Freq_Jump_Alarm = StarL8_Flag = 0;
	}
	else
		StarL8_Flag = Timer200ms = 0;
}

/* 串口中断服务函数 */
void Uartserver(void) interrupt 4
{
	if (RI)
	{
		Rec_Flag = 1;
		Sys_Tick = 0;
		if (Uart_Rec_Index < sizeof(Uart_Rec) - 1)
		{
			Uart_Rec[Uart_Rec_Index] = SBUF;
			Uart_Rec_Index++;
		}
		else
			Uart_Rec_Index = 0;
		RI = 0;
	}
}

void main()
{
	Sys_Init();
	Timer0Init();
	Timer1Init();
	UartInit();
	Set_Rtc(ucRtc);

	Disp_Mode = Data_Mode = 0;
	Level_High_Alarm = Level_Low_Alarm = 0;
	Freq_Value_Alarm = 0;
	StarL7_Flag = StarL8_Flag = 0;
	Freq_Trend_Alarm = Freq_Jump_Alarm = 0;

	while (1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
		Update_Alarm_State();
		Uart_Proc();
	}
}

void Sys_Init()
{
	P2 = 0x80;
	P0 = 0xff;
	P2 = 0x00;
	P2 = 0xa0;
	P0 = 0xa0;
	P2 = 0x00;
}

unsigned char Key_Read()
{
	unsigned char temp = 0;
	P44 = 0;
	P42 = 1;
	P35 = 1;
	if (P33 == 0)
		temp = 4;
	if (P32 == 0)
		temp = 5;
	P44 = 1;
	P42 = 0;
	P35 = 1;
	if (P33 == 0)
		temp = 8;
	if (P32 == 0)
		temp = 9;
	return temp;
}

void Seg_Disp(unsigned char wei, unsigned char table, bit point)
{
	P0 = 0xff;
	P2 = P2 & 0x1f | 0xe0;
	P2 &= 0x1f;

	P0 = seg_wei[wei];
	P2 = P2 & 0x1f | 0xc0;
	P2 &= 0x1f;

	P0 = seg_table[table];
	if (point)
		P0 &= 0x7f;
	P2 = P2 & 0x1f | 0xe0;
	P2 &= 0x1f;
}

void Led_Disp(unsigned char addr, bit enable)
{
	static unsigned char temp = 0x00;
	static unsigned char temp_old = 0xff;

	if (enable)
		temp |= 0x01 << addr;
	else
		temp &= ~(0x01 << addr);
	if (temp != temp_old)
	{
		P0 = ~temp;
		P2 = P2 & 0x1f | 0x80;
		P2 &= 0x1f;
		temp_old = temp;
	}
}
void Relay(bit enable)
{
	static unsigned char temp = 0x00;
	static unsigned char temp_old = 0xff;

	if (enable)
		temp |= 0x10;
	else
		temp &= ~0x10;
	if (temp != temp_old)
	{
		P0 = temp;
		P2 = P2 & 0x1f | 0xa0;
		P2 &= 0x1f;
		temp_old = temp;
	}
}
