/*
 * tkApp.h
 *
 *  Created on: 12 mar. 2020
 *      Author: pablo
 */

#ifndef SRC_SPX_TASKS_SPX_TKAPP_TKAPP_H_
#define SRC_SPX_TASKS_SPX_TKAPP_TKAPP_H_

#include "spx.h"
#include "tkComms.h"

#define DF_APP ( systemVars.debug == DEBUG_APLICACION )

void tkApp_off(void);

// CONSIGNA
void tkApp_consigna(void);
bool xAPP_consigna_config ( char *hhmm1, char *hhmm2 );
void xAPP_consigna_config_defaults(void);
bool xAPP_consigna_write( char *param0, char *param1, char *param2 );
uint8_t xAPP_consigna_checksum(void);
void xAPP_reconfigure_consigna(void);

// PLANTAPOT
void tkApp_plantapot(void);
uint8_t xAPP_plantapot_checksum(void);
bool xAPP_plantapot_config( char *param0,char *param1, char *param2, char *param3, char *param4 );
void xAPP_plantapot_config_defaults(void);
void xAPP_plantapot_servicio_tecnico( char * action, char * device );
void xAPP_plantapot_print_status( bool full );
void xAPP_plantapot_test(void);
void xAPP_plantapot_adjust_vars( st_dataRecord_t *dr);

// PERFORACION
void tkApp_perforacion(void);
uint8_t xAPP_perforacion_checksum(void);
void xAPP_perforacion_print_status( void );
void xAPP_perforacion_set_douts_remote( uint8_t dout );
void xAPP_perforacion_set_douts( uint8_t dout );

#endif /* SRC_SPX_TASKS_SPX_TKAPP_TKAPP_H_ */
