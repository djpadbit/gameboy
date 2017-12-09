#CFLAGS=-march=native -O2 -Wextra -Wall -Wno-switch -std=c99
#LDFLAGS=-lSDL
LDFLAGS = `fxsdk --libs`
CC = sh3eb-elf-gcc
CFLAGS = -mb -ffreestanding -nostdlib -ansi -pedantic -Wall -Wno-implicit -Wno-long-long -Ofast -fstrength-reduce -fthread-jumps  -fcse-follow-jumps -fcse-skip-blocks -frerun-cse-after-loop  -fexpensive-optimizations -fforce-addr -fomit-frame-pointer -std=c99
CFLAGS += `fxsdk --cflags`
all: clean fxgb

debug: CFLAGS += -g
debug: all

fxgb:
	$(CC) $(CFLAGS) *.c $(LDFLAGS) -o fxgb.elf
	sh3eb-elf-objcopy -R .comment -R .bss -O binary fxgb.elf fxgb.bin
	g1a-wrapper fxgb.bin -o fxgb.g1a -i MainIcon.bmp

clean:
	rm -f fxgb.bin fxgb.elf fxgb.g1a

send:
	p7 send -f fxgb.g1a