/*头文件引入*/
#include <STC15F2K60S2.H>
#include <onewire.h>
#include <ds1302.h>
#include <iic.H>
#include <ut.h>
#include <uart.h>
#include <stdio.h>

/* 底层变量 */
unsigned char Key_Slow=0,Key_Val,Key_Down,Key_Up,Key_Old;
unsigned int Seg_Slow,Update_Slow;
unsigned char Seg_Pos;
unsigned char Seg_Buf[8]={10,10,10,10,10,10,10,10};
unsigned char Seg_Point[8]={0,0,0,0,0,0,0,0};
unsigned char ucLed[8]={0,0,0,0,0,0,0,0};

unsigned char code seg_table[]={0xC0,0xF9,0xA4,0xB0,0x99,0x92,0x82,0xF8,0x80,0x90,0xff,0xbf,0x8c,0x86};
unsigned char code seg_wei[]={0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};

/* 底层函数声明 */
void Sys_Init();
unsigned char Key_Read();
void Seg_Disp(unsigned char wei,unsigned char addr,bit point);
void Led_Disp(unsigned char addr,bit enable);
void Relay(bit enable);

/* 计时变量 */
unsigned int freq;//频率
unsigned int Timer1000ms;
float temperature;
unsigned char ucRtc[3]={23,59,50};
unsigned int Speed_10x;// 超声波声速（温度补偿)
bit Speed_10x_EN; // 声速计算使能
unsigned int Voltage_Calibration_Mv;// 电压校准值
/*容器类型/尺寸参数 */
unsigned char Container_Type;//0-球 1-圆柱 2-长方体
float R_Para_Cm;//半径
float Cylinder_H_Cm;
float Cuboid_L_Cm, Cuboid_W_Cm;//长 宽
float Cuboid_H_Cm;//高
float Liquid_Height;//液位高度
unsigned int High_Val_100x=0;// 液位高度放大100倍
float Volume_10x;// 液体体积
float Liquid_Volume=0.0f; // 最终液体体积
unsigned char Remaining_Space;//剩余空间
float Weight;//重量
/*显示相关*/
unsigned char Disp_Mode=0;//0-数据界面 1-参数界面 2-标定界面
unsigned char Data_Mode=0;//0-时间 1-液位 2-体积 3-重量
unsigned int Parameter_Set[8]={10,1,2000,2,10,10,10,10};//0-H1 1-H2 2-F 3-S 4-r 5-L 6-W 7-H
unsigned char Set_Mode_Index=0;
/*重量标定参数 */
unsigned char Mark[5]={0,5,10,15,20};//标定重量
unsigned char Vol_Calibration_10x[5]={0,10,20,40,50};//标定电压值
unsigned char Vol_Calibration_10x_Con[5]={0,10,20,40,50};
unsigned char Mark_Index;//索引 
/*频率记录相关*/
unsigned int Freq_History[5]={0,0,0,0,0}; // 频率历史值
unsigned char Freq_History_Count;//频率值改变 索引
unsigned char Freq_Jump_Led_Tick;
unsigned char Timer100ms;
/* 串口 */
unsigned char Uart_Rec[64];
unsigned char Uart_Rec_Index;
bit Rec_Flag;
unsigned char Sys_Tick;
unsigned char Command[15]; /* (修改) 串口命令缓存 */
unsigned char Command_Num; /* (修改) 串口命令个数 */
unsigned char Uart_RIGHT_Flag; /* (修改) 串口成功标志 */
unsigned char Uart_ERROR_Flag; /* (修改) 串口错误标志 */
/* 异常与记录 */
unsigned char Last_Fault_Code;//最后故障报错码
bit Level_High_Alarm;   // 液位上限报警
bit Level_Low_Alarm;    // 液位下限报警
bit Freq_Value_Alarm;   // 频率超限报警
bit Freq_Trend_Alarm;   // 频率趋势报警
unsigned char Timer100ms;
bit StarL7_Flag;//闪烁L7
bit Freq_Trend_Dir;     // 频率趋势方向
bit Freq_Jump_Alarm;    // 频率突变报警
unsigned char Timer200ms;
bit StarL8_Flag;
/*获取参数真实值 10cm->m*/
unsigned int get_param_cm(unsigned char index)
{
	return Parameter_Set[index] * 10;
}
/*字符串对比*/
bit str_equal(unsigned char *str1, unsigned char *str2)
{
	unsigned char i = 0;
	while (str1[i] != '\0' && str2[i] != '\0')
	{
		if (str1[i] != str2[i])return 0; // 字符不同→不相等
		i++;
	}
	return (str1[i] == str2[i]);// 全部相同→相等
}
/*字符串转整数*/
unsigned int parse_integer(unsigned char *str)
{
	unsigned int val=0;
	unsigned char i=0;
	if(str[0]=='\0')return 0xFFFF; // 空字符串→错误
	while(str[i] != '\0')
	{
		if(str[i]<'0' || str[i]>'9')return 0xFFFF;// 非数字→错误
		val = val*10 + str[i]-'0';//ASCII码转数字
		i++;
	}
	return val;
}
/*字符串转小数（放大 10 倍）*/
unsigned int parse_m_to_10x(unsigned char *str)
{
	unsigned int integer=0;//整数部分
	unsigned char decimal=0;//小数部分
	unsigned char i=0;
	bit has_dot=0;//是否有小数点

	if(str[0]=='\0')return 0xFFFF;
	while(str[i] != '\0')
	{
		if(str[i]=='.')
		{
			if(has_dot)return 0xFFFF;// 多个小数点→错误
			has_dot=1;
		}
		else if(str[i]>='0' && str[i]<='9')
		{
			if(!has_dot)integer = integer*10 + str[i]-'0';//获取整数部分
			else
			{
				decimal = str[i]-'0';//获取 小数
				i++;
				if(str[i] != '\0')return 0xFFFF;
				return integer*10 + decimal;//返回整个数
			}
		}
		else return 0xFFFF;
		i++;
	}//跑完循环返回即可
	return integer*10;
}
void Command_Proc(void)
{
	unsigned int temp;
	unsigned char h, m, s;
	switch(Command[1])
	{
		case 'H':
			if(str_equal(Command, "(H1,?)"))
			{
				printf("(H1,%u.%u)", Parameter_Set[0]/10, Parameter_Set[0]%10);
			}
			else if(str_equal(Command, "(H2,?)"))
			{
				printf("(H2,%u.%u)", Parameter_Set[1]/10, Parameter_Set[1]%10);
			}
			else if(str_equal(Command, "(H,?)"))
			{
				printf("(H,%u.%u)", Parameter_Set[7]/10, Parameter_Set[7]%10);
			}
			else if(Command[2] == '1')
			{
				temp = parse_m_to_10x(Command + 4);
				if(temp != 0xFFFF && temp >= Parameter_Set[1])
				{
					Parameter_Set[0] = temp;
					Uart_RIGHT_Flag++;
				}
				else Uart_ERROR_Flag++;
			}
			else if(Command[2] == '2')
			{
				temp = parse_m_to_10x(Command + 4);
				if(temp != 0xFFFF && temp <= Parameter_Set[0])
				{
					Parameter_Set[1] = temp;
					Uart_RIGHT_Flag++;
				}
				else Uart_ERROR_Flag++;
			}
			else
			{
				temp = parse_m_to_10x(Command + 3);
				if(temp != 0xFFFF)
				{
					Parameter_Set[7] = temp;
					Uart_RIGHT_Flag++;
				}
				else Uart_ERROR_Flag++;
			}
		break;
		case 'F':
			if(str_equal(Command, "(F,?)"))printf("(F,%u)", Parameter_Set[2]);
			else
			{
				temp = parse_integer(Command + 3);
				if(temp != 0xFFFF)
				{
					Parameter_Set[2] = temp;
					Uart_RIGHT_Flag++;
				}
				else Uart_ERROR_Flag++;
			}
		break;
		case 'S':
			if(str_equal(Command, "(S,?)"))printf("(S,%u)", Parameter_Set[3]);
			else
			{
				temp = parse_integer(Command + 3);
				if(temp <= 2)
				{
					Parameter_Set[3] = temp;
					Uart_RIGHT_Flag++;
				}
				else Uart_ERROR_Flag++;
			}
		break;
		case 'r':
			if(str_equal(Command, "(r,?)"))printf("(r,%u.%u)", Parameter_Set[4]/10, Parameter_Set[4]%10);
			else
			{
				temp = parse_m_to_10x(Command + 3);
				if(temp != 0xFFFF)
				{
					Parameter_Set[4] = temp;
					Uart_RIGHT_Flag++;
				}
				else Uart_ERROR_Flag++;
			}
		break;
		case 'L':
			if(str_equal(Command, "(L,?)"))printf("(L,%u.%u)", Parameter_Set[5]/10, Parameter_Set[5]%10);
			else
			{
				temp = parse_m_to_10x(Command + 3);
				if(temp != 0xFFFF)
				{
					Parameter_Set[5] = temp;
					Uart_RIGHT_Flag++;
				}
				else Uart_ERROR_Flag++;
			}
		break;
		case 'W':
			if(str_equal(Command, "(W,?)"))printf("(W,%u.%u)", Parameter_Set[6]/10, Parameter_Set[6]%10);
			else
			{
				temp = parse_m_to_10x(Command + 3);
				if(temp != 0xFFFF)
				{
					Parameter_Set[6] = temp;
					Uart_RIGHT_Flag++;
				}
				else Uart_ERROR_Flag++;
			}
		break;
		case 'T':
			if(Command[0]=='(' && Command[2]==':' &&
				Command[3]>='0' && Command[3]<='9' &&
				Command[4]>='0' && Command[4]<='9' &&
				Command[5]>='0' && Command[5]<='9' &&
				Command[6]>='0' && Command[6]<='9' &&
				Command[7]>='0' && Command[7]<='9' &&
				Command[8]>='0' && Command[8]<='9')
			{
				h = (Command[3]-'0')*10 + (Command[4]-'0');
				m = (Command[5]-'0')*10 + (Command[6]-'0');
				s = (Command[7]-'0')*10 + (Command[8]-'0');
				if(h < 24 && m < 60 && s < 60)
				{
					ucRtc[0] = h;
					ucRtc[1] = m;
					ucRtc[2] = s;
					Set_Rtc(ucRtc);
					Uart_RIGHT_Flag++;
				}
				else Uart_ERROR_Flag++;
			}
			else Uart_ERROR_Flag++;
		break;
		default:
			Uart_ERROR_Flag++;
		break;
	}
}
/*故障上报 */
void Report_Fault(unsigned int fault_id, unsigned int value_100x)
{
	Read_Rtc(ucRtc);// 读取当前时间
	printf("[%u,", fault_id); // 打印故障ID
	printf("%02bu%02bu%02bu,", ucRtc[0],ucRtc[1],ucRtc[2]);

	if(fault_id==500 || fault_id==501)printf("%d.%02d]\r\n", (int)value_100x/100%10, (int)value_100x%100);
	else printf("%u]\r\n", value_100x);
	Last_Fault_Code = (unsigned char)(fault_id - 500 + 1);//最终报错
}
/*温度读取+速度校准值 */
void Get_Temperature()
{
	temperature = read_t();
	Speed_10x_EN = 1;//使能速度 先读温度再算校准值 再计算距离
	Speed_10x = (unsigned int)(3300 + 6 * temperature);//声速=330m/s+温度×0.6
	Voltage_Calibration_Mv = (unsigned int)(4* temperature * temperature/100.0f - 10.0f * temperature + 225.0f);
}
/*重量计算 AD 采集 +五点标定*/
void Get_Calc_Weight()
{
	unsigned char i;
	unsigned int ad_val;//AD值
	int voltage_mv;//采集mv值
	int cal_mv;//计算的mv值=（采集+校准）
	float lower_mv;//区间下限电压 mv
	float upper_mv;//区间上限电压 mv
	float k;

	ad_val = AD_Read(0x43);
	voltage_mv = (int)ad_val * 5000 / 255;
	cal_mv = voltage_mv + (int)Voltage_Calibration_Mv;
	//区间判断
	if(cal_mv <= (int)Vol_Calibration_10x[0] * 100){Weight = 0.0f;return;}
	if(cal_mv >= (int)Vol_Calibration_10x[4] * 100){Weight = 20.0f;return;}
	//找到RB2电压值对应的区间
	for(i=0;i<4;i++)
	{
		if(cal_mv < (int)Vol_Calibration_10x[i+1] * 100)break;
	}
	//获取区间上下限电压 mv
	lower_mv = (float)Vol_Calibration_10x[i] * 100.0f;
	upper_mv = (float)Vol_Calibration_10x[i+1] * 100.0f;
	//计算所占比例
	k = ((float)cal_mv - lower_mv) / (upper_mv - lower_mv);
	//计算出重量
	Weight = (float)Mark[i] + ((float)Mark[i+1] - (float)Mark[i]) * k;
}
/*超声波读距+计算*/ 
void Get_Distance()
{
	unsigned char temp_distance;
	float temp1;
	float total_volume_10x;

	if(!Speed_10x_EN)return;//若未读取t，直接返回（校准值未计算，速度值未校准）
	//获取计算参数
	Container_Type = (unsigned char)Parameter_Set[3];//容器类型
	R_Para_Cm = Parameter_Set[4] * 10.0f;//半径
	Cylinder_H_Cm = Parameter_Set[7] * 10.0f;//圆柱体的高度
	Cuboid_L_Cm = Parameter_Set[5] * 10.0f;//长
	Cuboid_W_Cm = Parameter_Set[6] * 10.0f;//宽
	Cuboid_H_Cm = Parameter_Set[7] * 10.0f;//高
	//读取速度值
	temp_distance = Ut_Wave_Data((unsigned char)(Speed_10x/10));
	//根据容器类型计算
	switch(Container_Type)
	{
		case 0://球
			if(temp_distance >= (unsigned char)(2 * R_Para_Cm))Liquid_Height = 0;
			else Liquid_Height = 2 * R_Para_Cm - temp_distance;

			temp1 = 3.14f * Liquid_Height * Liquid_Height * (3 * R_Para_Cm - Liquid_Height) / 3.0f;
			Volume_10x = temp1 / 100000.0f;
			total_volume_10x = 4.0f * 3.14f * R_Para_Cm * R_Para_Cm * R_Para_Cm / 3.0f / 100000.0f;

			temp1 = 3.14f * temp_distance * temp_distance * (3 * R_Para_Cm - temp_distance) / 3.0f;
			if(total_volume_10x > 0)Remaining_Space = (unsigned char)((temp1 / 100000.0f) * 100.0f / total_volume_10x + 0.5f);
			else Remaining_Space = 0;
		break;
		case 1://圆柱
			if(temp_distance >= (unsigned char)Cylinder_H_Cm)Liquid_Height = 0;
			else Liquid_Height = Cylinder_H_Cm - temp_distance;
			Volume_10x = 3.14f * R_Para_Cm * R_Para_Cm * Liquid_Height / 100000.0f;
			if(Cylinder_H_Cm > 0)Remaining_Space = (unsigned char)(temp_distance * 100.0f / Cylinder_H_Cm + 0.5f);
			else Remaining_Space = 0;
		break;
		case 2://长方体
			if(temp_distance >= (unsigned char)Cuboid_H_Cm)Liquid_Height = 0;
			else Liquid_Height = Cuboid_H_Cm - temp_distance;
			Volume_10x = Cuboid_L_Cm * Cuboid_W_Cm * Liquid_Height / 100000.0f;
			if(Cuboid_H_Cm > 0)Remaining_Space = (unsigned char)(temp_distance * 100.0f / Cuboid_H_Cm + 0.5f);
			else Remaining_Space = 0;
		break;
	}

	if(Remaining_Space > 100)Remaining_Space = 100;
	if(Liquid_Height < 0)Liquid_Height = 0;
	High_Val_100x = (unsigned int)(Liquid_Height + 0.5f);
	Liquid_Volume = Volume_10x / 10.0f;
}
/*更新报警状态*/
void Update_Alarm_State()
{
	unsigned char i;
	unsigned int cur_level_cm;//当前液位高度
	unsigned int h1_cm;//液位上限
	unsigned int h2_cm;//液位下限
	unsigned int diff;//频率差值
	bit up_flag=1, down_flag=1;//默认上升 下降趋势都是1
	
	bit cur_high, cur_low, cur_freq;//当前液面是否过高 过低 频率是否超限
	bit cur_trend=0;//频率趋势异常
	bit cur_dir=0;//趋势方向	0-下降 1-上升 
	//减速
	if(Update_Slow)return;
	Update_Slow=1;

	cur_level_cm = High_Val_100x;
	h1_cm = get_param_cm(0);//获取参数：液位上限 H1
	h2_cm = get_param_cm(1);//获取参数：液位下限 H2
	//液面过高过低判断
	cur_high = (cur_level_cm > h1_cm);
	cur_low = (cur_level_cm < h2_cm);
	//频率超限判断
	cur_freq = (freq > Parameter_Set[2]);
	//触发报警
	if(cur_high && !Level_High_Alarm)Report_Fault(500, cur_level_cm);
	if(cur_low && !Level_Low_Alarm)Report_Fault(501, cur_level_cm);
	if(cur_freq && !Freq_Value_Alarm)Report_Fault(502, freq);
	//复制给Alarm进行报警
	Level_High_Alarm = cur_high;
	Level_Low_Alarm = cur_low;
	Freq_Value_Alarm = cur_freq;
	
	//记录最近5次频率
	if(Freq_History_Count < 5)Freq_History[Freq_History_Count++] = freq;
	else
	{
		for(i=0;i<4;i++)Freq_History[i]=Freq_History[i+1];
		Freq_History[4]=freq;
	}
	//判断频率是否连续上升 / 下降
	if(Freq_History_Count >= 5)
	{
		for(i=0;i<4;i++)//0↔1、1↔2、2↔3、3↔4
		{
			if(Freq_History[i+1] <= Freq_History[i])up_flag = 0;//上升标志位置0
			if(Freq_History[i+1] >= Freq_History[i])down_flag = 0;//下降标志位置0
		}
		if(up_flag)//如果连续 5 次上升 → 趋势报警，方向 = 上升 (1)
		{
			cur_trend = 1;//频率变化异常了
			cur_dir = 1;//1-上升
		}
		else if(down_flag)
		{
			cur_trend = 1;//频率变化异常了
			cur_dir = 0;//0-下降
		}
	}
	//只要出现趋势 或 趋势方向变了 → 报警 503
	if(cur_trend && (!Freq_Trend_Alarm || Freq_Trend_Dir != cur_dir))Report_Fault(503, cur_dir);
	//更新变量
	Freq_Trend_Alarm = cur_trend;
	Freq_Trend_Dir = cur_dir;


	//判断频率突变：3 次内变化超 1000
	if(Freq_History_Count >= 3)
	{
		if(Freq_History[Freq_History_Count-1] >= Freq_History[Freq_History_Count-3])
			diff = Freq_History[Freq_History_Count-1] - Freq_History[Freq_History_Count-3];
		else
			diff = Freq_History[Freq_History_Count-3] - Freq_History[Freq_History_Count-1];

		if(diff > 1000)//突变报警
		{
			Report_Fault(504, diff);
			Freq_Jump_Alarm = 1;
			Freq_Jump_Led_Tick = 30;//闪烁3s开始倒计时
		}
	}
	if(Freq_Jump_Led_Tick==0)Freq_Jump_Alarm = 0;//30s时间到停止报警
}

