//#include <SDL/SDL.h>
//#include <sys/time.h>
#include <events.h>
#include <keyboard.h>
#include <display.h>
#include "disp.h"
#include "timek.h"
//static SDL_Surface *screen;
static unsigned int frames,framet1,framet2;
//static struct timeval tv1, tv2;
static int button_start, button_select, button_a, button_b, button_down, button_up, button_left, button_right;
static int mode,debug_info;
static int scalex,scaley;
static int updel,downdel,leftdel,rightdel;

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
					return 1;
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
					else if (mode == 2) lcd_gen_scale_arr(scalex--,scaley);
					break;
				case KEY_RIGHT:
					if (!mode) button_right = 1;
					else if (mode == 1) lcd_set_off(lcd_get_xoff()+1,lcd_get_yoff());
					else if (mode == 2) lcd_gen_scale_arr(scalex++,scaley);
					break;
				case KEY_DOWN:
					if (!mode) button_down = 1;
					else if (mode == 1) lcd_set_off(lcd_get_xoff(),lcd_get_yoff()+1);
					else if (mode == 2) lcd_gen_scale_arr(scalex,scaley++);
					break;
				case KEY_UP:
					if (!mode) button_up = 1;
					else if (mode == 1) lcd_set_off(lcd_get_xoff(),lcd_get_yoff()-1);
					else if (mode == 2) lcd_gen_scale_arr(scalex,scaley--);
					break;
			}
		}

		if (e.type == event_key_repeat) {
			switch (e.key.code) {
				case KEY_LEFT:
					if (mode == 1 && leftdel > 20 && (leftdel % 2) == 0) lcd_set_off(lcd_get_xoff()-1,lcd_get_yoff());
					else if (mode == 2 && leftdel > 20 && (leftdel % 2) == 0) lcd_gen_scale_arr(scalex--,scaley);
					leftdel++;
					break;
				case KEY_RIGHT:
					if (mode == 1 && rightdel > 20 && (rightdel % 2) == 0) lcd_set_off(lcd_get_xoff()+1,lcd_get_yoff());
					else if (mode == 2 && rightdel > 20 && (rightdel % 2) == 0) lcd_gen_scale_arr(scalex++,scaley);
					rightdel++;
					break;
				case KEY_DOWN:
					if (mode == 1 && downdel > 20 && (downdel % 2) == 0) lcd_set_off(lcd_get_xoff(),lcd_get_yoff()+1);
					else if (mode == 2 && downdel > 20 && (downdel % 2) == 0) lcd_gen_scale_arr(scalex,scaley++);
					downdel++;
					break;
				case KEY_UP:
					if (mode == 1 && updel > 20 && (updel % 2) == 0) lcd_set_off(lcd_get_xoff(),lcd_get_yoff()-1);
					else if (mode == 2 && updel > 20 && (updel % 2) == 0) lcd_gen_scale_arr(scalex,scaley--);
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
		char k[21];
		//sprintf(k,"%i", frames);locate(1,1,k);
		//float fps = 1.0/((((float)(framet2))-((float)(framet1)))/256.0);
		int fps = 1.0/((framet2-framet1)/256.0);
		sprintf(k,"%i", fps);locate(1,1,k);
		//sprintf(k,"%i", timertime);locate(1,3,k);
		sprintf(k,"%i", mode);locate(1,2,k);
		sprintf(k,"%i", lcd_get_xoff());locate(1,3,k);
		sprintf(k,"%i", lcd_get_yoff());locate(1,4,k);
		sprintf(k,"%i", scalex);locate(1,5,k);
		sprintf(k,"%i", scaley);locate(1,6,k);
		//sprintf(k,"%i", gfk());locate(1,7,k);
		//sprintf(k,"%i", gfk2());locate(1,8,k);
	}
	dupdate();
	dclear();
	framet1 = timertime;
	frames++;
	/*if(frames == 0)
		gettimeofday(&tv1, NULL);
	
	frames++;
	if(frames % 1000 == 0)
	{
		gettimeofday(&tv2, NULL);
		printf("Frames %d, seconds: %d, fps: %d\n", frames, tv2.tv_sec - tv1.tv_sec, frames/(tv2.tv_sec - tv1.tv_sec));
	}
	SDL_Flip(screen);*/
}

void sdl_quit()
{
	//SDL_Quit();
}