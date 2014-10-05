#include "hw.h"
#include "cpu.h"
#include "types.h"
#include "regs.h"
#include "lcd.h"
#include "../asm/lib.h"
//#include <nds/arm9/console.h> //basic print funcionality

#define GB_START	0x80
#define GB_SELECT	0x40
#define GB_B		0x20
#define GB_A		0x10
#define GB_UP		0x04
#define GB_DOWN		0x08
#define GB_LEFT		0x02
#define GB_RIGHT	0x01

#define KEY_A          (1 << 0)
#define KEY_B          (1 << 1)
#define KEY_SELECT     (1 << 2)
#define KEY_START      (1 << 3)
#define KEY_RIGHT      (1 << 4)
#define KEY_LEFT       (1 << 5)
#define KEY_UP         (1 << 6)
#define KEY_DOWN       (1 << 7)
#define KEY_R          (1 << 8)
#define KEY_L          (1 << 9)

#define KEYS           (*(vu16*)0x04000130)
#define READ_KEYS      ((~(KEYS)) & 0x3FF)
#define halt (cpu_a.halt)

#define IME (cpu_a.ime)
inline void poll_events();


struct hw hw;

void hw_interrupt(u8 i, u8 mask)
{
	u8 oldif = R_IF;
	i &= 0x1F & mask;
	R_IF |= i & (hw.ilines ^ i);

	if ((R_IF & (R_IF ^ oldif) & R_IE) && IME) halt = 0;

	hw.ilines &= ~mask;
	hw.ilines |= i;
}

void hw_hdma_cmd(u8 c)
{
	/* Begin or cancel HDMA */
	if ((hw.hdma|c) & 0x80)
	{
		hw.hdma = c;
		R_HDMA5 = c & 0x7f;
		return;
	}
	int cnt  = (c+1)<<4;
	u16 sa = (R_HDMA1 << 8) | (R_HDMA2&0xf0);
	u16 da = 0x8000 | ((R_HDMA3&0x1f) << 8) | (R_HDMA4&0xf0);
	/* Perform GDMA */
	u8 *r = mbc.rmap[sa>>12];
	u8 *w = mbc.wmap[da>>12]; 
	
	if (r) {
		if (w) {
			memcpyvd (w+da, r+sa, cnt);
		}
		else {
			while (cnt--)
				writeb(da++, r[sa++]);
		}
	}
	else {
		if (w) {
			int cnt = 16;
			while (cnt--)
				w[da++] = readb(sa++);
		}
		else {	
			int cnt = 16;
			while (cnt--)
				writeb(da++, readb(sa++));
		}
	}
	R_HDMA1 = sa >> 8;
	R_HDMA2 = sa & 0xF0;
	R_HDMA3 = 0x1F & (da >> 8);
	R_HDMA4 = da & 0xF0;
	R_HDMA5 = 0xFF;
}

/*
 * hw_dma performs plain old memory-to-oam dma, the original dmg
 * dma. Although on the hardware it takes a good deal of time, the cpu
 * continues running during this mode of dma, so no special tricks to
 * stall the cpu are necessary.
 */

void hw_dma(u8 b)
{
	u8 *p = mbc.rmap[b>>4];
	if (p) 
		memcpy160d (lcd.oam.mem, p+(b<<8));
	else {
		u8 i;
		u16 a;
		a = b << 8;
		for (i = 0; i < 160; i++, a++)
			lcd.oam.mem[i] = readb(a);
			
	}
}

/*
 * pad_refresh updates the P1 register from the pad states, generating
 * the appropriate interrupts (by quickly raising and lowering the
 * interrupt line) if a transition has been made.
 */

void pad_refresh()
{
	u8 oldp1 = R_P1;
	R_P1 &= 0x30;
	R_P1 |= 0xc0;
	poll_events();
	if (!(R_P1 & 0x10))
		R_P1 |= (hw.pad & 0x0F);
	if (!(R_P1 & 0x20))
		R_P1 |= (hw.pad >> 4);
	R_P1 ^= 0x0F;
	if (oldp1 & ~R_P1 & 0x0F)
	{
		hw_interrupt(IF_PAD, IF_PAD);
		hw_interrupt(0, IF_PAD);
	}
}

void hw_hdma()
{
	u16 sa = (R_HDMA1 << 8) | (R_HDMA2&0xf0);
	u16 da = 0x8000 | ((R_HDMA3&0x1f) << 8) | (R_HDMA4&0xf0);
	
	u8 * r = mbc.rmap[sa>>12];
	u8 * w = mbc.wmap[da>>12];
	
	if (r) {
		if (w) {
			memcpy16d (w+da, r+sa);
		}
		else {
			int cnt = 16;
			while (cnt--)
				writeb(da++, r[sa++]);
		}
	}
	else {
		if (w) {
			int cnt = 16;
			while (cnt--)
				w[da++] = readb(sa++);
		}
		else {	
			int cnt = 16;
			while (cnt--)
				writeb(da++, readb(sa++));
		}
	}
	R_HDMA1 = sa >> 8;
	R_HDMA2 = sa & 0xF0;
	R_HDMA3 = 0x1F & (da >> 8);
	R_HDMA4 = da & 0xF0;
	R_HDMA5--;
	hw.hdma--;
}

void hw_reset()
{
	hw.ilines = hw.pad = 0;

	memsetv(cpu_a.hi, 0, sizeof cpu_a.hi);

	R_P1 = 0xFF;
	R_LCDC = 0x91;
	R_BGP = 0xFC;
	R_OBP0 = 0xFF;
	R_OBP1 = 0xFF;
	R_SVBK = 0x01;
	R_HDMA5 = 0xFF;
	R_VBK = 0xFE;
	VBK1 = 0;
}

inline void poll_events()
{	
	u16 keys = READ_KEYS;
	hw.pad = 0;
	
	if ((keys & KEY_START) == KEY_START)
		hw.pad |= GB_START;
	if ((keys & KEY_SELECT) == KEY_SELECT)
		hw.pad |= GB_SELECT;
	if ((keys & KEY_B) == KEY_B)
		hw.pad |= GB_B;
	if ((keys & KEY_A) == KEY_A)
		hw.pad |= GB_A;
	if ((keys & KEY_LEFT) == KEY_LEFT)
		hw.pad |= GB_LEFT;
	if ((keys & KEY_RIGHT) == KEY_RIGHT)
		hw.pad |= GB_RIGHT;
	if ((keys & KEY_UP) == KEY_UP)
		hw.pad |= GB_UP;
	if ((keys & KEY_DOWN) == KEY_DOWN)
		hw.pad |= GB_DOWN;
}