void Led_Proc()
{
	unsigned char i;
	//互斥点亮
	if(Disp_Mode==0)
	{
		for(i=0;i<4;i++)ucLed[i]=(Data_Mode==i);
	}
	ucLed[4] = Level_High_Alarm || Level_Low_Alarm;//到达上限or下限 
	ucLed[5] = Freq_Value_Alarm;//频率超限
	ucLed[6] = (Freq_Trend_Alarm && StarL7_Flag);
	ucLed[7] = (Freq_Jump_Alarm && StarL8_Flag);
	//继电器
	Relay(Level_High_Alarm || Level_Low_Alarm || Freq_Value_Alarm || Freq_Trend_Alarm || Freq_Jump_Alarm);
}

/* 按键处理 */
void Key_Proc()
{
	unsigned char i;
	if(Key_Slow)return;
	Key_Slow=1;
	Key_Val=Key_Read();
	Key_Down=Key_Val&(Key_Val^Key_Old);
	Key_Up=~Key_Val&(Key_Val^Key_Old);
	Key_Old=Key_Val;

	switch(Key_Down)
	{
		case 4://主界面切换
		   
			Disp_Mode=(Disp_Mode+1)%3;
			Data_Mode=Set_Mode_Index=Mark_Index=0;
			//标定电压值合理判断
			if(Disp_Mode==0)
			{
				if(Vol_Calibration_10x_Con[0]<Vol_Calibration_10x_Con[1]
					&&Vol_Calibration_10x_Con[1]<Vol_Calibration_10x_Con[2]
					&&Vol_Calibration_10x_Con[2]<Vol_Calibration_10x_Con[3]
					&&Vol_Calibration_10x_Con[3]<Vol_Calibration_10x_Con[4]
					&&Vol_Calibration_10x_Con[4]<Vol_Calibration_10x_Con[5])
				{
					for(i=0;i<5;i++)Vol_Calibration_10x[i]=Vol_Calibration_10x_Con[i];//合理保存电压值
				}
				else//否则恢复初始值
				{
					for(i=0;i<5;i++)Vol_Calibration_10x_Con[i]=Vol_Calibration_10x[i];
				}
			}
		break;
		case 5://子界面切换
			if(Disp_Mode==0)Data_Mode=(Data_Mode+1)%4;
			else if(Disp_Mode==1)Set_Mode_Index=(Set_Mode_Index+1)%8;
			else Mark_Index=(Mark_Index+1)%5;
		break;
		case 8://+
			if(Disp_Mode==2 && ++Vol_Calibration_10x_Con[Mark_Index] > 50)Vol_Calibration_10x_Con[Mark_Index] = 0;
		break;
		case 9://-
			if(Disp_Mode==2 && --Vol_Calibration_10x_Con[Mark_Index] > 50)Vol_Calibration_10x_Con[Mark_Index] = 50;
		break;
	}
}

