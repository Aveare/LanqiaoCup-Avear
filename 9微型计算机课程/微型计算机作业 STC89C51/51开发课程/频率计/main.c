/*头文件声明*/
#include <reg51.h>
/*变量声明*/
unsigned char code Seg_dula[] = {0xfc,0x60,0xda,0xf2,0x66,0xb6,0xbe,0xe0,0xfe,0xf6,0xee,0x3e,0x9c,0x7a,0x9e,0x8e};
unsigned char code Seg_wela[] = {0xfe,0xfd,0xfb,0xf7,0xef,0xdf,0xbf,0x7f};
unsigned long count = 0;             // 计数器0脉冲数
unsigned long Disp = 0;              // 待显示的频率数值
unsigned char Timer50ms = 0;         // 50ms计数变量
bit Flag = 0;                        // 1s完成标志
unsigned char Seg_Buf[5] = {0,0,0,0,0};
unsigned char Timer250us = 0;        // 250us计数变量
unsigned char Seg_Pos = 0;           

/*数值拆分函数*/
void Seg_Proc(unsigned int temp_val)
{
    Seg_Buf[0] = temp_val % 10;        // 个
    Seg_Buf[1] = (temp_val / 10) % 10; // 十
    Seg_Buf[2] = (temp_val / 100) % 10;// 百
    Seg_Buf[3] = (temp_val / 1000) % 10;// 千
    Seg_Buf[4] = (temp_val / 10000) % 10;// 万
}

/*数码管显示函数（带消隐）*/
void Seg_Disp(void)
{
    P2 = 0xff;
    P1 = 0x00;
    P2 = Seg_wela[Seg_Pos];
    P1 = Seg_dula[Seg_Buf[Seg_Pos]];           
    if(++Seg_Pos >= 5)Seg_Pos = 0;
}

/*定时器/计数器初始化函数*/
void Init(void)
{
    TMOD=0x26;//让定时器处于工作方式2，可以自动重装处置，不占用系统时间
    TH0=0xff;
    TL0=0xff;
    TH1=TL1=256-250;  
    ET0=1;
    ET1=1;
    TR0=1;
    TR1=1;
    EA=1;
}

/*T0中断服务函数*/
void timer0_isr(void) interrupt 1
{
    count++;  // 脉冲计数
}

/*T1中断服务函数*/
void timer1_isr(void) interrupt 3
{
    if(++Timer250us >= 200)  // 200*250us = 50ms
    {
        Timer250us = 0;
        Timer50ms++;
        if(Timer50ms == 20) // 20*50ms=1s
        {
            Flag = 1;          // 标志置1   不使用中断执行操作，减少系统误差
            Disp = count;      // 保存脉冲数
            count = 0;         // 重置计数
            Timer50ms = 0;    
        }
    }
}

/*主函数*/
void main(void)
{
    Init();
    while(1)
    {
        if(Flag)
        {
            Seg_Proc(Disp);
            Flag = 0;
        }
     Seg_Disp();//循环刷新数码管
    }
}
