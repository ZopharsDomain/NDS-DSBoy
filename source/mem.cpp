#include "types.h"
#include "mem.h"
#include "regs.h"
#include "hw.h"
#include "rtc.h"
#include "lcd.h"
#include "lcdc.h"
#include <nds/arm9/console.h> //basic print funcionality
#include <stdlib.h>

struct mbc mbc;
struct ram ram;
const u8 * rom;


inline void mem_updateram ();
inline void mem_updaterom ();
inline u8 mem_read(u16 address);
inline void mem_write(u16 address, u8 value);
inline u8 ioreg_read(u8 r);
inline void ioreg_write(u8 r, u8 b);
inline void mbc_write(u16 a, u8 b);

s8 readsb(u16 address)
{
	u8 *p = mbc.rmap[address>>12];
	if (p) return (s8)p[address];
	else return (s8)mem_read(address);
}

u8 readb(u16 address)
{
	u8 *p = mbc.rmap[address>>12];
	if (p) return p[address];
	else return mem_read(address);
}

void writeb(u16 address, u8 value)
{
	u8 *p = mbc.wmap[address>>12];
	if (p) p[address] = value;
	else mem_write(address, value);
}

u16 readw(u16 address)
{
	if ((address+1) & 0xfff)
	{
		u8 *p = mbc.rmap[address>>12];
		if (p)
		{
			if (address&1) return p[address] | (p[address+1]<<8);
			return *(u16 *)(p+address);
		}
	}
	return mem_read(address) | (mem_read(address+1)<<8);
}

void writew(u16 address, u16 value)
{
	if ((address+1) & 0xfff)
	{
		u8 *p = mbc.wmap[address>>12];
		if (p)
		{
			if (address&1)
			{
				p[address] = value;
				p[address+1] = value >> 8;
				return;
			}
			*(u16 *)(p+address) = value;
			return;
		}
	}
	mem_write(address, value);
	mem_write(address+1, value>>8);
}

void mem_init () {
	mbc.rmap = (u8**)malloc (0x10 * sizeof (u8*));
	mbc.wmap = (u8**)malloc (0x10 * sizeof (u8*));
}

/*
 * mem_read is the basic read function...not useful for much anymore
 * with the read map, but it's still necessary for the final messy
 * region.
 */

inline u8 mem_read(u16 address)
{
	u8 ha = (address>>12) & 0xE;
	
	switch (ha)
	{
	case 0x0:
	case 0x2:
		return rom[address];
	case 0x4:
	case 0x6:
		return rom[(mbc.rombank*ROM_BANK_SIZE)+(address & 0x3FFF)];
	case 0x8:
		return lcd.vbank[VBK1][address & 0x1FFF];
	case 0xA:
		if (!mbc.enableram && mbc.type == MBC_HUC3)
			return 0x01;
		if (!mbc.enableram)
			return 0xFF;
		if (rtc.sel&8)
			return rtc.regs[rtc.sel&7];
		return ram.sbank[(mbc.rambank*RAM_BANK_SIZE)+(address & 0x1FFF)];
	case 0xC:
		if ((address & 0xF000) == 0xC000)
			return ram.ibank[0][address & 0x0FFF];
		u8 n = R_SVBK & 0x07;
		return ram.ibank[n?n:1][address & 0x0FFF];
	case 0xE:
		if (address < 0xFE00) return mem_read(address & 0xDFFF);
		if ((address & 0xFF00) == 0xFE00)
		{
			/* if (R_STAT & 0x02) return 0xFF; */
			if (address < 0xFEA0) return lcd.oam.mem[address & 0xFF];
			return 0xFF;
		}
		/* return readhi(a & 0xFF); */
		if (address == 0xFFFF) return REG(0xFF);
		if (address >= 0xFF10 && address <= 0xFF3F) {
			if ((address &0xFF) == 0x26) {
				REG(0x26) ^= 0x0F;
			}
			return REG(address & 0xFF);
			//return sound_read(address & 0xFF);
		}
		if ((address & 0xFF80) == 0xFF80)
			return cpu_a.hi[address & 0xFF];
		return ioreg_read(address & 0xFF);
	}
	return 0xff; /* not reached */
}

