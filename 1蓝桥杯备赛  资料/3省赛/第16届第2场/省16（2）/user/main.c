#include <STC15F2K60S2.H>
#include <intrins.h>
#include <stdio.h>
#include <string.h>
#include "sys.h"
#include "display.h"
#include "key.h"
#include "timer.h"
#include "ds1302.h"
#include "iic.h"
#include "onewire.h"
#include "chaoshen.h"


unsigned char LED[]={0,0,0,0,0,0,0,0};
unsigned char SMG[]={16,16,16,16,16,16,16,16};
unsigned char dot[]={0,0,0,0,0,0,0,0};
unsigned char show_state;//总界面切换变量，0-测距界面，1-时间参数，2-次数参数
unsigned char distance_state=2;//测距模式切换变量，0-单次，1-累加，2-清零

float tempera;//温度获取变量
unsigned int distance;//距离获取变量
unsigned char distance_dc;//单次测量结果
unsigned char dc_flag;//单次测量标志位，1-启动，0-停止
unsigned int distance_lj;//累加测量结果
unsigned int speed;//声速
unsigned char num_para_disp=3;//次数参数显示
unsigned char num_para_cltr=3;//次数参数保存
unsigned char num;//目前已累加个数
unsigned char jia_flag1=1;//时间加减标志位,0-减，1-加
unsigned char jia_flag2=1;//次数加减标志位,0-减，1-加
float output=0;//电压输出变量

//定义计算测距结果的函数
unsigned int distance_data(unsigned int v)//传入参数声速
{
	unsigned char x;
	x=cs_wave_data();
	return x*v/340;//由超声底层决定
}



void led_task()
{
	if(led_ms<10)
		return;
	led_ms=0;
	/*******LED区域*********/
	LED[0]=(show_state==0);
	if(distance_state==0)
	{
		if(distance_dc!=0&&distance_dc<20||distance_dc>100)
			LED[1]=1;
		else 
			LED[1]=0;
	}
	else if(distance_state==1)
	{
		if(start_plus==0)//累加结束后
		{
			if(distance_lj!=0&&(distance_lj<20||distance_lj>100))
				LED[1]=1;
			else 
				LED[1]=0;
		}
	}
	else if(distance_state==2)
		LED[1]=0;
	if(start_plus==1)
		LED[2]=led_flash;
	else
		LED[2]=0;
}



