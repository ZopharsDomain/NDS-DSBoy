#ifndef _CPU_A_H_
#define _CPU_A_H_

struct cpu_a
{
	u32 sp;
	u8 ime; 
	u8 ima;
	u8 speed;
	u8 halt;
	int div; 
	int tim;
	int lcdc;
	int snd;
	u8 hi[256];
};

extern "C" void cpu_reset();
extern "C" void emu_run();
extern "C" void cpu_emulate(int cycles);

extern struct cpu_a cpu_a;

#endif //_CPU_A_H_