/*
 * mem_write is the basic write function. Although it should only be
 * called when the write map contains a NULL for the requested address
 * region, it accepts writes to any address.
 */

inline void mem_write(u16 address, u8 b)
{
	u8 ha = (address>>12) & 0xE;
	
	/* printf("write to 0x%04X: 0x%02X\n", a, b); */
	switch (ha)
	{
	case 0x0:
	case 0x2:
	case 0x4:
	case 0x6:
		mbc_write(address, b);
		break;
	case 0x8:
		/* if ((R_STAT & 0x03) == 0x03) break; */
		vram_write(address & 0x1FFF, b);
		break;
	case 0xA:
		if (!mbc.enableram) break;
		if (rtc.sel&8)
		{
			rtc_write(b);
			break;
		}
		ram.sbank[(mbc.rambank*RAM_BANK_SIZE)+(address & 0x1FFF)] = b;
		break;
	case 0xC:
		if ((address & 0xF000) == 0xC000)
		{
			ram.ibank[0][address & 0x0FFF] = b;
			break;
		}
		u8 n = R_SVBK & 0x07;
		ram.ibank[n?n:1][address & 0x0FFF] = b;
		break;
	case 0xE:
		if (address < 0xFE00)
		{
			mem_write(address & 0xDFFF, b);
			break;
		}
		if ((address & 0xFF00) == 0xFE00)
		{
			/* if (R_STAT & 0x02) break; */
			if (address < 0xFEA0) lcd.oam.mem[address & 0xFF] = b;
			break;
		}
		/* return writehi(a & 0xFF, b); */
		if (address >= 0xFF10 && address <= 0xFF3F)
		{
			//sound_write(address & 0xFF, b);
			REG(address&0xFF) = b;
			break;
		}
		if ((address & 0xFF80) == 0xFF80 && address != 0xFFFF)
		{
			cpu_a.hi[address & 0xFF] = b;
			break;
		}
		ioreg_write(address & 0xFF, b);
	}
}

/*
 * ioreg_write handles output to io registers in the FF00-FF7F,FFFF
 * range. It takes the register number (low byte of the address) and a
 * byte value to be written.
 */

