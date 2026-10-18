#include <STC15F2K60S2.H>
#include "sys.h"
#include "led.h"
#include "smg.h"
#include "timer.h"
#include "key.h"
#include "iic.h"
#include "ds1302.h"
#include "onewire.h"


//模版变量
uchar temp[]={17,17,17,17,17,17,17,17};
uchar dot[]={0,0,0,0,0,0,0,0};
uchar LED[]={0,0,0,0,0,0,0,0};
uchar pos;
uint smg_dely;
uint dat_dely=200;
uchar key_dely;


uchar state_display,mode;  //界面显示  回显子界面  3温湿度界面
uchar hum_max;
uchar hum;
uint hum_all;
uint temperature_max;  //最大温度
uint temperature;  //当前温度 *10
uint temperature_all;  //温度总和
uint temperature_old;  //上一次温度
uchar hum_old;   //上一次湿度
uchar count;   //触发次数
uchar time_record[2];  //触发时间
uchar temperature_set=30;  //温度参数
uchar light;  //光敏电阻值
bit flag;  //亮暗状态标志  1亮 0暗
bit trig;  //触发标志  1为触发
uchar show_old;  //上一次界面
uint sec_trig;  //触发时间3s
bit state4;   //闪烁标志
bit useless=1;  //无效状态 1有效 0无效
void display()
{
	if(smg_dely<300)return;
	else smg_dely=0;
	switch(state_display)
	{
		case 0:
			dot[6]=0;
			temp[0]=time[2]/16;
      temp[1]=time[2]%16;
			temp[2]=16;
			temp[3]=time[1]/16;
			temp[4]=time[1]%16;
			temp[5]=16;
			temp[6]=time[0]/16;
			temp[7]=time[0]%16;break;
		case 1:
			switch(mode)
			{
				case 0:
					dot[6]=(count>0)?1:0;
					temp[0]=12;
					temp[1]=17;
				  temp[2]=(count>0)?temperature_max/100:17;
					temp[3]=(count>0)?temperature_max/10%10:17;
				  temp[4]=(count>0)?16:17;
				  temp[5]=(count>0)?(temperature_all/count)/100:17;
				  temp[6]=(count>0)?(temperature_all/count)/10%10:17;
				  temp[7]=(count>0)?(temperature_all/count)%10:17;break;
				case 1:
					dot[6]=(count>0)?1:0;
					temp[0]=18;
				  temp[2]=(count>0)?hum_max/10:17;
				  temp[3]=(count>0)?hum_max%10:17;
				  temp[4]=(count>0)?16:17;
				  temp[5]=(count>0)?(hum_all/count)/100:17;
				  temp[6]=(count>0)?(hum_all/count)/10%10:17;
				  temp[7]=(count>0)?(hum_all/count)%10:17;break;
				case 2:
					dot[6]=0;
					temp[0]=15;
					temp[1]=count/10;
					temp[2]=count%10;
				  temp[3]=(count>0)?time_record[1]/16:17;
				  temp[4]=(count>0)?time_record[1]%16:17;
				temp[5]=(count>0)?16:17;
				  temp[6]=(count>0)?time_record[0]/16:17;
				  temp[7]=(count>0)?time_record[0]%16:17;break;
			}break;
		case 2:
			dot[6]=0;
			temp[0]=19;
      temp[1]=17;
			temp[2]=17;
			temp[3]=17;
			temp[4]=17;
			temp[5]=17;
			temp[6]=temperature_set/10;
			temp[7]=temperature_set%10;break;
		case 3:
			dot[6]=0;
			temp[0]=14;
      temp[1]=17;
			temp[2]=17;
			temp[3]=temperature/100;
			temp[4]=temperature/10%10;
			temp[5]=16;
		  temp[6]=(hum>=10&&hum<=90)?hum/10:10;
			temp[7]=(hum>=10&&hum<=90)?hum%10:10;break;
		
		
	}
}

void dat_pro()
{
	if(dat_dely<400)return;
	else dat_dely=0;
	temp_read();
	ds1302_read();
	light=PCF8591_Read(0x01);
	if(light<50&&flag==1&&trig==0) //亮到暗状态
	{
		useless=1;
		flag=0;
		trig=1;
		show_old=state_display;
		temperature_old=temperature;
		hum_old=hum;
		temperature=temp_read();
		hum=fre/22.5+1.12;
		if(hum>=10&&hum<=90)//有效触发采集
		{
			count++;
			time_record[1]=time[2];
			time_record[0]=time[1];
			if(hum>hum_max)hum_max=hum;
			if(temperature>temperature_max)temperature_max=temperature;
			hum_all +=hum*10;  //方便显示小数点
			temperature_all +=temperature;  //底层已经*10
		}
		else
			useless=0;
		state_display=3;
	}
	else if(light>50)
		flag=1;
	if(sec_trig>=3000)
	{
		trig=0;
		sec_trig=0;
		state_display=show_old;
	}
}

void key_pro()
{
	uchar key;
	if(trig==1) return;
	if(key_dely<10) return;
	else key_dely=0;
	key=key_scan();
	switch(key)
	{
		case 13:
			mode=0;
			state_display=(state_display+1)%3;
			break;//S4
		case 9:
			if(state_display==1)
				mode=(mode+1)%3;
			break;//S5
		case 14:
			if(state_display==2)
			{
				if(temperature_set<99)
				temperature_set +=1;
			}
			break;//S8
		case 10:
			if(state_display==2)
			{
				if(temperature_set)
				temperature_set -=1;
			}
			break;//S9
		case 40:
			if(state_display==1&&mode==2)
			{
			count=0;
			temperature=0;
			hum=0;
			hum_all=0;
			temperature_all=0;
			hum_max=0;
			temperature_max=0;
			}
		
	}
	
}

void led_pro()
{
	LED[0]=(state_display==0)?1:0;
	LED[1]=(state_display==1)?1:0;
	LED[2]=(state_display==3)?1:0;
	LED[3]=(temperature/10.0>temperature_set)?state4:0;
	LED[4]=(useless==0)?1:0;
	LED[5]=(count>1&&(hum>hum_old&&temperature>temperature_old))?1:0;
}


void main()
{
	sys_init();
	ds1302_write();
	temp_read();
	PCF8591_Read(0x01);
	timer0_init();
	timer1_init();
	while(1)
	{
		key_pro();
		display();
		dat_pro();
		led_pro();
	}
}