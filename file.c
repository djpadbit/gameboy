#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Based on EDIT's source code (see FilePath function)
void file_make_path(uint16_t* dst,char* root,char *fold,char *fn)
{
	char *tp;
	tp = (char*)malloc(2+strlen(root)+1+strlen(fold)+1+strlen(fn)+1); // probably off by 1 or 2 bytes
	if(strlen(fold)==0) sprintf(tp,"\\\\%s\\%s",root,fn); //File path without folder
	else if(strlen(fn)==0) sprintf(tp,"\\\\%s\\%s",root,fn); //File path without file
	else sprintf(tp,"\\\\%s\\%s\\%s",root,fold,fn); //File path with file & folder
	for (int i=0;i<strlen(tp);i++) dst[i] = tp[i];
	dst[strlen(tp)] = 0;
	free(tp);
}