inline void ioreg_write(u8 r, u8 b)
{
	if (!hw.cgb)
	{
		switch (r)
		{
		case RI_VBK:
		case RI_BCPS:
		case RI_OCPS:
		case RI_BCPD:
		case RI_OCPD:
		case RI_SVBK:
		case RI_KEY1:
		case RI_HDMA1:
		case RI_HDMA2:
		case RI_HDMA3:
		case RI_HDMA4:
		case RI_HDMA5:
			return;
		}
	}
	
	switch(r)
	{
	case RI_TIMA:
	case RI_TMA:
	case RI_TAC:
	case RI_SCY:
	case RI_SCX:
	case RI_WY:
	case RI_WX:
		REG(r) = b;
		break;
	case RI_BGP:
		if (R_BGP == b) break;
		pal_write_dmg(0, b);
		pal_write_dmg(8, b);
		R_BGP = b;
		break;
	case RI_OBP0:
		if (R_OBP0 == b) break;
		pal_write_dmg(64,b);
		R_OBP0 = b;
		break;
	case RI_OBP1:
		if (R_OBP1 == b) break;
		pal_write_dmg(72,b);
		R_OBP1 = b;
		break;
	case RI_IF:
	case RI_IE:
		REG(r) = b & 0x1F;
		break;
	case RI_P1:
		REG(r) = b;
		pad_refresh();
		break;
	case RI_SC:
		/* FIXME - this is a hack for stupid roms that probe serial */
		if ((b & 0x81) == 0x81)
		{
			R_SB = 0xff;
			hw_interrupt(IF_SERIAL, IF_SERIAL);
			hw_interrupt(0, IF_SERIAL);
		}
		R_SC = b; /* & 0x7f; */
		break;
	case RI_DIV:
		REG(r) = 0;
		break;
	case RI_LCDC:
		lcdc_change(b);
		break;
	case RI_STAT:
		stat_write(b);
		break;
	case RI_LYC:
		REG(r) = b;
		stat_trigger();
		break;
	case RI_VBK:
		REG(r) = b | 0xFE;
		VBK1 = b&1;
		mbc.rmap[0x8] = mbc.rmap[0x9] = lcd.vbank[VBK1] - 0x8000;
		break;
	case RI_BCPS:
		R_BCPS = b & 0xBF;
		R_BCPD = lcd.pal[b & 0x3F];
		break;
	case RI_OCPS:
		R_OCPS = b & 0xBF;
		R_OCPD = lcd.pal[64 + (b & 0x3F)];
		break;
	case RI_BCPD:
		R_BCPD = b;
		pal_write(R_BCPS & 0x3F, b);
		if (R_BCPS & 0x80) R_BCPS = (R_BCPS+1) & 0xBF;
		break;
	case RI_OCPD:
		R_OCPD = b;
		pal_write(64 + (R_OCPS & 0x3F), b);
		if (R_OCPS & 0x80) R_OCPS = (R_OCPS+1) & 0xBF;
		break;
	case RI_SVBK:
		REG(r) = b & 0x07;
		u8 n = R_SVBK & 0x07;
		mbc.rmap[0xD] = mbc.wmap[0xD] = ram.ibank[n?n:1] - 0xD000;
		break;
	case RI_DMA:
		hw_dma(b);
		break;
	case RI_KEY1:
		REG(r) = (REG(r) & 0x80) | (b & 0x01);
		break;
	case RI_HDMA1:
		REG(r) = b;
		break;
	case RI_HDMA2:
		REG(r) = b & 0xF0;
		break;
	case RI_HDMA3:
		REG(r) = b & 0x1F;
		break;
	case RI_HDMA4:
		REG(r) = b & 0xF0;
		break;
	case RI_HDMA5:
		hw_hdma_cmd(b);
		break;
	}
	switch (r)
	{
	case RI_BGP:
	case RI_OBP0:
	case RI_OBP1:
		/* printf("palette reg %02X write %02X at LY=%02X\n", r, b, R_LY); */
	case RI_HDMA1:
	case RI_HDMA2:
	case RI_HDMA3:
	case RI_HDMA4:
	case RI_HDMA5:
		/* printf("HDMA %d: %02X\n", r - RI_HDMA1 + 1, b); */
		break;
	}
	/* printf("reg %02X => %02X (%02X)\n", r, REG(r), b); */
}


inline u8 ioreg_read(u8 r)
{
	switch(r)
	{
	case RI_SC:
		r = R_SC;
		R_SC &= 0x7f;
		return r;
	case RI_P1:
	case RI_SB:
	case RI_DIV:
	case RI_TIMA:
	case RI_TMA:
	case RI_TAC:
	case RI_LCDC:
	case RI_STAT:
	case RI_SCY:
	case RI_SCX:
	case RI_LY:
	case RI_LYC:
	case RI_BGP:
	case RI_OBP0:
	case RI_OBP1:
	case RI_WY:
	case RI_WX:
	case RI_IE:
	case RI_IF:
		return REG(r);
	case RI_VBK:
	case RI_BCPS:
	case RI_OCPS:
	case RI_BCPD:
	case RI_OCPD:
	case RI_SVBK:
	case RI_KEY1:
	case RI_HDMA1:
	case RI_HDMA2:
	case RI_HDMA3:
	case RI_HDMA4:
	case RI_HDMA5:
		if (hw.cgb) return REG(r);
	default:
		return 0xff;
	}
}

