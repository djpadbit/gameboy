#include "cpu.h"
#include "mem.h"
#include "file.h"
#include "disp.h"
#include "lcd.h"
#include <bfile.h>
#include <display.h>
#include <keyboard.h>
#include <string.h>

struct save
{
	//MEM config
	int DMA_pending, joypad_select_buttons, joypad_select_directions;
	unsigned int bank;
	//CPU config
	struct CPU c;
	int is_debugged,halted;
	//LCD config
	struct lcd_config lcd_c;
};

void gen_save(struct save *dst)
{
	dst->DMA_pending = DMA_pending;
	dst->joypad_select_directions = joypad_select_directions;
	dst->joypad_select_buttons = joypad_select_buttons;
	dst->bank = bank;
	dst->c = c;
	dst->is_debugged = is_debugged;
	dst->halted = halted;
	lcd_get_conf(&dst->lcd_c);
}

void apply_save(struct save *src)
{
	DMA_pending = src->DMA_pending;
	joypad_select_directions = src->joypad_select_directions;
	joypad_select_buttons = src->joypad_select_buttons;
	bank = src->bank;
	c = src->c;
	is_debugged = src->is_debugged;
	halted = src->halted;
	lcd_set_conf(&src->lcd_c);
}

int save_file(char* name)
{
	uint16_t path[64];
	file_make_path(path,"fls0","",name);
	int total_size = sizeof(struct save)+256+160+(8192*3);
	total_size += total_size%2;
	struct save s;
	gen_save(&s);
	int fd = BFile_Open(path,BFile_WriteOnly);
	if (fd > 0) {
		BFile_Close(fd);
		BFile_Remove(path);
	}
	fd = BFile_Create(path,BFile_File,&total_size);
	if (fd < 0) return -1;
	fd = BFile_Open(path,BFile_WriteOnly);
	BFile_Write(fd,topmem,256);
	BFile_Write(fd,oammem,160);
	BFile_Write(fd,mainmem,8192);
	BFile_Write(fd,extmem,8192);
	BFile_Write(fd,vmem,8192);
	BFile_Write(fd,&s,sizeof(struct save)+(sizeof(struct save)%2));
	BFile_Close(fd);
	return 0;
}

int load_file(char* name)
{
	uint16_t path[64];
	file_make_path(path,"fls0","",name);
	struct save s;
	int fd = BFile_Open(path,BFile_ReadOnly);
	if (fd < 0) return -1;
	BFile_Read(fd,topmem,256,0);
	BFile_Read(fd,oammem,160,-1);
	BFile_Read(fd,mainmem,8192,-1);
	BFile_Read(fd,extmem,8192,-1);
	BFile_Read(fd,vmem,8192,-1);
	BFile_Read(fd,&s,sizeof(struct save)+(sizeof(struct save)%2),-1);
	BFile_Close(fd);
	apply_save(&s);
	return 0;
}

void save_save_state()
{
	char saven[20];
	keyb_input((char*)&saven,"Enter save file name");
	dclear();
	if (save_file((char*)&saven) < 0) {
		mprint(1,1,"Failed to make file");
		mprint(1,2,"Not enough space ?");
		mprint(1,3,"You need 25k free");
		dupdate();
		dclear();
		getkey_opt(getkey_none,0);
		return;
	}
	mprint(1,1,"Finished saving");
	dupdate();
	dclear();
	getkey_opt(getkey_none,0);
}

void save_load_state()
{
	char saven[20];
	keyb_input((char*)&saven,"Enter save file name");
	dclear();
	if (load_file((char*)&saven) < 0) {
		mprint(1,1,"Failed to load file");
		mprint(1,2,"either file not found");
		mprint(1,3,"or it's gonna crash");
		dupdate();
		dclear();
		getkey_opt(getkey_none,0);
		return;
	}
	mprint(1,1,"Finished loading");
	dupdate();
	dclear();
	getkey_opt(getkey_none,0);
}