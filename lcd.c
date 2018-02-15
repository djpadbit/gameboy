#include "lcd.h"
#include "cpu.h"
#include "interrupt.h"
#include "sdl.h"
#include "mem.h"
#include <display.h>
#include "disp.h"

int lcd_line;
int lcd_ly_compare;


/* LCD STAT */
int ly_int;	/* LYC = LY coincidence interrupt enable */
/*static int mode2_oam_int;
static int mode1_vblank_int;
static int mode0_hblank_int;
static int ly_int_flag; Not used */
int lcd_mode;

/* LCD Control */
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

int bgpalette[] = {3, 2, 1, 0};
int sprpalette1[] = {0, 1, 2, 3};
int sprpalette2[] = {0, 1, 2, 3};
//static unsigned long colours[4] = {0xFFFFFF, 0xC0C0C0, 0x808080, 0x000000};
//static color_t colours[4] = {color_black,color_dark,color_light,color_white};

static int xoff=29;
static int yoff=0;

static unsigned char scalearrx[160]; /*= {0, 250, 1, 250, 2, 250, 3, 250, 250, 4, 250, 5, 250, 6, 250, 250, 7, 250, 8, 250, 9, 250, 10, 250, 250, 11, 250, 12, 250, 13, 250,
                                       250, 14, 250, 15, 250, 16, 250, 17, 250, 250, 18, 250, 19, 250, 20, 250, 250, 21, 250, 22, 250, 23, 250, 24, 250, 250, 25, 250, 26, 250,
                                       27, 250, 250, 28, 250, 29, 250, 30, 250, 31, 250, 250, 32, 250, 33, 250, 34, 250, 250, 35, 250, 36, 250, 37, 250, 38, 250, 250, 39, 250,
                                       40, 250, 41, 250, 250, 42, 250, 43, 250, 44, 250, 45, 250, 250, 46, 250, 47, 250, 48, 250, 250, 49, 250, 50, 250, 51, 250, 52, 250, 250,
                                       53, 250, 54, 250, 55, 250, 250, 56, 250, 57, 250, 58, 250, 59, 250, 250, 60, 250, 61, 250, 62, 250, 250, 63, 250, 64, 250, 65, 250, 66,
                                       250, 250, 67, 250, 68, 250, 69, 250, 250};*/

static unsigned char scalearry[144];/* = {0, 250, 1, 250, 2, 250, 3, 250, 250, 4, 250, 5, 250, 6, 250, 250, 7, 250, 8, 250, 9, 250, 10, 250, 250, 11, 250, 12, 250, 13, 250,
                                       250, 14, 250, 15, 250, 16, 250, 17, 250, 250, 18, 250, 19, 250, 20, 250, 250, 21, 250, 22, 250, 23, 250, 24, 250, 250, 25, 250, 26, 250,
                                       27, 250, 250, 28, 250, 29, 250, 30, 250, 31, 250, 250, 32, 250, 33, 250, 34, 250, 250, 35, 250, 36, 250, 37, 250, 38, 250, 250, 39, 250,
                                       40, 250, 41, 250, 250, 42, 250, 43, 250, 44, 250, 45, 250, 250, 46, 250, 47, 250, 48, 250, 250, 49, 250, 50, 250, 51, 250, 52, 250, 250,
                                       53, 250, 54, 250, 55, 250, 250, 56, 250, 57, 250, 58, 250, 59, 250, 250, 60, 250, 61, 250, 62, 250, 250};*/

struct sprite {
	int y, x, tile, flags;
};

enum {
	PRIO  = 0x80,
	VFLIP = 0x40,
	HFLIP = 0x20,
	PNUM  = 0x10
};

unsigned char lcd_get_stat(void)
{

	return (ly_int)<<6 | lcd_mode;
}

void lcd_write_bg_palette(unsigned char n)
{
	bgpalette[0] = (n>>0)&3;
	bgpalette[1] = (n>>2)&3;
	bgpalette[2] = (n>>4)&3;
	bgpalette[3] = (n>>6)&3;
}

