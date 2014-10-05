#include "types.h"
#include "lcd.h"
#include "regs.h"
#include "hw.h"
#include "../asm/lib.h"
#include <nds.h>
#include <nds/arm9/console.h> //basic print funcionality
#include <string.h>

#define VBLANK_INT	 546
#define BIGUP(n) (((n)<<24)|((n)<<16)|((n)<< 8)|(n))

struct lcd lcd;
struct scan scan;

inline void updatepatpix();
inline void updatepalette(u8 i);
inline void spr_enum();
inline void tilebuf();
inline void bg_scan_color();
inline void wnd_scan_color();
inline void wnd_scan_pri();
inline void bg_scan_pri();
inline void bg_scan();
inline void wnd_scan();
inline void spr_scan();
inline void vram_dirty();
inline void pal_dirty();	
	
u16 *vdest;
u16 anydirty;
int sprsort = 1;
u8 patpix[4096][8][8];
u8 patdirty[1024];
u16 quickpat [128];
u8 * quickkptr[896][8][8][4];
u8 VBK1;


#define BG (scan.bg)
#define WND (scan.wnd)
#define BUF (scan.buf)
#define PRI (scan.pri)

#define PAL (scan.pal)

#define VS (scan.vs) /* vissprites */
#define NS (scan.ns)

#define L (scan.l) /* line */
#define X (scan.x) /* screen position */
#define Y (scan.y)
#define S (scan.s) /* tilemap position */
#define T (scan.t)
#define U (scan.u) /* position within tile */
#define V (scan.v)
#define WX (scan.wx)
#define WY (scan.wy)
#define WT (scan.wt)
#define WV (scan.wv)

const u16 dmg_pal[4] =  {0xFFFF, 0xEE95, 0xD10A, 0x8000};
//const int wraptable[64] =
//	{
//		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
//		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,-32
//	};

void vram_write(u16 a, u8 b)
{
	lcd.vbank[VBK1][a] = b;
	if (a >= 0x1800) return;
	if (!patdirty[(VBK1<<9)+(a>>4)]) {
		patdirty[(VBK1<<9)+(a>>4)] = 1;
		if (anydirty < 128) 
			quickpat [anydirty] = (VBK1<<9)+(a>>4);
		anydirty ++;
	}
}

void pal_write(u8 i, u8 b)
{
	if (lcd.pal[i] == b) return;
	lcd.pal[i] = b;
	updatepalette(i>>1);
}

inline void updatepalette(u8 i)
{
	PAL[i] = 0x8000 | lcd.pal[i<<1] | (lcd.pal[(i<<1)|1] << 8);
}

void pal_write_dmg(u8 i, u8 d)
{
	if (hw.cgb) return;
	PAL[(i)>>1] = dmg_pal[d & 3];
	PAL[(i+2)>>1] = dmg_pal[(d >> 2) & 3];
	PAL[(i+4)>>1] = dmg_pal[(d >> 4) & 3];
	PAL[(i+6)>>1] = dmg_pal[(d >> 6) & 3];
}

void lcd_begin()
{
	vdest =((u16*)0x6000000) + 48 + 6144;
	WY = R_WY;
	//while (TIMER0_DATA < VBLANK_INT) {}
	TIMER0_CR = 0;
	consolePrintSet(0,10);
	consolePrintf ("WAIT: %d           \n", TIMER0_DATA);
	TIMER0_DATA = 0;
	// 33 ticks per milisecond
	TIMER0_CR = TIMER_DIV_1024;

}


