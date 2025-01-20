/*******************************************************************************
* 65xx.c
*
* 65xx processor emulator.
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "65xx.h"
#include "65xx_opc.h"
#include "65xx_addrm.h"


#include "65xx_opcList.c"

typedef struct {
	mos65xx_addr startAddr;
	mos65xx_addr endAddr;
} address_range;

U8 ROMAddrRangeCount;
address_range ROMAddrList[0xFF];
U8 RAMAddrRangeCount;
address_range RAMAddrList[0xFF];
U8 IOAddrRangeCount;
address_range IOAddrList[0xFF];

U8 opLogging;

void logOperation(
	Processor_65xx *	pProcessor,
	mos65xx_reg_st *		oldReg,
	U8						opCode,
	U8 *					operands
);

FILE * tmpLog;

/*******************************************************************************
* Initialize the processor.
*
* Arguments:
*	pProcessor - Pointer to a processor data structure.
*
* Returns: Nothing
*******************************************************************************/
void mos65xx_init(
	Processor_65xx *		pProcessor,
	Memory_areas *			pMemory,
	mos65xx_memRead			fnMemRead,
	mos65xx_memWrite		fnMemWrite,
	U8						debugLevel,
	void *					pUtil
) {

	memset(pProcessor, 0, sizeof(Processor_65xx));

	pProcessor->memAreas.ROM = pMemory->ROM;
	pProcessor->memAreas.RAM = pMemory->RAM;
	pProcessor->memAreas.IO = pMemory->IO;
	pProcessor->fnMemRead = fnMemRead;
	pProcessor->fnMemWrite = fnMemWrite;
	pProcessor->debugLevel = debugLevel;
	pProcessor->pUtil = pUtil;

	pProcessor->reg.PC = addrm_read16(pProcessor, vector_RESET);
	pProcessor->reg.SR = SR_FLAG_UNUSED | SR_FLAG_B | SR_FLAG_I;
	pProcessor->reg.SP = 0xFF;
	
	DBG_PRINT(pProcessor, printf("                                         |  Registers  | Flags  |              \n"));
	DBG_PRINT(pProcessor, printf("\033[4mADDR | OP       | ASM           | Memory | AC XR YR SP | NVDIZC | Cycles (Tot)\033[m\n"));

	DBG_LOG(pProcessor, tmpLog = fopen("./tempLog.txt", "w+"));
}

/*******************************************************************************
* Handle next instruction
*
* Arguments:
*	pProcessor - Pointer to a processor data structure.
*
* Returns: -
*******************************************************************************/
void mos65xx_handleOp(Processor_65xx * pProcessor) {

	mos65xx_reg_st oldReg = pProcessor->reg;
	U8 operands[2];
	U8 opCode;

	/* Read operands for debug printing. */
	operands[0] = addrm_read8(pProcessor, pProcessor->reg.PC + 1);
	operands[1] = addrm_read8(pProcessor, pProcessor->reg.PC + 2);

	/* Handle instruction. */
	pProcessor->cycles.currentOp = 0;
	opCode = addrm_read8(pProcessor, addrm_IMM(pProcessor));
	mos65xx__opCodes[opCode].handler(pProcessor, opCode, mos65xx__opCodes[opCode].addrm);

	pProcessor->lastOp = opCode;
	pProcessor->cycles.totalCycles += pProcessor->cycles.currentOp;

	logOperation(pProcessor, &oldReg, opCode, operands);
}

/*******************************************************************************
* Pin interrupt triggered
*
* Arguments:
*	pProcessor - Pointer to a processor data structure.
*
* Returns: -
*******************************************************************************/
void mos65xx_irqEnter(Processor_65xx * pProcessor) {

	if ((pProcessor->reg.SR & SR_FLAG_I) == 0 && pProcessor->interrupt == 0) {
		
		DBG_LOG(pProcessor, fprintf(tmpLog, "\r\n--- Entering interrupt routine ------------------------------------------------------------"));

		mos65xx_IRQ(pProcessor, 0, IMP);

		pProcessor->interrupt = 1;

	}
}

/*******************************************************************************
* Return from interrupt hit
*
* Arguments:
*	pProcessor - Pointer to a processor data structure.
*
* Returns: -
*******************************************************************************/
void mos65xx_irqLeave(Processor_65xx * pProcessor) {
	DBG_LOG(pProcessor, fprintf(tmpLog, "\r\n--- Leaving interrupt routine -------------------------------------------------------------"));

	pProcessor->interrupt = 0;
}

/*******************************************************************************
* Mark address range as ROM.
*
* Arguments:
*	startAddr	- Starting address of the ROM region
*	endAddr		- Ending address of the ROM region
*
* Returns: -
*******************************************************************************/
void addROMArea(mos65xx_addr startAddr, mos65xx_addr endAddr) {
	if (startAddr > endAddr) {
		printf("\n\nInvalid ROM area range (0x%04x - 0x%04x)...\n\n", startAddr, endAddr);
		exit(1);
	}

	ROMAddrList[ROMAddrRangeCount].startAddr = startAddr;
	ROMAddrList[ROMAddrRangeCount].endAddr = endAddr;

	ROMAddrRangeCount++;
}