/* 信息处理函数 */
void Seg_Proc()
{
	if(Seg_Slow)return;
	Seg_Slow=1;
	Get_Temperature();
	Get_Calc_Weight();
	Get_Distance();
	Read_Rtc(ucRtc);
	switch(Disp_Mode)
	{
		case 0://数据界面
			if(Data_Mode==0)//时间
			{
				Seg_Buf[0]=ucRtc[0]/10%10;
				Seg_Buf[1]=ucRtc[0]%10;
				Seg_Buf[2]=11;
				Seg_Buf[3]=ucRtc[1]/10%10;
				Seg_Buf[4]=ucRtc[1]%10;
				Seg_Buf[5]=11;
				Seg_Buf[6]=ucRtc[2]/10%10;
				Seg_Buf[7]=ucRtc[2]%10;
				Seg_Point[6]=Seg_Point[6]=0;
			}
			else if(Data_Mode==1)//液位
			{
				Seg_Buf[0]=10;
				if(temperature>=0)
				{
					Seg_Buf[1]=(int)temperature>=100?(int)temperature/100%10:10;
					Seg_Buf[2]=(int)temperature>=10?(int)temperature/10%10:10;
					Seg_Buf[3]=(int)temperature%10;
				}
				else
				{
					Seg_Buf[1]=11;//-
					Seg_Buf[2]=(int)temperature*(-1)>=10?(int)temperature*(-1)/10%10:10;
					Seg_Buf[3]=(int)temperature*(-1)%10;
				}
				Seg_Buf[4]=11;
				Seg_Buf[5]=High_Val_100x>=100?High_Val_100x/100%10:0;
				Seg_Buf[6]=High_Val_100x>=10?High_Val_100x/10%10:0;
				Seg_Buf[7]=High_Val_100x%10;
				Seg_Point[5]=1;
			}
			else if(Data_Mode==2)//体积
			{
				Seg_Buf[0]=Remaining_Space>=100?Remaining_Space/100%10:10;
				Seg_Buf[1]=Remaining_Space>=10?Remaining_Space/10%10:10;
				Seg_Buf[2]=Remaining_Space%10;
				Seg_Buf[3]=11;//-
				Seg_Buf[4]=(unsigned int)Liquid_Volume>=100?(unsigned int)Liquid_Volume/100%10:10;
				Seg_Buf[5]=(unsigned int)Liquid_Volume>=10?(unsigned int)Liquid_Volume/10%10:10;
				Seg_Buf[6]=(unsigned int)Liquid_Volume%10;
				Seg_Buf[7]=(unsigned int)(Liquid_Volume*10)%10;
				Seg_Point[5]=0;
				Seg_Point[6]=1;
			}
			else//重量
			{
				if(temperature>=0)
				{
					Seg_Buf[0]=(int)temperature>=100?(int)temperature/100%10:10;
					Seg_Buf[1]=(int)temperature>=10?(int)temperature/10%10:10;
					Seg_Buf[2]=(int)temperature%10;
				}
				else
				{
					Seg_Buf[0]=11;//-
					Seg_Buf[1]=(int)temperature*(-1)>=10?(int)temperature*(-1)/10%10:10;
					Seg_Buf[2]=(int)temperature*(-1)%10;
				}
				Seg_Buf[3]=11;
				Seg_Buf[4]=(unsigned int)Weight>=100?(unsigned int)Weight/100%10:10;
				Seg_Buf[5]=(unsigned int)Weight>=10?(unsigned int)Weight/10%10:10;
				Seg_Buf[6]=(unsigned int)Weight%10;
				Seg_Buf[7]=(unsigned int)(Weight*10)%10;
				Seg_Point[5]=0;
				Seg_Point[6]=1;
			}
		break;
		case 1://参数界面
			Seg_Buf[0]=12;//P
			Seg_Buf[1]=Set_Mode_Index+1;
			Seg_Buf[2]=11;//-
			switch(Set_Mode_Index)
			{
				case 2://频率参数F
					Seg_Buf[3]=Parameter_Set[Set_Mode_Index]>=10000?Parameter_Set[Set_Mode_Index]/10000%10:10;
					Seg_Buf[4]=Parameter_Set[Set_Mode_Index]>=1000?Parameter_Set[Set_Mode_Index]/1000%10:10;
					Seg_Buf[5]=Parameter_Set[Set_Mode_Index]>=100?Parameter_Set[Set_Mode_Index]/100%10:10;
					Seg_Buf[6]=Parameter_Set[Set_Mode_Index]>=10?Parameter_Set[Set_Mode_Index]/10%10:10;
					Seg_Buf[7]=Parameter_Set[Set_Mode_Index]%10;
					Seg_Point[5]=Seg_Point[6]=0;
				break;
				case 3://容器类型S
					Seg_Buf[3]=Seg_Buf[4]=Seg_Buf[5]=Seg_Buf[6]=10;
					Seg_Buf[7]=Parameter_Set[Set_Mode_Index];
					Seg_Point[5]=Seg_Point[6]=0;
				break;
				case 0://液体上限H1
				case 1://液体下限H2
				case 4://半径r
				case 5://长度L
				case 6://宽度W
				case 7://高度H
					Seg_Buf[3]=Parameter_Set[Set_Mode_Index]>=10000?Parameter_Set[Set_Mode_Index]/10000%10:10;
					Seg_Buf[4]=Parameter_Set[Set_Mode_Index]>=1000?Parameter_Set[Set_Mode_Index]/1000%10:10;
					Seg_Buf[5]=Parameter_Set[Set_Mode_Index]>=100?Parameter_Set[Set_Mode_Index]/100%10:10;
					Seg_Buf[6]=Parameter_Set[Set_Mode_Index]>=10?Parameter_Set[Set_Mode_Index]/10%10:0;
					Seg_Buf[7]=Parameter_Set[Set_Mode_Index]%10;
					Seg_Point[5]=0;
					Seg_Point[6]=1;
				break;
			}
		break;
		case 2:
			Seg_Buf[0]=13;//E
			Seg_Buf[1]=Mark[Mark_Index]/10%10;
			Seg_Buf[2]=Mark[Mark_Index]%10;
			Seg_Buf[3]=Seg_Buf[4]=Seg_Buf[5]=10;
			Seg_Buf[6]=Vol_Calibration_10x_Con[Mark_Index]/10%10;
			Seg_Buf[7]=Vol_Calibration_10x_Con[Mark_Index]%10;
			Seg_Point[5]=0;
			Seg_Point[6]=1;
		break;
	}
}
// 功能：从串口接收缓冲区中，解析出一条 (参数名,参数值) 格式的数据
// 输入：start=开始解析的位置 id_buf=存放参数名 val_buf=存放参数值
// 返回：解析完成后的位置，失败返回0xFF
unsigned char extract_one_param(unsigned char start,unsigned char *id_buf,unsigned char *val_buf)
{
	unsigned char i = start;    // i从指定起始位置开始遍历
	unsigned char j;            // 临时索引，用于填充id/val缓存

	// 1. 查找字符 '('，找到才开始解析
	while (i < Uart_Rec_Index && Uart_Rec[i] != '(')i++;
	// 如果遍历完都没找到'('，说明格式错误，返回0xFF
	if (i >= Uart_Rec_Index)return 0xFF;

	i++;                        // 跳过 '(' 这个字符
	j = 0;                      // 重置参数名缓存索引
	// 2. 读取参数名，直到遇到 ',' 或 ':' 停止
	while (i < Uart_Rec_Index && Uart_Rec[i] != ',' && Uart_Rec[i] != ':')
	{
		id_buf[j++] = Uart_Rec[i++];  // 把字符存入参数名缓存
	}
	id_buf[j] = '\0';           // 参数名末尾加结束符，变成标准字符串
	// 如果没找到','或':'，格式错误
	if (i >= Uart_Rec_Index)return 0xFF;

	i++;                        // 跳过 ',' 或 ':'
	j = 0;                      // 重置参数值缓存索引
	// 3. 读取参数值，直到遇到 ')' 停止
	while (i < Uart_Rec_Index && Uart_Rec[i] != ')')
	{
		val_buf[j++] = Uart_Rec[i++];  // 把字符存入参数值缓存
	}
	val_buf[j] = '\0';          // 参数值末尾加结束符
	// 如果没找到')'，格式错误
	if (i >= Uart_Rec_Index)return 0xFF;

	return i;                   // 返回当前解析到的位置，供下一次继续解析
}