void lcd_refreshline()
{		
	
	if (!(R_LCDC & 0x80))
		return; /* should not happen... */

	updatepatpix();
	
	L = R_LY;
	X = R_SCX;
	Y = (R_SCY + L) & 0xff;
	S = X >> 3;
	T = Y >> 3;
	U = X & 7;
	V = Y & 7;
	
	WX = R_WX - 7;
	if (WY>L || WY<0 || WY>143 || WX<-7 || WX>159 || !(R_LCDC&0x20))
		WX = 160;
	WT = (L - WY) >> 3;
	WV = (L - WY) & 7;
	
	spr_enum();

	tilebuf();
	if (hw.cgb)
	{
		bg_scan_color();
		wnd_scan_color();
		if (NS)
		{
			bg_scan_pri();
			wnd_scan_pri();
		}
	}
	else
	{
		bg_scan();
		wnd_scan();
		//recolor(BUF+WX, 0x04, 160-WX);
	}
	spr_scan();

	refresh_2(vdest, BUF, PAL);
	vdest += 256;
}

inline void updatepatpix()
{
	if (!anydirty) return;
	int i, j, k;
	int a, c;
	u8 *vram = lcd.vbank[0];
#ifdef quick_pix	
	if (anydirty <= 128) {
		u16 i2;
		for (i = 0; i < anydirty; i++) {
			i2 = quickpat[i];
			patdirty[i2] = 0;		
			for (j = 0; j < 8; j++)
			{
				a = ((i2<<4) | (j<<1));				
				for (k = 0; k < 8; k++)
				{
					c = vram[a] & (1<<k) ? 1 : 0;
					c |= vram[a+1] & (1<<k) ? 2 : 0;
					*(quickkptr[i2][j][k][0]) = 
					*(quickkptr[i2][j][k][1]) = 
					*(quickkptr[i2][j][k][2]) = 
					*(quickkptr[i2][j][k][3]) = c;										
				}					
			}	
		}
		anydirty = 0;
		return;
	}
#endif
	for (i = 0; i < 896; i++)
	{
		if (i == 384) i = 512;
		if (!patdirty[i]) continue;
		patdirty[i] = 0;		
		for (j = 0; j < 8; j++)
		{
			a = ((i<<4) | (j<<1));				
			for (k = 0; k < 8; k++)
			{
				c = vram[a] & (1<<k) ? 1 : 0;
				c |= vram[a+1] & (1<<k) ? 2 : 0;
				*(quickkptr[i][j][k][0]) = 
				*(quickkptr[i][j][k][1]) = 
				*(quickkptr[i][j][k][2]) = 
				*(quickkptr[i][j][k][3]) = c;										
			}					
		}
	}
	anydirty = 0;
}

inline void spr_enum()
{
	int i, j;
	struct obj *o;
	int v, pat;

	NS = 0;
	if (!(R_LCDC & 0x02)) return;

	o = lcd.oam.obj;
	
	for (i = 40; i; i--, o++)
	{
		if (L >= o->y || L + 16 < o->y)
			continue;
		if (L + 8 >= o->y && !(R_LCDC & 0x04))
			continue;
		VS[NS].x = /*(int)*/o->x - 8;
		v = L - /*(int)*/o->y + 16;
		if (hw.cgb)
		{
			pat = o->pat | ((o->flags & 0x60) << 5)
				| ((o->flags & 0x08) << 6);
			VS[NS].pal = 32 + ((o->flags & 0x07) << 2);
		}
		else
		{
			pat = o->pat | ((o->flags & 0x60) << 5);
			VS[NS].pal = 32 + ((o->flags & 0x10) >> 2);
		}
		VS[NS].pri = (o->flags & 0x80) >> 7;
		if ((R_LCDC & 0x04))
		{
			pat &= ~1;
			if (v >= 8)
			{
				v -= 8;
				pat++;
			}
			if (o->flags & 0x40) pat ^= 1;
		}
		VS[NS].buf = patpix[pat][v];
		if (++NS == 10) break;
	}
	if (!sprsort || hw.cgb) return;
	// Quick and dirty sort
 	struct vissprite value;
    for(i = 1; i < NS; i++) {
    	value = VS[i];
        j = i - 1;
        while (j >= 0 && VS[j].x > value.x) {
        	VS[j+1] = VS[j];
            j--;
        }
        VS[j+1] = value;
    } 
}


