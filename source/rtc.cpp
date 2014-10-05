#include "types.h"
#include "rtc.h"
#include <stdlib.h>

struct rtc rtc;

void rtc_init () {
	rtc.regs = (u8*)malloc (8);
}

void rtc_write(u8 b)
{
	/* printf("write %02X: %02X (%d)\n", rtc.sel, b, b); */
	if (!(rtc.sel & 8)) return;
	switch (rtc.sel & 7)
	{
	case 0:
		rtc.s = rtc.regs[0] = b;
		while (rtc.s >= 60) rtc.s -= 60;
		break;
	case 1:
		rtc.m = rtc.regs[1] = b;
		while (rtc.m >= 60) rtc.m -= 60;
		break;
	case 2:
		rtc.h = rtc.regs[2] = b;
		while (rtc.h >= 24) rtc.h -= 24;
		break;
	case 3:
		rtc.regs[3] = b;
		rtc.d = (rtc.d & 0x100) | b;
		break;
	case 4:
		rtc.regs[4] = b;
		rtc.d = (rtc.d & 0xff) | ((b&1)<<9);
		rtc.stop = (b>>6)&1;
		rtc.carry = (b>>7)&1;
		break;
	}
}

void rtc_latch(u8 b)
{
	if ((rtc.latch ^ b) & b & 1)
	{
		rtc.regs[0] = rtc.s;
		rtc.regs[1] = rtc.m;
		rtc.regs[2] = rtc.h;
		rtc.regs[3] = rtc.d;
		rtc.regs[4] = (rtc.d>>9) | (rtc.stop<<6) | (rtc.carry<<7);
		rtc.regs[5] = 0xff;
		rtc.regs[6] = 0xff;
		rtc.regs[7] = 0xff;
	}
	rtc.latch = b;
}
