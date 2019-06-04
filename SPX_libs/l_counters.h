/*
 * l_counters.h
 *
 *  Created on: 8 dic. 2018
 *      Author: pablo
 */

#ifndef SRC_SPX_LIBS_L_COUNTERS_H_
#define SRC_SPX_LIBS_L_COUNTERS_H_

#include "l_iopines.h"
#include "stdbool.h"
#include <avr/interrupt.h>
#include "FreeRTOS.h"
#include "task.h"

// Las entradas de pulsos no solo se configuran para pulso sino tambien
// para generar interrupciones.

uint32_t counter1;

//------------------------------------------------------------------------------------
// API publica

void COUNTERS_init( uint8_t cnt, TaskHandle_t taskHandle );
void COUNTERS_disable_interrupt( uint8_t cnt );
void COUNTERS_enable_interrupt( uint8_t cnt );
uint32_t COUNTERS_readCnt1(void);
void COUNTERS_resetCnt1(void);
void COUNTERS_set_counter1_HS(void);
void COUNTERS_set_counter1_LS(void);

// API end
//------------------------------------------------------------------------------------

#define CNT_config_CLRD()	PORT_SetPinAsOutput( &CLR_D_PORT, CLR_D_BITPOS)
#define CNT_set_CLRD()		PORT_SetOutputBit( &CLR_D_PORT, CLR_D_BITPOS)
#define CNT_clr_CLRD()		PORT_ClearOutputBit( &CLR_D_PORT, CLR_D_BITPOS)

#define CNT_config_CNT0()	IO_config_PB2()
#define CNT_config_CNT1()	IO_config_PA2()

#define CNT_read_CNT0()	IO_read_PB2()
#define CNT_read_CNT1()	IO_read_PA2()



#endif /* SRC_SPX_LIBS_L_COUNTERS_H_ */
