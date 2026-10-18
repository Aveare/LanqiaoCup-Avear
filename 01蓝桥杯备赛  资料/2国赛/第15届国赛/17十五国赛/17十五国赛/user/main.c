#include <STC15F2K60S2.H>
#include <stdio.H>
#include <string.H>
#include <intrins.H>
#include <init.H>
#include <seg.H>
#include <led.H>
#include <uart.H>
#include <key.H>
#include <csb.H>
#include <iic.H>
#include <math.H>

unsigned char key_val,key_up,key_down,key_old;
idata unsigned char seg_buff[]={10,10,10,10,10,10,10,10};
idata unsigned char seg_point[]={0,0,0,0,0,0,0,0};
idata unsigned char ucled[]={0,0,0,0,0,0,0,0};
unsigned char slow_down_key;
unsigned char slow_down_num;
unsigned int slow_down_seg;
unsigned int sys_index;
unsigned char reuart[10];
unsigned char uart_index;
bit flat_uart;
bit flat_freq=0;
unsigned int time_1s;
unsigned int freq;
unsigned char seg_mod=0;
int weizhi_x=0;
int weizhi_y=0;
int mudi_x=0;
int mudi_y=0;
unsigned char run_mod=0;//0空闲，1忙碌，2等待
unsigned char zhangai_l;//障碍物距离
float run_v;
char r=10;
char b=0;
char real_r=10;
char real_b=0;
bit flat_zhangai=0;
bit flat_run=0;
bit flat_rb=0;
bit run_play=0;
long int zhixian_l;
unsigned int time_200ms;
unsigned char guangmin;
bit flat_led0=0;
unsigned char time_100ms;
bit flat_led3=0;
unsigned int time_3s;
float temp_x;
float temp_y;

void key_proc()
{
	if(slow_down_key<10)return;
	slow_down_key=0;
	key_val=read_key();
	key_down=key_val&(key_old^key_val);
	key_up=~key_val&(key_old^key_val);
	key_old=key_val;
	switch(key_down)
	{
		case 4:
		if(flat_run==1&&run_mod==0)
		{
			flat_run=0;
			if(run_mod==0)
			{
				run_play=1;
				run_mod=1;
			}
			
		}
		else if(run_mod==1)
			run_mod=2;
		else if(run_mod==2)
		{
			if(zhangai_l>30)
				run_mod=1;
		}
		break;
		case 5:
		if(run_mod==0)
		{
			weizhi_x=0;
			weizhi_y=0;
		}
		break;
		case 8:
		seg_mod++;
		if(seg_mod==3)
			seg_mod=0;
		if(seg_mod==0)
		{
			flat_rb=0;
			real_r=r;
			real_b=b;
		}
		break;
		case 9:
		flat_rb^=1;
		break;
		case 12:
		if(flat_rb==0)
		{
			r+=1;
			if(r==21)
				r=20;
		}
		else
		{
			b+=5;
			if(b==95)
				b=90;
		}
		break;
		case 13:
		if(flat_rb==0)
		{
			r-=1;
			if(r==-1)
				r=0;
		}
		else
		{
			b-=5;
			if(b==-95)
				b=-90;
		}
		break;
	}
}

