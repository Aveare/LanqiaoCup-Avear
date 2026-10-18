#include <reg52.h>

// 共阴极数码管段码表（0~F，dp为小数点，此处不使用）
// 段码对应 a b c d e f g dp（高位到低位）
unsigned char code SEG_TABLE[] = {
    0x3F, // 0: 00111111
    0x06, // 1: 00000110
    0x5B, // 2: 01011011
    0x4F, // 3: 01001111
    0x66, // 4: 01100110
    0x6D, // 5: 01101101
    0x7D, // 6: 01111101
    0x07, // 7: 00000111
    0x7F, // 8: 01111111
    0x6F, // 9: 01101111
    0x77, // A: 01110111
    0x7C, // B: 01111100
    0x39, // C: 00111001
    0x5E, // D: 01011110
    0x79, // E: 01111001
    0x71  // F: 01110001
};

// 位选信号：P2.2~P2.6分别控制5位数码管（高电平有效）
sbit WE1 = P2^2; // 第1位数码管
sbit WE2 = P2^3; // 第2位数码管
sbit WE3 = P2^4; // 第3位数码管
sbit WE4 = P2^5; // 第4位数码管
sbit WE5 = P2^6; // 第5位数码管

// 延时函数（约1ms，根据11.0592MHz晶振调整）
void delay_ms(unsigned int ms) {
    unsigned int i, j;
    for(i = 0; i < ms; i++)
        for(j = 0; j < 112; j++);
}

// 动态扫描显示函数：依次扫描5位数码管，显示指定数字
// 参数：num数组存储5个要显示的数字（0~15对应0~F）
void display(unsigned char num[5]) {
    // 第1位数码管
    WE1 = 1; WE2 = 0; WE3 = 0; WE4 = 0; WE5 = 0;
    P0 = SEG_TABLE[num[0]]; // 段选信号输出
    delay_ms(2); // 扫描延时（视觉暂留）
    
    // 第2位数码管
    WE1 = 0; WE2 = 1; WE3 = 0; WE4 = 0; WE5 = 0;
    P0 = SEG_TABLE[num[1]];
    delay_ms(2);
    
    // 第3位数码管
    WE1 = 0; WE2 = 0; WE3 = 1; WE4 = 0; WE5 = 0;
    P0 = SEG_TABLE[num[2]];
    delay_ms(2);
    
    // 第4位数码管
    WE1 = 0; WE2 = 0; WE3 = 0; WE4 = 1; WE5 = 0;
    P0 = SEG_TABLE[num[3]];
    delay_ms(2);
    
    // 第5位数码管
    WE1 = 0; WE2 = 0; WE3 = 0; WE4 = 0; WE5 = 1;
    P0 = SEG_TABLE[num[4]];
    delay_ms(2);
}

void main() {
    unsigned char show_num[5] = {1, 4, 7, 2, 9}; // 要显示的数字（示例：1 4 7 2 9）
    
    while(1) {
        display(show_num); // 循环扫描显示
    }
}