// 功能：根据解析出的参数名和参数值，执行对应的操作（查询/设置）
// 输入：id=参数名 val=参数值 has_ok=是否设置成功 has_err=是否出错 has_set=是否是设置指令
void handle_param(unsigned char *id,unsigned char *val,unsigned char *has_ok,unsigned char *has_err,unsigned char *has_set)
{
	unsigned int temp;          // 临时变量，存放转换后的参数值
	unsigned char h, m, s;       // 时、分、秒

	// 如果参数值是 '?' → 表示查询参数，返回当前值
	if (val[0] == '?')
	{
		if (str_equal(id, "H1"))printf("(H1,%u.%u)\r\n", Parameter_Set[0]/10, Parameter_Set[0]%10);
		else if (str_equal(id, "H2"))printf("(H2,%u.%u)\r\n", Parameter_Set[1]/10, Parameter_Set[1]%10);
		else if (str_equal(id, "F"))printf("(F,%u)\r\n", Parameter_Set[2]);
		else if (str_equal(id, "S"))printf("(S,%u)\r\n", Parameter_Set[3]);
		else if (str_equal(id, "r"))printf("(r,%u.%u)\r\n", Parameter_Set[4]/10, Parameter_Set[4]%10);
		else if (str_equal(id, "L"))printf("(L,%u.%u)\r\n", Parameter_Set[5]/10, Parameter_Set[5]%10);
		else if (str_equal(id, "W"))printf("(W,%u.%u)\r\n", Parameter_Set[6]/10, Parameter_Set[6]%10);
		else if (str_equal(id, "H"))printf("(H,%u.%u)\r\n", Parameter_Set[7]/10, Parameter_Set[7]%10);
		else *has_err = 1;       // 不认识的参数名 → 错误
		return;                  // 查询完成，退出
	}

	*has_set = 1;                // 标记：这是一条设置指令

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
			*has_err = 1;         // 格式非法 → 错误
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
		*has_ok = 1;              // 设置成功
		return;
	}

	// ============== 设置液位上限 H1 ==============
	if (str_equal(id, "H1"))
	{
		temp = parse_m_to_10x(val);       // 把字符串转成小数×10
		// 格式错误 或 H1 小于 H2（下限）→ 非法
		if(temp == 0xFFFF || temp < Parameter_Set[1])
		{
			*has_err = 1;
			return;
		}
		Parameter_Set[0] = temp;          // 保存参数
		*has_ok = 1;                      // 设置成功
		return;
	}
	// ============== 设置液位下限 H2 ==============
	if (str_equal(id, "H2"))
	{
		temp = parse_m_to_10x(val);
		// 格式错误 或 H2 大于 H1（上限）→ 非法
		if(temp == 0xFFFF || temp > Parameter_Set[0])
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
		temp = parse_integer(val);        // 字符串转整数
		if(temp == 0xFFFF)                // 格式错误
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
		if(temp > 2)                      // 只能是0/1/2
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
		if(temp == 0xFFFF)
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
		if(temp == 0xFFFF)
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
		if(temp == 0xFFFF)
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
		if(temp == 0xFFFF)
		{
			*has_err = 1;
			return;
		}
		Parameter_Set[7] = temp;
		*has_ok = 1;
		return;
	}

	*has_err = 1;  // 以上参数都不匹配 → 未知指令
}