/*
 * Memory bank controllers typically intercept write attempts to
 * 0000-7FFF, using the address and byte written as instructions to
 * change rom or sram banks, control special hardware, etc.
 *
 * mbc_write takes an address (which should be in the proper range)
 * and a byte value written to the address.
 */

inline void mbc_write(u16 a, u8 b)
{
	u8 ha = (a>>12);

	/* printf("mbc %d: rom bank %02X -[%04X:%02X]-> ", mbc.type, mbc.rombank, a, b); */
	switch (mbc.type)
	{
	case MBC_MBC1:
		switch (ha & 0xE)
		{
		case 0x0:
			mbc.enableram = ((b & 0x0F) == 0x0A);
			break;
		case 0x2:
			if ((b & 0x1F) == 0) b = 0x01;
			mbc.rombank = (mbc.rombank & 0x60) | (b & 0x1F);
			mem_updaterom();
			break;
		case 0x4:
			if (mbc.model)
			{
				mbc.rambank = b & 0x03;
				mem_updateram ();
				break;
			}
			mbc.rombank = (mbc.rombank & 0x1F) | ((b&3)<<5);
			mem_updaterom();
			break;
		case 0x6:
			mbc.model = b & 0x1;
			break;
		}
		break;
	case MBC_MBC2: /* is this at all right? */
		if ((a & 0x0100) == 0x0000)
		{
			mbc.enableram = ((b & 0x0F) == 0x0A);
			break;
		}
		if ((a & 0xE100) == 0x2100)
		{
			mbc.rombank = b & 0x0F;
			mem_updaterom();
			break;
		}
		break;
	case MBC_MBC3:
		switch (ha & 0xE)
		{
		case 0x0:
			mbc.enableram = ((b & 0x0F) == 0x0A);
			break;
		case 0x2:
			if ((b & 0x7F) == 0) b = 0x01;
			mbc.rombank = b & 0x7F;
			mem_updaterom();
			break;
		case 0x4:
			rtc.sel = b & 0x0f;
			mbc.rambank = b & 0x03;
			mem_updateram ();
			break;
		case 0x6:
			rtc_latch(b);
			break;
		}
		break;
	case MBC_RUMBLE:
		switch (ha & 0xF)
		{
		case 0x4:
		case 0x5:
			/* FIXME - save high bit as rumble state */
			/* mask off high bit */
			b &= 0x7;
			break;
		}
		/* fall thru */
	case MBC_MBC5:
		switch (ha & 0xF)
		{
		case 0x0:
		case 0x1:
			mbc.enableram = ((b & 0x0F) == 0x0A);
			break;
		case 0x2:
			if ((b & 0xFF) == 0) b = 0x01;
			mbc.rombank = (mbc.rombank & 0x100) | (b & 0xFF);
			mem_updaterom();
			break;
		case 0x3:
			mbc.rombank = (mbc.rombank & 0xFF) | ((b&1)<<8);
			mem_updaterom();
			break;
		case 0x4:
		case 0x5:
			mbc.rambank = b & 0x0f;
			mem_updateram ();
			break;
		}
		break;
	case MBC_HUC1: /* FIXME - this is all guesswork -- is it right??? */
		switch (ha & 0xE)
		{
		case 0x0:
			mbc.enableram = ((b & 0x0F) == 0x0A);
			break;
		case 0x2:
			if ((b & 0x1F) == 0) b = 0x01;
			mbc.rombank = (mbc.rombank & 0x60) | (b & 0x1F);
			mem_updaterom();
			break;
		case 0x4:
			if (mbc.model)
			{
				mbc.rambank = b & 0x03;
				mem_updateram ();
				break;
			}
			mbc.rombank = (mbc.rombank & 0x1F) | ((b&3)<<5);
			mem_updaterom();
			break;
		case 0x6:
			mbc.model = b & 0x1;
			break;
		}
		break;
	case MBC_HUC3:
		switch (ha & 0xE)
		{
		case 0x0:
			mbc.enableram = ((b & 0x0F) == 0x0A);
			break;
		case 0x2:
			b &= 0x7F;
			mbc.rombank = b ? b : 1;
			mem_updaterom();
			break;
		case 0x4:
			rtc.sel = b & 0x0f;
			mbc.rambank = b & 0x03;
			mem_updateram ();
			break;
		case 0x6:
			rtc_latch(b);
			break;
		}
		break;
	}
}

