#ifndef MEM_H
#define MEM_H

#include "rom.h"
extern unsigned char *topmem;
extern unsigned char *oammem;
extern unsigned char *mainmem;
extern unsigned char *extmem;
extern unsigned char *vmem;
extern int DMA_pending, joypad_select_buttons, joypad_select_directions;
extern unsigned int bank;

void mem_init(void);
unsigned char mem_get_byte(unsigned short);
unsigned short mem_get_word(unsigned short);
void mem_write_byte(unsigned short, unsigned char);
void mem_write_word(unsigned short, unsigned short);
void mem_bank_switch(unsigned int);
unsigned char mem_get_raw(unsigned short);
void mem_write_raw(unsigned short d, unsigned char i);
void mem_free();
#endif