void lcd_write_spr_palette1(unsigned char n)
{
	sprpalette1[0] = 0;
	sprpalette1[1] = (n>>2)&3;
	sprpalette1[2] = (n>>4)&3;
	sprpalette1[3] = (n>>6)&3;
}

void lcd_write_spr_palette2(unsigned char n)
{
	sprpalette2[0] = 0;
	sprpalette2[1] = (n>>2)&3;
	sprpalette2[2] = (n>>4)&3;
	sprpalette2[3] = (n>>6)&3;
}

void lcd_write_scroll_x(unsigned char n)
{
//	printf("x scroll changed to %02x\n", n);
	scroll_x = n;
}

void lcd_write_scroll_y(unsigned char n)
{
	scroll_y = n;
}

int lcd_get_line(void)
{
	return lcd_line;
}

void lcd_write_stat(unsigned char c)
{
	ly_int = !!(c&0x40);
}

void lcd_write_control(unsigned char c)
{
//	printf("LCDC set to %02x\n", c);
//	cpu_print_debug();
	bg_enabled            = !!(c & 0x01);
	sprites_enabled       = !!(c & 0x02);
	sprite_size           = !!(c & 0x04);
	tilemap_select        = !!(c & 0x08);
	bg_tiledata_select    = !!(c & 0x10);
	window_enabled        = !!(c & 0x20);
	window_tilemap_select = !!(c & 0x40);
	lcd_enabled           = !!(c & 0x80);
}

void lcd_set_ly_compare(unsigned char c)
{
	lcd_ly_compare = c;
}

void lcd_set_window_y(unsigned char n) {
	window_y = n;
}

void lcd_set_window_x(unsigned char n) {
	window_x = n;
}

static void swap(struct sprite *a, struct sprite *b)
{
	struct sprite c;

	 c = *a;
	*a = *b;
	*b =  c;
}

static void sort_sprites(struct sprite *s, int n)
{
	int swapped, i;

	do
	{
		swapped = 0;
		for(i = 0; i < n-1; i++)
		{
			if(s[i].x < s[i+1].x)
			{
				swap(&s[i], &s[i+1]);
				swapped = 1;
			}
		}
	}
	while(swapped);
}

void lcd_set_off(int x,int y)
{
	xoff = x;
	yoff = y;
}

int lcd_get_xoff()
{
	return xoff;
}

int lcd_get_yoff()
{
	return yoff;
}

void lcd_gen_scale_arr(unsigned char w,unsigned char h)
{
	for (int i=0;i<160;i++) scalearrx[i] = 250;
	for (int j=0;j<144;j++) scalearry[j] = 250;
	float x_ratio = 160.0/(float)w;
	float y_ratio = 144.0/(float)h;
	for (int i=0;i<h;i++) {
		for (int j=0;j<w;j++) {
			scalearrx[(int)(j*x_ratio)] = j;
			scalearry[(int)(i*y_ratio)] = i;
		}
	}
}

static inline void sdpixel(unsigned char x, unsigned char y, color_t operator)
{
	if (scalearrx[x] != 250 && scalearry[y] != 250) dpixel(xoff+scalearrx[x],yoff+scalearry[y],operator);
}

