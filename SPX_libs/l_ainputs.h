/*
 * l_ain.h
 *
 *  Created on: 8 dic. 2018
 *      Author: pablo
 */

#ifndef SRC_SPX_LIBS_L_AINPUTS_H_
#define SRC_SPX_LIBS_L_AINPUTS_H_

#include "frtos-io.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include <avr/pgmspace.h>
#include "l_ina3221.h"
#include "l_printf.h"

//------------------------------------------------------------------------------------
// API publica

void AINPUTS_init( uint8_t io_board );
uint16_t AINPUTS_read_ina( uint8_t io_board, uint8_t channel_id );

// Solo para SPX_5CH
#define AINPUTS_read_battery()	AINPUTS_read_ina(0, 99)
#define AINPUTS_prender_12V()	IO_set_SENS_12V_CTL()
#define AINPUTS_apagar_12V()	IO_clr_SENS_12V_CTL()
//
// API END
//------------------------------------------------------------------------------------

#endif /* SRC_SPX_LIBS_L_AINPUTS_H_ */
