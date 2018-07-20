#include "disp.h"
#include <keyboard.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <events.h>
#include <string.h>
/*
void locate(int x, int y, const char* str)
{
	if(x < 1 || x > 21 || y < 1 || y > 8) return;
	if(gray_runs()) gtext(x * 6 - 5, y * 8 - 8, str);
	else dtext(x * 6 - 5, y * 8 - 8, str);
}*/

void mprint(int x,int y, const char* fmt, ...)
{
	if(x < 1 || x > 21 || y < 1 || y > 8) return;
	char k[50];
	va_list args;
	va_start(args, fmt);
	vsprintf(k,fmt,args);
	va_end(args);
	if(gray_runs()) gtext(x * 6 - 5, y * 8 - 8, k);
	else dtext(x * 6 - 5, y * 8 - 8, k);
}

void keyb_input(char* buf,const char* ask)
{
	int key,ptr;
	ptr = 0;
	key = 0;
	int run = 1;
	int lower = 1;
	int shift = 0;
	int ls = 0;
	int alpha = 0;
	int la = 0;
	memset(buf,0,20);
	while (run) {
		dclear();
		mprint(1,1,ask);
		mprint(1,2,buf);
		dline(ptr*6,8,ptr*6,7+7,color_black);
		if (lower) mprint(1,3,"Lowercase");
		else mprint(1,3,"Uppercase");
		mprint(1,4,"Use OPTN to switch");
		mprint(1,5,alpha == 2 ? "Alpha lock" : alpha == 1 ? "Alpha" : shift ? "Shift" : "");
		dupdate();
		key = getkey_opt(getkey_none,0);
		switch (key) {
			case KEY_LEFT:
				if (ptr > 0) ptr--;
				break;
			case KEY_RIGHT: 
				if (buf[ptr] != 0) ptr++;
				break;
			case KEY_EXE:
				if (ptr != 0) run = 0;
				break;
			case KEY_DEL: 
				if (ptr > 0) {buf[ptr--] = 0;buf[ptr] = 0;}
				break;
			case KEY_OPTN:
				lower = !lower;
				break;
			case KEY_SHIFT:
				shift = !shift;
				break;
			case KEY_ALPHA:
				if (shift) alpha = 2;
				else alpha = !alpha;
				break;
			default:
				if (alpha) key |= MOD_ALPHA;
				if (shift) key |= MOD_SHIFT;
				if (key_char(key) != 0 && ptr < 21) buf[ptr++] = lower ? tolower(key_char(key)) : key_char(key);
				break;
		}
		if (ls) shift = 0;
		if (la == 1) alpha = 0;
		ls = shift;
		la = alpha;
		
	}
	dclear();
	dupdate();
}