static void draw_bg_and_window(int line)
{
	int x;

	for(x = 0; x < 160; x++)
	{
		unsigned int map_select, map_offset, tile_num, tile_addr, xm, ym;
		unsigned char b1, b2, mask, colour;

		/* Convert LCD x,y into full 256*256 style internal coords */
		if(line >= window_y && window_enabled && line - window_y < 144)
		{
			xm = x;
			ym = line - window_y;
			map_select = window_tilemap_select;
		}
		else {
			if(!bg_enabled)
			{
				//b[line*640 + x] = 0;
				//dpixel(x,line,color_white);
				//if (scalearrx[x] != 250 && scalearry[line] != 250) dpixel(scalearrx[x],scalearry[line],color_white);
				sdpixel(x,line,color_white);
				return;
			}
			xm = (x + scroll_x)%256;
			ym = (line + scroll_y)%256;
			map_select = tilemap_select;
		}

		/* Which pixel is this tile on? Find its offset. */
		/* (y/8)*32 calculates the offset of the row the y coordinate is on.
		 * As 256/32 is 8, divide by 8 to map one to the other, this is the row number.
		 * Then multiply the row number by the width of a row, 32, to find the offset.
		 * Finally, add x/(256/32) to find the offset within that row. 
		 */
		map_offset = (ym/8)*32 + xm/8;

		tile_num = vmem[0x1800 + map_select*0x400 + map_offset];//mem_get_raw(0x9800 + map_select*0x400 + map_offset);
		if(bg_tiledata_select)
			tile_addr = tile_num*16;
		else
			tile_addr = 0x1000 + ((signed char)tile_num)*16;

		b1 = vmem[tile_addr+(ym%8)*2];//mem_get_raw(tile_addr+(ym%8)*2);
		b2 = vmem[tile_addr+(ym%8)*2+1];//mem_get_raw(tile_addr+(ym%8)*2+1);
		mask = 128>>(xm%8);
		colour = (!!(b2&mask)<<1) | !!(b1&mask);
		//b[line*640 + x] = colours[bgpalette[colour]];
		//dpixel(x,line,bgpalette[colour] <= 2 ? color_black : color_white);
		//if (scalearrx[x] != 250 && scalearry[line] != 250) dpixel(scalearrx[x],scalearry[line],bgpalette[colour] <= 2 ? color_black : color_white);
		//sdpixel(x,line,colours[bgpalette[colour]]); //
		sdpixel(x,line,bgpalette[colour] <= 2 ? color_white : color_black);
	}
}

static void draw_sprites(int line, int nsprites, struct sprite *s)
{
	int i;

	for(i = 0; i < nsprites; i++)
	{
		unsigned int b1, b2, tile_addr, sprite_line, x;

		/* Sprite is offscreen */
		if(s[i].x < -7)
			continue;

		/* Which line of the sprite (0-7) are we rendering */
		sprite_line = s[i].flags & VFLIP ? (sprite_size ? 15 : 7)-(line - s[i].y) : line - s[i].y;

		/* Address of the tile data for this sprite line */
		tile_addr = (s[i].tile*16) + sprite_line*2;

		/* The two bytes of data holding the palette entries */
		b1 = vmem[tile_addr];//mem_get_raw(tile_addr);
		b2 = vmem[tile_addr+1];//mem_get_raw(tile_addr+1);

		/* For each pixel in the line, draw it */
		for(x = 0; x < 8; x++)
		{
			unsigned char mask, colour;
			int *pal;

			if((s[i].x + x) >= 160)
				continue;

			mask = s[i].flags & HFLIP ? 128>>(7-x) : 128>>x;
			colour = ((!!(b2&mask))<<1) | !!(b1&mask);
			if(colour == 0)
				continue;


			pal = (s[i].flags & PNUM) ? sprpalette2 : sprpalette1;
			/* Sprite is behind BG, only render over palette entry 0 */
			if(s[i].flags & PRIO)
			{
				//unsigned int temp = b[line*640+(x + s[i].x)];
				unsigned int temp = 0;
				if(x < 128 && line < 64) {
					if (scalearrx[x] != 250 && scalearry[line] != 250) {
						uint32_t *video	= display_getCurrentVRAM() + (line << 2) + ((x+29) >> 5);
						uint32_t mask	= 0x80000000 >> ((x+29) & 31);
						temp = *video & mask;
					}
				}
				if(temp != bgpalette[0])
					continue;
			}
			//b[line*640+(x + s[i].x)] = colours[pal[colour]];
			//dpixel((x + s[i].x),line,pal[colour] <= 2 ? color_black : color_white);
			//if (scalearrx[(x + s[i].x)] != 250 && scalearry[line] != 250) dpixel(scalearrx[(x + s[i].x)],scalearry[line],pal[colour] <= 2 ? color_black : color_white);
			//sdpixel((x + s[i].x),line,colours[pal[colour]]);//pal[colour] <= 2 ? color_black : color_white);
			sdpixel((x + s[i].x),line,pal[colour] <= 2 ? color_white : color_black);
		}
	}
}

