#ifndef _HW_H_
#define _HW_H_

#include "types.h"

#define IF_VBLANK 0x01
#define IF_STAT   0x02
#define IF_TIMER  0x04
#define IF_SERIAL 0x08
#define IF_PAD    0x10

struct hw
{
	u8 ilines;
	u8 pad;
	u8 cgb, gba;
	int hdma;
};


extern struct hw hw;

void pad_refresh();

void hw_interrupt(u8 i, u8 mask);
void hw_hdma_cmd(u8 c);
void hw_dma(u8 b);
void hw_hdma();
void hw_reset();

#endif //_HW_H_
