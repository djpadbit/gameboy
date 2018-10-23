#include <stdlib.h>
#include <string.h>
#include "mem.h"
#include "rom.h"
#include "lcd.h"
#include "mbc.h"
#include "interrupt.h"
#include "timer.h"
#include "sdl.h"
#include "cpu.h"
unsigned char *topmem;  // Top memory      0xFF00 - 0xFFFF - 256 bytes
unsigned char *oammem;  // OAM memory      0xFE00 - 0xFE9F - 160 bytes
unsigned char *mainmem; // Work memory     0xC000 - 0xDFFF - 8192 bytes
unsigned char *extmem;  // External memory 0xA000 - 0xBFFF - 8192 bytes
unsigned char *vmem;    // Video memory    0x8000 - 0x9FFF - 8192 bytes
int DMA_pending = 0;
int joypad_select_buttons, joypad_select_directions;
unsigned int bank = 0;

void mem_bank_switch(unsigned int n)
{
	//unsigned char *b = rom_getbytes();
	bank = n;
	//memcpy(&mem[0x4000], &b[n * 0x4000], 0x4000);
}

/*unsigned char inline rom_read_byte_bank(unsigned short i)
{
	if (i>=0x4000 && bank != 0) {
		return rom_read_byte((bank*0x4000)+(i-0x4000));
	}
	return rom_read_byte(i);
}*/

/* LCD's access to VRAM */
/*inline unsigned char mem_get_raw(unsigned short p)
{
	return vmem[p-0x8000];
}*/
unsigned char inline mem_get_raw(unsigned short i) {
	if(i < 0xFF00) {
		if (i < 0xFEA0 && i >= 0xFE00) {        // OAM memory
			return oammem[i-0xFE00];
		} else if (i < 0xFE00 && i >= 0xE000) { // Shadow work ram 
			return mainmem[i-0xE000];
		} else if (i < 0xE000 && i >= 0xC000) { // Work ram
			return mainmem[i-0xC000];
		} else if (i < 0xC000 && i >= 0xA000) { // External ram
			return extmem[i-0xA000];
		} else if (i < 0xA000 && i >= 0x8000) { // Video ram
			return vmem[i-0x8000];
		} else if (i < 0x8000) {                // Cartradge rom
			return rom_read_byte(i>=0x4000 && bank != 0 ? (bank*0x4000)+(i-0x4000) : i);//rom_read_byte_bank(i);
		}
	}
	if (i >= 0xFF00) {                          // Top ram
		return topmem[i-0xFF00];
	}
	return 0;
}

unsigned char mem_get_byte(unsigned short i)
{
	unsigned long elapsed;
	unsigned char mask = 0;

	if(DMA_pending && i < 0xFF80)
	{
		elapsed = c.cycles - DMA_pending;
		if(elapsed >= 160)
			DMA_pending = 0;
		else
		{
			return oammem[elapsed];
		}
	}

	if(i < 0xFF00) {
		if (i < 0xFEA0 && i >= 0xFE00) {        // OAM memory
			return oammem[i-0xFE00];
		} else if (i < 0xFE00 && i >= 0xE000) { // Shadow work ram 
			return mainmem[i-0xE000];
		} else if (i < 0xE000 && i >= 0xC000) { // Work ram
			return mainmem[i-0xC000];
		} else if (i < 0xC000 && i >= 0xA000) { // External ram
			return extmem[i-0xA000];
		} else if (i < 0xA000 && i >= 0x8000) { // Video ram
			return vmem[i-0x8000];
		} else if (i < 0x8000) {                // Cartradge rom
			return rom_read_byte(i>=0x4000 && bank != 0 ? (bank*0x4000)+(i-0x4000) : i);//rom_read_byte_bank(i);
		}
	}

	switch(i)
	{
		case 0xFF00:	/* Joypad */
			if(!joypad_select_buttons)
				mask = sdl_get_buttons();
			if(!joypad_select_directions)
				mask = sdl_get_directions();
			return 0xC0 | (0xF^mask) | (joypad_select_buttons | joypad_select_directions);
		break;
		case 0xFF01: /* Link port data */
			//fprintf(stderr, "%c", i);
		break;
		case 0xFF02: /* Link port setup */
			// Todo
		break;
		case 0xFF04:
			return timer_get_div();
		break;
		case 0xFF05:
			return timer_get_counter();
		break;
		case 0xFF06:
			return timer_get_modulo();
		break;
		case 0xFF07:
			return timer_get_tac();
		break;
		case 0xFF0F:
			return interrupt_get_IF();
		break;
		case 0xFF41:
			return lcd_get_stat();
		break;
		case 0xFF44:
			return lcd_get_line();
		break;
		case 0xFF45:
			return lcd_get_ly_compare();
		break;
		case 0xFF4D:	/* GBC speed switch */
			return 0xFF;
		break;
		case 0xFFFF:
			return interrupt_get_mask();
		break;
	}

	if (i >= 0xFF00) {
		return topmem[i-0xFF00];
	}
	return 0;
}

unsigned short mem_get_word(unsigned short i)
{
	unsigned long elapsed;

	if(DMA_pending && i < 0xFF80)
	{
		elapsed = c.cycles - DMA_pending;
		if(elapsed >= 160)
			DMA_pending = 0;
		else
		{
			return oammem[elapsed];
		}
	}

	return mem_get_raw(i) | (mem_get_raw(i+1)<<8);
}