inline void tilebuf()
{
	int i, cnt;
	int base;
	u8 *tilemap, *attrmap;
	int *tilebuf;
	//const int *wrap;
	int wrap2;

	base = ((R_LCDC&0x08)?0x1C00:0x1800) + (T<<5) + S;
	tilemap = lcd.vbank[0] + base;
	attrmap = lcd.vbank[1] + base;
	tilebuf = BG;
	//wrap = wraptable + S;
	wrap2 = S;
	cnt = ((WX + 7) >> 3) + 1;

	if (hw.cgb)
	{
		if (R_LCDC & 0x10)
			for (i = cnt; i > 0; i--)
			{
				*(tilebuf++) = *tilemap
					| ((/*(int)*/*attrmap & 0x08) << 6)
					| ((/*(int)*/*attrmap & 0x60) << 5);
				*(tilebuf++) = ((/*(int)*/*attrmap & 0x07) << 2);
				if (wrap2++ == 31) {
					attrmap -= 31;
					tilemap -= 31;
				}
				else {
					attrmap++;
					tilemap++;
				}					
				//attrmap += *wrap + 1;
				//tilemap += *(wrap++) + 1;	
			}
		else
			for (i = cnt; i > 0; i--)
			{
				*(tilebuf++) = (256 + ((s8)*tilemap))
					| ((/*(int)*/*attrmap & 0x08) << 6)
					| ((/*(int)*/*attrmap & 0x60) << 5);
				*(tilebuf++) = ((/*(int)*/*attrmap & 0x07) << 2);
				if (wrap2++ == 31) {
					attrmap -= 31;
					tilemap -= 31;
				}
				else {
					attrmap++;
					tilemap++;
				}
				//attrmap += *wrap + 1;
				//tilemap += *(wrap++) + 1;
			}
	}
	else
	{
		if (R_LCDC & 0x10)		
			for (i = cnt; i > 0; i--)
			{
				*(tilebuf++) = *(tilemap++);		
				if (wrap2++ == 31) tilemap -= 32;
			}
		else
			for (i = cnt; i > 0; i--)
			{
				*(tilebuf++) = (256 + ((s8)*(tilemap++)));
				if (wrap2++ == 31) tilemap -= 32;
			}
	}

	if (WX >= 160) return;
	
	base = ((R_LCDC&0x40)?0x1C00:0x1800) + (WT<<5);
	tilemap = lcd.vbank[0] + base;
	attrmap = lcd.vbank[1] + base;
	tilebuf = WND;
	cnt = ((160 - WX) >> 3) + 1;

	if (hw.cgb)
	{
		if (R_LCDC & 0x10)
			for (i = cnt; i > 0; i--)
			{
				*(tilebuf++) = *(tilemap++)
					| ((/*(int)*/*attrmap & 0x08) << 6)
					| ((/*(int)*/*attrmap & 0x60) << 5);
				*(tilebuf++) = ((/*(int)*/*(attrmap++)&7) << 2);
			}
		else
			for (i = cnt; i > 0; i--)
			{
				*(tilebuf++) = (256 + ((s8)*(tilemap++)))
					| ((/*(int)*/*attrmap & 0x08) << 6)
					| ((/*(int)*/*attrmap & 0x60) << 5);
				*(tilebuf++) = ((/*(int)*/*(attrmap++)&7) << 2);
			}
	}
	else
	{
		if (R_LCDC & 0x10)
			for (i = cnt; i > 0; i--)
				*(tilebuf++) = *(tilemap++);
		else
			for (i = cnt; i > 0; i--)
				*(tilebuf++) = (256 + ((s8)*(tilemap++)));
	}
}

