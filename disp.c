#include "disp.h"

void locate(int x, int y, const char *str)
{
	if(x < 1 || x > 21 || y < 1 || y > 8) return;
	if(gray_runs()) gtext(x * 6 - 5, y * 8 - 8, str);
	else dtext(x * 6 - 5, y * 8 - 8, str);
}