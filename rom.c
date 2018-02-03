#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <bfile.h>
#include <keyboard.h>
#include <mpu.h>
#include "disp.h"
#include "rom.h"
#include "timek.h"

#define max(a,b) ((a) > (b) ? (a) : (b))
#define min(a,b) ((a) < (b) ? (a) : (b))

/* TODO

 - 2 (or more) Rom buffers

*/

static int fd;
static unsigned char *buf1;
static unsigned char *magicaddr = (unsigned char*) 0x88040000;
unsigned int buf1lp = 0;
unsigned int mapper;
static int romsize = 32768;

static char *carts[] = {
	[0x00] = "ROM ONLY",
	[0x01] = "MBC1",
	[0x02] = "MBC1+RAM",
	[0x03] = "MBC1+RAM+BATTERY",
	[0x05] = "MBC2",
	[0x06] = "MBC2+BATTERY",
	[0x08] = "ROM+RAM",
	[0x09] = "ROM+RAM+BATTERY",
	[0x0B] = "MMM01",
	[0x0C] = "MMM01+RAM",
	[0x0D] = "MMM01+RAM+BATTERY",
	[0x0F] = "MBC3+TIMER+BATTERY",
	[0x10] = "MBC3+TIMER+RAM+BATTERY",
	[0x11] = "MBC3",
	[0x12] = "MBC3+RAM",
	[0x13] = "MBC3+RAM+BATTERY",
	[0x15] = "MBC4",
	[0x16] = "MBC4+RAM",
	[0x17] = "MBC4+RAM+BATTERY",
	[0x19] = "MBC5",
	[0x1A] = "MBC5+RAM",
	[0x1B] = "MBC5+RAM+BATTERY",
	[0x1C] = "MBC5+RUMBLE",
	[0x1D] = "MBC5+RUMBLE+RAM",
	[0x1E] = "MBC5+RUMBLE+RAM+BATTERY",
	[0xFC] = "POCKET CAMERA",
	[0xFD] = "BANDAI TAMA5",
	[0xFE] = "HuC3",
	[0xFF] = "HuC1+RAM+BATTERY",
};

static char *banks[] = {
	" 32KiB",
	" 64KiB",
	"128KiB",
	"256KiB",
	"512KiB",
	"  1MiB",
	"  2MiB",
	"  4MiB",
	/* 0x52 */
	"1.1MiB",
	"1.2MiB",
	"1.5MiB",
	"Unknown"
};

static int bankssize[] = {
	32768,
	32768*2,
	32768*4,
	32768*8,
	32768*16,
	32768*32,
	32768*64,
	32768*128,
	/* 0x52 */
	(32768*32)+(32768*4),
	(32768*32)+(32768*8),
	(32768*32)+(32768*16),
	32768
};

static char *rams[] = {
	"None",
	"  2KiB",
	"  8KiB",
	" 32KiB",
	"Unknown"
};

static char *regions[] = {
	"Japan",
	"Non-Japan",
	"Unknown"
};

static unsigned char header[] = {
	0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B,
	0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
	0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E,
	0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,
	0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC,
	0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E
};

char k[21];

unsigned char inline rom_read_byte(int i) // I have no fucking idea why it doesn't work with an unsinged int but does with a signed one but it used to works with a unsigned short
{
	if (isSH3()) { // SH3 doesn't have the 256Kb buffer
		int lb;
		i = (unsigned short)i;
		if (i < buf1lp || i > (buf1lp+20000)) {
			lb = max(0,(i-(20000/2)));
			if (((lb+20000)-romsize) > 0) lb -= ((lb+20000)-romsize);
			BFile_Read(fd,buf1,20000,lb);
			buf1lp = lb;
		}
		return buf1[i-buf1lp];
	} else {
		if (romsize<=32768*8) { // 256Kb and lower roms
			return magicaddr[i];
		} else { 				// just put the buffer thing with the 256k buffer
			int lb;				// Definitively not the best thing, shoule make a better thing
			i = (unsigned short)i;
			if (i < buf1lp || i > (buf1lp+(32768*8))) {
				lb = max(0,(i-((32768*8)/2)));
				if (((lb+(32768*8))-romsize) > 0) lb -= ((lb+(32768*8))-romsize);
				BFile_Read(fd,magicaddr,(32768*8),lb);
				buf1lp = lb;
			}
			return magicaddr[i-buf1lp];
		}
	}
}