inline void bg_scan_color()
{
	int cnt;
	u8 *src, *dest;
	int *tile;

	if (WX <= 0) return;
	cnt = WX;
	tile = BG;
	dest = BUF;
	

	src = patpix[*(tile++)][V] + U;	
	blendcpy(dest, src, *(tile++), 8-U);
	dest += 8-U;
	cnt -= 8-U;
	if (cnt <= 0) return;
	if (((u32)dest&0x3) == 0){
		while (cnt >= 8)
		{
			src = patpix[*(tile++)][V];
			blendcpy8d(dest, src, BIGUP(*(tile)));
			tile++;
			dest += 8;
			cnt -= 8;
		}
	}
	else{
		while (cnt >= 8)
		{
			src = patpix[*(tile++)][V];
			blendcpy8(dest, src, *(tile++));
			dest += 8;
			cnt -= 8;
		}
	}
	src = patpix[*(tile++)][V];
	blendcpy(dest, src, *(tile++), cnt);
}


inline void wnd_scan_color()
{
	int cnt;
	u8 *src, *dest;
	int *tile;

	if (WX >= 160) return;
	cnt = 160 - WX;
	tile = WND;
	dest = BUF + WX;
	if (((u32)dest&0x3) == 0){
	while (cnt >= 8)
	{	
		src = patpix[*(tile++)][WV];
		blendcpy8d(dest, src, BIGUP(*(tile)));
		tile++;
		dest += 8;
		cnt -= 8;
	}
	}
	else{
	while (cnt >= 8)
	{	
		src = patpix[*(tile++)][WV];
		blendcpy8(dest, src, *(tile++));
		dest += 8;
		cnt -= 8;
	}
	}
	src = patpix[*(tile++)][WV];
	blendcpy(dest, src, *(tile++), cnt);
}

inline void bg_scan_pri()
{
	int cnt, i;
	u8 *src, *dest;

	if (WX <= 0) return;
	i = S;
	cnt = WX;
	dest = PRI;
	src = lcd.vbank[1] + ((R_LCDC&0x08)?0x1C00:0x1800) + (T<<5);

	if (!priused(src))
	{
		memset(dest, 0, cnt);
		return;
	}
	
	memsetv(dest, src[i++&31]&128, 8-U);
	dest += 8-U;
	cnt -= 8-U;
	if (cnt <= 0) return;
	if (((u32)dest&0x3) == 0){
		while (cnt >= 8)
		{
			memset8d(dest, BIGUP(src[i&31]&128));
			i++;
			dest += 8;
			cnt -= 8;
		}
	}
	else {
	while (cnt >= 8)
	{
		memset8(dest, src[i++&31]&128);
		dest += 8;
		cnt -= 8;
	}
	}
	memsetv(dest, src[i&31]&128, cnt);
}

inline void wnd_scan_pri()
{
	int cnt, i;
	u8 *src, *dest;

	if (WX >= 160) return;
	i = 0;
	cnt = 160 - WX;
	dest = PRI + WX;
	src = lcd.vbank[1] + ((R_LCDC&0x40)?0x1C00:0x1800) + (WT<<5);
	
	if (!priused(src))
	{
		memset(dest, 0, cnt);
		return;
	}
	if (((u32)dest&0x3) == 0){
		while (cnt >= 8)
		{
			memset8d(dest, BIGUP(src[i]&128));
			i++;
			dest += 8;
			cnt -= 8;
		}
	}
	else{
		while (cnt >= 8)
		{
			memset8(dest, src[i++]&128);
			dest += 8;
			cnt -= 8;
		}
	}
	memsetv(dest, src[i]&128, cnt);
}