/* 串口处理：接收完成后解析指令 */
void Uart_Proc_Old()
{
	unsigned char pos = 0;        // 解析位置索引
	unsigned char id[5];          // 存放参数名
	unsigned char val[12];        // 存放参数值
	unsigned char has_ok = 0;     // 是否有成功设置的参数
	unsigned char has_err = 0;    // 是否有错误
	unsigned char has_set = 0;   // 是否是设置指令

	if(Uart_Rec_Index==0)return;  // 没有接收数据，直接退出
	if(Sys_Tick>=10)              // 接收超时（10ms），表示一帧数据接收完成
	{
		Rec_Flag=0;               // 清除接收标志
		Sys_Tick=0;               // 清空超时计时

		// 循环解析缓冲区里的所有指令
		while (pos < Uart_Rec_Index)
		{
			// 提取一条 (参数名,参数值)
			pos = extract_one_param(pos, id, val);
			if (pos == 0xFF)      // 解析失败，格式错误
			{
				has_err = 1;
				break;
			}
			// 处理这条参数
			handle_param(id, val, &has_ok, &has_err, &has_set);
			pos++;                // 位置+1，继续解析下一条
		}

		// 如果是设置指令，返回结果
		if(has_set)
		{
			if (has_err && has_ok)printf("WARN\r\n");  // 既有成功又有错误
			else if (has_err)printf("ERROR\r\n");      // 全部错误
			else if (has_ok)printf("OK\r\n");          // 全部成功
		}

		Uart_Rec_Index=0;         // 清空接收缓冲区，准备下一次接收
	}
}
/*定时器0初始化*/
void Timer0Init(void)		//0毫秒@12.000MHz
{
	AUXR &= 0x7F;		//定时器时钟12T模式
	TMOD &= 0xF0;		//设置定时器模式
	TMOD|=0x05;
	TL0 = 0x00;		//设置定时初值
	TH0 = 0x00;		//设置定时初值
	TF0 = 0;		//清除TF0标志
	TR0 = 1;		//定时器0开始计时
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

/* 定时器1中断服务函数 */
void Timer1server(void) interrupt 3
{
	if(++Key_Slow==10)Key_Slow=0;
	if(++Seg_Slow==300)Seg_Slow=0;
	if(++Update_Slow==1000)Update_Slow=0;
	if(++Seg_Pos==8)Seg_Pos=0;
	Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],Seg_Point[Seg_Pos]);
	Led_Disp(Seg_Pos,ucLed[Seg_Pos]);
	//串口超时解析
	if(Rec_Flag)Sys_Tick++;
	//1s读一次
	if(++Timer1000ms==1000)
	{
		Timer1000ms=0;
		freq=TH0<<8|TL0;
		TH0=TL0=0;
	}
	//L7闪烁
	if(Freq_Trend_Alarm)
	{
		if(++Timer100ms==100)
		{
			Timer100ms=0;
			StarL7_Flag^=1;
		}
	}
	else Timer100ms=StarL7_Flag=0;
	//L8闪烁  
	if(Freq_Jump_Alarm)
	{
		if(++Timer200ms==100)
		{
			Timer200ms=0;
			StarL8_Flag^=1;
		}
	}
	else Timer200ms=StarL8_Flag=0;
	//剧烈变化 点亮3s
	if(Freq_Jump_Alarm)
	{
		// 3秒计时（100ms 递减一次）
		if(++Timer200ms == 100)  
		{
			Timer200ms = 0;
			StarL8_Flag ^= 1; // 闪烁
			if(Freq_Jump_Led_Tick > 0)Freq_Jump_Led_Tick--;
		}
		if(Freq_Jump_Led_Tick == 0)Freq_Jump_Alarm=StarL8_Flag=0;
	}
	else StarL8_Flag =Timer200ms= 0;

}

