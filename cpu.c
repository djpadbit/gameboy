#include <stdio.h>
#include <keyboard.h>
#include "mem.h"
#include "rom.h"
#include "disp.h"
#include "interrupt.h"
#include "cpu.h"

#define set_HL(x) do {unsigned int macro = (x); c.L = macro&0xFF; c.H = macro>>8;} while(0)
#define set_BC(x) do {unsigned int macro = (x); c.C = macro&0xFF; c.B = macro>>8;} while(0)
#define set_DE(x) do {unsigned int macro = (x); c.E = macro&0xFF; c.D = macro>>8;} while(0)
#define set_AF(x) do {unsigned int macro = (x); c.F = macro&0xFF; c.A = macro>>8;} while(0)

#define get_AF() ((c.A<<8) | c.F)
#define get_BC() ((c.B<<8) | c.C)
#define get_DE() ((c.D<<8) | c.E)
#define get_HL() ((c.H<<8) | c.L)

/* Flags */
#define set_Z(x) c.F = ((c.F&0x7F) | ((x)<<7))
#define set_N(x) c.F = ((c.F&0xBF) | ((x)<<6))
#define set_H(x) c.F = ((c.F&0xDF) | ((x)<<5))
#define set_C(x) c.F = ((c.F&0xEF) | ((x)<<4))

#define flag_Z !!((c.F & 0x80))
#define flag_N !!((c.F & 0x40))
#define flag_H !!((c.F & 0x20))
#define flag_C !!((c.F & 0x10))

/* Opcodes */
#define INC(x) \
	x++; \
	set_Z(!x); \
	set_H((x & 0xF) == 0); \
	set_N(0); \
	c.PC += 1; \
	c.cycles += 1;

#define DEC(x) \
	x--; \
	set_Z(!x); \
	set_N(1); \
	set_H((x & 0xF) == 0xF); \
	c.PC += 1; \
	c.cycles += 1;

#define LDRR(x, y) \
	x = y; \
	c.PC += 1; \
	c.cycles += 1;

#define LDRIMM8(x) \
	x = mem_get_byte(c.PC+1); \
	c.PC += 2; \
	c.cycles += 2;

#define ANDR(x) \
	c.A &= x; \
	set_Z(!c.A); \
	set_H(1); \
	set_N(0); \
	set_C(0); \
	c.PC += 1; \
	c.cycles += 1;

#define XORR(x) \
	c.A ^= x; \
	set_Z(!c.A); \
	set_H(0); \
	set_N(0); \
	set_C(0); \
	c.PC += 1; \
	c.cycles += 1;

#define ORR(x) \
	c.A |= x; \
	set_Z(!c.A); \
	set_H(0); \
	set_N(0); \
	set_C(0); \
	c.PC += 1; \
	c.cycles += 1;

#define CPR(x) \
	set_C((c.A - x) < 0); \
	set_H(((c.A - x)&0xF) > (c.A&0xF)); \
	set_Z(c.A == x); \
	set_N(1); \
	c.PC += 1; \
	c.cycles += 1;

#define SUBR(x) \
	set_C((c.A - x) < 0); \
	set_H(((c.A - x)&0xF) > (c.A&0xF)); \
	c.A -= x; \
	set_Z(!c.A); \
	set_N(1); \
	c.PC += 1; \
	c.cycles += 1;

struct CPU c;
int is_debugged;
int halted;

void cpu_init(void)
{
	set_AF(0x01B0);
	set_BC(0x0013);
	set_DE(0x00D8);
	set_HL(0x014D);
	c.SP = 0xFFFE;
	c.PC = 0x0100;
	c.cycles = 0;
}

static void RLC(unsigned char reg)
{
	unsigned char t, old;

	switch(reg)
	{
		case 0:	/* B */
			old = !!(c.B&0x80);
			c.B = (c.B << 1) | old;
			set_C(old);
			set_Z(!c.B);
		break;
		case 1:	/* C */
			old = !!(c.C&0x80);
			set_C(old);
			c.C = c.C<<1 | old;
			set_Z(!c.C);
		break;
		case 2:	/* D */
			old = !!(c.D&0x80);
			set_C(old);
			c.D = c.D<<1 | old;
			set_Z(!c.D);
		break;
		case 3:	/* E */
			old = !!(c.E&0x80);
			set_C(old);
			c.E = c.E<<1 | old;
			set_Z(!c.E);
		break;
		case 4:	/* H */
			old = !!(c.H&0x80);
			set_C(old);
			c.H = c.H<<1 | old;
			set_Z(!c.H);
		break;
		case 5:	/* L */
			old = !!(c.L&0x80);
			set_C(old);
			c.L = c.L<<1 | old;
			set_Z(!c.L);
		break;
		case 6:	/* (HL) */
			t = mem_get_byte(get_HL());
			old = !!(t&0x80);
			set_C(old);
			t = t<<1 | old;
			mem_write_byte(get_HL(), t);
			set_Z(!t);
		break;
		case 7:	/* A */
			old = !!(c.A&0x80);
			c.A = (c.A<<1) | old;
			set_C(old);
			set_Z(!c.A);
		break;
	}

	set_N(0);
	set_H(0);
}

static void RRC(unsigned char reg)
{
	unsigned char t, old;

	switch(reg)
	{
		case 0:	/* B */
			old = c.B&1;
			set_C(old);
			c.B = c.B>>1 | old<<7;
			set_Z(!c.B);
		break;
		case 1:	/* C */
			old = c.C&1;
			set_C(old);
			c.C = c.C>>1 | old<<7;
			set_Z(!c.C);
		break;
		case 2:	/* D */
			old = c.D&1;
			set_C(old);
			c.D = c.D>>1 | old<<7;
			set_Z(!c.D);
		break;
		case 3:	/* E */
			old = c.E&1;
			set_C(old);
			c.E = c.E>>1 | old<<7;
			set_Z(!c.E);
		break;
		case 4:	/* H */
			old = c.H&1;
			set_C(old);
			c.H = c.H>>1 | old<<7;
			set_Z(!c.H);
		break;
		case 5:	/* L */
			old = c.L&1;
			set_C(old);
			c.L = c.L>>1 | old<<7;
			set_Z(!c.L);
		break;
		case 6:	/* (HL) */
			t = mem_get_byte(get_HL());
			old = t;
			set_C(old);
			t = t>>1 | old<<7;
			c.cycles += 2;
			mem_write_byte(get_HL(), t);
			set_Z(!t);
		break;
		case 7:	/* A */
			old = c.A&1;
			set_C(old);
			c.A = c.A>>1 | old<<7;
			set_Z(!c.A);
		break;
	}
	set_N(0);
	set_H(0);
}

static void RL(unsigned char reg)
{
	unsigned char t, t2;

	switch(reg)
	{
		case 0: /* B */
			t2 = flag_C;
			set_C(!!(c.B&0x80));
			c.B = (c.B << 1) | !!(t2);
			set_Z(!c.B);
		break;
		case 1: /* C */
			t2 = flag_C;
			set_C(!!(c.C&0x80));
			c.C = (c.C << 1) | !!(t2);
			set_Z(!c.C);
		break;
		case 2: /* D */
			t2 = flag_C;
			set_C(!!(c.D&0x80));
			c.D = (c.D << 1) | !!(t2);
			set_Z(!c.D);
		break;
		case 3: /* E */
			t2 = flag_C;
			set_C(!!(c.E&0x80));
			c.E = (c.E << 1) | !!(t2);
			set_Z(!c.E);
		break;
		case 4: /* H */
			t2 = flag_C;
			set_C(!!(c.H&0x80));
			c.H = (c.H << 1) | !!(t2);
			set_Z(!c.H);
		break;
		case 5: /* L */
			t2 = flag_C;
			set_C(!!(c.L&0x80));
			c.L = (c.L << 1) | !!(t2);
			set_Z(!c.L);
		break;
		case 6: /* (HL) */
			t = mem_get_byte(get_HL());
			t2 = flag_C;
			set_C(!!(t&0x80));
			t = (t << 1) | !!(t2);
			mem_write_byte(get_HL(), t);
			set_Z(!t);
			c.cycles += 2;
		break;
		case 7: /* A */
			t2 = flag_C;
			set_C(!!(c.A&0x80));
			c.A = (c.A << 1) | t2;
			set_Z(!c.A);
		break;
	}

	set_N(0);
	set_H(0);
}

