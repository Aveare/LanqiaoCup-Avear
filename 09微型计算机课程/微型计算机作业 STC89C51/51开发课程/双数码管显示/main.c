/*头文件声明*/
#include <REGX51.H>

/*变量声明*/
unsigned char seg_dula[] = {0xc0,0xf9,0xa4,0xb0,0x99,0x92,0x82,0xf8,0x80,0x90,0xff,0xbf};
unsigned char seg_wela[] = {0x01,0x02};
unsigned char count;//递增显示
unsigned char Seg_Pos;//数码管扫描变量
unsigned char Seg_Buf[2];
unsigned char Timer100ms;//100ms计时变量
/*数码管显示函数*/
void Seg_Disp(unsigned char wela,dula)
{
	  P0 = 0xff;  // 消影：段码全灭
    P2 = 0x00;  // 消影：位选全关
    
    P0 = seg_dula[dula]; // 送段码
    P2 = seg_wela[wela]; // 送位选
}
/*数码管处理函数*/
void Seg_Proc()
{
	Seg_Buf[0]=count/10%10;//显示十位
	Seg_Buf[1]=count/1%10;//显示个位
}

/* 定时器0中断初始化函数 */
void Timer0Init(void)		//1毫秒@12.000MHz
{
	TMOD &= 0xF0;		//设置定时器模式
	TMOD |= 0x01;		//设置定时器模式
	TL0 = 0x18;		//设置定时初始值
	TH0 = 0xFC;		//设置定时初始值
	TF0 = 0;		//清除TF0标志
	TR0 = 1;		//定时器0开始计时
	ET0 = 1;    //定时器0中断打开
	EA = 1;     //总中断打开
}

/* 定时器0中断服务函数 */
void Timer0Server() interrupt 1
{
 	TL0 = 0x18;		//设置定时初始值
	TH0 = 0xFC;		//设置定时初始值   
	if(++Seg_Pos == 2) Seg_Pos = 0;//数码管显示专用
	Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos]);//扫描数码管实现动态显示
	
	if(++Timer100ms==100)// count递增 100ms加一次
	{
		Timer100ms=0;
		count++;
	}
}
/*Main*/
void main()
{
	Timer0Init();//初始化
	while(1)
	{
		Seg_Proc();
	}
}