/*===============================================第十二届国赛==================================================
@Author:小殷同学
@Date:2023.6.4
===============================================================================================================*/
#include "public.h"
#include "iic.h"
#include "ds1302.h"
/*=============================================下面为变量和宏定义相关============================================*/
//数码管段码（0~9、shut-off、"-"、）
code uchar smg_data[] = {0xc0,0xf9,0xa4,0xb0,0x99,0x92,0x82,0xf8,0x80,0x90,0xff,
                         0xbf,0xc7,0xc6,0x89,0x8c,0x8e};

//初始化，使所有数码管熄灭（必须是9个元素）

uchar L[9];                                                           //LED
xdata uchar smg_bit[11] = {10,10,10,10,10,10,10,10,10};               //数码管位
xdata uint avg_dis = 0,max_dis = 0,min_dis = 0;                       //平均值、最大值、最小值
uchar interface_mode = 1;                                             //界面模式 1（数据） 2（参数）
uchar data_interface_index = 1;                                       //数据界面索引
uchar param_interface_index = 1;                                      //参数界面索引
uchar record_index = 1;                                               //记录索引
uchar hour =20,min = 20,sec = 1;                                      //时分秒
uchar param_set_dis = 20,set_dis = 20;                                             //距离参数
xdata uchar param_set_time[5] = {2,3,5,7,9},param_time_index = 0,set_time_index = 0;   //时间参数
xdata uint dis = 0;                                                   //实时距离
uchar triger_mode = 1;                                                //1(触发模式) 2(定时模式)
                 
bit light_mode  = 0;                                                  //光线状态标志位
bit param_valid = 0;                                                  //参数设置生效标志位
uchar last_adc_value = 0,adc_value = 0;
bit dis_flag = 0;                                                     //测距标志
bit l5_flag = 0;                                                     //LED5标志
uchar dis_count = 0;                                                  //测距次数
uint dis_str[12] = {0};                                              //存储测距数据 这里设置12次
uint dis_sum = 0;                                                    //总距离和
xdata uint dis1 = 0,dis2 = 0,dis3 = 0;
//刷新频率
uchar time_feq = 0;                                                   //时间刷新
uchar key_feq = 0;                                                    //按键刷新
uchar led_feq = 0;                                                    //LED刷新
uchar adc_feq = 0;                                                    //adc采集刷新
uint dis_feq = 0;                                                    //测距刷新
/*==============================================下面为函数声明相关================================================*/
void smg_task(void);                               //数码管显示任务
void data_task(void);                              //数据处理任务
void logical_task(void);                           //逻辑处理任务
void key_task(void);                               //按键任务 
void Init_System(void);                            //系统初始化
uint Get_Dis_Max(void);                            //获取最大值
uint Get_Dis_Min(void);                            //获取最小值
float Get_Dis_Avg(void);                           //获取平均值
/*===============================================下面为函数实现相关================================================*/

uint Get_Dis_Max(void)
{
	uchar i = 0;
	uint temp_dis = dis_str[0];
	for(i = 0;i<dis_count;i++)
	{
		if(temp_dis < dis_str[i])
		{
			temp_dis = dis_str[i];
		}
	}			
	return temp_dis;
}


uint Get_Dis_Min(void)
{
	uchar i = 0;
	uint temp_dis = dis_str[0];
	for(i = 0;i<dis_count;i++)
	{
		if(temp_dis > dis_str[i])
		{
			temp_dis = dis_str[i];
		}
	}			
	return temp_dis;
}

float Get_Dis_Avg(void)
{
	uchar i = 0;
	float sum_dis = 0;
	for(i = 0;i<dis_count;i++)
	{
		sum_dis += dis_str[i];
	}			
	return sum_dis/1.0/dis_count;
}
	
