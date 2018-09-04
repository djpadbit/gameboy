#include "lcd.h"
#include "cpu.h"
#include "interrupt.h"
#include "sdl.h"
#include "mem.h"
#include <display.h>
#include <string.h>
#include "disp.h"

static int lcd_line;
static int lcd_ly_compare;


/* LCD STAT */
static int ly_int;	/* LYC = LY coincidence interrupt enable */
static int oam_int;
static int vblank_int;
static int hblank_int;
static int lcd_mode;

/* LCD Control */
static int lcd_enabled;
static int window_tilemap_select;
static int window_enabled;
static int tilemap_select;
static int bg_tiledata_select;
static int sprite_size;
static int sprites_enabled;
static int bg_enabled;
static int scroll_x, scroll_y;
static int window_x, window_y;

static int bgpalette[] = {3, 2, 1, 0};
static int sprpalette1[] = {0, 1, 2, 3};
static int sprpalette2[] = {0, 1, 2, 3};
//static unsigned long colours[4] = {0xFFFFFF, 0xC0C0C0, 0x808080, 0x000000};
//static color_t colours[4] = {color_black,color_dark,color_light,color_white};

static int xoff=29;
static int yoff=0;

static unsigned char scalearrx[160];
static unsigned char scalearry[144];

struct sprite {
	int y, x, tile, flags;
};

enum {
	PRIO  = 0x80,
	VFLIP = 0x40,
	HFLIP = 0x20,
	PNUM  = 0x10
};

void lcd_get_conf(struct lcd_config *dst)
{
	dst->lcd_line = lcd_line;
	dst->lcd_ly_compare = lcd_ly_compare;
	dst->oam_int = oam_int;
	dst->vblank_int = vblank_int;
	dst->hblank_int = hblank_int;
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
	memcpy(&dst->bgpalette,&bgpalette,sizeof(bgpalette));
	memcpy(&dst->sprpalette1,&sprpalette1,sizeof(sprpalette1));
	memcpy(&dst->sprpalette2,&sprpalette2,sizeof(sprpalette2));
}