static void RR(unsigned char reg)
{
	unsigned char t, t2;

	switch(reg)
	{
		case 0:	/* B */
			t2 = flag_C;
			set_C(c.B&1);
			c.B = (c.B >> 1) | t2<<7;
			set_Z(!c.B);
		break;
		case 1: /* C */
			t2 = flag_C;
			set_C(c.C&1);
			c.C = (c.C >> 1) | t2<<7;
			set_Z(!c.C);
		break;
		case 2: /* D */
			t2 = flag_C;
			set_C(c.D&1);
			c.D = (c.D >> 1) | t2<<7;
			set_Z(!c.D);
		break;
		case 3: /* E */
			t2 = flag_C;
			set_C(c.E&1);
			c.E = (c.E >> 1) | t2<<7;
			set_Z(!c.E);
		break;
		case 4: /* H */
			t2 = flag_C;
			set_C(c.H&1);
			c.H = (c.H >> 1) | t2<<7;
			set_Z(!c.H);
		break;
		case 5: /* L */
			t2 = flag_C;
			set_C(c.L&1);
			c.L = (c.L >> 1) | t2<<7;
			set_Z(!c.L);
		break;
		case 6: /* (HL) */
			t = mem_get_byte(get_HL());
			t2 = flag_C;
			set_C(t&1);
			t = (t >> 1) | t2<<7;
			set_Z(!t);
			mem_write_byte(get_HL(), t);
			c.cycles += 2;
		break;
		case 7: /* A */
			t2 = flag_C;
			set_C(c.A&1);
			c.A = (c.A >> 1) | (t2<<7);
			set_Z(!c.A);
		break;
	}
	set_N(0);
	set_H(0);
}

static void SLA(unsigned char reg)
{
	unsigned char t;

	switch(reg)
	{
		case 0: /* B */
			set_C(!!(c.B & 0x80));
			c.B = c.B << 1;
			set_Z(!c.B);
		break;
		case 1: /* C */
			set_C(!!(c.C & 0x80));
			c.C = c.C << 1;
			set_Z(!c.C);
		break;
		case 2: /* D */
			set_C(!!(c.D & 0x80));
			c.D = c.D << 1;
			set_Z(!c.D);
		break;
		case 3: /* E */
			set_C(!!(c.E & 0x80));
			c.E = c.E << 1;
			set_Z(!c.E);
		break;
		case 4: /* H */
			set_C(!!(c.H & 0x80));
			c.H = c.H << 1;
			set_Z(!c.H);
		break;
		case 5: /* L */
			set_C(!!(c.L & 0x80));
			c.L = c.L << 1;
			set_Z(!c.L);
		break;
		case 6: /* (HL) */
			t = mem_get_byte(get_HL());
			set_C(!!(t & 0x80));
			t = t << 1;
			mem_write_byte(get_HL(), t);
			set_Z(!t);
			c.cycles += 2;
		break;
		case 7: /* A */
			set_C(!!(c.A & 0x80));
			c.A = c.A << 1;
			set_Z(!c.A);
		break;
	}

	set_H(0);
	set_N(0);
}

static void SRA(unsigned char reg)
{
	unsigned char old, t;

	switch(reg)
	{
		case 0: /* B */
			set_C(c.B&1);
			old = c.B&0x80;
			c.B = c.B >> 1 | old;
			set_Z(!c.B);
		break;
		case 1: /* C */
			set_C(c.C&1);
			old = c.C&0x80;
			c.C = c.C >> 1 | old;
			set_Z(!c.C);
		break;
		case 2: /* D */
			set_C(c.D&1);
			old = c.D&0x80;
			c.D = c.D >> 1 | old;
			set_Z(!c.D);
		break;
		case 3: /* E */
			set_C(c.E&1);
			old = c.E&0x80;
			c.E = c.E >> 1 | old;
			set_Z(!c.E);
		break;
		case 4: /* H */
			set_C(c.H&1);
			old = c.H&0x80;
			c.H = c.H >> 1 | old;
			set_Z(!c.H);
		break;
		case 5: /* L */
			set_C(c.L&1);
			old = c.L&0x80;
			c.L = c.L >> 1 | old;
			set_Z(!c.L);
		break;
		case 6: /* (HL) */
			t = mem_get_byte(get_HL());
			set_C(t&1);
			old = t&0x80;
			t = t >> 1 | old;
			mem_write_byte(get_HL(), t);
			set_Z(!t);
		break;
		case 7: /* A */
			set_C(c.A&1);
			old = c.A&0x80;
			c.A = c.A >> 1 | old;
			set_Z(!c.A);
		break;
	}

	set_H(0);
	set_N(0);
}

static void SRL(unsigned char reg)
{
	unsigned char t;

	switch(reg)
	{
		case 0: /* B */
			set_C(c.B & 1);
			c.B = c.B >> 1;
			set_Z(!c.B);
		break;
		case 1: /* C */
			set_C(c.C & 1);
			c.C = c.C >> 1;
			set_Z(!c.C);
		break;
		case 2: /* D */
			set_C(c.D & 1);
			c.D = c.D >> 1;
			set_Z(!c.D);
		break;
		case 3: /* E */
			set_C(c.E & 1);
			c.E = c.E >> 1;
			set_Z(!c.E);
		break;
		case 4: /* H */
			set_C(c.H & 1);
			c.H = c.H >> 1;
			set_Z(!c.H);
		break;
		case 5: /* L */
			set_C(c.L & 1);
			c.L = c.L >> 1;
			set_Z(!c.L);
		break;
		case 6: /* (HL) */
			t = mem_get_byte(get_HL());
			set_C(t & 1);
			t = t >> 1;
			mem_write_byte(get_HL(), t);
			set_Z(!t);
			c.cycles += 2;
		break;
		case 7: /* A */
			set_C(c.A & 1);
			c.A = c.A >> 1;
			set_Z(!c.A);
		break;
	}

	set_H(0);
	set_N(0);
}

static void SWAP(unsigned char reg)
{
	unsigned char t;

	switch(reg)
	{
		case 0: /* B */
			c.B = ((c.B&0xF)<<4) | ((c.B&0xF0)>>4);
			c.F = (!c.B)<<7;
		break;
		case 1: /* C */
			c.C = ((c.C&0xF)<<4) | ((c.C&0xF0)>>4);
			c.F = (!c.C)<<7;
		break;
		case 2: /* D */
			c.D = ((c.D&0xF)<<4) | ((c.D&0xF0)>>4);
			c.F = (!c.D)<<7;
		break;
		case 3: /* E */
			c.E = ((c.E&0xF)<<4) | ((c.E&0xF0)>>4);
			c.F = (!c.E)<<7;
		break;
		case 4: /* H */
			c.H = ((c.H&0xF)<<4) | ((c.H&0xF0)>>4);
			c.F = (!c.H)<<7;
		break;
		case 5: /* L */
			c.L = ((c.L&0xF)<<4) | ((c.L&0xF0)>>4);
			c.F = (!c.L)<<7;
		break;
		case 6: /* (HL) */
			t = mem_get_byte(get_HL());
			t = ((t&0xF)<<4) | ((t&0xF0)>>4);
			mem_write_byte(get_HL(), t);
			c.F = (!t)<<7;
			c.cycles += 2;
		break;
		case 7: /* A */
			c.A = ((c.A&0xF)<<4) | ((c.A&0xF0)>>4);
			c.F = (!c.A)<<7;
		break;
	}
}

static void BIT(unsigned char bit, unsigned char reg)
{
	unsigned char t, f = 0 /* Make GCC happy */;

	switch(reg)
	{
		case 0: /* B */
		    f = !(c.B & bit);
		break;
		case 1: /* C */
		    f = !(c.C & bit);
		break;
		case 2: /* D */
		    f = !(c.D & bit);
		break;
		case 3: /* E */
		    f = !(c.E & bit);
		break;
		case 4: /* H */
		    f = !(c.H & bit);
		break;
		case 5: /* L */
		    f = !(c.L & bit);
		break;
		case 6: /* (HL) */
			t = mem_get_byte(get_HL());
			f = !(t & bit);
			c.cycles += 1;
		break;
		case 7: /* A */
		    f = !(c.A & bit);
		break;
	}

	set_Z(f);
	set_N(0);
	set_H(1);
}

