#include <STC15F2K60S2.h>
#include <intrins.h>
#include <stdio.h>

// 数码管段码表(共阴)：0-9、熄灭、-、C
unsigned char code seg_table[] = {0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F,0x00,0x40,0x39};
// 全局变量
unsigned char seg_Buf[8] = {0};    // 数码管显示缓冲区（存储数值）
unsigned seg_Dot[8] = {0};              // 小数点标记数组（0-无 1-有）
unsigned char seg_Pos = 0;         // 数码管扫描位置
unsigned char key_Scan_Slow = 0;   // 按键扫描节流变量
unsigned char seg_Scan_Slow = 0;   // 数码管刷新节流变量
unsigned char display_mode = 0;    // 显示模式：0-时间 1-温度
unsigned char current_time[3] = {12,59,55}; // 时分秒初始值
float temp_value = 25.6;           // 温度值
unsigned char key_Val, key_Old, key_Down, key_Up; // 按键变量
unsigned int relay_timer = 0;      // 继电器计时
bit relay_status = 0;              // 继电器状态：0-断开 1-吸合
unsigned char led_Buf[8] = {0};    // LED缓冲区

// 系统初始化：关闭所有外设、清屏、LED灭
void System_Init(void)
{
    P0 = 0x00; P2 = (P2 & 0x1F) | 0x80; P2 &= 0x1F; // 关闭LED
    P0 = 0xFF; P2 = (P2 & 0x1F) | 0xA0; P2 &= 0x1F; // 关闭继电器/蜂鸣器
    P0 = 0x00; P2 = (P2 & 0x1F) | 0xC0; P2 &= 0x1F; // 数码管位选清0
    P0 = 0x00; P2 = (P2 & 0x1F) | 0xE0; P2 &= 0x1F; // 数码管段选清0
}

// 数码管显示函数：pos-位置(0-7)，num-段码索引，dot-小数点(0-无 1-有)
void Seg_Display(unsigned char pos, unsigned char num, unsigned char dot)
{
    P0 = 0x00;
    P2 = (P2 & 0x1F) | 0xC0; P2 &= 0x1F; // 选数码管位
    // 修复：将1强制转为unsigned char，避免类型不匹配
    P0 = (unsigned char)(1 << pos);
    P2 = (P2 & 0x1F) | 0xE0; P2 &= 0x1F; // 送段码
    // 修复：dot改用unsigned char，避免bit类型的<<运算问题
    P0 = seg_table[num] | ((unsigned char)(dot << 7));
}

// LED显示函数：buf-LED数据，len-长度
void Led_Display(unsigned char *buf, unsigned char len)
{
    unsigned char i, temp = 0;
    for(i=0; i<len; i++)
    {
        if(buf[i]) temp |= (1 << i);
    }
    P0 = ~temp;
    P2 = (P2 & 0x1F) | 0x80; P2 &= 0x1F;
}

// 继电器控制函数：status-0断开 1吸合
void Relay(bit status)
{
    P0 = status ? 0x10 : 0x00;
    P2 = (P2 & 0x1F) | 0xA0; P2 &= 0x1F;
}

// 按键读取函数：返回S4-S7键值(4-7)，无按键返回0
unsigned char Key_Read(void)
{
    unsigned char key = 0;
    P3 = 0x0F;
    if(P3 != 0x0F)
    {
        _nop_();_nop_();_nop_();_nop_(); // 消抖
        if(P3 == 0x0E) key = 4; // S4
        else if(P3 == 0x0D) key = 5; // S5
        else if(P3 == 0x0B) key = 6; // S6
        else if(P3 == 0x07) key = 7; // S7
    }
    return key;
}

// DS18B20读取温度（简化版，竞赛通用）
float Read_Temperature(void)
{
    // 此处替换为DS18B20标准读温度代码，返回浮点型温度值
    return temp_value; // 测试用，实际需替换
}

// RTC读取时间：buf-存储时分秒
void Read_Rtc(unsigned char *buf)
{
    // 此处替换为RTC芯片读时间代码（如DS1302）
    buf[0] = current_time[0];
    buf[1] = current_time[1];
    buf[2] = current_time[2];
}

// RTC设置时间：buf-时分秒数据
void Set_Rtc(unsigned char *buf)
{
    // 此处替换为RTC芯片写时间代码（如DS1302）
    current_time[0] = buf[0];
    current_time[1] = buf[1];
    current_time[2] = buf[2];
}

