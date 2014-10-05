#include "types.h"
#include "hw.h"
#include "regs.h"
#include "cpu.h"
#include "lcd.h"

#define C (cpu_a.lcdc)
#define halt (cpu_a.halt)
inline void stat_change(u8 stat);

/*
 * stat_trigger updates the STAT interrupt line to reflect whether any
 * of the conditions set to be tested (by bits 3-6 of R_STAT) are met.
 * This function should be called whenever any of the following occur:
 * 1) LY or LYC changes.
 * 2) A state transition affects the low 2 bits of R_STAT (see below).
 * 3) The program writes to the upper bits of R_STAT.
 * stat_trigger also updates bit 2 of R_STAT to reflect whether LY=LYC.
 */

void stat_trigger()
{
	const u8 condbits[4] = { 0x08, 0x30, 0x20, 0x00 };
	u8 flag = 0;

	if ((R_LY < 0x91) && (R_LY == R_LYC))
	{
		R_STAT |= 0x04;
		if (R_STAT & 0x40) flag = IF_STAT;
	}
	else R_STAT &= ~0x04;

	if (R_STAT & condbits[R_STAT&3]) flag = IF_STAT;

	if (!(R_LCDC & 0x80)) flag = 0;
	
	hw_interrupt(flag, IF_STAT);
}

void stat_write(u8 b)
{
	R_STAT = (R_STAT & 0x07) | (b & 0x78);
	if (!hw.cgb && !(R_STAT & 2)) /* DMG STAT write bug => interrupt */
		hw_interrupt(IF_STAT, IF_STAT);
	stat_trigger();
}




void lcdc_trans()
{	
	if (!(R_LCDC & 0x80))
	{
		switch ((R_STAT & 3))
		{
		case 0:
		case 1:
			stat_change(2);
			C += 40;
			break;
		case 2:
			stat_change(3);
			C += 86;
			break;
		case 3:
			stat_change(0);
			if (hw.hdma & 0x80)
				hw_hdma();
			else
				C += 102;
			break;
		}
		return;
	}
	
	while (C <= 0)
	{
		switch ((R_STAT & 3))
		{
		case 1:			
			if (!(hw.ilines & IF_VBLANK))
			{
				C += 218;
				hw_interrupt(IF_VBLANK, IF_VBLANK);
				break;
			}
			if (R_LY == 0)
			{
				lcd_begin();
				stat_change(2);
				C += 40;
				break;
			}
			else if (R_LY < 152)
				C += 228;
			else if (R_LY == 152)
				C += 28;
			else
			{
				R_LY = (u8)-1;
				C += 200;
			}
			R_LY++;
			stat_trigger();
			break;
		case 2:
			lcd_refreshline();
			stat_change(3);
			C += 86;
			
			break;
		case 3:
			stat_change(0);
			if (hw.hdma & 0x80)
				hw_hdma();
			C += 102;
			break;
		case 0:
			
			if (++R_LY >= 144)
			{
				if (halt)
				{
					hw_interrupt(IF_VBLANK, IF_VBLANK);
					C += 228;
				}
				else C += 10;
				stat_change(1);
				break;
			}
			stat_change(2);
			C += 40;
			break;
		}
	}
	
}

/*
 * stat_change is called when a transition results in a change to the
 * LCD STAT condition (the low 2 bits of R_STAT).  It raises or lowers
 * the VBLANK interrupt line appropriately and calls stat_trigger to
 * update the STAT interrupt line.
 */

inline void stat_change(u8 stat)
{
	R_STAT = (R_STAT & 0x7C) | stat;

	if (stat != 1) hw_interrupt(0, IF_VBLANK);
	stat_trigger();
}

void lcdc_change(u8 b)
{
	u8 old = R_LCDC;
	R_LCDC = b;
	if ((R_LCDC ^ old) & 0x80) /* lcd on/off change */
	{
		R_LY = 0;
		stat_change(2);
		C = 40;
		lcd_begin();
	}
}
