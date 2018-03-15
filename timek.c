#include "timek.h"
#include <rtc.h>
#include <timer.h>
//#include <ctype.h>

long timertime = 0;
//static int cbid;
static timer_t *htimer = NULL;

void timek_init()
{
	//cbid = rtc_cb_add(rtc_freq_256Hz,timer_callback,0);
	uint32_t delay = clock_setting(1024, clock_Hz);
	htimer = htimer_setup(timer_user, delay, timer_Po_4, 0);
	timer_attach(htimer, timer_callback, NULL);
	timer_start(htimer);
}

void timer_callback()
{
	timertime++;
}

void timek_stop()
{
	//rtc_cb_end(cbid);
	timer_stop(htimer);
}