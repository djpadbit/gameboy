#include "timek.h"

long timertime = 0;
static int cbid;

void timek_init()
{
	cbid = rtc_cb_add(rtc_freq_256Hz,rtc_callback,0);
}

void rtc_callback()
{
	timertime++;
}

void timek_stop()
{
	rtc_cb_end(cbid);
}