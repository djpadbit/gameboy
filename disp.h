#ifndef DISP_H
#define DISP_H

#include <display.h>
#include <gray.h>

//void locate(int x, int y, const char* str);
void mprint(int x,int y, const char* format, ...);
void keyb_input(char* buf,const char* ask);

#endif