//#include <SDL/SDL.h>
//#include <sys/time.h>
#include <stdio.h>
#include <events.h>
#include <keyboard.h>
#include <display.h>
#include "disp.h"
#include "timek.h"
#include "lcd.h"
#include "save.h"
//static SDL_Surface *screen;
static unsigned int frames,framet1,framet2;
//static struct timeval tv1, tv2;
static int button_start, button_select, button_a, button_b, button_down, button_up, button_left, button_right;
static int debug_info;
static int scalex,scaley;
static int updel,downdel,leftdel,rightdel;
int mode;

void sdl_init(void)
{
	keyboard_init();
	mode = 0;
	debug_info = 1;
	scalex = 71;
	scaley = 64;
	lcd_gen_scale_arr(scalex,scaley);
	//gray_start();
	//SDL_Init(SDL_INIT_VIDEO);
	//screen = SDL_SetVideoMode(640, 480, 32, SDL_HWSURFACE | SDL_DOUBLEBUF);
}

int sdl_update(void)
{
	event_t e;
	e = pollevent();
	while(1)
	{
		if (e.type == event_none) 
			break;

		if (e.type == event_key_press) {
			switch (e.key.code) {
				case KEY_MENU:
					mode = 3;
					break;
				case KEY_F1:
					mode = 0;
					break;
				case KEY_F2:
					mode = 1;
					break;
				case KEY_F3:
					mode = 2;
					break;
				case KEY_F4:
					lcd_set_off(29,0);
					scalex = 71;
					scaley = 64;
					lcd_gen_scale_arr(scalex,scaley);
					break;
				case KEY_F5:
					debug_info = !debug_info;
					break;
				case KEY_SHIFT:
					button_a = 1;
					break;
				case KEY_ALPHA:
					button_b = 1;
					break;
				case KEY_OPTN:
					button_select = 1;
					break;
				case KEY_VARS:
					button_start = 1;
					break;
				case KEY_LEFT:
					if (!mode) button_left = 1;
					else if (mode == 1) lcd_set_off(lcd_get_xoff()-1,lcd_get_yoff());
					else if (mode == 2 && scalex-1 > 0) lcd_gen_scale_arr((scalex--)-1,scaley);
					break;
				case KEY_RIGHT:
					if (!mode) button_right = 1;
					else if (mode == 1) lcd_set_off(lcd_get_xoff()+1,lcd_get_yoff());
					else if (mode == 2 && scalex < 160) lcd_gen_scale_arr((scalex++)+1,scaley);
					break;
				case KEY_DOWN:
					if (!mode) button_down = 1;
					else if (mode == 1) lcd_set_off(lcd_get_xoff(),lcd_get_yoff()+1);
					else if (mode == 2 && scaley < 144) lcd_gen_scale_arr(scalex,(scaley++)+1);
					break;
				case KEY_UP:
					if (!mode) button_up = 1;
					else if (mode == 1) lcd_set_off(lcd_get_xoff(),lcd_get_yoff()-1);
					else if (mode == 2 && scaley-1 > 0) lcd_gen_scale_arr(scalex,(scaley--)-1);
					break;
			}
		}

		if (e.type == event_key_repeat) {
			switch (e.key.code) {
				case KEY_LEFT:
					if (mode == 1 && leftdel > 20 && (leftdel % 2) == 0) lcd_set_off(lcd_get_xoff()-1,lcd_get_yoff());
					else if (mode == 2 && leftdel > 20 && (leftdel % 2) == 0 && scalex-1 > 0) lcd_gen_scale_arr(scalex--,scaley);
					leftdel++;
					break;
				case KEY_RIGHT:
					if (mode == 1 && rightdel > 20 && (rightdel % 2) == 0) lcd_set_off(lcd_get_xoff()+1,lcd_get_yoff());
					else if (mode == 2 && rightdel > 20 && (rightdel % 2) == 0 && scalex < 160) lcd_gen_scale_arr(scalex++,scaley);
					rightdel++;
					break;
				case KEY_DOWN:
					if (mode == 1 && downdel > 20 && (downdel % 2) == 0) lcd_set_off(lcd_get_xoff(),lcd_get_yoff()+1);
					else if (mode == 2 && downdel > 20 && (downdel % 2) == 0 && scaley < 144) lcd_gen_scale_arr(scalex,scaley++);
					downdel++;
					break;
				case KEY_UP:
					if (mode == 1 && updel > 20 && (updel % 2) == 0) lcd_set_off(lcd_get_xoff(),lcd_get_yoff()-1);
					else if (mode == 2 && updel > 20 && (updel % 2) == 0 && scaley-1 > 0) lcd_gen_scale_arr(scalex,scaley--);
					updel++;
					break;
			}
		}

		if (e.type == event_key_release) {
			switch (e.key.code) {
				case KEY_SHIFT:
					button_a = 0;
					break;
				case KEY_ALPHA:
					button_b = 0;
					break;
				case KEY_OPTN:
					button_select = 0;
					break;
				case KEY_VARS:
					button_start = 0;
					break;
				case KEY_LEFT:
					if (!mode) button_left = 0;
					else leftdel = 0;
					break;
				case KEY_RIGHT:
					if (!mode) button_right = 0;
					else rightdel = 0;
					break;
				case KEY_DOWN:
					if (!mode) button_down = 0;
					else downdel = 0;
					break;
				case KEY_UP:
					if (!mode) button_up = 0;
					else updel = 0;
					break;
			}
		}
		e = pollevent();
	}

	return 0;
}

unsigned int sdl_get_buttons(void)
{
	return (button_start*8) | (button_select*4) | (button_b*2) | button_a;
}

unsigned int sdl_get_directions(void)
{
	return (button_down*8) | (button_up*4) | (button_left*2) | button_right;
}
/*
unsigned int *sdl_get_framebuffer(void)
{
	//return screen->pixels;
}*/

void sdl_frame(void)
{
	framet2 = timertime;
	if (debug_info) {
		int fps = 1.0/((framet2-framet1)/1024.0);
		mprint(1,1,"%i",fps);
		mprint(1,2,"%i",mode);
		mprint(1,3,"%i",lcd_get_xoff());
		mprint(1,4,"%i",lcd_get_yoff());
		mprint(1,5,"%i",scalex);
		mprint(1,6,"%i",scaley);
	}
	dupdate();
	dclear();
	framet1 = timertime;
	frames++;
}

int sdl_menu()
{
	int sel=0,key;
	while (1) {
		mprint(1,1,"Menu:");
		mprint(1,2,"Load Savestate");
		mprint(1,3,"Save Savestate");
		mprint(1,4,"Quit");
		//mprint(1,5,"%i",sel);
		drect(1, (sel+2) * 8 - 8,22 * 6 - 5, ((sel+3) * 8 - 8)-1,color_invert);
		dupdate();
		dclear();
		key = getkey_opt(getkey_none,0);
		switch (key) {
			case KEY_EXIT:
				mode = 0;
				return 0;
				break;
			case KEY_DOWN:
				if (sel < 2) sel++;
				break;
			case KEY_UP:
				if (sel > 0) sel--;
				break;
			case KEY_EXE:
				if (sel==0) save_load_state();
				if (sel==1) save_save_state();
				if (sel==2) return 1;
				break;
		}
	}
}

void sdl_quit()
{
	//SDL_Quit();
}