static void render_line(int line)
{
	int i, c = 0;

	struct sprite s[10];
	//unsigned int *b = sdl_get_framebuffer();

	for(i = 0; i<40; i++)
	{
		int y;

		y = oammem[i*4] - 16;//mem_get_raw(0xFE00 + (i*4)) - 16;
		if(line < y || line >= y + 8+(sprite_size*8))
			continue;

		s[c].y     = y;
		s[c].x     = oammem[(i*4)+1]-8;//mem_get_raw(0xFE00 + (i*4) + 1)-8;
		s[c].tile  = oammem[(i*4)+2];//mem_get_raw(0xFE00 + (i*4) + 2);
		s[c].flags = oammem[(i*4)+3];//mem_get_raw(0xFE00 + (i*4) + 3);
		c++;

		if(c == 10)
			break;
	}

	if(c)
		sort_sprites(s, c);

	/* Draw the background layer */
	draw_bg_and_window(line);

	draw_sprites(line, c, s);


}
/*
static void draw_stuff(void)
{
	unsigned int *b = sdl_get_framebuffer();
	int y, tx, ty;

	for(ty = 0; ty < 24; ty++) {
		for(tx = 0; tx < 16; tx++) {
			for(y = 0; y<8; y++) {
				unsigned char b1, b2;
				int tileaddr = 0x8000 +  ty*0x100 + tx*16 + y*2;

				b1 = mem_get_raw(tileaddr);
				b2 = mem_get_raw(tileaddr+1);
				b[(ty*640*8)+(tx*8) + (y*640) + 0 + 0x1F400] = colours[(!!(b1&0x80))<<1 | !!(b2&0x80)];
				b[(ty*640*8)+(tx*8) + (y*640) + 1 + 0x1F400] = colours[(!!(b1&0x40))<<1 | !!(b2&0x40)];
				b[(ty*640*8)+(tx*8) + (y*640) + 2 + 0x1F400] = colours[(!!(b1&0x20))<<1 | !!(b2&0x20)];
				b[(ty*640*8)+(tx*8) + (y*640) + 3 + 0x1F400] = colours[(!!(b1&0x10))<<1 | !!(b2&0x10)];
				b[(ty*640*8)+(tx*8) + (y*640) + 4 + 0x1F400] = colours[(!!(b1&0x8))<<1 | !!(b2&0x8)];
				b[(ty*640*8)+(tx*8) + (y*640) + 5 + 0x1F400] = colours[(!!(b1&0x4))<<1 | !!(b2&0x4)];
				b[(ty*640*8)+(tx*8) + (y*640) + 6 + 0x1F400] = colours[(!!(b1&0x2))<<1 | !!(b2&0x2)];
				b[(ty*640*8)+(tx*8) + (y*640) + 7 + 0x1F400] = colours[(!!(b1&0x1))<<1 | !!(b2&0x1)];
			}
		}
	}
}
*/
int lcd_cycle(void)
{
	int cycles = cpu_get_cycles();
	int this_frame/*, subframe_cycles*/;
	static int prev_line;
	//char k[21];

	this_frame = cycles % (70224/4);
	lcd_line = this_frame / (456/4);

	if(this_frame < 204/4)
		lcd_mode = 2;
	else if(this_frame < 284/4)
		lcd_mode = 3;
	else if(this_frame < 456/4)
		lcd_mode = 0;
	if(lcd_line >= 144)
		lcd_mode = 1;
		
	if(lcd_line != prev_line && lcd_line < 144)
		render_line(lcd_line);

	if(ly_int && lcd_line == lcd_ly_compare)
		interrupt(INTR_LCDSTAT);

	/*if (timertime%256==0) {
		dclear();
		sprintf(k,"%i", lcd_line);locate(1,6,k);
		sprintf(k,"%i", cycles);locate(1,7,k);
		dupdate();
	}*/

	if(prev_line == 143 && lcd_line == 144)
	{
		//draw_stuff();
		interrupt(INTR_VBLANK);
		sdl_frame();
		if(sdl_update())
			return 0;
	}
	prev_line = lcd_line;
	return 1;
}