void smg_task(void)
{
	

	//数据界面
	if(interface_mode == 1)
	{
		if(data_interface_index == 1)  //时间显示
		{
			smg_bit[1] = hour/10;
			smg_bit[2] = hour%10;
			smg_bit[3] = 11;  // '-'
			smg_bit[4] = min/10;
			smg_bit[5] = min%10;
			smg_bit[6] = 11;  // '-'
			smg_bit[7] = sec/10;
			smg_bit[8] = sec%10;
		}
		else if(data_interface_index == 2) //距离显示
		{
			//触发模式
			if(triger_mode == 1) 
			{				
				smg_bit[1] = 12; // L 1110 0011 0xc7
				smg_bit[2] = 13; // C 0110 0011 0xc6
				smg_bit[3] = 10;
				smg_bit[4] = 10;
				smg_bit[5] = 10;
				smg_bit[6] = (dis > 99)?(dis/100):(10);
				smg_bit[7] = (dis > 9)?(dis/10%10):(10);
				smg_bit[8] = dis%10;
			}
			//定时模式
			else if(triger_mode == 2)
			{
				smg_bit[1] = 12; // L 1110 0011 0xc7
				smg_bit[2] = 16; // F 0111 0001 0x8e
				smg_bit[3] = 10;
				smg_bit[4] = 10;
				smg_bit[5] = 10;
				smg_bit[6] = 10;
				smg_bit[7] = dis/10;
				smg_bit[8] = dis%10;
			}
		}
		else if(data_interface_index == 3) //数据记录显示
		{
			if(record_index == 1)
			{
				smg_bit[1] = 14; // H 1001 0001 0x89
				smg_bit[2] = record_index;
				smg_bit[3] = 10;
				smg_bit[4] = 10;
				smg_bit[5] = (max_dis >999)?(max_dis/1000):(10);
				smg_bit[6] = (max_dis > 99)?(max_dis/100%10):(10);
				smg_bit[7] = (max_dis > 9)?(max_dis/10%10):(10);
				smg_bit[8] = max_dis%10;
			}
			else if(record_index == 2)
			{
				smg_bit[1] = 14; // H 1001 0001 0x89
				smg_bit[2] = record_index;
				smg_bit[3] = 10;
				smg_bit[4] = 10;
				smg_bit[5] = (min_dis >999)?(min_dis/1000):(10);
				smg_bit[6] = (min_dis > 99)?(min_dis/100%10):(10);
				smg_bit[7] = (min_dis > 9)?(min_dis/10):(10);
				smg_bit[8] = min_dis%10;
				
			}
			else if(record_index == 3)
			{
				smg_bit[1] = 14; // H 1001 0001 0x89
				smg_bit[2] = record_index;
				smg_bit[3] = 10;
				smg_bit[4] = 10;
				smg_bit[5] = (avg_dis >999)?(avg_dis/1000):(10);
				smg_bit[6] = (avg_dis > 99)?(avg_dis/100%10):(10);
				smg_bit[7] = (avg_dis > 9)?(avg_dis/10%10):(10);
				smg_bit[8] = avg_dis%10;
			}
		}
	
	}

	//参数界面
	else if(interface_mode == 2)
	{
		if(param_interface_index == 1)  //采集时间参数
		{
				smg_bit[1] = 15; // P 0011 0001 0x8c
				smg_bit[2] = param_interface_index;
				smg_bit[3] = 10;
				smg_bit[4] = 10;
				smg_bit[5] = 10;
				smg_bit[6] = 10;
				smg_bit[7] = param_set_time[set_time_index]/10;
				smg_bit[8] = param_set_time[set_time_index]%10;
		}
		else if(param_interface_index == 2) //距离参数
		{
				smg_bit[1] = 15; // P 0011 0001 0x8c
				smg_bit[2] = param_interface_index;
				smg_bit[3] = 10;
				smg_bit[4] = 10;
				smg_bit[5] = 10;
				smg_bit[6] = 10;
				smg_bit[7] = set_dis/10;
				smg_bit[8] = set_dis%10;
		}
	}
}
void data_task(void)
{
	uchar time[3];
	static uchar count_flag = 0;
	
	if(T2H < 0xd9)
	{
		//时间刷新
		if(time_feq > 200)
		{
			time_feq = 1;
			time[0] = Read_Ds1302_Byte(0x81);
			time[1] = Read_Ds1302_Byte(0x83);
			time[2] = Read_Ds1302_Byte(0x85);
			
			sec  = (time[0]/16*10 + time[0]%16);
			min  = (time[1]/16*10 + time[1]%16);
			hour = (time[2]/16*10 + time[2]%16);
		}
		//距离刷新  
		//单次触发测距
		if(dis_flag == 1 && triger_mode == 1)
		{
			if(dis_feq > 120)
			{
				dis_feq = 1;
				dis_flag = 0;
				dis = Get_Distance();
				dis_str[dis_count] = dis;      //存储测距以及次数加加
				dis_count++;
			}				
		}
		//定时触发测距 3次

		else if(dis_flag ==1 && triger_mode == 2)
		{
			if(dis_feq > param_set_time[set_time_index]*1000)  //dis_feq为毫秒
			{
				dis_feq = 1;
				if(count_flag == 0)   //第一次
				{
					dis1 = Get_Distance();
					dis_str[dis_count++] = dis1;      //存储测距以及次数加加
					count_flag = 1;
				}
				else if(count_flag == 1)  //第二次
				{
					dis2 = Get_Distance();
					dis_str[dis_count++] = dis2;      //存储测距以及次数加加
					count_flag = 2;
				}
				else if(count_flag == 2)
				{
					dis3 = Get_Distance();
					dis_str[dis_count++] = dis3;      //存储测距以及次数加加
					count_flag = 3;
				}
				else if(count_flag == 3)
				{
					count_flag = 0;
					dis_flag = 0;                   //结束定时测距
				}
				
			}
		}
		//LED刷新
		if(led_feq > 50)
		{
			led_feq = 1;
			//下面为LED控制
			Control_IO(0x80,~(L[1] << 0|L[2] << 1|L[3] << 2|L[4]<<3|L[5] << 4|L[6] << 5));
		}
		//触发模式
		if(triger_mode == 1)
		{
			if(adc_feq > 120)
			{
				adc_feq = 1;
				last_adc_value = adc_value;  //保存上一次的值
				adc_value = Read_ADC(0x41);
			}
		}
	}
}




