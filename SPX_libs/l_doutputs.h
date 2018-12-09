/*
 * l_doutputs.h
 *
 *  Created on: 8 dic. 2018
 *      Author: pablo
 */

#ifndef SRC_SPX_LIBS_L_DOUTPUTS_H_
#define SRC_SPX_LIBS_L_DOUTPUTS_H_

#include <avr/io.h>

#define CHECK_BIT_IS_SET(var,pos) ((var) & (1<<(pos)))

// SALIDAS DIGITALES ( Solo en SPX_8CH )

//------------------------------------------------------------------------------------
// API publica

void DOUTPUTS_init(void);
void IO_set_DOUT(uint8_t pin);
void IO_clr_DOUT(uint8_t pin);
void IO_reflect_DOUTPUTS(uint8_t output_value );
//
// API END
//------------------------------------------------------------------------------------

#endif /* SRC_SPX_LIBS_L_DOUTPUTS_H_ */
