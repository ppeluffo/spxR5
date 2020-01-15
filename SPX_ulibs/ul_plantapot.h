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
#define OPIN_LUZ_ROJA			0
#define OPIN_LUZ_VERDE			1
#define OPIN_LUZ_AMARILLA		2
#define OPIN_LUZ_NARANJA		3
#define OPIN_SIRENA				4
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

#define IPIN_SENSOR_PUERTA_1		CNT0
#define IPIN_SENSOR_PUERTA_2		CNT1

#define SECS_ALM_LEVEL_1	360
#define SECS_ALM_LEVEL_2	140
#define SECS_ALM_LEVEL_3	120

#define SECS_BOTON_PRESSED	5

typedef enum {st_NORMAL = 0, st_ALARMADO, st_STANDBY, st_MANTENIMIENTO } t_appalm_states;
typedef enum { act_OFF = 0, act_ON,act_FLASH } t_dev_action;
typedef enum { alm_NOT_PRESENT = -1, alm_NOT_FIRED = 0 , alm_FIRED_L1, alm_FIRED_L2, alm_FIRED_L3   } t_alarm_fired;

// Tiempo dentro del estado standby ( luego de haber reconocido las alarmas )
#define TIME_IN_STANDBY			120

typedef struct {
	bool master;		// Se monitorea el cambio c/segundo
	bool  slave;		// Vuelve al estado normal solo luego de un poleo para no perder de trasnmitir el cambio.
}t_digital_state_var;

struct {
	t_digital_state_var llave_mantenimiento_on;
	t_digital_state_var boton_alarma_pressed;
	t_digital_state_var sensor_puerta_1_open;
	t_digital_state_var sensor_puerta_2_open;
} alarmVars;

// Estas son las mismas variables de estado pero cambian solo
// con los timerpoll y son las que transmito.

typedef struct {
	bool enabled;
	float value;
	uint16_t L1_timer;
	uint16_t L2_timer;
	uint16_t L3_timer;
	uint8_t alm_fired;
} t_alm_channels;

t_alm_channels alm_sysVars[NRO_CANALES_MONITOREO];

bool vt_MODO_TESTING;

void appalarma_stk(void);
bool appalarma_init(void);

uint8_t appalarma_checksum(void);
bool appalarma_config( char *param0,char *param1, char *param2, char *param3, char *param4 );
void appalarma_config_defaults(void);

void appalarma_servicio_tecnico( char * action, char * device );
void appalarma_print_status( bool full );

void appalarma_set_modo_testing(void);

#endif /* SRC_SPX_ULIBS_UL_ALARMAS_OSE_H_ */