inline void mem_write_raw(unsigned short d, unsigned char i)
{
	if(d >= 0xFF00) {                       // Top ram
		topmem[d-0xFF00] = i;
	} else if (d < 0xFEA0 && d >= 0xFE00) { // OAM memory
		oammem[d-0xFE00] = i;
	} else if (d < 0xFE00 && d >= 0xE000) { // Shadow work ram 
		mainmem[d-0xE000] = i;
	} else if (d < 0xE000 && d >= 0xC000) { // Work ram
		mainmem[d-0xC000] = i;
	} else if (d < 0xC000 && d >= 0xA000) { // External ram
		extmem[d-0xA000] = i;
	} else if (d < 0xA000 && d >= 0x8000) { // Video ram
		vmem[d-0x8000] = i;
	}
}

void mem_write_byte(unsigned short d, unsigned char i)
{
	unsigned int filtered = 0;

	switch(rom_get_mapper())
	{
		case NROM:
			if(d < 0x8000)
				filtered = 1;
		break;
		case MBC2:
		case MBC3:
			filtered = MBC3_write_byte(d, i);
		break;
		case MBC1:
			filtered = MBC1_write_byte(d, i);
		break;
	}

	if(filtered)
		return;

	switch(d)
	{
		case 0xFF00:	/* Joypad */
			joypad_select_buttons = i&0x20;
			joypad_select_directions = i&0x10;
		break;
		case 0xFF01: /* Link port data */
			//fprintf(stderr, "%c", i);
		break;
		case 0xFF02: /* Link port setup */
			// Todo
		break;
		case 0xFF04:
			timer_set_div(i);
		break;
		case 0xFF05:
			timer_set_counter(i);
		break;
		case 0xFF06:
			timer_set_modulo(i);
		break;
		case 0xFF07:
			timer_set_tac(i);
		break;
		case 0xFF0F:
			interrupt_set_IF(i);
		break;
		case 0xFF40:
			lcd_write_control(i);
		break;
		case 0xFF41:
			lcd_write_stat(i);
		break;
		case 0xFF42:
			lcd_write_scroll_y(i);
		break;
		case 0xFF43:
			lcd_write_scroll_x(i);
		break;
		case 0xFF45:
			lcd_set_ly_compare(i);
		break;
		case 0xFF46: /* OAM DMA */
			/* Copy bytes from i*0x100 to OAM */
			//memcpy(&mem[0xFE00], &mem[i*0x100], 0xA0);
			for (int at=0;at<0xA0;at++) oammem[at] = mem_get_byte((i*0x100)+at);
			DMA_pending = c.cycles;
		break;
		case 0xFF47:
			lcd_write_bg_palette(i);
		break;
		case 0xFF48:
			lcd_write_spr_palette1(i);
		break;
		case 0xFF49:
			lcd_write_spr_palette2(i);
		break;
		case 0xFF4A:
			lcd_set_window_y(i); break;
		case 0xFF4B:
			lcd_set_window_x(i); break;
		case 0xFFFF:
			interrupt_set_mask(i);
			return;
		break;
	}
	

	if(d >= 0xFF00) {                       // Top ram
		topmem[d-0xFF00] = i;
	} else if (d < 0xFEA0 && d >= 0xFE00) { // OAM memory
		oammem[d-0xFE00] = i;
	} else if (d < 0xFE00 && d >= 0xE000) { // Shadow work ram 
		mainmem[d-0xE000] = i;
	} else if (d < 0xE000 && d >= 0xC000) { // Work ram
		mainmem[d-0xC000] = i;
	} else if (d < 0xC000 && d >= 0xA000) { // External ram
		extmem[d-0xA000] = i;
	} else if (d < 0xA000 && d >= 0x8000) { // Video ram
		vmem[d-0x8000] = i;
	}/* else if (d < 0x8000) {              // Cartradge rom
		return rom_getbytes()[d];
	}*/
	//mem[d] = i;
}

void mem_write_word(unsigned short d, unsigned short i)
{
	mem_write_raw(d, i&0xFF);
	mem_write_raw(d+1, i>>8);
}

void mem_init(void)
{
	//unsigned char *bytes = rom_getbytes();

	vmem = calloc(1, 8192);
	extmem = calloc(1, 8192);
	mainmem = calloc(1, 8192);
	oammem = calloc(1, 160);
	topmem = calloc(1, 256);

	//memcpy(&mem[0x0000], &bytes[0x0000], 0x4000);
	//memcpy(&mem[0x4000], &bytes[0x4000], 0x4000);

	topmem[(0xFF10)-(0xFF00)] = 0x80;
	topmem[(0xFF11)-(0xFF00)] = 0xBF;
	topmem[(0xFF12)-(0xFF00)] = 0xF3;
	topmem[(0xFF14)-(0xFF00)] = 0xBF;
	topmem[(0xFF16)-(0xFF00)] = 0x3F;
	topmem[(0xFF19)-(0xFF00)] = 0xBF;
	topmem[(0xFF1A)-(0xFF00)] = 0x7F;
	topmem[(0xFF1B)-(0xFF00)] = 0xFF;
	topmem[(0xFF1C)-(0xFF00)] = 0x9F;
	topmem[(0xFF1E)-(0xFF00)] = 0xBF;
	topmem[(0xFF20)-(0xFF00)] = 0xFF;
	topmem[(0xFF23)-(0xFF00)] = 0xBF;
	topmem[(0xFF24)-(0xFF00)] = 0x77;
	topmem[(0xFF25)-(0xFF00)] = 0xF3;
	topmem[(0xFF26)-(0xFF00)] = 0xF1;
	topmem[(0xFF40)-(0xFF00)] = 0x91;
	topmem[(0xFF47)-(0xFF00)] = 0xFC;
	topmem[(0xFF48)-(0xFF00)] = 0xFF;
	topmem[(0xFF49)-(0xFF00)] = 0xFF;
}

void mem_free()
{
	free(vmem);
	free(extmem);
	free(mainmem);
	free(oammem);
	free(topmem);
}