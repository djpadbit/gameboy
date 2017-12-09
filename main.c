#include <stdio.h>
#include "timer.h"
#include "rom.h"
#include "mem.h"
#include "cpu.h"
#include "lcd.h"
#include "sdl.h"
#include "disp.h"
#include "timek.h"

int main()
{
	int r;
	char romn[20];

	timek_init();

	dclear();
	keyb_input(&romn,"Enter rom file name");
	r = rom_load(romn);
	if(!r) {
		locate(1,1,"ROM FAILED!");
		dupdate();
		getkey();
		return 0;
	}
	dclear();
	sdl_init();
	//printf("ROM OK!\n");
	locate(1,1,"ROM OK!");
	dupdate();
	getkey();

	mem_init();
	//printf("Mem OK!\n");
	locate(1,2,"Mem OK!");
	dupdate();
	getkey();

	cpu_init();
	//printf("CPU OK!\n");
	locate(1,3,"CPU OK!");
	dupdate();
	getkey();

	while(1)
	{
		if(!cpu_cycle())
			break;

		if(!lcd_cycle())
			break;

		timer_cycle();
	}
	
	timek_stop();
	mem_free();
	rom_close();
	sdl_quit();

	return 0;
}
