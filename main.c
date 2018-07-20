#include <stdio.h>
#include <stdlib.h>
#include <keyboard.h>
#include "timer.h"
#include "rom.h"
#include "mem.h"
#include "cpu.h"
#include "lcd.h"
#include "sdl.h"
#include "disp.h"
#include "timek.h"

void exit_routine()
{
	timek_stop();
	mem_free();
	rom_close();
	sdl_quit();
}

int main()
{
	char romn[20];
	atexit(exit_routine);

	timek_init();

	dclear();
	keyb_input((char*)&romn,"Enter rom file name");
	int r = rom_load(romn);
	if(!r) {
		mprint(1,1,"ROM FAILED!");
		dupdate();
		getkey();
		return 0;
	}
	dclear();
	sdl_init();
	//printf("ROM OK!\n");
	mprint(1,1,"ROM OK!");
	dupdate();
	getkey();

	mem_init();
	//printf("Mem OK!\n");
	mprint(1,2,"Mem OK!");
	dupdate();
	getkey();

	cpu_init();
	//printf("CPU OK!\n");
	mprint(1,3,"CPU OK!");
	dupdate();
	getkey();

	int exit_c = 1;
	while(1)
	{
		if (mode<3) {
			if(!(exit_c = cpu_cycle()))
				break;

			if(!lcd_cycle())
				break;
			timer_cycle();
		} else {
			if (sdl_menu())
				break;
		}
	}
	if (!exit_c)
		cpu_print_info();
	
	exit_routine();

	return 0;
}