static void RES(unsigned char bit, unsigned char reg)
{
	unsigned char t;

	switch(reg)
	{
		case 0: /* B */
			c.B &= ~bit;
		break;
		case 1: /* C */
			c.C &= ~bit;
		break;
		case 2: /* D */
			c.D &= ~bit;
		break;
		case 3: /* E */
			c.E &= ~bit;
		break;
		case 4: /* H */
			c.H &= ~bit;
		break;
		case 5: /* L */
			c.L &= ~bit;
		break;
		case 6: /* (HL) */
			t = mem_get_byte(get_HL());
			t &= ~bit;
			mem_write_byte(get_HL(), t);
			c.cycles += 2;
		break;
		case 7: /* A */
			c.A &= ~bit;
		break;
	}
}

static void SET(unsigned char bit, unsigned char reg)
{
	unsigned char t;

	switch(reg)
	{
		case 0: /* B */
			c.B |= bit;
		break;
		case 1: /* C */
			c.C |= bit;
		break;
		case 2: /* D */
			c.D |= bit;
		break;
		case 3: /* E */
			c.E |= bit;
		break;
		case 4: /* H */
			c.H |= bit;
		break;
		case 5: /* L */
			c.L |= bit;
		break;
		case 6: /* (HL) */
			t = mem_get_byte(get_HL());
			t |= bit;
			mem_write_byte(get_HL(), t);
			c.cycles += 2;
		break;
		case 7: /* A */
			c.A |= bit;
		break;
	}
}

/*
00000xxx = RLC xxx
00001xxx = RRC xxx
00010xxx = RL xxx
00011xxx = RR xxx
00100xxx = SLA xxx
00101xxx = SRA xxx
00110xxx = SWAP xxx
00111xxx = SRL xxx
01yyyxxx = BIT yyy, xxx
10yyyxxx = RES yyy, xxx
11yyyxxx = SET yyy, xxx
*/
static void decode_CB(unsigned char t)
{
	unsigned char reg, opcode, bit;
	void (*f[])(unsigned char) = {RLC, RRC, RL, RR, SLA, SRA, SWAP, SRL};
	void (*f2[])(unsigned char, unsigned char) = {BIT, RES, SET};

	reg = t&7;
	opcode = t>>3;
	if(opcode < 8)
	{
		f[opcode](reg);
		return;
	}

	bit = opcode&7;
	opcode >>= 3;
	f2[opcode-1](1<<bit, reg);
}

int cpu_halted(void)
{
	return halted;
}

void cpu_unhalt(void)
{
	halted = 0;
}

void cpu_halt(void)
{
	halted = 1;
}

void cpu_interrupt(unsigned short vector)
{
	cpu_unhalt();

	c.SP -= 2;
	mem_write_word(c.SP, c.PC);
	c.PC = vector;
	interrupt_disable();
}

void cpu_print_debug(void)
{
	//printf("%04X: %02X\n", c.PC, mem_get_byte(c.PC));
	//printf("\tAF: %02X%02X, BC: %02X%02X, DE: %02X%02X, HL: %02X%02X SP: %04X, cycles %d\n",
	//	c.A, c.F, c.B, c.C, c.D, c.E, c.H, c.L, c.SP, c.cycles);
}

int isprint(unsigned char c)
{
	if (c > 0x1f && c != 0x7f) return 1;
	return 0;
}

static inline char ths(char c)
{
	if (!isprint(c)) return '.';
	return c;
}

void cpu_print_info()
{
	dclear();
	mprint(1,1,"CPU Debug:");
	mprint(1,2," Regs:");
	mprint(1,3,"  AF:%02X%02X  BC:%02X%02X",c.A,c.F,c.B,c.C);
	mprint(1,4,"  DE:%02X%02X  HL:%02X%02X",c.D,c.E,c.H,c.L);
	mprint(1,5,"  SP:%04X  PC:%04X",c.SP,c.PC);
	mprint(1,6," Opcode: %02X",mem_get_raw(c.PC));
	mprint(1,7,"  %04X:%02X%02X%02X%02X %c%c%c%c",c.PC-4,mem_get_raw(c.PC-4),mem_get_raw(c.PC-3),mem_get_raw(c.PC-2),mem_get_raw(c.PC-1),ths(mem_get_raw(c.PC-4)),ths(mem_get_raw(c.PC-3)),ths(mem_get_raw(c.PC-2)),ths(mem_get_raw(c.PC-1)));
	mprint(1,8,"  %04X:%02X%02X%02X%02X %c%c%c%c",c.PC,mem_get_raw(c.PC),mem_get_raw(c.PC+1),mem_get_raw(c.PC+2),mem_get_raw(c.PC+3),ths(mem_get_raw(c.PC)),ths(mem_get_raw(c.PC+1)),ths(mem_get_raw(c.PC+2)),ths(mem_get_raw(c.PC+3)));
	dupdate();
	getkey_opt(getkey_none,0);
}

static int ins_0()
{
	c.PC++;
	c.cycles += 1;
	return 1;
}

static int ins_1()
{
	unsigned short s;
	s = mem_get_word(c.PC+1);
	set_BC(s);
	c.PC += 3;
	c.cycles += 3;
	return 1;
}

static int ins_2()
{
	mem_write_byte(get_BC(), c.A);
	c.PC += 1;
	c.cycles += 2;
	return 1;
}

static int ins_3()
{
	set_BC(get_BC()+1);
	c.PC += 1;
	c.cycles += 2;
	return 1;
}

static int ins_4()
{
	INC(c.B);
	return 1;
}

static int ins_5()
{
	DEC(c.B);
	return 1;
}

static int ins_6()
{
	LDRIMM8(c.B);
	return 1;
}

static int ins_7()
{
	RLC(7);
	set_Z(0);
	c.PC += 1;
	c.cycles += 1;
	return 1;
}

static int ins_8()
{
	mem_write_word(mem_get_word(c.PC+1), c.SP);
	c.PC += 3;
	c.cycles += 5;
	return 1;
}

static int ins_9()
{
	unsigned int i;
	i = get_HL() + get_BC();
	set_N(0);
	set_C(i >= 0x10000);
	set_H((i&0xFFF) < (get_HL()&0xFFF));
	set_HL(i&0xFFFF);
	c.PC += 1;
	c.cycles += 2;
	return 1;
}

static int ins_10()
{
	c.A = mem_get_byte(get_BC());
	c.PC += 1;
	c.cycles += 2;
	return 1;
}

static int ins_11()
{
	unsigned short s;
	s = get_BC();
	s--;
	set_BC(s);
	c.PC += 1;
	c.cycles += 2;
	return 1;
}

static int ins_12()
{
	INC(c.C);
	return 1;
}

static int ins_13()
{
	DEC(c.C);
	return 1;
}

static int ins_14()
{
	LDRIMM8(c.C);
	return 1;
}

static int ins_15()
{
	RRC(7);
	set_Z(0);
	c.PC += 1;
	c.cycles += 1;
	return 1;
}

static int ins_17()
{
	unsigned short s;
	s = mem_get_word(c.PC+1);
	set_DE(s);
	c.PC += 3;
	c.cycles += 3;
	return 1;
}

static int ins_18()
{
	mem_write_byte(get_DE(), c.A);
	c.PC += 1;
	c.cycles += 2;
	return 1;
}

static int ins_19()
{
	unsigned short s;
	s = get_DE();
	s++;
	set_DE(s);
	c.PC += 1;
	c.cycles += 2;
	return 1;
}

static int ins_20()
{
	INC(c.D);
	return 1;
}

static int ins_21()
{
	DEC(c.D);
	return 1;
}

static int ins_22()
{
	LDRIMM8(c.D);
	return 1;
}

static int ins_23()
{
	RL(7);
	set_Z(0);
	c.PC += 1;
	c.cycles += 1;
	return 1;
}

static int ins_24()
{
	c.PC += (signed char)mem_get_byte(c.PC+1) + 2;
	c.cycles += 3;
	return 1;
}

static int ins_25()
{
	unsigned int i;
	i = get_HL() + get_DE();
	set_H((i&0xFFF) < (get_HL()&0xFFF));
	set_HL(i);
	set_N(0);
	set_C(i > 0xFFFF);
	c.PC += 1;
	c.cycles += 3;
	return 1;
}

