#ifndef ROM_H
#define ROM_H
int rom_load(char *);
unsigned char *rom_getbytes(void);
unsigned int rom_get_mapper(void);
unsigned char rom_read_byte(int i);
void rom_close();

enum {
	NROM,
	MBC1,
	MBC2,
	MMM01,
	MBC3,
	MBC4,
	MBC5,
};
#endif
