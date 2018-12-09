/*
 * l_dinputs.h
 *
 *  Created on: 8 dic. 2018
 *      Author: pablo
 */

#ifndef SRC_SPX_LIBS_L_DINPUTS_H_
#define SRC_SPX_LIBS_L_DINPUTS_H_

#include "l_iopines.h"
#include "l_mcp23018.h"

// SPX_5CH

//------------------------------------------------------------------------------------
// API publica
void DINPUTS_init( void );
#define DIN_read_D0()	IO_read_PA0()
#define DIN_read_D1()	IO_read_PB7()

// SPX_8CH:
uint8_t DIN_read_port( uint8_t pin);

// API END
//------------------------------------------------------------------------------------

#define DIN_config_D0()	IO_config_PA0()
#define DIN_config_D1()	IO_config_PB7()

//------------------------------------------------------------------------------------

#endif /* SRC_SPX_LIBS_L_DINPUTS_H_ */