/*
 * In order to make reads and writes efficient, we keep tables
 * (indexed by the high nibble of the address) specifying which
 * regions can be read/written without a function call. For such
 * ranges, the pointer in the map table points to the base of the
 * region in host system memory. For ranges that require special
 * processing, the pointer is NULL.
 *
 * mem_updatemap is called whenever bank changes or other operations
 * make the old maps potentially invalid.
 */

inline void mem_updateram () {
	mbc.rambank &= (mbc.ramsize - 1);
	if (mbc.enableram && !(rtc.sel&8))
	{
		mbc.rmap[0xA] = mbc.rmap[0xB] = mbc.wmap[0xA] = mbc.wmap[0xB] = ram.sbank + (mbc.rambank*RAM_BANK_SIZE) - 0xA000;
	}
	else mbc.rmap[0xA] = mbc.rmap[0xB] = mbc.wmap[0xA] = mbc.wmap[0xB] = NULL;
}	


inline void mem_updaterom () {
	mbc.rombank &= (mbc.romsize - 1);
	mbc.rmap[0x4] = mbc.rmap[0x5] = mbc.rmap[0x6] = mbc.rmap[0x7] = (u8*)rom + (mbc.rombank*ROM_BANK_SIZE) - 0x4000;
}

inline void mem_updatemapOnce() // only run once
{
	mbc.rmap[0x0] = mbc.rmap[0x1] = mbc.rmap[0x2] = mbc.rmap[0x3] = (u8*)rom;	
	mbc.rmap[0x4] = mbc.rmap[0x5] = mbc.rmap[0x6] = mbc.rmap[0x7] = (u8*)rom + (mbc.rombank*ROM_BANK_SIZE) - 0x4000;
	mbc.rmap[0x8] = mbc.rmap[0x9] = lcd.vbank[VBK1] - 0x8000;
	if (mbc.enableram && !(rtc.sel&8))
	{
		mbc.rmap[0xA] = mbc.rmap[0xB] = mbc.wmap[0xA] = mbc.wmap[0xB] = ram.sbank + (mbc.rambank*RAM_BANK_SIZE) - 0xA000;
	}
	else mbc.rmap[0xA] = mbc.rmap[0xB] = mbc.wmap[0xA] = mbc.wmap[0xB] = NULL;
	
	mbc.rmap[0xC] = mbc.wmap[0xC] = ram.ibank[0] - 0xC000;
	u8 n = R_SVBK & 0x07;
	mbc.rmap[0xD] = mbc.wmap[0xD] = ram.ibank[n?n:1] - 0xD000;
	mbc.rmap[0xE] = mbc.wmap[0xE] = ram.ibank[0] - 0xE000;
	mbc.wmap[0x0] = mbc.wmap[0x1] = mbc.wmap[0x2] = mbc.wmap[0x3] = 
	mbc.wmap[0x4] = mbc.wmap[0x5] = mbc.wmap[0x6] = mbc.wmap[0x7] = 
	mbc.wmap[0x8] = mbc.wmap[0x9] = mbc.wmap[0xF] = mbc.rmap[0xF] = NULL;
}

void mbc_reset()
{
	mbc.rombank = 1;
	mbc.rambank = 0;
	mbc.enableram = 0;
	mem_updatemapOnce();
}
