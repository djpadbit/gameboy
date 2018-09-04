#ifndef CPU_H
#define CPU_H

struct CPU {
	unsigned char H;
	unsigned char L;

	unsigned char D;
	unsigned char E;

	unsigned char B;
	unsigned char C;

	unsigned char A;
	unsigned char F;

	unsigned short SP;
	unsigned short PC;
	unsigned int cycles;
};

extern struct CPU c;
extern int is_debugged;
extern int halted;

void cpu_init(void);
int cpu_cycle(void);
void cpu_interrupt(unsigned short);
void cpu_print_info();
void cpu_unhalt(void);
int cpu_halted(void);
#endif
