/*
 * l_doutputs.h
 *
 *  Created on: 8 dic. 2018
 *      Author: pablo
 */

#ifndef SRC_SPX_LIBS_L_DOUTPUTS_H_
#define SRC_SPX_LIBS_L_DOUTPUTS_H_

#include <avr/io.h>
#include "stdbool.h"

#define CHECK_BIT_IS_SET(var,pos) ((var) & (1<<(pos)))

// SALIDAS DIGITALES ( Solo en SPX_8CH )

//------------------------------------------------------------------------------------
// API publica

void DOUTPUTS_init(void);
bool DOUTPUTS_set_pin(uint8_t pin);
bool DOUTPUTS_clr_pin(uint8_t pin);
void DOUTPUTS_reflect_byte(uint8_t output_value );
//
// API END
//------------------------------------------------------------------------------------

#endif /* SRC_SPX_LIBS_L_DOUTPUTS_H_ */
