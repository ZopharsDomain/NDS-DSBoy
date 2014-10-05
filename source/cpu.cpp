#include "cpu.h"
#include "hw.h"
#include "mem.h"
#include "lcdc.h"
#include "../asm/cpu_a.h"
#include <nds/arm9/console.h> //basic print funcionality

inline void div_advance(int cnt);
inline void timer_advance(int cnt);
inline void lcdc_advance(int cnt);
inline void sound_advance(int cnt);
inline void cpu_timers(int cnt);

struct cpu_a cpu_a;

inline void div_advance(int cnt)
{
	cpu_a.div += (cnt<<1);
	if (cpu_a.div >= 256)
	{
		R_DIV += (cpu_a.div >> 8);
		cpu_a.div &= 0xff;
	}
}

inline void timer_advance(int cnt)
{
	int unit, tima;
	
	if (!(R_TAC & 0x04)) return;

	unit = ((-R_TAC) & 3) << 1;
	cpu_a.tim += (cnt<<unit);

	if (cpu_a.tim >= 512)
	{
		tima = R_TIMA + (cpu_a.tim >> 9);
		cpu_a.tim &= 0x1ff;
		if (tima >= 256)
		{
			hw_interrupt(IF_TIMER, IF_TIMER);
			hw_interrupt(0, IF_TIMER);
		}
		while (tima >= 256)
			tima = tima - 256 + R_TMA;
		R_TIMA = tima;
	}
}

inline void lcdc_advance(int cnt)
{
	cpu_a.lcdc -= cnt;
	if (cpu_a.lcdc <= 0) lcdc_trans();
}

inline void sound_advance(int cnt)
{
	cpu_a.snd += cnt;
}

int cpu_idle(int max)
{
	int cnt, unit;
	if (!(cpu_a.halt && cpu_a.ime)) return 0;
	if (R_IF & R_IE)
	{
		cpu_a.halt = 0;
		return 0;
	}

	/* Make sure we don't miss lcdc status events! */
	if ((R_IE & (IF_VBLANK | IF_STAT)) && (max > cpu_a.lcdc))
		max = cpu_a.lcdc;
	
	/* If timer interrupt cannot happen, this is very simple! */
	if (!((R_IE & IF_TIMER) && (R_TAC & 0x04)))
	{
		cpu_timers(max);
		return max;
	}

	/* Figure out when the next timer interrupt will happen */
	unit = ((-R_TAC) & 3) << 1;
	cnt = (511 - cpu_a.tim + (1<<unit)) >> unit;
	cnt += (255 - R_TIMA) << (9 - unit);

	if (max < cnt)
		cnt = max;
	
	cpu_timers(cnt);
	return cnt;
}

inline void cpu_timers(int cnt)
{
	div_advance(cnt << cpu_a.speed);
	timer_advance(cnt << cpu_a.speed);
	lcdc_advance(cnt);
	sound_advance(cnt);
}
