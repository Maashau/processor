/*******************************************************************************
* c64_sid.c
*
* Commodore 64 SID chip source file.
*******************************************************************************/
#include <stdio.h>
#include <assert.h>

#include "../65xx.h"
#include "c64.h"
#include "c64_sid.h"



/*******************************************************************************
* Init SID.
*
* Arguments:
*		-
*
* Returns: -
*******************************************************************************/
void c64_sidInit(C64_t * pC64) {

}

/*******************************************************************************
* Tick SID.
*
* Arguments:
*		-
*
* Returns: -
*******************************************************************************/
void c64_sidTick(C64_t * pC64) {

}

/*******************************************************************************
* Read SID register.
*
* Arguments:
*		-
*
* Returns: -
*******************************************************************************/
U8 c64_sidRead(C64_t * pC64, mos65xx_addr address) {
	assert(0);
}

/*******************************************************************************
* Write SID register.
*
* Arguments:
*		-
*
* Returns: -
*******************************************************************************/
void c64_sidWrite(Processor_65xx * pProcessor, mos65xx_addr address, U8 value) {
	assert(0);
}
