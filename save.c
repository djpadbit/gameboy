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
	int lcd_line;
	int lcd_ly_compare;
	int ly_int;
	int lcd_mode;
	int lcd_enabled;
	int window_tilemap_select;
	int window_enabled;
	int tilemap_select;
	int bg_tiledata_select;
	int sprite_size;
	int sprites_enabled;
	int bg_enabled;
	int scroll_x, scroll_y;
	int window_x, window_y;
	int bgpalette[4];
	int sprpalette1[4];
	int sprpalette2[4];
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
	dst->lcd_line = lcd_line;
	dst->lcd_ly_compare = lcd_ly_compare;
	dst->ly_int = ly_int;
	dst->lcd_mode = lcd_mode;
	dst->lcd_enabled = lcd_enabled;
	dst->window_tilemap_select = window_tilemap_select;
	dst->window_enabled = window_enabled;
	dst->tilemap_select = tilemap_select;
	dst->bg_tiledata_select = bg_tiledata_select;
	dst->sprite_size = sprite_size;
	dst->sprites_enabled = sprites_enabled;
	dst->bg_enabled = bg_enabled;
	dst->scroll_x = scroll_x;
	dst->scroll_y = scroll_y;
	dst->window_x = window_x;
	dst->window_y = window_y;
	memcpy(bgpalette,dst->bgpalette,sizeof(bgpalette));
	memcpy(sprpalette1,dst->sprpalette1,sizeof(sprpalette1));
	memcpy(sprpalette2,dst->sprpalette2,sizeof(sprpalette2));
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
	lcd_ly_compare = src->lcd_ly_compare;
	ly_int = src->ly_int;
	lcd_mode = src->lcd_mode;
	lcd_enabled = src->lcd_enabled;
	window_tilemap_select = src->window_tilemap_select;
	window_enabled = src->window_enabled;
	tilemap_select = src->tilemap_select;
	bg_tiledata_select = src->bg_tiledata_select;
	sprite_size = src->sprite_size;
	sprites_enabled = src->sprites_enabled;
	bg_enabled = src->bg_enabled;
	scroll_x = src->scroll_x;
	scroll_y = src->scroll_y;
	window_x = src->window_x;
	window_y = src->window_y;
	memcpy(src->bgpalette,bgpalette,sizeof(bgpalette));
	memcpy(src->sprpalette1,sprpalette1,sizeof(sprpalette1));
	memcpy(src->sprpalette2,sprpalette2,sizeof(sprpalette2));
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