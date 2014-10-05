#include "types.h"
#include "cpu.h"
#include "regs.h"
#include "lcd.h"
#include "rtc.h"
#include "hw.h"

#include "gbarom_bin.h"
#include <stdlib.h>
#include <string.h>
#include <nds/arm9/console.h> //basic print funcionality


#define WAIT_CR       (*(vu16*)0x04000204)
#define SRAM          ((u8*)0x0A000000)

int is_homebrew_cartridge();

int framecount;

char card_id[5] = { 0,0,0,0,0 };

const int mbc_table[256] =
{
	0, 1, 1, 1, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 3,
	3, 3, 3, 3, 0, 0, 0, 0, 0, 5, 5, 5, MBC_RUMBLE, MBC_RUMBLE, MBC_RUMBLE, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, MBC_HUC3, MBC_HUC1
};

const int rtc_table[256] =
{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0
};

const int batt_table[256] =
{
	0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0,
	1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0,
	0
};

const int romsize_table[256] =
{
	2, 4, 8, 16, 32, 64, 128, 256, 512,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 128, 128, 128
	/* 0, 0, 72, 80, 96  -- actual values but bad to use these! */
};

const int ramsize_table[256] =
{
	1, 1, 1, 4, 16,
	4 /* FIXME - what value should this be?! */
};


int rom_load()
{
	int gbamode = 0;
	int c;

	WAIT_CR &= ~0x80;
	rom = gbarom_bin;	
   		      

	c = rom[0x0147];
	mbc.type = mbc_table[c];
	mbc.batt = batt_table[c] ;
	rtc.batt = rtc_table[c];
	mbc.romsize = romsize_table[rom[0x0148]];
	mbc.ramsize = ramsize_table[rom[0x0149]];

    memcpy(card_id, (char*)0x080000AC, 4);
     
    if(is_homebrew_cartridge()) {
     	ram.sbank = SRAM;
    } else {
		ram.sbank = (u8*)malloc(32768);
    }


	mbc.rombank = 1;
	mbc.rambank = 0;

	c = rom[0x0143];
	hw.cgb = ((c == 0x80) || (c == 0xC0));
	hw.gba = (hw.cgb && gbamode);


	return 0;
}

void emu_reset()
{
	hw_reset();
	lcd_reset();
	cpu_reset();
	mbc_reset();
}

int is_homebrew_cartridge() {
    return 
      card_id[0] == 'P' &&
      card_id[1] == 'A' &&
      card_id[2] == 'S' &&
      card_id[3] == 'S';
}