void seg_proc()
{
	if(slow_down_seg<280)return;
	slow_down_seg=0;
	switch(seg_mod)
	{
		case 0:
		if(run_mod==0)
		{
			seg_buff[0]=12;
			seg_buff[1]=(weizhi_x<100)?10:weizhi_x/100%10;
			seg_buff[2]=(weizhi_x<10)?10:weizhi_x/10%10;
			seg_point[2]=0;
			seg_buff[3]=weizhi_x%10;
			seg_buff[4]=11;
			seg_buff[5]=(weizhi_y<100)?10:weizhi_y/100%10;
			seg_buff[6]=(weizhi_y<10)?10:weizhi_y/10%10;
			seg_point[6]=0;
			seg_buff[7]=weizhi_y%10;
		}
		else if(run_mod==1||run_mod==2)
		{
			seg_buff[0]=12;
			seg_buff[1]=(mudi_x<100)?10:mudi_x/100%10;
			seg_buff[2]=(mudi_x<10)?10:mudi_x/10%10;
			seg_point[2]=0;
			seg_buff[3]=mudi_x%10;
			seg_buff[4]=11;
			seg_buff[5]=(mudi_y<100)?10:mudi_y/100%10;
			seg_buff[6]=(mudi_y<10)?10:mudi_y/10%10;
			seg_point[6]=0;
			seg_buff[7]=mudi_y%10;
		}
		break;
		case 1:
		if(run_mod==1)
		{
			seg_buff[0]=13;
			seg_buff[1]=1;
			seg_buff[2]=10;
			seg_point[2]=0;
			seg_buff[3]=(run_v<1000)?10:(unsigned int)run_v/1000%10;
			seg_buff[4]=(run_v<100)?10:(unsigned int)run_v/100%10;
			seg_buff[5]=(run_v<10)?10:(unsigned int)run_v/10%10;
			seg_buff[6]=(unsigned int)run_v%10;
			seg_point[6]=1;
			seg_buff[7]=(unsigned int)(run_v*10)%10;
		}
		else if(run_mod==0)
		{
			seg_buff[0]=13;
			seg_buff[1]=2;
			seg_buff[2]=10;
			seg_point[2]=0;
			seg_buff[3]=11;
			seg_buff[4]=11;
			seg_buff[5]=11;
			seg_buff[6]=11;
			seg_point[6]=0;
			seg_buff[7]=11;
		}
		else if(run_mod==2)
		{
			seg_buff[0]=13;
			seg_buff[1]=3;
			seg_buff[2]=10;
			seg_point[2]=0;
			seg_buff[3]=(zhangai_l<10000)?10:zhangai_l/10000%10;
			seg_buff[4]=(zhangai_l<1000)?10:zhangai_l/1000%10;
			seg_buff[5]=(zhangai_l<100)?10:zhangai_l/100%10;
			seg_buff[6]=(zhangai_l<10)?10:zhangai_l/10%10;
			seg_point[6]=0;
			seg_buff[7]=zhangai_l%10;
		}
		break;
		case 2:
			seg_buff[0]=14;
			seg_buff[1]=10;
			seg_buff[2]=r/10%10;
			seg_point[2]=1;
			seg_buff[3]=r%10;
			seg_buff[4]=10;
			if(b<0)
			{
				seg_buff[5]=(-b>=10)?11:10;
				seg_buff[6]=(-b<10)?11:(-b)/10%10;
				seg_point[6]=0;
				seg_buff[7]=(-b)%10;
			}
			else
			{
				seg_buff[5]=(b<100)?10:b/100%10;
				seg_buff[6]=(b<10)?10:b/10%10;
				seg_point[6]=0;
				seg_buff[7]=b%10;
			}
		break;
	}
}

void led_proc()
{
	if(run_mod==0)
	{
		ucled[0]=0;
		flat_led0=0;
	}
	else if(run_mod==1)
	{
		ucled[0]=1;
		flat_led0=0;
	}
	else if(run_mod==2)
	{
		flat_led0=1;
	}
}

void num_proc()
{
	if(slow_down_num<200)return;
	slow_down_num=0;
	run_v=(3.14*r/10*freq/100)+b;
	zhangai_l=read_csb();
	guangmin=read_ad(0x01);
	if(zhangai_l<30)
	{
		if(run_mod==1)
			run_mod=2;
	}
	if(run_mod==1&&run_play==1)
	{
		zhixian_l=sqrt((float)(mudi_x-weizhi_x)*(mudi_x-weizhi_x)+(float)(mudi_y-weizhi_y)*(mudi_y-weizhi_y));
		temp_x=(float)(mudi_x-weizhi_x)/zhixian_l;
		temp_y=(float)(mudi_y-weizhi_y)/zhixian_l;
		run_play=0;
	}
	if(run_mod==1)
	{
		if(zhixian_l<=0)
		{
			zhixian_l=0;
			weizhi_x=mudi_x;
			weizhi_y=mudi_y;
			run_mod=0;
			flat_led3=1;
			time_3s=0;
		}
	}
	if(run_mod==1&&guangmin<61)
		ucled[1]=1;
	else
		ucled[1]=0;
	if(run_mod==1)
		relay(1);
	else
		relay(0);
}