/* 串口中断服务函数 */
void Uartserver(void) interrupt 4
{
	if(RI)
	{
		Rec_Flag=1;
		Sys_Tick=0;
		if(Uart_Rec_Index < sizeof(Uart_Rec)-1)
		{
			Uart_Rec[Uart_Rec_Index]=SBUF;
			Uart_Rec_Index++;
		}
		else Uart_Rec_Index=0;
		RI=0;
	}
}

void Uart_Proc(void)
{
	unsigned char i;
	char *token;
	if(Uart_Rec_Index == 0)return;
	if(Sys_Tick < 10)return;

	Rec_Flag = 0;
	Sys_Tick = 0;
	Command_Num = 0;

	for(i = 0; i < Uart_Rec_Index; i++)
	{
		if(Uart_Rec[i] == ')')Command_Num++;
	}

	for(i = 0; i < Command_Num; i++)
	{
		memset(Command, 0, sizeof(Command));
		if(i == 0)token = strtok((char *)Uart_Rec, ")");
		else token = strtok(NULL, ")");
		if(token == NULL)
		{
			Uart_ERROR_Flag++;
			break;
		}
		strncpy((char *)Command, token, sizeof(Command)-1);
		Command_Proc();
	}

	if((Uart_RIGHT_Flag == 0) && (Uart_ERROR_Flag != 0))printf("ERROR\r\n");
	else if((Uart_RIGHT_Flag != 0) && (Uart_ERROR_Flag != 0))printf("WARN\r\n");
	else if((Uart_RIGHT_Flag != 0) && (Uart_ERROR_Flag == 0))printf("OK\r\n");

	Uart_RIGHT_Flag = 0;
	Uart_ERROR_Flag = 0;
	memset(Uart_Rec, 0, sizeof(Uart_Rec));
	Uart_Rec_Index = 0;
	Command_Num = 0;
}

