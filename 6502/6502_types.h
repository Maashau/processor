/*******************************************************************************
* MOS 6502 emulator type definitions.
*******************************************************************************/
#ifndef FILE_6502_TYPES_H
#define FILE_6502_TYPES_H
typedef unsigned char U8;
typedef signed char I8;
typedef unsigned short U16;
typedef unsigned long long U64;

typedef U16 mos6502_addr;

typedef struct {
	mos6502_addr PC;
	U8 AC;
	U8 X;
	U8 Y;
	U8 SR;
	U8 SP;
} mos6502_reg_st;

typedef U8				(*mos6502_memRead8)(mos6502_addr address);
typedef mos6502_addr	(*mos6502_memRead16)(mos6502_addr address);
typedef void			(*mos6502_memWrite8)(mos6502_addr address, U8 value);
typedef void			(*mos6502_memWrite16)(mos6502_addr address, U16 value);

typedef struct {
	mos6502_memRead8	read8;
	mos6502_memRead16	read16;
	mos6502_memWrite8	write8;
	mos6502_memWrite16	write16;
} mos6502_memInterface_st;

typedef struct {
	mos6502_reg_st reg;
	mos6502_memInterface_st memIf;
	U64 cycleCount;
} mos6502_processor_st;

enum {
	SR_FLAG_C		= (1 << 0),
	SR_FLAG_Z		= (1 << 1),
	SR_FLAG_I		= (1 << 2),
	SR_FLAG_D		= (1 << 3),
	SR_FLAG_B		= (1 << 4),
	SR_FLAG_UNUSED	= (1 << 5),
	SR_FLAG_V		= (1 << 6),
	SR_FLAG_N		= (1 << 7),
};

typedef enum {
	NON,	// none (JAMs)
	IMP,	// implied
	ACC,	// accumulator
	IMM,	// immediate
	REL,	// relative
	ABS,	// absolute
	ABX,	// absoluteXind
	ABY,	// absoluteYind
	ZPG,	// zeropage
	ZPX,	// zeropageXind
	ZPY,	// zeropageYind
	IND,	// indirect
	INX,	// indexedIndirect
	INY 	// indirectIndexed
} mos6502_addrm;

typedef U8 (*opFn)(mos6502_processor_st * processor, U8 opCodeIndex);

typedef struct {
	opFn handler;
	char * mnemonic;
	mos6502_addrm addrMode;
	U8 bytes;
} opCode_st;

#endif