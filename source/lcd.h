#ifndef _LCD_H_
#define _LCD_H_

struct vissprite
{
	u8 *buf;
	int x;
	u8 pal;
	u8 pri; 
	u8 pad[6];
};

struct scan
{
	int  bg[64];
	int wnd[64];
	u8 buf[256];
	u16 pal[64];
	u8 pri[256];
	struct vissprite vs[16];
	u8  l, x,y,s,t,u,v;
	int ns, wx, wy, wt, wv;
	//int ns, l, x, y, s, t, u, v, wx, wy, wt, wv;
};

struct obj
{
	u8 y;
	u8 x;
	u8 pat;
	u8 flags;
};

struct lcd
{
	u8 vbank[2][8192];
	union
	{
		u8 mem[256];
		struct obj obj[40];
	} oam;
	u8 pal[128];
};


extern struct lcd lcd;
extern u8 VBK1;

void lcd_begin();
void lcd_refreshline();
void lcd_reset();

void vram_write(u16 a, u8 b);
void pal_write(u8 i, u8 b);
void pal_write_dmg(u8 i, u8 d);

#define quick_pix 1



#endif //_LCD_H_
