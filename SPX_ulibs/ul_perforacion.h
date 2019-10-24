/*
 * ul_perforaciones.h
 *
 *  Created on: 23 oct. 2019
 *      Author: pablo
 */

#ifndef SRC_SPX_ULIBS_UL_PERFORACION_H_
#define SRC_SPX_ULIBS_UL_PERFORACION_H_


#include "spx.h"

#define TIMEOUT_O_TIMER_BOYA	 60
#define TIMEOUT_O_TIMER_SISTEMA	 600

void perforacion_stk(void);
bool perforacion_init(void);
void perforacion_set_douts( uint8_t dout );
void perforacion_set_douts_from_gprs( uint8_t dout );
uint16_t perforacion_read_timer_activo(void);
uint8_t perforacion_read_control_mode(void);
uint8_t perforacion_checksum(void);


#endif /* SRC_SPX_ULIBS_UL_PERFORACION_H_ */
