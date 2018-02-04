#ifndef SDL_H
#define SDL_H
extern int mode;

int sdl_update(void);
void sdl_init(void);
void sdl_frame(void);
int sdl_menu();
void sdl_quit(void);
//unsigned int *sdl_get_framebuffer(void);
unsigned int sdl_get_buttons(void);
unsigned int sdl_get_directions(void);
#endif