static int ins_26()
{
	c.A = mem_get_byte(get_DE());
	c.PC += 1;
	c.cycles += 2;
	return 1;
}

static int ins_27()
{
	unsigned short s;
	s = get_DE();
	s--;
	set_DE(s);
	c.PC += 1;
	c.cycles += 2;
	return 1;
}

static int ins_28()
{
	INC(c.E);
	return 1;
}

static int ins_29()
{
	DEC(c.E);
	return 1;
}

static int ins_30()
{
	LDRIMM8(c.E);
	return 1;
}

static int ins_31()
{
	RR(7);
	set_Z(0);
	c.PC += 1;
	c.cycles += 1;
	return 1;
}

static int ins_32()
{
	if(flag_Z == 0)
	{
		c.PC += (signed char)mem_get_byte(c.PC+1) + 2;
		c.cycles += 3;
	} else {
		c.PC += 2;
		c.cycles += 2;
	}
	return 1;
}

static int ins_33()
{
	unsigned short s;
	s = mem_get_word(c.PC+1);
	set_HL(s);
	c.PC += 3;
	c.cycles += 3;
	return 1;
}

static int ins_34()
{
	unsigned int i;
	i = get_HL();
	mem_write_byte(i, c.A);
	i++;
	set_HL(i);
	c.PC += 1;
	c.cycles += 2;
	return 1;
}

static int ins_35()
{
	unsigned short s;
	s = get_HL();
	s++;
	set_HL(s);
	c.PC += 1;
	c.cycles += 2;
	return 1;
}

static int ins_36()
{
	INC(c.H);
	return 1;
}

static int ins_37()
{
	DEC(c.H);
	return 1;
}

static int ins_38()
{
	LDRIMM8(c.H);
	return 1;
}

static int ins_39()
{
	unsigned short s;
	s = c.A;
	if(flag_N)
	{
		if(flag_H)
			s = (s - 0x06)&0xFF;
		if(flag_C)
			s -= 0x60;
	}
	else
	{
		if(flag_H || (s & 0xF) > 9)
			s += 0x06;
		if(flag_C || s > 0x9F)
			s += 0x60;
	}
	c.A = s;
	set_H(0);
	set_Z(!c.A);
	if(s >= 0x100)
		set_C(1);
	c.PC += 1;
	c.cycles += 1;
	return 1;
}

static int ins_40()
{
	if(flag_Z == 1)
	{
		c.PC += (signed char)mem_get_byte(c.PC+1) + 2;
		c.cycles += 3;
	} else {
		c.PC += 2;
		c.cycles += 2;
	}
	return 1;
}

static int ins_41()
{
	unsigned int i;
	i = get_HL()*2;
	set_H((i&0x7FF) < (get_HL()&0x7FF));
	set_C(i > 0xFFFF);
	set_HL(i);
	set_N(0);
	c.PC += 1;
	c.cycles += 2;
	return 1;
}

static int ins_42()
{
	unsigned short s;
	s = get_HL();
	c.A = mem_get_byte(s);
	set_HL(s+1);
	c.PC += 1;
	c.cycles += 2;
	return 1;
}

static int ins_43()
{
	set_HL(get_HL()-1);
	c.PC += 1;
	c.cycles += 2;
	return 1;
}

static int ins_44()
{
	INC(c.L);
	return 1;
}

static int ins_45()
{
	DEC(c.L);
	return 1;
}

static int ins_46()
{
	LDRIMM8(c.L);
	return 1;
}

static int ins_47()
{
	c.A = ~c.A;
	set_N(1);
	set_H(1);
	c.PC += 1;
	c.cycles += 1;
	return 1;
}

static int ins_48()
{
	if(flag_C == 0)
	{
		c.PC += (signed char)mem_get_byte(c.PC+1) + 2;
		c.cycles += 3;
	} else {
		c.PC += 2;
		c.cycles += 2;
	}
	return 1;
}

static int ins_49()
{
	c.SP = mem_get_word(c.PC+1);
	c.PC += 3;
	c.cycles += 3;
	return 1;
}

static int ins_50()
{
	unsigned int i;
	i = get_HL();
	mem_write_byte(i, c.A);
	set_HL(i-1);
	c.PC += 1;
	c.cycles += 2;
	return 1;
}

static int ins_51()
{
	c.SP++;
	c.PC += 1;
	c.cycles += 2;
	return 1;
}

static int ins_52()
{
	unsigned char t;
	t = mem_get_byte(get_HL());
	t++;
	mem_write_byte(get_HL(), t);
	set_Z(!t);
	set_N(0);
	set_H((t & 0xF) == 0);
	c.PC += 1;
	c.cycles += 1;
	return 1;
}

static int ins_53()
{
	unsigned char t;
	t = mem_get_byte(get_HL());
	t--;
	mem_write_byte(get_HL(), t);
	set_Z(!t);
	set_N(1);
	set_H((t & 0xF) == 0xF);
	c.PC += 1;
	c.cycles += 1;
	return 1;
}

static int ins_54()
{
	unsigned char t;
	t = mem_get_byte(c.PC+1);
	mem_write_byte(get_HL(), t);
	c.PC += 2;
	c.cycles += 3;
	return 1;
}

static int ins_55()
{
	set_N(0);
	set_H(0);
	set_C(1);
	c.PC += 1;
	c.cycles += 1;
	return 1;
}

static int ins_56()
{
	if(flag_C == 1)
	{
		c.PC += (signed char)mem_get_byte(c.PC+1) + 2;
		c.cycles += 3;
	} else {
		c.PC += 2;
		c.cycles += 2;
	}
	return 1;
}

static int ins_57()
{
	unsigned int i;
	i = get_HL() + c.SP;
	set_H((i&0x7FF) < (get_HL()&0x7FF));
	set_C(i > 0xFFFF);
	set_N(0);
	set_HL(i);
	c.PC += 1;
	c.cycles += 2;
	return 1;
}

static int ins_58()
{
	c.A = mem_get_byte(get_HL());
	set_HL(get_HL()-1);
	c.PC += 1;
	c.cycles += 2;
	return 1;
}

static int ins_59()
{
	c.SP--;
	c.PC += 1;
	c.cycles += 2;
	return 1;
}

static int ins_60()
{
	INC(c.A);
	return 1;
}

static int ins_61()
{
	DEC(c.A);
	return 1;
}

static int ins_62()
{
	LDRIMM8(c.A);
	return 1;
}

static int ins_63()
{
	set_N(0);
	set_H(0);
	set_C(!flag_C);
	c.PC += 1;
	c.cycles += 1;
	return 1;
}

static int ins_64()
{
	LDRR(c.B, c.B);
	return 1;
}

static int ins_65()
{
	LDRR(c.B, c.C);
	return 1;
}

static int ins_66()
{
	LDRR(c.B, c.D);
	return 1;
}

static int ins_67()
{
	LDRR(c.B, c.E);
	return 1;
}

static int ins_68()
{
	LDRR(c.B, c.H);
	return 1;
}

static int ins_69()
{
	LDRR(c.B, c.L);
	return 1;
}

static int ins_70()
{
	c.B = mem_get_byte(get_HL());
	c.PC += 1;
	c.cycles += 2;
	return 1;
}

static int ins_71()
{
	LDRR(c.B, c.A);
	return 1;
}

static int ins_72()
{
	LDRR(c.C, c.B);
	return 1;
}

static int ins_73()
{
	LDRR(c.C, c.C);
	return 1;
}

static int ins_74()
{
	LDRR(c.C, c.D);
	return 1;
}

static int ins_75()
{
	LDRR(c.C, c.E);
	return 1;
}

static int ins_76()
{
	LDRR(c.C, c.H);
	return 1;
}

static int ins_77()
{
	LDRR(c.C, c.L);
	return 1;
}

static int ins_78()
{
	c.C = mem_get_byte(get_HL());
	c.PC += 1;
	c.cycles += 2;
	return 1;
}

static int ins_79()
{
	LDRR(c.C, c.A);
	return 1;
}

static int ins_80()
{
	LDRR(c.D, c.B);
	return 1;
}

static int ins_81()
{
	LDRR(c.D, c.C);
	return 1;
}

