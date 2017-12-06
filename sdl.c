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

/* TODO

 - OSD with scaling and positioning
 - SDL cycle for that ^
 - kinda 60fps frame regulation ?

*/


void sdl_init(void)
{
	keyboard_init();
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
					button_left = 1;
					break;
				case KEY_RIGHT:
					button_right = 1;
					break;
				case KEY_DOWN:
					button_down = 1;
					break;
				case KEY_UP:
					button_up = 1;
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
					button_left = 0;
					break;
				case KEY_RIGHT:
					button_right = 0;
					break;
				case KEY_DOWN:
					button_down = 0;
					break;
				case KEY_UP:
					button_up = 0;
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
	char k[21];
	sprintf(k,"%i", frames);locate(1,1,k);
	//float fps = 1.0/((((float)(framet2))-((float)(framet1)))/256.0);
	int fps = 1.0/((framet2-framet1)/256.0);
	sprintf(k,"%i", fps);locate(1,2,k);
	//sprintf(k,"%i", timertime);locate(1,3,k);
	sprintf(k,"%i", sdl_get_buttons());locate(1,3,k);
	sprintf(k,"%i", sdl_get_directions());locate(1,4,k);
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