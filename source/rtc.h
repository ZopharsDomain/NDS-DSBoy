#ifndef _RTC_H_
#define _RTC_H_

#include "types.h"

struct rtc
{
	int batt;
	int sel;
	int latch;
	int d, h, m, s, t;
	int stop, carry;
	u8 *  regs;//[8];
};

extern struct rtc rtc;

void rtc_write(u8 b);
void rtc_latch(u8 b);

#endif //_RTC_H_