void main()
{
	Sys_Init();
	Timer0Init();
	Timer1Init();
	UartInit();
	Set_Rtc(ucRtc);
	while(1)
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
	P2=0x80;P0=0xff;P2=0x00;
	P2=0xa0;P0=0xa0;P2=0x00;
}

unsigned char Key_Read()
{
	unsigned char temp=0;
	P44=0;P42=1;P35=1;
	if(P33==0)temp=4;
	if(P32==0)temp=5;
	P44=1;P42=0;P35=1;
	if(P33==0)temp=8;
	if(P32==0)temp=9;
	return temp;
}

void Seg_Disp(unsigned char wei,unsigned char table,bit point)
{
	P2=0xe0;P0=0x00;P2=0x00;
	P2=0xc0;P0=seg_wei[wei];P2=0x00;
	P2=0xe0;P0=seg_table[table];
	if(point)P0&=~(1<<7);
	P2=0x00;
}

void Led_Disp(unsigned char addr,bit enable)
{
	unsigned char temp=0x00;
	if(enable)temp|=(0x01<<addr);
	else temp&=~(0x01<<addr);
	P2=0x80;P0=~temp;P2=0x00;
}
void Relay(bit enable)
{
	static unsigned char temp=0x00;
	if(enable)temp|=0x10;
	else temp&=~0x10;
	P2=0xa0;P0=temp;P2=0x00;
}
