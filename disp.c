#include "disp.h"
#include <keyboard.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
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
	char k[22];
	va_list args;
	va_start(args, fmt);
	vsprintf(k,fmt,args);
	va_end(args);
	if(gray_runs()) gtext(x * 6 - 5, y * 8 - 8, k);
	else dtext(x * 6 - 5, y * 8 - 8, k);
}

void keyb_input(char* buf,const char* ask)
{
	int key,ptr,i;
	ptr = 0;
	key = 0;
	int run = 1;
	int lower = 1;
	//char buf[20];
	//char k[21];
	for (i=0;i<20;i++) buf[i] = 0;
	while (run) {
		dclear();
		mprint(1,1,ask);
		mprint(1,2,buf);
		dline(ptr*6,8,ptr*6,7+7,color_black);
		if (lower) mprint(1,3,"Lowercase");
		else mprint(1,3,"Uppercase");
		mprint(1,4,"Use OPTN to switch");
		//sprintf(k,"%i", ptr);locate(1,4,k);
		//sprintf(k,"%i", key);locate(1,5,k);
		//sprintf(k,"%i", key_type(key));locate(1,6,k);
		dupdate();
		key = getkey();
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
			default:
				if (key_char(key) != 0 && ptr < 21) buf[ptr++] = lower ? tolower(key_char(key)) : key_char(key);
				break;
		}
		
	}
	dclear();
	dupdate();
	/*for (i=0;;i++) {
		if (!buf[i]) break;
		out[i] = buf[i];
	}*/
}