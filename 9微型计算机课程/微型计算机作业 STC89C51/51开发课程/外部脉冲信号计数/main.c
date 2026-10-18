/*头文件声明*/
#include <REGX51.H>
/*变量声明*/
unsigned char code seg_table[] = {0xc0,0xf9,0xa4,0xb0,0x99,0x92,0x82,0xf8,0x80,0x90,0xff};
unsigned int count = 0;  // 改为16位变量，支持0~65535计数

/*延迟函数*/
void Delay_ms(unsigned int t) 
{
    unsigned int i,j;
    for(i=t; i>0; i--)
        for(j=120; j>0; j--);
}

/*计数器0初始化*/
void T0_Init() 
{
    TMOD &= 0xF0;  // 清空T0模式位
    TMOD |= 0x05;  // T0=计数器模式+方式1（16位）
    TH0 = 0x00;    // 初始计数值0
    TL0 = 0x00;
    TR0 = 1;       // 启动T0计数
    ET0 = 1;       // 使能T0溢出中断
    EA = 1;        // 开总中断
}

/*数码管显示函数*/
void Seg_Proc(unsigned int num) 
{
    unsigned char ten = (num / 10) % 10;  // 十位（取商的个位）
    unsigned char unit = num % 10;        // 个位 
  
    P0 = seg_table[ten];//十位
    P2_0 = 1;  
    Delay_ms(1);
    P2_0 = 0;  
    
    P0 = seg_table[unit];//个位
    P2_1 = 1;  
    Delay_ms(1);
    P2_1 = 0;  
}

/*计时器0中断服务函数*/
void T0_ISR() interrupt 1 
{
    TH0 = 0x00; 
    TL0 = 0x00;//重置
}

void main() 
{
    T0_Init();
    while(1) 
		{
        count = (TH0 << 8) | TL0;  //高八位左移，或上低八位实现对count的赋值
        Seg_Proc(count);           // 动态扫描显示
    }
}