/*******************************************************************************
* Check if given address is on ROM region.
*
* Arguments:
*	address		- Address to be checked
*
* Returns: 1 if ROM, otherwise 0
*******************************************************************************/
U8 isROMAddress(mos65xx_addr address) {
	for (int areaIdx = 0; areaIdx < ROMAddrRangeCount; areaIdx++) {
		if (ROMAddrList[areaIdx].startAddr <= address
		&&	ROMAddrList[areaIdx].endAddr >= address
		) {
			return 1;
		}
	}

	return 0;
}

/*******************************************************************************
* Mark address range as RAM.
*
* Arguments:
*	startAddr	- Starting address of the RAM region
*	endAddr		- Ending address of the RAM region
*
* Returns: -
*******************************************************************************/
void addRAMArea(mos65xx_addr startAddr, mos65xx_addr endAddr) {
	if (startAddr > endAddr) {
		printf("\n\nInvalid RAM area range (0x%04x - 0x%04x)...\n\n", startAddr, endAddr);
		exit(1);
	}

	RAMAddrList[RAMAddrRangeCount].startAddr = startAddr;
	RAMAddrList[RAMAddrRangeCount].endAddr = endAddr;

	RAMAddrRangeCount++;
}

/*******************************************************************************
* Check if given address is on RAM region.
*
* Arguments:
*	address		- Address to be checked
*
* Returns: 1 if RAM, otherwise 0
*******************************************************************************/
U8 isRAMAddress(mos65xx_addr address) {
	for (int areaIdx = 0; areaIdx < RAMAddrRangeCount; areaIdx++) {
		if (RAMAddrList[areaIdx].startAddr <= address
		&&	RAMAddrList[areaIdx].endAddr >= address
		) {
			return 1;
		}
	}

	return 0;
}

/*******************************************************************************
* Mark address range as IO.
*
* Arguments:
*	startAddr	- Starting address of the RAM region
*	endAddr		- Ending address of the RAM region
*
* Returns: -
*******************************************************************************/
void addIOArea(mos65xx_addr startAddr, mos65xx_addr endAddr) {
	if (startAddr > endAddr) {
		printf("\n\nInvalid IO area range (0x%04x - 0x%04x)...\n\n", startAddr, endAddr);
		exit(1);
	}

	IOAddrList[IOAddrRangeCount].startAddr = startAddr;
	IOAddrList[IOAddrRangeCount].endAddr = endAddr;

	IOAddrRangeCount++;
}

/*******************************************************************************
* Check if given address is on IO region.
*
* Arguments:
*	address		- Address to be checked
*
* Returns: 1 if IO, otherwise 0
*******************************************************************************/
U8 isIOAddress(mos65xx_addr address) {
	for (int areaIdx = 0; areaIdx < IOAddrRangeCount; areaIdx++) {
		if (IOAddrList[areaIdx].startAddr <= address
		&&	IOAddrList[areaIdx].endAddr >= address
		) {
			return 1;
		}
	}

	return 0;
}

/*******************************************************************************
* Loads binary from given path.
*
* Arguments:
*	pMemory - Pointer to a memory allocation.
*	pPath	- Pointer to a file path.
*
*
* Returns: Nothing.
*******************************************************************************/
void loadFile(U8 * pMemory, U16 offset, const char * pPath, mos65xx_fileType fileType) {
	FILE * fpPrg;

	fpPrg = fopen(pPath, "rb");

	if (fpPrg == NULL) {
		printf("\n\nError: file (%s) not found...\n\n", pPath);
		exit(1);
	}

	switch (fileType)
	{
	case mos65xx_BIN:
		break;
	case mos65xx_HEX:
	case mos65xx_ASM:
		printf("Filetype not yet supported...\n");
		exit(1);
	}

	for (int memIdx = offset; memIdx <= 0xFFFF; memIdx++) {
		int tempByte = fgetc(fpPrg);

		if (tempByte != EOF) {
			pMemory[memIdx] = (U8)tempByte;
		} else {
			break;
		}
	}

	fclose(fpPrg);
}