static int ins_82()
{
	LDRR(c.D, c.D);
	return 1;
}

static int ins_83()
{
	LDRR(c.D, c.E);
	return 1;
}

static int ins_84()
{
	LDRR(c.D, c.H);
	return 1;
}

static int ins_85()
{
	LDRR(c.D, c.L);
	return 1;
}

static int ins_86()
{
	c.D = mem_get_byte(get_HL());
	c.PC += 1;
	c.cycles += 2;
	return 1;
}

static int ins_87()
{
	LDRR(c.D, c.A);
	return 1;
}

static int ins_88()
{
	LDRR(c.E, c.B);
	return 1;
}

static int ins_89()
{
	LDRR(c.E, c.C);
	return 1;
}

static int ins_90()
{
	LDRR(c.E, c.D);
	return 1;
}

static int ins_91()
{
	LDRR(c.E, c.E);
	return 1;
}

static int ins_92()
{
	LDRR(c.E, c.H);
	return 1;
}

static int ins_93()
{
	LDRR(c.E, c.L);
	return 1;
}

static int ins_94()
{
	c.E = mem_get_byte(get_HL());
	c.PC += 1;
	c.cycles += 2;
	return 1;
}

static int ins_95()
{
	LDRR(c.E, c.A);
	return 1;
}

static int ins_96()
{
	LDRR(c.H, c.B);
	return 1;
}

static int ins_97()
{
	LDRR(c.H, c.C);
	return 1;
}

static int ins_98()
{
	LDRR(c.H, c.D);
	return 1;
}

static int ins_99()
{
	LDRR(c.H, c.E);
	return 1;
}

static int ins_100()
{
	LDRR(c.H, c.H);
	return 1;
}

static int ins_101()
{
	LDRR(c.H, c.L);
	return 1;
}

static int ins_102()
{
	c.H = mem_get_byte(get_HL());
	c.PC += 1;
	c.cycles += 2;
	return 1;
}

static int ins_103()
{
	LDRR(c.H, c.A);
	return 1;
}

static int ins_104()
{
	LDRR(c.L, c.B);
	return 1;
}

static int ins_105()
{
	LDRR(c.L, c.C);
	return 1;
}

static int ins_106()
{
	LDRR(c.L, c.D);
	return 1;
}

static int ins_107()
{
	LDRR(c.L, c.E);
	return 1;
}

static int ins_108()
{
	LDRR(c.L, c.H);
	return 1;
}

static int ins_109()
{
	LDRR(c.L, c.L);
	return 1;
}

static int ins_110()
{
	c.L = mem_get_byte(get_HL());
	c.PC += 1;
	c.cycles += 2;
	return 1;
}

static int ins_111()
{
	LDRR(c.L, c.A);
	return 1;
}

static int ins_112()
{
	mem_write_byte(get_HL(), c.B);
	c.PC += 1;
	c.cycles += 2;
	return 1;
}

static int ins_113()
{
	mem_write_byte(get_HL(), c.C);
	c.PC += 1;
	c.cycles += 2;
	return 1;
}

static int ins_114()
{
	mem_write_byte(get_HL(), c.D);
	c.PC += 1;
	c.cycles += 2;
	return 1;
}

static int ins_115()
{
	mem_write_byte(get_HL(), c.E);
	c.PC += 1;
	c.cycles += 2;
	return 1;
}

static int ins_116()
{
	mem_write_byte(get_HL(), c.H);
	c.PC += 1;
	c.cycles += 2;
	return 1;
}

static int ins_117()
{
	mem_write_byte(get_HL(), c.L);
	c.PC += 1;
	c.cycles += 2;
	return 1;
}

static int ins_118()
{
	//printf("CPU halted, IF: %02X, IE: %02X\n", interrupt_get_IF(), interrupt_get_mask() );
	halted = 1;
	c.PC += 1;
	c.cycles += 1;
	return 1;
}

static int ins_119()
{
	mem_write_byte(get_HL(), c.A);
	c.PC += 1;
	c.cycles += 2;
	return 1;
}

static int ins_120()
{
	LDRR(c.A, c.B);
	return 1;
}

static int ins_121()
{
	LDRR(c.A, c.C);
	return 1;
}

static int ins_122()
{
	LDRR(c.A, c.D);
	return 1;
}

static int ins_123()
{
	LDRR(c.A, c.E);
	return 1;
}

static int ins_124()
{
	LDRR(c.A, c.H);
	return 1;
}

static int ins_125()
{
	LDRR(c.A, c.L);
	return 1;
}

static int ins_126()
{
	c.A = mem_get_byte(get_HL());
	c.PC += 1;
	c.cycles += 2;
	return 1;
}

static int ins_127()
{
	LDRR(c.A, c.A);
	return 1;
}

static int ins_128()
{
	unsigned int i;
	i = c.A + c.B;
	set_H((c.A&0xF)+(c.B&0xF) > 0xF);
	set_C(i > 0xFF);
	set_N(0);
	c.A = i;
	set_Z(!c.A);
	c.PC += 1;
	c.cycles += 1;
	return 1;
}

static int ins_129()
{
	unsigned int i;
	i = c.A + c.C;
	set_H((c.A&0xF)+(c.C&0xF) > 0xF);
	set_C(i > 0xFF);
	set_N(0);
	c.A = i;
	set_Z(!c.A);
	c.PC += 1;
	c.cycles += 1;
	return 1;
}

static int ins_130()
{
	unsigned int i;
	i = c.A + c.D;
	set_H((c.A&0xF)+(c.D&0xF) > 0xF);
	set_C(i > 0xFF);
	set_N(0);
	c.A = i;
	set_Z(!c.A);
	c.PC += 1;
	c.cycles += 1;
	return 1;
}

static int ins_131()
{
	unsigned int i;
	i = c.A + c.E;
	set_H((c.A&0xF)+(c.E&0xF) > 0xF);
	set_C(i > 0xFF);
	set_N(0);
	c.A = i;
	set_Z(!c.A);
	c.PC += 1;
	c.cycles += 1;
	return 1;
}

static int ins_132()
{
	unsigned int i;
	i = c.A + c.H;
	set_H((c.A&0xF)+(c.H&0xF) > 0xF);
	set_C(i > 0xFF);
	set_N(0);
	c.A = i;
	set_Z(!c.A);
	c.PC += 1;
	c.cycles += 1;
	return 1;
}

static int ins_133()
{
	unsigned int i;
	i = c.A + c.L;
	set_H((c.A&0xF)+(c.L&0xF) > 0xF);
	set_C(i > 0xFF);
	set_N(0);
	c.A = i;
	set_Z(!c.A);
	c.PC += 1;
	c.cycles += 1;
	return 1;
}

static int ins_134()
{
	unsigned int i;
	i = c.A + mem_get_byte(get_HL());
	set_H((i&0xF) < (c.A&0xF));
	set_C(i > 0xFF);
	set_N(0);
	c.A = i;
	set_Z(!c.A);
	c.PC += 1;
	c.cycles += 2;
	return 1;
}

static int ins_135()
{
	unsigned int i;
	i = c.A + c.A;
	set_H((c.A&0xF)+(c.A&0xF) > 0xF);
	set_C(i > 0xFF);
	set_N(0);
	c.A = i;
	set_Z(!c.A);
	c.PC += 1;
	c.cycles += 1;
	return 1;
}

static int ins_136()
{
	unsigned int i;
	i = c.A + c.B + flag_C >= 0x100;
	set_N(0);
	set_H(((c.A&0xF) + (c.B&0xF) + flag_C) >= 0x10);
	c.A = c.A + c.B + flag_C;
	set_C(i);
	set_Z(!c.A);
	c.PC += 1;
	c.cycles += 1;
	return 1;
}

static int ins_137()
{
	unsigned int i;
	i = c.A + c.C + flag_C >= 0x100;
	set_N(0);
	set_H(((c.A&0xF) + (c.C&0xF) + flag_C) >= 0x10);
	c.A = c.A + c.C + flag_C;
	set_C(i);
	set_Z(!c.A);
	c.PC += 1;
	c.cycles += 1;
	return 1;
}

