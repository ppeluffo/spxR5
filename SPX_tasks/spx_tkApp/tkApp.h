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

void tkApp_off(void);

// CONSIGNA
void tkApp_consigna(void);
bool xAPP_consigna_config ( char *hhmm1, char *hhmm2 );
void xAPP_consigna_config_defaults(void);
bool xAPP_consigna_write( char *param0, char *param1, char *param2 );
uint8_t xAPP_consigna_checksum(void);
void xAPP_reconfigure_consigna(void);


// PLANTAPOT
void app_plantapot(void);
bool appalarma_init(void);

uint8_t appalarma_checksum(void);
bool appalarma_config( char *param0,char *param1, char *param2, char *param3, char *param4 );
void appalarma_config_defaults(void);

void appalarma_servicio_tecnico( char * action, char * device );
void appalarma_print_status( bool full );
void appalarma_reconfigure_app(void);
void appalarma_reconfigure_sms_by_gprsinit( const char *gprsbuff );
void appalarma_reconfigure_levels_by_gprsinit(const char *gprsbuff );


#endif /* SRC_SPX_TASKS_SPX_TKAPP_TKAPP_H_ */
