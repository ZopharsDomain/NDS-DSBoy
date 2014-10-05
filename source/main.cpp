//////////////////////////////////////////////////////////////////////
// Simple consol print demo
// -- dovoto
//////////////////////////////////////////////////////////////////////
#include <nds.h>
#include <nds/arm9/console.h> //basic print funcionality
#include "../asm/cpu_a.h"

void mem_init ();
void rtc_init();
void emu_reset();
void rom_load();

int main(void)
{
    videoSetMode(MODE_5_2D | DISPLAY_BG3_ACTIVE); 
	videoSetModeSub(MODE_0_2D | DISPLAY_BG0_ACTIVE); //sub bg 0 will be used to print text
	vramSetMainBanks(VRAM_A_MAIN_BG_0x6000000, VRAM_B_MAIN_BG_0x6020000, 
                     VRAM_C_SUB_BG , VRAM_D_SUB_SPRITE);
	
	SUB_BG0_CR = BG_MAP_BASE(31);
	
	BG_PALETTE_SUB[255] = RGB15(31,31,31);//by default font will be rendered with color 255
	
	consoleInitDefault((u16*)SCREEN_BASE_BLOCK_SUB(31), (u16*)CHAR_BASE_BLOCK_SUB(0), 16);

	TIMER0_DATA = 0;
	TIMER0_CR = TIMER_DIV_1024;
	
	BG3_CR = BG_BMP16_256x256;
	BG3_XDX = 1 << 8;
    BG3_XDY = 0;
    BG3_YDX = 0;
    BG3_YDY = 1 << 8;

  	mem_init ();
	rtc_init();
	rom_load();
	emu_reset();
	emu_run();
  	
  	
	return 0;
}