void smg_task()
{
	if(smg_ms<70)
		return;
	smg_ms=0;
	/**********数据处理区域************/
	tempera=read_tempera();//实时获取温度数据
	speed=330+0.6*tempera;//计算声速
	if(distance_state==0)//单次
	{
		if(dc_flag==1)//启动单次测距
		{
			distance=distance_data(speed);
			distance_dc=distance;
			if(distance_dc<=20)
				output=1;
			else if(distance_dc>=100)
				output=5;
			else
				output=(1/25.0*(distance_dc-20)+1)*1.0;
		}
		dc_flag=0;
		pcf8591_write(output*51.0);
	}
		
	else if(distance_state==1)//累加
	{
		start_plus=1;//开始累加测距
		if(start_plus==1)
		{
			if(num==0)
			{
				num++;
				distance=distance_data(speed);
				distance_lj=distance_lj+distance;
			}
			if(num!=0)
			{
				if(plus_flag==1)//只在整参数时间置1
				{
					num++;
					distance=distance_data(speed);
					distance_lj=distance_lj+distance;
					plus_flag=0;
				}
			}
			if(distance_lj<=20)
				output=1;
			else if(distance_lj>=100)
				output=5;
			else
				output=(1/25.0*(distance_lj-20)+1)*1.0;
		}
		if(num==num_para_cltr)//次数到位，结束累加测距
			start_plus=0;
		pcf8591_write(output*51.0);
	}
	else if(distance_state==2)//清零
		output=0;
	
	
	
	/**********数据显示区域************/
	switch (show_state)
	{
		case 0://测距
			switch (distance_state)
			{
				case 0://单次
					SMG[0]=10;
					SMG[1]=12;
					SMG[2]=(unsigned char)(tempera)/10%10;
					SMG[3]=(unsigned char)(tempera)%10;
					SMG[4]=(distance_dc<1000)?16:distance_dc/1000%10;
					SMG[5]=(distance_dc<100)?16:distance_dc/100%10;
					SMG[6]=(distance_dc<10)?16:distance_dc/10%10;
					SMG[7]=distance_dc%10;
				break;
				case 1://累加
					SMG[0]=14;
					SMG[1]=12;
					SMG[2]=(unsigned char)(tempera)/10%10;
					SMG[3]=(unsigned char)(tempera)%10;
					SMG[4]=(distance_lj<1000)?16:distance_lj/1000%10;
					SMG[5]=(distance_lj<100)?16:distance_lj/100%10;
					SMG[6]=(distance_lj<10)?16:distance_lj/10%10;
					SMG[7]=distance_lj%10;
				break;
				case 2://清零或未启动
					SMG[0]=12;
					SMG[1]=12;
					SMG[2]=(unsigned char)(tempera)/10%10;
					SMG[3]=(unsigned char)(tempera)%10;
					SMG[4]=16;
					SMG[5]=16;
					SMG[6]=16;
					SMG[7]=16;
				break;
			}
		break;
		case 1://时间参数
			SMG[0]=17;
			SMG[1]=1;
			SMG[2]=16;
			SMG[3]=16;
			SMG[4]=16;
			SMG[5]=16;
			SMG[6]=time_para_disp/10;
			SMG[7]=time_para_disp%10;
		break;
		case 2://次数参数
			SMG[0]=17;
			SMG[1]=2;
			SMG[2]=16;
			SMG[3]=16;
			SMG[4]=16;
			SMG[5]=16;
			SMG[6]=num_para_disp/10;
			SMG[7]=num_para_disp%10;
		break;
	}
}



void key_task()
{
	unsigned char keynum;
	if(key_ms<15)
		return;
	key_ms=0;
	keynum=key_scan2();
	switch (keynum)
	{
		case 13://启动单次
			if(show_state==0)
			{
				dc_flag=1;
				distance_state=0;
				distance_lj=0;//单次可以结束累加
				num=0;
			}
		break;
		case 9://启动累加
			if(show_state==0)
			{
				distance_state=1;
				distance_dc=0;//清零单次数据
				distance_lj=0;//再次按下累加开始首次累加
				num=0;
			}
		break;
		case 14:
			if(start_plus==0)//累加结束才能按
			{
				if(++show_state==3)
					show_state=0;
				if(show_state==0)//参数只在退出参数界面生效
					num_para_cltr=num_para_disp;
				if(show_state==2)
				{
					time_para_cltr=time_para_disp;
					num_para_disp=num_para_cltr;
				}
				if(show_state==1)
					time_para_disp=time_para_cltr;
			}
		break;
		case 10:
			if(show_state==1)//调时间参数
			{
				if(jia_flag1==1)
					time_para_disp+=2;
				else if(jia_flag1==0)
					time_para_disp-=2;
				if(time_para_disp==6)
					jia_flag1=0;//减
				if(time_para_disp==2)
					jia_flag1=1;//加
			}
			if(show_state==2)//调次数参数
			{
				if(jia_flag2==1)
					num_para_disp+=2;
				else if(jia_flag2==0)
					num_para_disp-=2;
				if(num_para_disp==7)
					jia_flag2=0;//减
				if(num_para_disp==3)
					jia_flag2=1;//加
			}
		break;
		case 15:
			if(start_plus==0)//累加结束才能按
			{
				if(show_state==0)
				{
					distance_dc=0;//单次数据清零
					distance_lj=0;//累加数据清零
					num=0;
					distance_state=2;
					output=0;
				}
			}
		break;
	}
}


void Delay750ms(void)	//@12.000MHz
{
	unsigned char data i, j, k;

	_nop_();
	_nop_();
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
	read_tempera();
	Delay750ms();
	sys_init();
	Timer0_Init();
	while(1)
	{
		led_task();
		smg_task();
		key_task();
	}
}