inline void bg_scan()
{
	int cnt;
	u8 *src, *dest;
	int *tile;

	if (WX <= 0) return;
	cnt = WX;
	tile = BG;
	dest = BUF;
	
	src = patpix[*(tile++)][V] + U;

	memcpyv(dest, src, 8-U);
	dest += 8-U;
	cnt -= 8-U;
	if (cnt <= 0) return;
	if (((u32)dest&0x3) == 0){
		while (cnt >= 8)
		{	
			src = patpix[*(tile++)][V];
			memcpy8d(dest, src);
			dest += 8;
			cnt -= 8;
		}
	}
	else {
		while (cnt >= 8)
		{	
			src = patpix[*(tile++)][V];
			memcpy8s(dest, src);
			dest += 8;
			cnt -= 8;
		}
	}
	src = patpix[*tile][V];
	memcpyv(dest, src, cnt);
}

inline void wnd_scan()
{
	int cnt;
	u8 *src, *dest;
	int *tile;

	if (WX >= 160) return;
	cnt = 160 - WX;
	tile = WND;
	dest = BUF + WX;
	
	if (((u32)dest&0x3) == 0){
	while (cnt >= 8)
	{
		src = patpix[*(tile++)][WV];
		memcpy8d(dest, src);
		dest += 8;
		cnt -= 8;
	}
	}
	else 
	{
		while (cnt >= 8)
	{
		src = patpix[*(tile++)][WV];
		memcpy8s(dest, src);
		dest += 8;
		cnt -= 8;
	}
	}
	src = patpix[*tile][WV];
	memcpyv(dest, src, cnt);
}


inline void spr_scan()
{
	int i, x;
	u8 pal, b, ns = NS;
	u8 *src, *dest, *bg, *pri;
	struct vissprite *vs;
	static u8 bgdup[256];

	if (!ns) return;
	memcpy256d(bgdup, BUF);	
	vs = &VS[ns-1];
	
	for (; ns; ns--, vs--)
	{
		x = vs->x;
		if (x >= 160) continue;
		if (x <= -8) continue;
		if (x < 0)
		{
			src = vs->buf - x;
			dest = BUF;
			i = 8 + x;
		}
		else
		{
			src = vs->buf;
			dest = BUF + x;
			if (x > 152) i = 160 - x;
			else i = 8;
		}
		pal = vs->pal;
		if (vs->pri)
		{
			bg = bgdup + (dest - BUF);
			while (i--)
			{
				b = src[i];
				if (b && !(bg[i]&3)) dest[i] = pal|b;
			}
		}
		else if (hw.cgb)
		{
			bg = bgdup + (dest - BUF);
			pri = PRI + (dest - BUF);
			while (i--)
			{
				b = src[i];
				if (b && (!pri[i] || !(bg[i]&3)))
					dest[i] = pal|b;
			}
		}
		else while (i--) if (src[i]) dest[i] = pal|src[i];
	}
}

inline void vram_dirty()
{
	anydirty = 1;
	memsetv(patdirty, 0, sizeof patdirty);
}

inline void pal_dirty()
{
	int i;
	for (i = 0; i < 128; i++) 
		PAL[i] = 0;
	if (!hw.cgb)
	{
		pal_write_dmg(0, R_BGP);
		pal_write_dmg(8, R_BGP);
		pal_write_dmg(64, R_OBP0);
		pal_write_dmg(72, R_OBP1);
	}	
	
	for (i = 0; i < 64; i++)
		updatepalette(i);

}

void lcd_reset()
{
	u8 * flipper = (u8*)(&lcd);
	for (u16 i = 0; i < sizeof lcd; i++) 
		flipper[i] = 0;	
	lcd_begin();
	vram_dirty();
	pal_dirty();
	for (u16 i = 0; i < 896; i++) 
		for (u16 j = 0; j < 8; j++) 
			for (u16 k = 0; k < 8; k++) {
				quickkptr[i][j][k][0] = &(patpix[i+1024][j][k]);
				quickkptr[i][j][k][1] = &(patpix[i][j][7-k]); 
				quickkptr[i][j][k][2] = &(patpix[i+2048][7-j][7-k]); 
				quickkptr[i][j][k][3] = &(patpix[i+3072][7-j][k]); 
			}
}