static int ins_138()
{
	unsigned int i;
	i = c.A + c.D + flag_C >= 0x100;
	set_N(0);
	set_H(((c.A&0xF) + (c.D&0xF) + flag_C) >= 0x10);
	c.A = c.A + c.D + flag_C;
	set_C(i);
	set_Z(!c.A);
	c.PC += 1;
	c.cycles += 1;
	return 1;
}

static int ins_139()
{
	unsigned int i;
	i = c.A + c.E + flag_C >= 0x100;
	set_N(0);
	set_H(((c.A&0xF) + (c.E&0xF) + flag_C) >= 0x10);
	c.A = c.A + c.E + flag_C;
	set_C(i);
	set_Z(!c.A);
	c.PC += 1;
	c.cycles += 1;
	return 1;
}

static int ins_140()
{
	unsigned int i;
	i = c.A + c.H + flag_C >= 0x100;
	set_N(0);
	set_H(((c.A&0xF) + (c.H&0xF) + flag_C) >= 0x10);
	c.A = c.A + c.H + flag_C;
	set_C(i);
	set_Z(!c.A);
	c.PC += 1;
	c.cycles += 1;
	return 1;
}

static int ins_141()
{
	unsigned int i;
	i = c.A + c.L + flag_C >= 0x100;
	set_N(0);
	set_H(((c.A&0xF) + (c.L&0xF) + flag_C) >= 0x10);
	c.A = c.A + c.L + flag_C;
	set_C(i);
	set_Z(!c.A);
	c.PC += 1;
	c.cycles += 1;
	return 1;
}

static int ins_142()
{
	unsigned char t;
	unsigned int i;
	t = mem_get_byte(get_HL());
	i = c.A + t + flag_C >= 0x100;
	set_N(0);
	set_H(((c.A&0xF) + (t&0xF) + flag_C) >= 0x10);
	c.A = c.A + t + flag_C;
	set_C(i);
	set_Z(!c.A);
	c.PC += 1;
	c.cycles += 2;
	return 1;
}

static int ins_143()
{
	unsigned int i;
	i = c.A + c.A + flag_C >= 0x100;
	set_N(0);
	set_H(((c.A&0xF) + (c.A&0xF) + flag_C) >= 0x10);
	c.A = c.A + c.A + flag_C;
	set_C(i);
	set_Z(!c.A);
	c.PC += 1;
	c.cycles += 1;
	return 1;
}

static int ins_144()
{
	SUBR(c.B);
	return 1;
}

static int ins_145()
{
	SUBR(c.C);
	return 1;
}

static int ins_146()
{
	SUBR(c.D);
	return 1;
}

static int ins_147()
{
	SUBR(c.E);
	return 1;
}

static int ins_148()
{
	SUBR(c.H);
	return 1;
}

static int ins_149()
{
	SUBR(c.L);
	return 1;
}

static int ins_150()
{
	unsigned char t;
	t = mem_get_byte(get_HL());
	set_C((c.A - t) < 0);
	set_H(((c.A - t)&0xF) > (c.A&0xF));
	c.A -= t;
	set_Z(!c.A);
	set_N(1);
	c.PC += 1;
	c.cycles += 2;
	return 1;
}

static int ins_151()
{
	SUBR(c.A);
	return 1;
}

static int ins_152()
{
	unsigned char t;
	t = flag_C + c.B;
	set_H(((c.A&0xF) - (c.B&0xF) - flag_C) < 0);
	set_C((c.A - c.B - flag_C) < 0);
	set_N(1);
	c.A -= t;
	set_Z(!c.A);
	c.PC += 1;
	c.cycles += 1;
	return 1;
}

static int ins_153()
{
	unsigned char t;
	t = flag_C + c.C;
	set_H(((c.A&0xF) - (c.C&0xF) - flag_C) < 0);
	set_C((c.A - c.C - flag_C) < 0);
	set_N(1);
	c.A -= t;
	set_Z(!c.A);
	c.PC += 1;
	c.cycles += 1;
	return 1;
}

static int ins_154()
{
	unsigned char t;
	t = flag_C + c.D;
	set_H(((c.A&0xF) - (c.D&0xF) - flag_C) < 0);
	set_C((c.A - c.D - flag_C) < 0);
	set_N(1);
	c.A -= t;
	set_Z(!c.A);
	c.PC += 1;
	c.cycles += 1;
	return 1;
}

static int ins_155()
{
	unsigned char t;
	t = flag_C + c.E;
	set_H(((c.A&0xF) - (c.E&0xF) - flag_C) < 0);
	set_C((c.A - c.E - flag_C) < 0);
	set_N(1);
	c.A -= t;
	set_Z(!c.A);
	c.PC += 1;
	c.cycles += 1;
	return 1;
}

static int ins_156()
{
	unsigned char t;
	t = flag_C + c.H;
	set_H(((c.A&0xF) - (c.H&0xF) - flag_C) < 0);
	set_C((c.A - c.H - flag_C) < 0);
	set_N(1);
	c.A -= t;
	set_Z(!c.A);
	c.PC += 1;
	c.cycles += 1;
	return 1;
}

static int ins_157()
{
	unsigned char t;
	t = flag_C + c.L;
	set_H(((c.A&0xF) - (c.L&0xF) - flag_C) < 0);
	set_C((c.A - c.L - flag_C) < 0);
	set_N(1);
	c.A -= t;
	set_Z(!c.A);
	c.PC += 1;
	c.cycles += 1;
	return 1;
}

static int ins_158()
{
	unsigned char b;
	unsigned char t;
	t = mem_get_byte(get_HL());
	b = flag_C + t;
	set_H(((c.A&0xF) - (t&0xF) - flag_C) < 0);
	set_C((c.A - t - flag_C) < 0);
	set_N(1);
	c.A -= b;
	set_Z(!c.A);
	c.PC += 1;
	c.cycles += 2;
	return 1;
}

static int ins_159()
{
	unsigned char t;
	t = flag_C + c.A;
	set_H(((c.A&0xF) - (c.A&0xF) - flag_C) < 0);
	set_C((c.A - c.A - flag_C) < 0);
	set_N(1);
	c.A -= t;
	set_Z(!c.A);
	c.PC += 1;
	c.cycles += 1;
	return 1;
}

static int ins_160()
{
	ANDR(c.B);
	return 1;
}

static int ins_161()
{
	ANDR(c.C);
	return 1;
}

static int ins_162()
{
	ANDR(c.D);
	return 1;
}

static int ins_163()
{
	ANDR(c.E);
	return 1;
}

static int ins_164()
{
	ANDR(c.H);
	return 1;
}

static int ins_165()
{
	ANDR(c.L);
	return 1;
}

static int ins_166()
{
	c.A &= mem_get_byte(get_HL());
	set_Z(!c.A);
	set_H(1);
	set_N(0);
	set_C(0);
	c.PC += 1;
	c.cycles += 1;
	return 1;
}

static int ins_167()
{
	ANDR(c.A);
	return 1;
}

static int ins_168()
{
	XORR(c.B);
	return 1;
}

static int ins_169()
{
	XORR(c.C);
	return 1;
}

static int ins_170()
{
	XORR(c.D);
	return 1;
}

static int ins_171()
{
	XORR(c.E);
	return 1;
}

static int ins_172()
{
	XORR(c.H);
	return 1;
}

static int ins_173()
{
	XORR(c.L);
	return 1;
}

static int ins_174()
{
	c.A ^= mem_get_byte(get_HL());
	c.F = (!c.A)<<7;
	c.PC += 1;
	c.cycles += 1;
	return 1;
}

static int ins_175()
{
	XORR(c.A);
	return 1;
}

static int ins_176()
{
	ORR(c.B);
	return 1;
}

static int ins_177()
{
	ORR(c.C);
	return 1;
}

static int ins_178()
{
	ORR(c.D);
	return 1;
}

static int ins_179()
{
	ORR(c.E);
	return 1;
}

static int ins_180()
{
	ORR(c.H);
	return 1;
}

static int ins_181()
{
	ORR(c.L);
	return 1;
}

static int ins_182()
{
	c.A |= mem_get_byte(get_HL());
	c.F = (!c.A)<<7;
	c.PC += 1;
	c.cycles += 2;
	return 1;
}

static int ins_183()
{
	ORR(c.A);
	return 1;
}

static int ins_184()
{
	CPR(c.B);
	return 1;
}

static int ins_185()
{
	CPR(c.C);
	return 1;
}