// 数码管数据准备与刷新
void Seg_Proc()
{
	unsigned char i;
    if(seg_Scan_Slow) return;
    seg_Scan_Slow = 1;

    /* 信息读取 */
    Read_Rtc(current_time);     // 读取RTC时钟
    temp_value = Read_Temperature();  // 读取温度

    // 清空小数点标记
    for(i=0; i<8; i++) seg_Dot[i] = 0;

    /* 显示模式处理 */
    switch((unsigned char)display_mode)
    {
        case 0: // 时间界面：HH-MM-SS
            seg_Buf[0] = current_time[0] / 10 % 10;  // 时十位
            seg_Buf[1] = current_time[0] % 10;       // 时个位
            seg_Buf[2] = 11; // 分隔符-
            seg_Buf[3] = current_time[1] / 10 % 10;  // 分十位
            seg_Buf[4] = current_time[1] % 10;       // 分个位
            seg_Buf[5] = 11; // 分隔符-
            seg_Buf[6] = current_time[2] / 10 % 10;  // 秒十位
            seg_Buf[7] = current_time[2] % 10;       // 秒个位
            break;

        case 1: // 温度界面：C XX.X
            seg_Buf[0] = 12;  // 显示C
            seg_Buf[1] = 10; // 熄灭
            seg_Buf[2] = 10; // 熄灭
            seg_Buf[3] = 10; // 熄灭
            seg_Buf[4] = 10; // 熄灭
            seg_Buf[5] = (unsigned char)temp_value / 10 % 10;        // 温度十位
            seg_Buf[6] = (unsigned char)temp_value % 10;             // 温度个位
            seg_Buf[7] = (unsigned int)(temp_value * 10) % 10;       // 温度小数位
            seg_Dot[6] = 1; // 标记第6位（个位）显示小数点
            break;
    }
}

// 按键核心处理：边沿检测+功能执行
void Key_Proc()
{
    if(key_Scan_Slow) return;
    key_Scan_Slow = 1;

    key_Val = Key_Read();
    key_Down = key_Val & (key_Val ^ key_Old); // 按下沿检测
    key_Up = ~key_Val & (key_Val ^ key_Old);   // 松开沿检测
    key_Old = key_Val;

    switch(key_Down)
    {
        case 4: // S4：界面切换（时间?温度）
            display_mode ^= 1;
            break;
        case 5: // S5：时+1，边界保护23→23
            if(!display_mode && ++current_time[0] == 24)
                current_time[0] = 23;
            Set_Rtc(current_time);
            break;
        case 6: // S6：分+1，边界保护59→59
            if(!display_mode && ++current_time[1] == 60)
                current_time[1] = 59;
            Set_Rtc(current_time);
            break;
        case 7: // S7：秒清零，仅时间界面有效
            if(!display_mode)
                current_time[2] = 0;
            Set_Rtc(current_time);
            break;
    }
}

// LED指示+整点报时+继电器控制
void Led_Proc()
{
    // 整点报时触发：分=0且秒=0时吸合继电器
    if(current_time[1] == 0 && current_time[2] == 0)
    {
        relay_timer = 0;
        relay_status = 1;
    }

    // LED界面指示：L0-时间界面  L1-温度界面
    led_Buf[0] = !display_mode;
    led_Buf[1] = display_mode;

    // 继电器硬件控制
    Relay(relay_status);
}

// 定时器1初始化：1ms@12MHz（12T模式）
void Timer1_Init(void)
{
    AUXR &= 0xBF;        // 定时器时钟12T模式
    TMOD &= 0x0F;        // 设置定时器模式
    TL1 = 0x18;          // 定时初始值
    TH1 = 0xFC;          // 定时初始值
    TF1 = 0;             // 清除TF1标志
    TR1 = 1;             // 定时器1开始计时
    EA = 1;              // 开总中断
    ET1 = 1;             // 开定时器1中断
}

// 定时器1中断服务函数：多任务调度+数码管扫描+LED刷新
void Timer1_Server() interrupt 3
{
    // 按键扫描节流：10ms一次
    if(++key_Scan_Slow == 10) key_Scan_Slow = 0;
    // 数码管刷新节流：100ms一次
    if(++seg_Scan_Slow == 100) seg_Scan_Slow = 0;

    // 继电器3秒定时：吸合后3000ms自动断开
    if(relay_status == 1 && ++relay_timer == 3000)
    {
        relay_status = 0;
    }

    // 数码管动态扫描：1ms切换一位，8ms一轮
    if(++seg_Pos == 8) seg_Pos = 0;
    // 直接使用小数点标记数组，无需类型转换
    Seg_Display(seg_Pos, seg_Buf[seg_Pos], seg_Dot[seg_Pos]);

    // LED实时刷新
    Led_Display(led_Buf, 8);
}

void main(void)
{
    /* 系统初始化 */
    System_Init();
    while(Read_Temperature() == 85);  // 等待DS18B20稳定（避免初始85℃）
    Set_Rtc(current_time);            // 初始化RTC时间
    Timer1_Init();                    // 初始化定时器1

    /* 主循环：轮询处理各模块 */
    while (1)
    {
        Key_Proc();  // 按键处理
        Seg_Proc();  // 数码管处理
        Led_Proc();  // LED及继电器处理
    }
}