void logical_task(void)
{
	//下面为DAC电压输出
	if(dis >= 0 && dis <= 10)
	{
		Write_DAC(51); //1V
	}
	else if(dis > 10 && dis <= 80)
	{
		//y = kx + b  y = 3x + 21;
		Write_DAC(3*dis + 21);
	}
	else
	{
		//dis > 80  5V
		Write_DAC(255);
	}
	
	//下面为LED的逻辑控制
	L[1] = (data_interface_index == 1)?(1):(0);  //时间数据显示界面
	L[2] = (data_interface_index == 2)?(1):(0);  //距离显示界面
	L[3] = (data_interface_index == 3)?(1):(0);  //数据记录界面
	L[4] = (triger_mode == 1)?(1):(0);           //触发模式
	L[5] = (triger_mode == 2 && l5_flag== 1)?(1):(0);           //定时模式
	L[6] = (light_mode == 1)?(1):(0);            //光线状态为亮时点亮
	
	//下面为设置参数生效赋值
	if(param_valid == 1)
	{
		param_set_dis = set_dis;
		param_time_index = set_time_index;
	}
	
	//下面为触发模式逻辑
	if(adc_value < 60 && (last_adc_value > adc_value + 6) && triger_mode == 1)
	{
		dis_flag = 1;        //开始测距
		light_mode = 0;
	}
	else if(adc_value > 60)
	{
		light_mode = 1;
	}
	//下面为定时触发逻辑 实时获取的秒值可以整除设置参数时触发一次距离数据的采集和刷新
	if(sec %  param_set_time[set_time_index] == 0 && triger_mode == 2)
	{
		dis_flag = 1;        //开始测距
	}
	//三次在参数附件则点亮L5
	if((dis1 > param_set_dis-5 && dis1 < param_set_dis+5) && (dis2 > param_set_dis-5 && dis2 < param_set_dis+5) && (dis3 > param_set_dis-5 && dis3 < param_set_dis+5))
	{
		l5_flag = 1;
	}
	else
	{
		l5_flag = 0;
	}
	//下面为最大值 最小值 平均值处理
	max_dis = Get_Dis_Max();
	min_dis = Get_Dis_Min();
	avg_dis = Get_Dis_Avg()*10;
	
}