static int ins_186()
{
	CPR(c.D);
	return 1;
}

static int ins_187()
{
	CPR(c.E);
	return 1;
}

static int ins_188()
{
	CPR(c.H);
	return 1;
}

static int ins_189()
{
	CPR(c.L);
	return 1;
}

static int ins_190()
{
	unsigned char t;
	t = mem_get_byte(get_HL());
	set_Z(c.A == t);
	set_H(((c.A - t)&0xF) > (c.A&0xF));
	set_N(1);
	set_C((c.A - t) < 0);
	c.PC += 1;
	c.cycles += 1;
	return 1;
}

static int ins_191()
{
	CPR(c.A);
	return 1;
}

static int ins_192()
{
	if(!flag_Z)
	{
		c.PC = mem_get_word(c.SP);
		c.SP += 2;
		c.cycles += 3;
	} else {
		c.PC += 1;
		c.cycles += 1;
	}
	return 1;
}

static int ins_193()
{
	unsigned short s;
	s = mem_get_word(c.SP);
	set_BC(s);
	c.SP += 2;
	c.PC += 1;
	c.cycles += 3;
	return 1;
}

static int ins_194()
{
	if(flag_Z == 0)
	{
		c.PC = mem_get_word(c.PC+1);
	} else {
		c.PC += 3;
	}
	c.cycles += 3;
	return 1;
}

static int ins_195()
{
	c.PC = mem_get_word(c.PC+1);
	c.cycles += 4;
	return 1;
}

static int ins_196()
{
	if(flag_Z == 0)
	{
		c.SP -= 2;
		mem_write_word(c.SP, c.PC+3);
		c.PC = mem_get_word(c.PC+1);
		c.cycles += 6;
	} else {
		c.PC += 3;
		c.cycles += 3;
	}
	return 1;
}

static int ins_197()
{
	c.SP -= 2;
	mem_write_word(c.SP, get_BC());
	c.PC += 1;
	c.cycles += 3;
	return 1;
}

static int ins_198()
{
	unsigned char t;
	t = mem_get_byte(c.PC+1);
	set_C((c.A + t) >= 0x100);
	set_H(((c.A + t)&0xF) < (c.A&0xF));
	c.A += t;
	set_N(0);
	set_Z(!c.A);
	c.PC += 2;
	c.cycles += 2;
	return 1;
}

static int ins_199()
{
	c.SP -= 2;
	mem_write_word(c.SP, c.PC+1);
	c.PC = 0;
	c.cycles += 3;
	return 1;
}

static int ins_200()
{
	if(flag_Z == 1)
	{
		c.PC = mem_get_word(c.SP);
		c.SP += 2;
		c.cycles += 3;
	} else {
		c.PC += 1;
		c.cycles += 1;
	}
	return 1;
}

static int ins_201()
{
	c.PC = mem_get_word(c.SP);
	c.SP += 2;
	c.cycles += 3;
	return 1;
}

static int ins_202()
{
	if(flag_Z == 1)
	{
		c.PC = mem_get_word(c.PC+1);
	} else {
		c.PC += 3;
	}
	c.cycles += 3;
	return 1;
}

static int ins_203()
{
	decode_CB(mem_get_byte(c.PC+1));
	c.PC += 2;
	c.cycles += 2;
	return 1;
}

static int ins_204()
{
	if(flag_Z == 1)
	{
		c.SP -= 2;
		mem_write_word(c.SP, c.PC+3);
		c.PC = mem_get_word(c.PC+1);
		c.cycles += 6;
	} else {
		c.PC += 3;
		c.cycles += 3;
	}
	return 1;
}

static int ins_205()
{
	c.SP -= 2;
	mem_write_word(c.SP, c.PC+3);
	c.PC = mem_get_word(c.PC+1);
	c.cycles += 6;
	return 1;
}

static int ins_206()
{
	unsigned char t;
	unsigned int i;
	t = mem_get_byte(c.PC+1);
	i = c.A + t + flag_C >= 0x100;
	set_N(0);
	set_H(((c.A&0xF) + (t&0xF) + flag_C) >= 0x10);
	c.A = c.A + t + flag_C;
	set_C(i);
	set_Z(!c.A);
	c.PC += 2;
	c.cycles += 2;
	return 1;
}

static int ins_207()
{
	c.SP -= 2;
	mem_write_word(c.SP, c.PC+1);
	c.PC = 0x0008;
	c.cycles += 4;
	return 1;
}

static int ins_208()
{
	if(flag_C == 0)
	{
		c.PC = mem_get_word(c.SP);
		c.SP += 2;
		c.cycles += 3;
	} else {
		c.PC += 1;
		c.cycles += 1;
	}
	return 1;
}

static int ins_209()
{
	unsigned short s;
	s = mem_get_word(c.SP);
	set_DE(s);
	c.SP += 2;
	c.PC += 1;
	c.cycles += 3;
	return 1;
}

static int ins_210()
{
	if(flag_C == 0)
	{
		c.PC = mem_get_word(c.PC+1);
	} else {
		c.PC += 3;
	}
	c.cycles += 3;
	return 1;
}

static int ins_212()
{
	if(flag_C == 0)
	{
		c.SP -= 2;
		mem_write_word(c.SP, c.PC+3);
		c.PC = mem_get_word(c.PC+1);
		c.cycles += 6;
	} else {
		c.PC += 3;
		c.cycles += 3;
	}
	return 1;
}

static int ins_213()
{
	c.SP -= 2;
	mem_write_word(c.SP, get_DE());
	c.PC += 1;
	c.cycles += 3;
	return 1;
}

static int ins_214()
{
	unsigned char t;
	t = mem_get_byte(c.PC+1);
	set_C((c.A - t) < 0);
	set_H(((c.A - t)&0xF) > (c.A&0xF));
	c.A -= t;
	set_N(1);
	set_Z(!c.A);
	c.PC += 2;
	c.cycles += 2;
	return 1;
}

static int ins_215()
{
	c.SP -= 2;
	mem_write_word(c.SP, c.PC+1);
	c.PC = 0x0010;
	c.cycles += 4;
	return 1;
}

static int ins_216()
{
	if(flag_C == 1)
	{
		c.PC = mem_get_word(c.SP);
		c.SP += 2;
		c.cycles += 3;
	} else {
		c.PC += 1;
		c.cycles += 1;
	}
	return 1;
}

static int ins_217()
{
	c.PC = mem_get_word(c.SP);
	c.SP += 2;
	c.cycles += 4;
	interrupt_enable();
	return 1;
}

static int ins_218()
{
	if(flag_C)
	{
		c.PC = mem_get_word(c.PC+1);
	} else {
		c.PC += 3;
	}
	c.cycles += 3;
	return 1;
}

static int ins_220()
{
	if(flag_C == 1)
	{
		c.SP -= 2;
		mem_write_word(c.SP, c.PC+3);
		c.PC = mem_get_word(c.PC+1);
		c.cycles += 6;
	} else {
		c.PC += 3;
		c.cycles += 3;
	}
	return 1;
}

static int ins_222()
{
	unsigned char b;
	unsigned char t;
	t = mem_get_byte(c.PC+1);
	b = flag_C;
	set_H(((t&0xF) + flag_C) > (c.A&0xF));
	set_C(t + flag_C > c.A);
	set_N(1);
	c.A -= (b + t);
	set_Z(!c.A);
	c.PC += 2;
	c.cycles += 2;
	return 1;
}

static int ins_223()
{
	c.SP -= 2;
	mem_write_word(c.SP, c.PC+1);
	c.PC = 0x0018;
	c.cycles += 3;
	return 1;
}

static int ins_224()
{
	unsigned char t;
	t = mem_get_byte(c.PC+1);
	mem_write_byte(0xFF00 + t, c.A);
	c.PC += 2;
	c.cycles += 3;
	return 1;
}

static int ins_225()
{
	unsigned int i;
	i = mem_get_word(c.SP);
	set_HL(i);
	c.SP += 2;
	c.PC += 1;
	c.cycles += 3;
	return 1;
}

static int ins_226()
{
	unsigned short s;
	s = 0xFF00 + c.C;
	mem_write_byte(s, c.A);
	c.PC += 1;
	c.cycles += 2;
	return 1;
}

