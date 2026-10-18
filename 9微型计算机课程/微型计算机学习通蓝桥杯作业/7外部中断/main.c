/* 头文件声明区 */
#include <STC15F2K60S2.H>
#include <Init.h>
#include <Seg.h>

/* 变量声明区 */
unsigned char Seg_Buf[8] = {10,10,10,10,10,10,10,10}; // 数码管显示数据
unsigned char Seg_Point[8] = {0,0,0,0,0,0,0,0};       // 小数点数据
unsigned char Seg_Pos = 0;                            // 当前扫描位
unsigned char count = 0;                              // 显示值
unsigned char key0_state = 0;  // 0:空闲 1:防抖中 2:等待释放
unsigned char key1_state = 0;
unsigned int key0_timer = 0;   // 按键计时器
unsigned int key1_timer = 0;

sbit INT0_Flag = P3^2;
sbit INT1_Flag = P3^3;

/* INT0初始化函数 */
void INT0_Init(void)
{
	IT0 = 1;    // 设置INT0为下降沿触发
	EX0 = 1;    // 使能INT0外部中断
}

/* INT1初始化函数 */
void INT1_Init(void)
{
	IT1 = 1;    // 设置INT1为下降沿触发
	EX1 = 1;    // 使能INT1外部中断
}

/* 按键处理状态机*/
void Key_Process()
{
    // 处理INT0
    switch(key0_state)
    {
        case 0: // 空闲状态
            if(INT0_Flag == 0)
            {
                key0_state = 1;       // 进入防抖状态
                key0_timer = 0;       // 重置计时器
            }
            break;

        case 1: // 防抖中 
            if(INT0_Flag == 0)
            {
                if(++key0_timer >= 200) // 200ms防抖时间
                {
                    if(count < 9) count++; // 执行按键操作
                    key0_state = 2;       // 进入等待释放状态
                }
            }
            else
            {
                key0_state = 0; // 抖动，返回空闲
            }
            break;

        case 2: // 等待释放
            if(INT0_Flag)
            {
                key0_state = 0; // 释放后返回空闲
            }
            break;
    }

    // 处理INT1
    switch(key1_state)
    {
        case 0:
            if(INT1_Flag == 0)
            {
                key1_state = 1;
                key1_timer = 0;
            }
            break;

        case 1:
            if(INT1_Flag == 0)
            {
                if(++key1_timer >= 200)
                {
                    if(count > 0) count--;
                    key1_state = 2;
                }
            }
            else
            {
                key1_state = 0;
            }
            break;

        case 2:
            if(INT1_Flag)
            {
                key1_state = 0;
            }
            break;
    }
}

/* 数码管扫描函数*/
void Seg_Scan()
{
    static unsigned int scan_timer = 0;

    if(++scan_timer >= 1)
    {
        scan_timer = 0;
        Seg_Disp(Seg_Pos, Seg_Buf[Seg_Pos], Seg_Point[Seg_Pos]);
        if(++Seg_Pos >= 8) Seg_Pos = 0;
    }
}

/* 信息处理函数 */
void Seg_Proc()
{
    Seg_Buf[7] = count;
}

/*Main*/
void main()
{
    System_Init();
    INT0_Init();
    INT1_Init();

    while (1)
    {
        Key_Process();
        Seg_Proc();
        Seg_Scan();
    }
}
