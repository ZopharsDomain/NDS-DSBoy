#ifndef _CPU_H_
#define _CPU_H_

#include "regs.h"
#include "types.h"
#include "../asm/cpu_a.h"

#define IF R_IF
#define IE R_IE

int cpu_idle(int max);
#endif //_CPU_H_