static int rom_init()
{
	char buf[17];
	int type, bank_index, ram, region, version, i, pass;
	unsigned char checksum = 0;
	//if(memcmp(&rombytes[0x104], header, sizeof(header)) != 0)
	//	return 0;
	for (int pos=0;pos<sizeof(header);pos++) {
		if (rom_read_byte((0x104)+pos) != header[pos]) {
			return 0;
		}
	}

	//memcpy(buf, &rombytes[0x134], 16);
	for (int pos=0;pos<16;pos++) {
		buf[pos] = rom_read_byte(0x134+pos);
	}
	buf[16] = '\0';
	//printf("Rom title: %s\n", buf);
	sprintf(k,"Rom title: %s", buf);locate(1,1,k);

	//type = rombytes[0x147];
	type = rom_read_byte(0x147);

	//printf("Cartridge type: %s (%02X)\n", carts[type], type);
	locate(1,2,"Cartridge type:");sprintf(k,"%s (%02X)", carts[type], type);locate(1,3,k);

	//bank_index = rombytes[0x148];
	bank_index = rom_read_byte(0x148);
	/* Adjust for the gap in the bank indicies */
	if(bank_index >= 0x52 && bank_index <= 0x54)
		bank_index -= 74;
	else if(bank_index > 7)
		bank_index = 11;

	//printf("Rom size: %s\n", banks[bank_index]);
	sprintf(k,"Rom size: %s", banks[bank_index]);locate(1,4,k);
	//romsize = 32768*(power(2,bank_index));
	romsize = bankssize[bank_index];
	if (romsize<=32768*8) BFile_Read(fd,magicaddr,romsize,0);
	else BFile_Read(fd,magicaddr,32768*8,0);

	//ram = rombytes[0x149];
	ram = rom_read_byte(0x149);
	if(ram > 3)
		ram = 4;

	//printf("RAM size: %s\n", rams[ram]);
	sprintf(k,"RAM size: %s", rams[ram]);locate(1,5,k);

	//region = rombytes[0x14A];
	region = rom_read_byte(0x14A);
	if(region > 2)
		region = 2;
	//printf("Region: %s\n", regions[region]);
	sprintf(k,"Region: %s", regions[region]);locate(1,6,k);

	//version = rombytes[0x14C];
	version = rom_read_byte(0x14C);
	//printf("Version: %02X\n", version);
	sprintf(k,"Version: %02X", version);locate(1,7,k);

	//for(i = 0x134; i <= 0x14C; i++)
	//	checksum = checksum - rombytes[i] - 1;
	for(i = 0x134; i <= 0x14C; i++)
		checksum = checksum - rom_read_byte(i) - 1;

	//pass = rombytes[0x14D] == checksum;
	pass = rom_read_byte(0x14D) == checksum;

	//printf("Checksum: %s (%02X)\n", pass ? "OK" : "FAIL", checksum);
	sprintf(k,"Checksum: %s (%02X)", pass ? "OK" : "FAIL", checksum);locate(1,8,k);
	if(!pass)
		return 0;

	dupdate();
	getkey();
	dclear();

	/*sprintf(k,"%i", romsize);locate(1,4,k);

	dupdate();
	getkey();
	dclear();*/

	//bytes = rombytes;

	switch(type)
	{
		case 0x00:
		case 0x08:
		case 0x09:
			mapper = NROM;
		break;
		case 0x01:
		case 0x02:
		case 0x03:
			mapper = MBC1;
		break;
		case 0x05:
		case 0x06:
			mapper = MBC2;
		break;
		case 0x0B:
		case 0x0C:
			mapper = MMM01;
		break;
		case 0x0F:
		case 0x10:
		case 0x11:
		case 0x12:
		case 0x13:
			mapper = MBC3;
		break;
		case 0x15:
		case 0x16:
		case 0x17:
			mapper = MBC4;
		break;
		case 0x19:
		case 0x1A:
		case 0x1B:
		case 0x1C:
		case 0x1D:
		case 0x1E:
			mapper = MBC5;
		break;
	}

	return 1;
}

unsigned int rom_get_mapper(void)
{
	return mapper;
}

// Based on EDIT's source code (see FilePath function)
void make_path(uint16_t* dst,char* root,char *fold,char *fn)
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

int rom_load(char *filename)
{
	uint16_t path[64];
	make_path(path,"fls0","",filename);
	fd = BFile_Open(path,BFile_ReadOnly);

	if(fd < 0)
		return 0;

	if (isSH3()) {
		buf1 = (unsigned char*)calloc(1, 20000); //16384
		BFile_Read(fd,buf1,20000,0);
		buf1lp = 0;
	} else {
		memset(magicaddr,0,32768*8); // Clear out the 256Kb
		BFile_Read(fd,magicaddr,32768,0);
	}
	
	return rom_init();
}

void rom_close()
{
	BFile_Close(fd);
	if (isSH3())
		free(buf1);
	else
		memset(magicaddr,0,32768*8);
}

/*unsigned char *rom_getbytes(void)
{
	return bytes;
}*/