void uart_proc()
{
	unsigned char i;
	if(uart_index==0)return;
	if(sys_index>10)
	{
		flat_uart=sys_index=0;
		if(uart_index>=5&&reuart[0]=='('&&reuart[uart_index-1]==')'&&uart_index<10)
		{
			if(run_mod==0)
			{
				mudi_x=0;
				mudi_y=0;
				flat_run=1;
				for(i=1;i<uart_index;i++)
				{
					if(reuart[i]!=',')
					{
						mudi_x*=10;mudi_x+=(reuart[i]-'0');
					}
					else
					break;
				}
				i++;
				for(;i<uart_index;i++)
				{
					if(reuart[i]!=')')
					{
						mudi_y*=10;mudi_y+=(reuart[i]-'0');
					}
					else
					break;
				}
				printf("Got it");
			}
			else if(run_mod==1)
			{
				printf("Busy");
			}
		}
		else if(uart_index==1&&reuart[0]=='?')
		{
			if(run_mod==0)
				printf("Idle");
			else if(run_mod==1)
				printf("Busy");
			else if(run_mod==2)
				printf("Wait");
		}
		else if(uart_index==1&&reuart[0]=='#')
		{
			printf("(%d,%d)",(unsigned int)weizhi_x,(unsigned int)weizhi_y);
		}
		else
			printf("Error");
		memset(reuart,0,uart_index);
		uart_index=0;
	}
}

void Timer1Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0xBF;		//定时器时钟12T模式
	TMOD &= 0x0F;		//设置定时器模式
	TL1 = 0x18;		//设置定时初值
	TH1 = 0xFC;		//设置定时初值
	TF1 = 0;		//清除TF1标志
	TR1 = 1;		//定时器1开始计时
	ET1 = 1;
	EA = 1;
}

void Timer0Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0x7F;		//定时器时钟12T模式
	TMOD &= 0xF0;		//设置定时器模式
	TMOD |= 0x05;
	TL0 = 0;		//设置定时初值
	TH0 = 0;		//设置定时初值
	TF0 = 0;		//清除TF0标志
	TR0 = 1;		//定时器0开始计时
}


void time1proc() interrupt 3
{
	slow_down_seg++;
	slow_down_key++;
	slow_down_num++;
	if(flat_uart)
		sys_index++;
	if(++time_1s==1000)
	{
		TR0=0;
		time_1s=0;
		freq=(TH0<<8)|TL0;
		TH0=TL0=0;
		TR0=1;
		if(flat_freq==0)
		{
			freq=0;flat_freq=1;
		}
	}
	if(run_mod==1&&zhixian_l>=0&&time_200ms++==500)
	{
		time_200ms=0;
		zhixian_l-=(run_v*0.5);
		weizhi_x+=(run_v*0.5)*temp_x;
		weizhi_y+=(run_v*0.5)*temp_y;
	}
	if(flat_led0==1&&time_100ms++==100)
	{
		time_100ms=0;
		ucled[0]^=1;
	}
	if(flat_led3==1)
	{
		ucled[2]=1;
		if(time_3s++==3000)
		{
			ucled[2]=0;
			flat_led3=0;
		}
	}
	seg_disp(slow_down_seg%8,seg_buff[slow_down_seg%8],seg_point[slow_down_seg%8]);
	led_disp(slow_down_seg%8,ucled[slow_down_seg%8]);
}

void uart1proc() interrupt 4
{
	if(RI==1)
	{
		flat_uart=1;
		sys_index=0;
		reuart[uart_index]=SBUF;
		uart_index++;
		RI=0;
	}
	if(uart_index>=10)
		uart_index=0;
}

void main()
{
	sys_init();
	Timer1Init();
	Timer0Init();
	UartInit();
	while(1)
	{
		key_proc();
		seg_proc();
		led_proc();
		uart_proc();
		num_proc();
	}
}