void lcd_set_conf(struct lcd_config *src)
{
	lcd_line = src->lcd_line;
	lcd_ly_compare = src->lcd_ly_compare;
	oam_int = src->oam_int;
	vblank_int = src->vblank_int;
	hblank_int = src->hblank_int;
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
	memcpy(&bgpalette,&src->bgpalette,sizeof(bgpalette));
	memcpy(&sprpalette1,&src->sprpalette1,sizeof(sprpalette1));
	memcpy(&sprpalette2,&src->sprpalette2,sizeof(sprpalette2));
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

unsigned char lcd_get_stat(void)
{
	unsigned char coincidence = (lcd_line == lcd_ly_compare) << 2;
	return 0x80 | ly_int | oam_int | vblank_int | hblank_int | coincidence | lcd_mode;
}

void lcd_write_stat(unsigned char c)
{
	ly_int     = c&0x40;
	oam_int    = c&0x20;
	vblank_int = c&0x10;
	hblank_int = c&0x08;
}

void lcd_write_control(unsigned char c)
{
	bg_enabled            = !!(c & 0x01);
	sprites_enabled       = !!(c & 0x02);
	sprite_size           = !!(c & 0x04);
	tilemap_select        = !!(c & 0x08);
	bg_tiledata_select    = !!(c & 0x10);
	window_enabled        = !!(c & 0x20);
	window_tilemap_select = !!(c & 0x40);
	lcd_enabled           = !!(c & 0x80);
}

unsigned char lcd_get_ly_compare(void)
{
	return lcd_ly_compare;
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

static inline void swap(struct sprite *a, struct sprite *b)
{
	struct sprite c;

	 c = *a;
	*a = *b;
	*b =  c;
}

static inline void sort_sprites(struct sprite *s, int n)
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
	memset(scalearrx,250,160);
	memset(scalearry,250,144);
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

static void draw_bg_and_window(int line, int ox, int d)
{
	int x;

	for(x = ox; x < ox+d; x++)
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
		//sdpixel(x,line,colours[bgpalette[colour]]);
		sdpixel(x,line,bgpalette[colour] <= 2 ? color_white : color_black);
	}
}

static inline void draw_sprites(int line, int nsprites, struct sprite *s)
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

static void render_line(int line, int x, int d)
{
	int i;
	static int c;

	static struct sprite s[10];
	//unsigned int *b = sdl_get_framebuffer();

	/* Pretend this is lcd mode 2, see if there will be sprites this line */
	if(x == 0)
	{
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
	}

	/* Draw the background layer */
	draw_bg_and_window(line, x, d);

	/* If there were sprites this line, paste them over what we drew */
	if(x+d >= 160 && c)
	{
		draw_sprites(line, c, s);
		c = 0;
	}
	/*b[line*640+168] =
	b[line*640+169] = bg_tiledata_select ? 0x00ff00 : 0xff0000;
	b[line*640+172] =
	b[line*640+173] = tilemap_select ? 0x00ff00 : 0xff0000;*/
}
/*
static void draw_stuff(void)
{
	unsigned int *b = sdl_get_framebuffer();
	int y, tx, ty, mx, my;
	int x1, y1;

	// tile data 
	x1 = 320;
	y1 = 0;
	for(ty = 0; ty < 24; ty++)
	{
	for(tx = 0; tx < 16; tx++)
	{
	for(y = 0; y<8; y++)
	{
		unsigned char b1, b2;
		int tileaddr = 0x8000 +  ty*0x100 + tx*16 + y*2;

		b1 = mem_get_raw(tileaddr);
		b2 = mem_get_raw(tileaddr+1);
		b[(ty*640*8)+(tx*8) + (y*640) + x1 + (y1*640) + 0] = colours[(!!(b1&0x80))<<1 | !!(b2&0x80)];
		b[(ty*640*8)+(tx*8) + (y*640) + x1 + (y1*640) + 1] = colours[(!!(b1&0x40))<<1 | !!(b2&0x40)];
		b[(ty*640*8)+(tx*8) + (y*640) + x1 + (y1*640) + 2] = colours[(!!(b1&0x20))<<1 | !!(b2&0x20)];
		b[(ty*640*8)+(tx*8) + (y*640) + x1 + (y1*640) + 3] = colours[(!!(b1&0x10))<<1 | !!(b2&0x10)];
		b[(ty*640*8)+(tx*8) + (y*640) + x1 + (y1*640) + 4] = colours[(!!(b1&0x8))<<1 | !!(b2&0x8)];
		b[(ty*640*8)+(tx*8) + (y*640) + x1 + (y1*640) + 5] = colours[(!!(b1&0x4))<<1 | !!(b2&0x4)];
		b[(ty*640*8)+(tx*8) + (y*640) + x1 + (y1*640) + 6] = colours[(!!(b1&0x2))<<1 | !!(b2&0x2)];
		b[(ty*640*8)+(tx*8) + (y*640) + x1 + (y1*640) + 7] = colours[(!!(b1&0x1))<<1 | !!(b2&0x1)];
	}
	}
	}

	// bg1 map 
	x1 = 8;
	y1 = 200;
	for(my = 0; my < 32; my++)
	{
	for(mx = 0; mx < 32; mx++)
	{
	for(y = 0; y<8; y++)
	{
		unsigned char b1, b2, t;
		int mapaddr = 0x9800 + my*32 + mx;
		int tileaddr;

		t = mem_get_raw(mapaddr);

		tileaddr = 0x9000 + (signed char)t*16;
		b1 = mem_get_raw(tileaddr+y*2);
		b2 = mem_get_raw(tileaddr+y*2+1);
		b[(my*640*8)+(mx*8) + (y*640) + x1 + y1*640 + 0] = colours[(!!(b1&0x80))<<1 | !!(b2&0x80)];
		b[(my*640*8)+(mx*8) + (y*640) + x1 + y1*640 + 1] = colours[(!!(b1&0x40))<<1 | !!(b2&0x40)];
		b[(my*640*8)+(mx*8) + (y*640) + x1 + y1*640 + 2] = colours[(!!(b1&0x20))<<1 | !!(b2&0x20)];
		b[(my*640*8)+(mx*8) + (y*640) + x1 + y1*640 + 3] = colours[(!!(b1&0x10))<<1 | !!(b2&0x10)];
		b[(my*640*8)+(mx*8) + (y*640) + x1 + y1*640 + 4] = colours[(!!(b1&0x8))<<1 | !!(b2&0x8)];
		b[(my*640*8)+(mx*8) + (y*640) + x1 + y1*640 + 5] = colours[(!!(b1&0x4))<<1 | !!(b2&0x4)];
		b[(my*640*8)+(mx*8) + (y*640) + x1 + y1*640 + 6] = colours[(!!(b1&0x2))<<1 | !!(b2&0x2)];
		b[(my*640*8)+(mx*8) + (y*640) + x1 + y1*640 + 7] = colours[(!!(b1&0x1))<<1 | !!(b2&0x1)];
	}
	}
	}

	// bg2(win) map 
	x1 = 256+16;
	y1 = 200;
	for(my = 0; my < 32; my++)
	{
	for(mx = 0; mx < 32; mx++)
	{
	for(y = 0; y<8; y++)
	{
		unsigned char b1, b2, t;
		int mapaddr = 0x9c00 + my*32 + mx;
		int tileaddr;

		t = mem_get_raw(mapaddr);

		tileaddr = 0x8000 + t*16;
		b1 = mem_get_raw(tileaddr+y*2);
		b2 = mem_get_raw(tileaddr+y*2+1);
		b[(my*640*8)+(mx*8) + (y*640) + x1 + y1*640 + 0] = colours[(!!(b1&0x80))<<1 | !!(b2&0x80)];
		b[(my*640*8)+(mx*8) + (y*640) + x1 + y1*640 + 1] = colours[(!!(b1&0x40))<<1 | !!(b2&0x40)];
		b[(my*640*8)+(mx*8) + (y*640) + x1 + y1*640 + 2] = colours[(!!(b1&0x20))<<1 | !!(b2&0x20)];
		b[(my*640*8)+(mx*8) + (y*640) + x1 + y1*640 + 3] = colours[(!!(b1&0x10))<<1 | !!(b2&0x10)];
		b[(my*640*8)+(mx*8) + (y*640) + x1 + y1*640 + 4] = colours[(!!(b1&0x8))<<1 | !!(b2&0x8)];
		b[(my*640*8)+(mx*8) + (y*640) + x1 + y1*640 + 5] = colours[(!!(b1&0x4))<<1 | !!(b2&0x4)];
		b[(my*640*8)+(mx*8) + (y*640) + x1 + y1*640 + 6] = colours[(!!(b1&0x2))<<1 | !!(b2&0x2)];
		b[(my*640*8)+(mx*8) + (y*640) + x1 + y1*640 + 7] = colours[(!!(b1&0x1))<<1 | !!(b2&0x1)];
	}
	}
	}
}*/

int lcd_cycle(void)
{
	int cycles = c.cycles;
	int this_frame, subframe_cycles;
	static int prev_line, prev_mode, prev_cycles;
	static int /*draw_line,*/ drawn_pixels;

	this_frame = c.cycles % (70224/4);
	lcd_line = this_frame / (456/4);
	subframe_cycles = this_frame % (456/4);

	if(subframe_cycles < 80/4)
		lcd_mode = 2;
	else if(subframe_cycles < 252/4)
		lcd_mode = 3;
	else if(subframe_cycles < 456/4)
		lcd_mode = 0;
	if(lcd_line >= 144)
		lcd_mode = 1;

	/* Check if any STAT interrupts need to fire */
	if(ly_int && lcd_line == lcd_ly_compare)
		interrupt(INTR_LCDSTAT);
	else if(hblank_int && lcd_mode == 0 && prev_mode != 0)
		interrupt(INTR_LCDSTAT);
	else if(vblank_int && lcd_mode == 1 && prev_mode != 1)
		interrupt(INTR_LCDSTAT);
	else if(oam_int && lcd_mode == 2 && prev_mode != 2)
		interrupt(INTR_LCDSTAT);

	{
		int x/*, y*/;
		//int delta;
		//int start;

		/* Actual x position of the 'real' LCD */
		x = subframe_cycles*4 - 80;

		/* New line has started, reset imaginary raster position and finish previous line if needed */
		if(lcd_line != prev_line)
		{
			/* Line is unfinished and the current (prev_line) scanline is onscreen */
			if(drawn_pixels < 160 && prev_line < 144)
				render_line(prev_line, drawn_pixels, 160-drawn_pixels);
			drawn_pixels = 0;
		}

		/* Imaginary raster position is onscreen, draw some pixels to catch up */
		if(x > 0 && x < 160 && lcd_line < 144)
		{
//			printf(GREEN "%d %d %d %d\n" RESET, lcd_line, drawn_pixels, x - drawn_pixels, x);
			render_line(lcd_line, drawn_pixels, x - drawn_pixels);
			/* Update imaginary raster position to real raster position */
			drawn_pixels = x;
		}
	}

	/*if (timertime%256==0) {
		dclear();
		sprintf(k,"%i", lcd_line);locate(1,6,k);
		sprintf(k,"%i", cycles);locate(1,7,k);
		dupdate();
	}*/

	if(prev_line == 143 && lcd_line == 144)
	{
		//draw_stuff();
		sdl_frame();
		interrupt(INTR_VBLANK);
		if(sdl_update())
			return 0;
		//memset(sdl_get_framebuffer(), 0x440022, 640*144*4);
	}

	prev_line = lcd_line;
	prev_mode = lcd_mode;

	prev_cycles = cycles;

	return 1;
}

