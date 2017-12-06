#ifndef TIMEK_H
#define TIMEK_H

#include <rtc.h>

extern long timertime;

void timek_init();
void timek_stop();
void rtc_callback();

#endif