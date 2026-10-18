#ifndef __TIMER_H
#define __TIMER_H

#include "sys.h"
#include "led.h"
#include "smg.h"
#include "key.h"
void timer0_init();
void timer1_init();
extern uchar temp[];
extern	uchar dot[];
extern	uchar LED[];
extern	uchar pos;
extern	uint smg_dely;
extern	uint dat_dely;
extern	uchar key_dely;
extern uint fre;
extern uint sec_trig;
extern bit trig;
extern bit state4;
#endif