static int ins_229()
{
	c.SP -= 2;
	mem_write_word(c.SP, get_HL());
	c.PC += 1;
	c.cycles += 3;
	return 1;
}

static int ins_230()
{
	unsigned char t;
	t = mem_get_byte(c.PC+1);
	set_N(0);
	set_H(1);
	set_C(0);
	c.A = t & c.A;
	set_Z(!c.A);
	c.PC += 2;
	c.cycles += 2;
	return 1;
}

static int ins_231()
{
	c.SP -= 2;
	mem_write_word(c.SP, c.PC+1);
	c.PC = 0x20;
	c.cycles += 4;
	return 1;
}

static int ins_232()
{
	unsigned int i;
	i = mem_get_byte(c.PC+1);
	set_Z(0);
	set_N(0);
	set_C(((c.SP+i)&0xFF) < (c.SP&0xFF));
	set_H(((c.SP+i)&0xF) < (c.SP&0xF));
	c.SP = c.SP + (signed char)i;
	c.PC += 2;
	c.cycles += 4;
	return 1;
}

static int ins_233()
{
	c.PC = get_HL();
	c.cycles += 1;
	return 1;
}

static int ins_234()
{
	unsigned short s;
	s = mem_get_word(c.PC+1);
	mem_write_byte(s, c.A);
	c.PC += 3;
	c.cycles += 4;
	return 1;
}

static int ins_238()
{
	c.A ^= mem_get_byte(c.PC+1);
	c.F = (!c.A)<<7;
	c.PC += 2;
	c.cycles += 2;
	return 1;
}

static int ins_239()
{
	c.SP -= 2;
	mem_write_word(c.SP, c.PC+1);
	c.PC = 0x28;
	c.cycles += 4;
	return 1;
}

static int ins_240()
{
	unsigned char t;
	t = mem_get_byte(c.PC+1);
	c.A = mem_get_byte(0xFF00 + t);
	c.PC += 2;
	c.cycles += 3;
	return 1;
}

static int ins_241()
{
	unsigned short s;
	s = mem_get_word(c.SP);
	set_AF(s&0xFFF0);
	c.SP += 2;
	c.PC += 1;
	c.cycles += 3;
	return 1;
}

static int ins_242()
{
	c.A = mem_get_byte(0xFF00 + c.C);
	c.PC += 1;
	c.cycles += 2;
	return 1;
}

static int ins_243()
{
	c.PC += 1;
	c.cycles += 1;
	interrupt_disable();
	return 1;
}

static int ins_245()
{
	c.SP -= 2;
	mem_write_word(c.SP, get_AF());
	c.PC += 1;
	c.cycles += 3;
	return 1;
}

static int ins_246()
{
	c.A |= mem_get_byte(c.PC+1);
	c.F = (!c.A)<<7;
	c.PC += 2;
	c.cycles += 2;
	return 1;
}

static int ins_247()
{
	c.SP -= 2;
	mem_write_word(c.SP, c.PC+1);
	c.PC = 0x30;
	c.cycles += 4;
	return 1;
}

static int ins_248()
{
	unsigned int i;
	i = mem_get_byte(c.PC+1);
	set_N(0);
	set_Z(0);
	set_C(((c.SP+i)&0xFF) < (c.SP&0xFF));
	set_H(((c.SP+i)&0xF) < (c.SP&0xF));
	set_HL(c.SP + (signed char)i);
	c.PC += 2;
	c.cycles += 3;
	return 1;
}

static int ins_249()
{
	c.SP = get_HL();
	c.PC += 1;
	c.cycles += 2;
	return 1;
}

static int ins_250()
{
	unsigned short s;
	s = mem_get_word(c.PC+1);
	c.A = mem_get_byte(s);
	c.PC += 3;
	c.cycles += 4;
	return 1;
}

static int ins_251()
{
	interrupt_enable();
	c.PC += 1;
	c.cycles += 1;
	return 1;
}

static int ins_254()
{
	unsigned char t;
	t = mem_get_byte(c.PC+1);
	set_Z(c.A == t);
	set_N(1);
	set_H(((c.A - t)&0xF) > (c.A&0xF));
	set_C(c.A < t);
	c.PC += 2;
	c.cycles += 2;
	return 1;
}

static int ins_255()
{
	c.SP -= 2;
	mem_write_word(c.SP, c.PC+1);
	c.PC = 0x0038;
	c.cycles += 4;
	return 1;
}

static int ins_default()
{
	//printf("cycles: %d\n", c.cycles);
	return 0;
}

static int (*ins_table[256])() = {ins_0,ins_1,ins_2,ins_3,ins_4,ins_5,ins_6,ins_7,ins_8,ins_9,ins_10,ins_11,ins_12,ins_13,ins_14,ins_15,ins_default,ins_17,ins_18,ins_19,ins_20,ins_21,ins_22,ins_23,ins_24,ins_25,ins_26,ins_27,ins_28,ins_29,ins_30,ins_31,ins_32,ins_33,ins_34,ins_35,ins_36,ins_37,ins_38,ins_39,ins_40,ins_41,ins_42,ins_43,ins_44,ins_45,ins_46,ins_47,ins_48,ins_49,ins_50,ins_51,ins_52,ins_53,ins_54,ins_55,ins_56,ins_57,ins_58,ins_59,ins_60,ins_61,ins_62,ins_63,ins_64,ins_65,ins_66,ins_67,ins_68,ins_69,ins_70,ins_71,ins_72,ins_73,ins_74,ins_75,ins_76,ins_77,ins_78,ins_79,ins_80,ins_81,ins_82,ins_83,ins_84,ins_85,ins_86,ins_87,ins_88,ins_89,ins_90,ins_91,ins_92,ins_93,ins_94,ins_95,ins_96,ins_97,ins_98,ins_99,ins_100,ins_101,ins_102,ins_103,ins_104,ins_105,ins_106,ins_107,ins_108,ins_109,ins_110,ins_111,ins_112,ins_113,ins_114,ins_115,ins_116,ins_117,ins_118,ins_119,ins_120,ins_121,ins_122,ins_123,ins_124,ins_125,ins_126,ins_127,ins_128,ins_129,ins_130,ins_131,ins_132,ins_133,ins_134,ins_135,ins_136,ins_137,ins_138,ins_139,ins_140,ins_141,ins_142,ins_143,ins_144,ins_145,ins_146,ins_147,ins_148,ins_149,ins_150,ins_151,ins_152,ins_153,ins_154,ins_155,ins_156,ins_157,ins_158,ins_159,ins_160,ins_161,ins_162,ins_163,ins_164,ins_165,ins_166,ins_167,ins_168,ins_169,ins_170,ins_171,ins_172,ins_173,ins_174,ins_175,ins_176,ins_177,ins_178,ins_179,ins_180,ins_181,ins_182,ins_183,ins_184,ins_185,ins_186,ins_187,ins_188,ins_189,ins_190,ins_191,ins_192,ins_193,ins_194,ins_195,ins_196,ins_197,ins_198,ins_199,ins_200,ins_201,ins_202,ins_203,ins_204,ins_205,ins_206,ins_207,ins_208,ins_209,ins_210,ins_default,ins_212,ins_213,ins_214,ins_215,ins_216,ins_217,ins_218,ins_default,ins_220,ins_default,ins_222,ins_223,ins_224,ins_225,ins_226,ins_default,ins_default,ins_229,ins_230,ins_231,ins_232,ins_233,ins_234,ins_default,ins_default,ins_default,ins_238,ins_239,ins_240,ins_241,ins_242,ins_243,ins_default,ins_245,ins_246,ins_247,ins_248,ins_249,ins_250,ins_251,ins_default,ins_default,ins_254,ins_255};

int cpu_cycle(void)
{
	unsigned char b, t;
	unsigned short s;
	unsigned int i;
	
	/* If any interrupts are pending, do them now */
	interrupt_flush();

	/* If the cpu is halted, do nothing instead */
	if(halted)
	{
		/*if(is_debugged)
			printf("Cpu is halted, cycles++, %d\n", halted);*/
		c.cycles += 1;
		return 1;
	}

	/* Otherwise, execute as normal */
	b = mem_get_byte(c.PC);
#ifdef EBUG
	
#endif
	if(is_debugged)
	{
		cpu_print_debug();
	}

	return (*ins_table[b])();
}
