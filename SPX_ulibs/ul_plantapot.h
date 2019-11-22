/*
 * ul_alarmas_ose.h
 *
 *  Created on: 5 nov. 2019
 *      Author: pablo
 */

#ifndef SRC_SPX_ULIBS_UL_ALARMAS_OSE_H_
#define SRC_SPX_ULIBS_UL_ALARMAS_OSE_H_


#include "spx.h"

#define TIMER_POLEO_ENTRADAS	30
#define NRO_CANALES_MONITOREO	6

#define TIME_IN_STANDBY			1800

// Pines de salida
#define OPIN_SIRENA				0
#define OPIN_LUZ_VERDE			1
#define OPIN_LUZ_ROJA			2
#define OPIN_LUZ_AMARILLA		3
#define OPIN_LUZ_NARANJA		4

// Pines de entrada
#define IPIN_CH0					0
#define IPIN_CH1					1
#define IPIN_CH2					2
#define IPIN_CH3					3
#define IPIN_CH4					4
#define IPIN_CH5					5
#define IPIN_LLAVE_MANTENIMIENTO	6
#define IPIN_SENSOR_PUERTA			7

#define NRO_NIVEL_ALARMAS 4

typedef enum { ALARMA_NIVEL_0 = 0, ALARMA_NIVEL_1, ALARMA_NIVEL_2, ALARMA_NIVEL_3 } nivel_alarma_t;
typedef enum {st_ppot_NORMAL = 0, st_ppot_ALARMADO, st_ppot_STANDBY, st_ppot_MANTENIMIENTO } t_ppot_states;
typedef enum { act_OFF = 0, act_ON,act_FLASH } t_dev_action;

#define TIMEOUT_ALARMA_NIVEL_0	-1
#define TIMEOUT_ALARMA_NIVEL_1	360
#define TIMEOUT_ALARMA_NIVEL_2	240
#define TIMEOUT_ALARMA_NIVEL_3	120

typedef struct {
	bool enabled;					// Canal habilitado o no de acuerdo al switch
	float value;					// Valor analogico leido
	nivel_alarma_t nivel_alarma;	// Banda de alarma en que se encuentra el canal de acuerdo al valor leido
	int16_t timer;					// Tiempo en que el canal se encuentra en la banda de alarma
} channel_t;

channel_t l_ppot_input_channels[NRO_CANALES_MONITOREO];		// Lista con los canales de entrada que se monitorean

struct {
	bool llave_mantenimiento;
	bool sensor_puerta_open;
} alarmVars;


void plantapot_stk(void);

// Acciones
void action_luz_verde( t_dev_action action);
void action_luz_roja( t_dev_action action);
void action_luz_amarilla( t_dev_action action);
void action_luz_naranja( t_dev_action action);
void action_sirena(t_dev_action action);
uint8_t alarmas_checksum(void);
bool alarma_config( char *canal_str, char *alarm_str, char *limit_str, char *value_str );
void ppot_config_defaults(void);
uint8_t ppot_read_status_sensor_puerta(void);
uint8_t ppot_read_status_mantenimiento(void);
void ppot_servicio_tecnico( char * action, char * device );


#endif /* SRC_SPX_ULIBS_UL_ALARMAS_OSE_H_ */
