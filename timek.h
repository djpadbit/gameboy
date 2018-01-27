#ifndef TIMEK_H
#define TIMEK_H

extern long timertime;

void timek_init();
void timek_stop();
void timer_callback();

#endif