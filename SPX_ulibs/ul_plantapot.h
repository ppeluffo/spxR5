/*
 * ul_alarmas_ose.h
 *
 *  Created on: 5 nov. 2019
 *      Author: pablo
 */

#ifndef SRC_SPX_ULIBS_UL_ALARMAS_OSE_H_
#define SRC_SPX_ULIBS_UL_ALARMAS_OSE_H_

#include "spx.h"

#define NRO_CANALES_MONITOREO	6

// Pines de salida
#define OPIN_SIRENA				0
#define OPIN_LUZ_VERDE			1
#define OPIN_LUZ_ROJA			2
#define OPIN_LUZ_AMARILLA		3
#define OPIN_LUZ_NARANJA		4
#define OPIN_LUZ_AZUL			5

// Pines de entrada
#define IPIN_CH0					0
#define IPIN_CH1					1
#define IPIN_CH2					2
#define IPIN_CH3					3
#define IPIN_CH4					4
#define IPIN_CH5					5
#define IPIN_LLAVE_MANTENIMIENTO	6
#define IPIN_BOTON_ALARMA			7

#define IPIN_SENSOR_PUERTA			CNT0

typedef enum { act_OFF = 0, act_ON,act_FLASH } t_dev_action;

struct {
	bool llave_mantenimiento_on;
	bool boton_alarma_pressed;
	bool sensor_puerta_open;
} alarmVars;

void appalarma_stk(void);
bool appalarma_init(void);

uint8_t appalarma_checksum(void);
bool appalarma_config( char *param0,char *param1, char *param2, char *param3, char *param4 );
void appalarma_config_defaults(void);

void appalarma_servicio_tecnico( char * action, char * device );
void appalarma_print_status(void);

#endif /* SRC_SPX_ULIBS_UL_ALARMAS_OSE_H_ */
