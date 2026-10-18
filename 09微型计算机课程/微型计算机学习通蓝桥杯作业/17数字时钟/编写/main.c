#include <STC15F2K60S2.H>
#include"ds1302.h"
#define uchar unsigned char
#define uint unsigned int
uchar display_num[]={0xc0,0xf9,0xa4,0xb0,0x99,0x92,0x82,0xf8,0x80,0x90,0xbf};//0-9数码管编码表
uchar adress[]={0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};//位选表左到右
uchar buff[8],flag=0;//buff数组存储要显示的每位数字 flag=1才能进行加操作
uchar Trg,cont,i=0,shi,fen,miao,keycount=0;keyflag=0;
static uchar setfen,setmiao,setshi;
void init() 				//初始化单片机
{
	P2=0xa0;P0=0x00;P2=0x00;//关继电器蜂鸣器
	P2=0x80;P0=0xff;P2=0x00;//关led灯
}
void stop()//启停子函数
{
	Write_Ds1302_Byte(0x8e,0);//写保护，置零，关掉保护才能写数据
	if(miao<0x80)
	{
		Write_Ds1302_Byte(0x80,miao|0x80);//高位为1，停止
		flag=1;//可以进行加操作
	}
	else
	{
		Write_Ds1302_Byte(0x80,miao&0x7f);//高位值0，启动
		flag=0;
	}
	Write_Ds1302_Byte(0x8e,0x80);//打开保护
}
void readtime()//读取时间
{
	    EA=0;//关中断
		shi=Read_Ds1302_Byte(0x85);//时存在0x85里
		fen=Read_Ds1302_Byte(0x83);//同理，这些地址都是固定的
		miao=Read_Ds1302_Byte(0x81);
		EA=1;//开中断
	  	buff[0]=shi/16;//注意读出来的时bsd码，十六进制，要转换成十进制
		buff[1]=shi%16;
		buff[2]=10;//显示横杠
		buff[3]=fen/16;
		buff[4]=fen%16;
		buff[5]=10;
		buff[6]=(miao/16)&0x07;
		buff[7]=miao%16;	
}
void readkey()//读取键值
{
		uchar ReadData=P3^0xff;
		Trg=ReadData&(ReadData^cont);
		cont=ReadData;
}
void key_function()//按键功能
{
	if(Trg&0x08)//启停
		stop();
	if((Trg&0x01)&&flag==1)//s7￡???
	{
		setmiao=Read_Ds1302_Byte(0x81);//读秒，注意setmiao是bcd码
		Write_Ds1302_Byte(0x8e,0x00);//关写保护
		setmiao=((setmiao>>4)&0x07)*10+(setmiao&0x0f)+1;//变成十进制，加一
		setmiao=setmiao%60;//边界情况
		Write_Ds1302_Byte(0x80,(setmiao/10<<4|setmiao%10)|0x80);//写入，但是要注意|0x80高位置1，不然数据出错，意思是暂停，停止秒
		Write_Ds1302_Byte(0x8e,0x80);//关闭写保护
	}
			if((Trg&0x02)&&flag==1)//同理设置分，但是发送时不要|0x80
			{
				setfen=Read_Ds1302_Byte(0x83);
				Write_Ds1302_Byte(0x8e,0x00);
				setfen=(setfen/16)*10+setfen%16+1;
				setfen=setfen%60;
				Write_Ds1302_Byte(0x82,(setfen/10)*16+setfen%10);
				Write_Ds1302_Byte(0x8e,0x80);
			}
			if((Trg&0x04)&&flag==1)//s5,时
			{
				setshi=Read_Ds1302_Byte(0x85);
				Write_Ds1302_Byte(0x8e,0x00);
				setshi=(setshi/16)*10+setshi%16+1;
				setshi=setshi%24;
				Write_Ds1302_Byte(0x84,(setshi/10)*16+setshi%10);
				Write_Ds1302_Byte(0x8e,0x80);
			}
}
void Timer0Init(void)		//1ms,STC生成
{
	EA=1;ET0=1;//这两个启动一定要加，否则不生效
	AUXR |= 0x80;		//
	TMOD &= 0xF0;		//
	TMOD |= 0x01;		
	TL0 = 0xCD;		//赋初值
	TH0 = 0xD4;		
	TF0 = 0;		//清除标志位
	TR0 = 1;		//启动T0
}

void time0()interrupt 1
{
    TL0 = 0xCD;		//可以不要，会自动重载的
	TH0 = 0xD4;		
	keycount++;
	if(keycount==10)
	{
		keyflag=1;
		keycount=0;
	}//10ms扫描一次键盘
	P2=0xc0;P0=adress[i];P2=0x00;
	P2=0xe0;P0=display_num[buff[i]];P2=0x00;
	i++;
	if(i==8)
		i=0;
}
void set_sfm(uchar shi,uchar fen,uchar miao)//发送时分秒
{
	Write_Ds1302_Byte(0x8e,0);
	Write_Ds1302_Byte(0x80,(miao/10)*16+miao%10);
	Write_Ds1302_Byte(0x82,(fen/10)*16+fen%10);
	Write_Ds1302_Byte(0x84,(shi/10)*16+shi%10);
	Write_Ds1302_Byte(0x8e,0x80);
}
void main()
{
	init();
	Timer0Init();
	set_sfm(23,59,50);//初始化时间
	while(1)
	{	
		readtime();//读取时间
		if(keyflag)
		{
			readkey();
			key_function();	
		}
	}
}

