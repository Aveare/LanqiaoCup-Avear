#ifndef __TIMER_H__
#define __TIMER_H__
void Timer0_Init(void);

extern unsigned char led_ms;
extern unsigned char key_ms;
extern unsigned int smg_ms;
extern unsigned char LED[];
extern unsigned char SMG[];
extern unsigned char dot[];
extern unsigned char start_plus;//启动累加标志位
extern unsigned char plus_flag;//累加
extern unsigned char time_para_disp;
extern unsigned char time_para_cltr;
extern unsigned char led_flash;//LED闪烁标志位
#endif