/*******************************************************************************
* Prints information of current operation.
*
* Arguments:
*	pProcessor	- Pointer to a processor data structure.
*	PC			- Program counter value for printed operation.
*	opCode		- Numeric value representing the op code.
*	operBytes	- Operand bytes for current operation.
*
* Returns: Nothing
*******************************************************************************/
void logOperation(
	Processor_65xx *	pProcessor,
	mos65xx_reg_st *		oldReg,
	U8						opCode,
	U8 *					operBytes
) {
	char * asmOperand = calloc(9, sizeof(char));
	U8 operands[4];
	char memValue[7] = {0};
	char * opLine = NULL;

	operands[0] = operBytes[0] >> 4;
	operands[1] = operBytes[0] & 0xF;
	operands[2] = operBytes[1] >> 4;
	operands[3] = operBytes[1] & 0xF;

	for (U64 operIdx = 0; operIdx < sizeof(operands); operIdx++) {
		if (operands[operIdx] < 10) {
			operands[operIdx] += '0';
		} else {
			operands[operIdx] += 'A' - 10;
		}
	}

	opLogging = 1;

	switch (mos65xx__opCodes[opCode].addrm)
	{
		case ACC:
			sprintf(asmOperand, " A      ");
			break;
		case ABS:
			sprintf(memValue, "$%02X", addrm_read8(pProcessor, (mos65xx_addr)(operBytes[0] | (operBytes[1] << 8))));
			
			sprintf(asmOperand, " $%c%c%c%c  ", operands[2], operands[3], operands[0], operands[1]);
			break;
		case ABX:
			sprintf(memValue, "$%02X", addrm_read8(pProcessor, (mos65xx_addr)(operBytes[0] | (operBytes[1] << 8)) + oldReg->X));
			
			sprintf(asmOperand, " $%c%c%c%c,X", operands[2], operands[3], operands[0], operands[1]);
			break;
		case ABY:
			sprintf(memValue, "$%02X", addrm_read8(pProcessor, (mos65xx_addr)(operBytes[0] | (operBytes[1] << 8)) + oldReg->Y));
			
			sprintf(asmOperand, " $%c%c%c%c,Y", operands[2], operands[3], operands[0], operands[1]);
			break;
		case IMM:
			sprintf(asmOperand, "#$%c%c    ", operands[0], operands[1]);
			break;
		case IND: {
			mos65xx_addr indAddress = addrm_read16(pProcessor, (mos65xx_addr)(operBytes[0] | (operBytes[1] << 8)));
			sprintf(memValue, "$%02X", addrm_read8(pProcessor, indAddress));

			sprintf(asmOperand, "($%c%c%c%c) ", operands[2], operands[3], operands[0], operands[1]);
			break;
		}
		case INX: {
			mos65xx_addr indAddress = addrm_read16(pProcessor, (mos65xx_addr)operBytes[0] + oldReg->X);
			sprintf(memValue, "$%02X", addrm_read8(pProcessor, indAddress));
			
			sprintf(asmOperand, "($%c%c,X) ", operands[0], operands[1]);
			break;
		}
		case INY: {
			mos65xx_addr indAddress = addrm_read16(pProcessor, (mos65xx_addr)operBytes[0]) + oldReg->Y;
			sprintf(memValue, "$%02X", addrm_read8(pProcessor, indAddress));

			sprintf(asmOperand, "($%c%c),Y ", operands[0], operands[1]);
			break;
		}
		case REL:
			sprintf(asmOperand, "  %-6d", (signed char)operBytes[0]);
			break;
		case ZPG:
			sprintf(memValue, "$%02X", addrm_read8(pProcessor, operBytes[0]));

			sprintf(asmOperand, " $%c%c    ", operands[0], operands[1]);
			break;
		case ZPX:
			sprintf(memValue, "$%02X", addrm_read8(pProcessor, operBytes[0] + oldReg->X));

			sprintf(asmOperand, " $%c%c,X  ", operands[0], operands[1]);
			break;
		case ZPY:
			sprintf(memValue, "$%02X", addrm_read8(pProcessor, operBytes[0] + oldReg->Y));

			sprintf(asmOperand, " $%c%c,Y  ", operands[0], operands[1]);
			break;
		
		case NON:
		case IMP:
			sprintf(asmOperand, "        ");
			break;
	}

	opLogging = 0;

	opLine = (char *)malloc(512);

	sprintf(opLine, "\r\n%04X | %02X %c%c %c%c | %-4s %s | %-3s | %02X %02X %02X %02X | %d%d%d%d%d%d | %1d (%llu)",
		oldReg->PC,
		opCode,
		mos65xx__opCodes[opCode].bytes > 1 ? operands[0] : ' ',
		mos65xx__opCodes[opCode].bytes > 1 ? operands[1] : ' ',
		mos65xx__opCodes[opCode].bytes > 2 ? operands[2] : ' ',
		mos65xx__opCodes[opCode].bytes > 2 ? operands[3] : ' ',
		mos65xx__opCodes[opCode].mnemonic,
		asmOperand,
		memValue,
		pProcessor->reg.AC,
		pProcessor->reg.X,
		pProcessor->reg.Y,
		pProcessor->reg.SP,
		pProcessor->reg.SR & SR_FLAG_N ? 1 : 0,
		pProcessor->reg.SR & SR_FLAG_V ? 1 : 0,
		pProcessor->reg.SR & SR_FLAG_D ? 1 : 0,
		pProcessor->reg.SR & SR_FLAG_I ? 1 : 0,
		pProcessor->reg.SR & SR_FLAG_Z ? 1 : 0,
		pProcessor->reg.SR & SR_FLAG_C ? 1 : 0,
		pProcessor->cycles.currentOp,
		pProcessor->cycles.totalCycles
	);

	DBG_PRINT(pProcessor, printf("%s", opLine));
	DBG_LOG(pProcessor, fprintf(tmpLog, "%s", opLine));

	free(opLine);
	free(asmOperand);
}