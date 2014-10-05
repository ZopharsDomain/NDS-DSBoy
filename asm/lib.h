#ifndef _LIBASM_H_
#define _LIBASM_H_

extern "C" void memcpy8(u8 *dest, u8 *src);
extern "C" void memcpy8d(u8 *dest, u8 *src);
extern "C" void memcpy8s(u8 *dest, u8 *src);
extern "C" void memcpy16d(u8 *dest, u8 *src);
extern "C" void memcpy256d(u8 *dest, u8 *src);
extern "C" void memcpy160d(u8 *dest, u8 *src);
extern "C" void memcpyv(u8 *dest, u8 *src, int cnt);
extern "C" void memcpyvd(u8 *dest, u8 *src, int cnt);

extern "C" void memset8(u8 *dest, u8 src);
extern "C" void memset8d(u8 *dest, u32 src);
extern "C" void memsetv(u8 *dest, u8 src, int cnt);

extern "C" void blendcpy8d(u8 *dest, u8 *src, u32 b);
extern "C" void refresh_2(u16 *dest, u8 *src, u16 *pal);
extern "C" void blendcpy8(u8 *dest, u8 *src, u8 b);
extern "C" void blendcpy(u8 *dest, u8 *src, u8 b, int cnt);
extern "C" void recolor(u8 *buf, u8 fill, int cnt);
extern "C" int priused(void *attr);
extern "C" u8 tester();
#endif //_LIB_H_
