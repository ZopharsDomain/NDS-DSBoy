#ifndef _MEM_H_
#define _MEM_H_

#include "types.h"

#define ROM_BANK_SIZE 16384 
#define RAM_BANK_SIZE 8192

#define MBC_NONE 0
#define MBC_MBC1 1
#define MBC_MBC2 2
#define MBC_MBC3 3
#define MBC_MBC5 5
#define MBC_RUMBLE 15
#define MBC_HUC1 0xC1
#define MBC_HUC3 0xC3

struct mbc
{
	u8 type;
	u8 model;
	u16 rombank;
	u8 rambank;
	u16 romsize;
	u8 ramsize;
	u8 enableram;
	u8 batt;
    u8 **rmap;
	u8 **wmap;
};

struct ram
{
/*	u8 hi[256];*/
	u8 ibank[8][4096];
	u8 *sbank;
};

extern struct ram ram;
extern struct mbc mbc;
extern const u8 * rom;

void writeb(u16 address, u8 value);
void writew(u16 address, u16 value);
u8 readb(u16 address);
u16 readw(u16 address);
s8 readsb(u16 address);

void mem_init ();
void mbc_reset();
#endif //_MEM_H_
