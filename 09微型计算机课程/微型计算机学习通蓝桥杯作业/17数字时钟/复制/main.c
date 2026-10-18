/*头文件声明区 */
#include <STC15F2K60S2.H>
#include <Init.h>
#include <Key.h>
#include <Seg.h>
#include <ds1302.h>

/* 变量声明区 */
unsigned char Key_Val, Key_Down, Key_Old, Key_Up; //按键专用变量
unsigned char Key_Slow_Down;                      //按键减速专用变量
unsigned char Seg_Buf[8] = {10,10,10,10,10,10,10,10}; //数码管显示数据存放数组
unsigned char Seg_Point[8] = {0,0,0,0,0,0,0,0};   //数码管小数点数据存放数组
unsigned char Seg_Pos;                            //数码管扫描专用变量
unsigned int Seg_Slow_Down;                       //数码管减速专用变量
bit flag = 0;                                     //调时使能标志位（1=可调时，0=不可调时）
unsigned char shi, fen, miao = 0;                 //时分秒缓存变量
static unsigned char setfen, setmiao, setshi;     //调时临时缓存变量

/*启停函数*/
void Stop()
{
    Write_Ds1302_Byte(0x8e,0);//关写保护
    if(miao<0x80)
    {
        Write_Ds1302_Byte(0x80,miao|0x80);//高位置1，暂停计时
        flag=1;//标志位置1，允许操作
    }
    else
    {
        Write_Ds1302_Byte(0x80,miao&0x7f);//高位置0，启动计时
        flag=0; //标志位置0，不允许操作
    }
    Write_Ds1302_Byte(0x8e,0x80);//打开写保护
}

/* 键盘处理函数 */
void Key_Proc()
{
    if(Key_Slow_Down) return;
    Key_Slow_Down = 1; //键盘减速程序

    Key_Val=Key_Read(); //实时读取键码值
    Key_Down=Key_Val&(Key_Old^Key_Val); //捕捉按键下降沿
    Key_Old=Key_Val;

    switch(Key_Down)
    {
        case 4: //启停
            Stop();
            break;
        case 5: //时+1
            if(flag==1)
            {
                setshi=Read_Ds1302_Byte(0x85);
                Write_Ds1302_Byte(0x8e,0x00);//关写保护
                setshi=(setshi/16)*16+setshi%16+1;//BCD转十进制+1
                setshi=setshi%24;//0-23
                Write_Ds1302_Byte(0x84,(setshi/10)*16+setshi%10);//转BCD
                Write_Ds1302_Byte(0x8e,0x80);//开写保护
            }
            break;
        case 6: //分+1
            if(flag==1)
            {
                setfen=Read_Ds1302_Byte(0x83);
                Write_Ds1302_Byte(0x8e,0x00); //关写保护
                setfen=(setfen/16)*10+setfen%16+1;//BCD转十进制+1
                setfen=setfen%60;//0-59
                Write_Ds1302_Byte(0x82,(setfen/10)*16+setfen%10);//十转BCD
                Write_Ds1302_Byte(0x8e,0x80);//开写保护
            }
            break;
        case 7: //秒+1
            if(flag==1)
            {
                setmiao =Read_Ds1302_Byte(0x81); //读秒
                Write_Ds1302_Byte(0x8e,0x00); //关写保护
                setmiao=((setmiao >> 4)&0x07)*10+(setmiao&0x0f)+1;//转BCD
                setmiao=setmiao%60; //0-59
                Write_Ds1302_Byte(0x80,((setmiao/10)<<4|setmiao%10)|0x80);
                Write_Ds1302_Byte(0x8e,0x80); //关写保护
            }
            break;
    }
}

void readtime() 
{
    EA = 0; //关中断
    shi = Read_Ds1302_Byte(0x85); //读时
    fen = Read_Ds1302_Byte(0x83); //读分
    miao = Read_Ds1302_Byte(0x81); //读秒
    EA = 1; //开中断
    Seg_Buf[0]=shi/16;              
    Seg_Buf[1]=shi%16; 
    Seg_Buf[2]=10;//-
    Seg_Buf[3]=fen/16;               
    Seg_Buf[4]=fen%16;               
    Seg_Buf[5]=10;//-
    Seg_Buf[6]=(miao/16)&0x07;      
    Seg_Buf[7]=miao%16;               
}

void Timer0Init(void) //定时器0初始化（1ms@12MHz，稳定扫描）
{
    AUXR |= 0x80;        // 定时器时钟1T模式
    TMOD &= 0xF0;        // 清除定时器0模式
    TMOD |= 0x01;        // 定时器0工作在模式1（16位定时器）
    TL0 = 0xCD;          // 设置定时初始值
    TH0 = 0xD4;          // 设置定时初始值
    TF0 = 0;             // 清除TF0标志
    TR0 = 1;             // 启动定时器0
    EA = 1;              // 开启总中断
    ET0 = 1;             // 开启定时器0中断
}

/* 定时器0中断服务函数 */
void Timer0Server() interrupt 1
{  
    TL0 = 0xCD;
    TH0 = 0xD4;
    if(++Key_Slow_Down == 10) Key_Slow_Down = 0;
    if(++Seg_Slow_Down == 500) Seg_Slow_Down = 0;
    if(++Seg_Pos == 8) Seg_Pos = 0;
    Seg_Disp(Seg_Pos, Seg_Buf[Seg_Pos], Seg_Point[Seg_Pos]);
}

void Time_Set(unsigned char shi, unsigned char fen, unsigned char miao)
{
    Write_Ds1302_Byte(0x8e, 0); //关写保护
    Write_Ds1302_Byte(0x80, (miao / 10) * 16 + miao % 10);
    Write_Ds1302_Byte(0x82, (fen / 10) * 16 + fen % 10);
    Write_Ds1302_Byte(0x84, (shi / 10) * 16 + shi % 10);
    Write_Ds1302_Byte(0x8e, 0x80); //开写保护
}

void main()
{
    System_Init();
    Timer0Init();  
    Time_Set(23, 59, 50); //初始化时间
    while(1)
    {
        readtime();
        Key_Proc();
    }
}