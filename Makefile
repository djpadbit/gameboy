#CFLAGS=-march=native -O2 -Wextra -Wall -Wno-switch -std=c99
#LDFLAGS=-lSDL
LDFLAGS = `fxsdk --libs`
CC = sh3eb-elf-gcc
CFLAGS = -mb -ffreestanding -nostdlib -ansi -pedantic -Wall -Wno-implicit -Wno-long-long -Ofast -fstrength-reduce -fthread-jumps  -fcse-follow-jumps -fcse-skip-blocks -frerun-cse-after-loop  -fexpensive-optimizations -fforce-addr -fomit-frame-pointer -std=c99
CFLAGS += `fxsdk --cflags`
all: clean gameboy

debug: CFLAGS += -g
debug: all

gameboy:
	$(CC) $(CFLAGS) *.c $(LDFLAGS) -o gameboy.elf
	sh3eb-elf-objcopy -R .comment -R .bss -O binary gameboy.elf gameboy.bin
	g1a-wrapper gameboy.bin -o gameboy.g1a -i MainIcon.bmp

clean:
	rm -f gameboy.bin gameboy.elf gameboy.g1a

send:
	p7 send -f gameboy.g1a