void key_task(void)
{
	uchar key_value = 0;
	if(key_feq > 20)
	{
		key_feq = 1;
		key_value = Read_Key();
	}
	switch(key_value)
	{
		//界面显示切换（数据和参数）
		case 4: 
			if(interface_mode == 1)
			{
				interface_mode = 2;
				param_valid = 0;             //参数失效标志 
				param_interface_index = 1;  //每次从数据切换参数界面时显示时间参数界面
			}
			else if(interface_mode == 2)
			{
				interface_mode = 1;
				param_valid = 1;             //退出参数界面时 参数生效标志 
				data_interface_index = 1;    //每次从参数设置界面进入数据即人们默认为时间数据显示
			}
			break;
		//数据界面切换*时间、距离、数据记录） 参数界面切换（时间、距离）
		case 5:
		   //数据界面
			if(interface_mode == 1)
			{
				if(++data_interface_index > 3)
				{
					data_interface_index = 1;
				}
			}
			//参数界面
			else if(interface_mode == 2)
			{
				if(++param_interface_index > 2)
				{
					param_interface_index = 1;
				}
			}
			break;
		//测距界面下模式切换（触发模式、定时模式）
		case 8:
			//距离
			if(data_interface_index == 2)
			{
				if(++triger_mode > 2)
				{
					triger_mode = 1;
				}
			}
			//记录
			else if(data_interface_index == 3)
			{
				//最大值  最小值  平均值
				if(++record_index > 3)
				{
					record_index = 1;
				}
			}
			break;
		//参数设置
		case 9:
			//在时间参数下2 3 5 7 9s切换
			if(param_interface_index == 1)
			{
				//数组存放时间
				if(++set_time_index > 4)
				{
					set_time_index = 0;
				}
			}
			//距离参数切换 每次加10
			else if(param_interface_index == 2)
			{
				if(set_dis < 80)
				{
					set_dis += 10;
				}
				else
				{
					set_dis = 10;
				}
			}
			break;
		default:break;
	}
}


void Init_System(void)
{
	Control_IO(0x80,0xff);                   //关闭LED
	Control_IO(0xa0,0x00);                   //关闭蜂鸣器和继电器
	Control_IO(0xc0,0x00);                   //关闭数码管
	
	Timer2Init();                            //定时器初始化
	Write_DS1320_Init(hour,min,sec);         //时间初始化
}
void main(void)
{
	Init_System();                  //系统初始化
	while(1)
	{
		data_task();               //数据采集任务
		logical_task();            //逻辑处理任务
		smg_task();                //数码管显示任务
		key_task();                //按键任务
	}
}

/*==================================================下面为中断相关================================================*/
void Timer2_Server() interrupt 12
{
	static uchar dsp_smg = 1;
	
	//下面为数码管控制
	Control_IO(0xc0,0x00);
	if(interface_mode == 1 && data_interface_index == 3 && record_index == 3 && dsp_smg == 7)
	{
		Control_IO(0xe0,smg_data[smg_bit[dsp_smg]] & 0x7f);
	}
	else
	{
		Control_IO(0xe0,smg_data[smg_bit[dsp_smg]]);
	}
	
	Control_IO(0xc0,1 << (dsp_smg - 1));
	if(++dsp_smg > 8)
	{
		dsp_smg = 1;
	}
	//下面为数据刷新变量
	time_feq++;
	key_feq++;
	led_feq++;
	adc_feq++;
	dis_feq++;
}

