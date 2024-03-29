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

typedef enum { APP_OFF = 0, APP_CONSIGNA, APP_PERFORACION, APP_PLANTAPOT, APP_CAUDALIMETRO, APP_EXTERNAL_POLL, APP_PILOTO, APP_MODBUS } aplicacion_t;
typedef enum { APP_PERF_NORMAL = 0, APP_PERF_CTLFREQ } t_modo_perforacion;
typedef enum { CONSIGNA_OFF = 0, CONSIGNA_DIURNA, CONSIGNA_NOCTURNA } consigna_t;
typedef enum { PERF_CTL_EMERGENCIA, PERF_CTL_SISTEMA } perforacion_control_t;
typedef enum { ALARMA_NIVEL_0 = 0, ALARMA_NIVEL_1, ALARMA_NIVEL_2, ALARMA_NIVEL_3 } nivel_alarma_t;

// CONSIGNA
typedef struct {
	st_time_t hhmm_c_diurna;
	st_time_t hhmm_c_nocturna;
	consigna_t c_aplicada;
} st_consigna_t;

// PERFORACION / CTLFREQ
typedef struct {
	uint8_t outs;
	uint8_t	control;
} st_perforacion_t;

// Numeros de SMS a los que enviar las alarmas
#define MAX_NRO_SMS 		9

#define WDG_APP_TIMEOUT	WDG_TO120

//---------------------------------------------------------------------------
// Estructuras para el manejo del sistema de alarmas en plantas de potabilizacion de OSE
// Cada canal tiene 3 alarmas asociadas.
// Cada alarma tiene un nivel superior y uno inferior.
// Debemos tener entonces una lista l_niveles_alarma de tamanio NRO_CANALES_ALM donde almacenemos
// los mismos.
//
// Por otro lado, cada SMS tiene un nivel de alarma asociado.
// Cuando se genera una alarma de un tipo, se debe mandar un SMS a todos los nros. con dicho
// nivel asociado.
// Creamos una lista alm_level de tamanio MAX_NRO_SMS con el nivel asociado a dicho SMS.
//

// Canales de datos de entradas.
#define NRO_CANALES_ALM	6

typedef struct {
	float lim_inf;
	float lim_sup;
} st_limites_alarma_t;

/* Estructura que define un nro.de sms que se usa con las alarmas.
 * Tiene asociado el nivel de disparo
 */

typedef struct {
	st_limites_alarma_t alarma0;		// Banda normal
	st_limites_alarma_t alarma1;		// Alarma 1: Amarillo
	st_limites_alarma_t alarma2;		// Alarma 2: Naranja
	st_limites_alarma_t alarma3;		// Alarma 3: Rojo
} st_limites_alarma_ch_t;

/*
 * Estructura que define una lista de canales con los niveles de c/alarma
 * y una lista de sms con niveles asociados.
 */

typedef struct {
	st_limites_alarma_ch_t l_niveles_alarma[NRO_CANALES_ALM];
	nivel_alarma_t alm_level[MAX_NRO_SMS];
} st_alarmas_t;

typedef struct {
	uint16_t pulse_width;
	uint16_t factor_caudal;
	uint16_t range_actual;
	uint16_t periodo_actual;
} st_caudalimetros_t;


// Configuracion de piloto
#define MAX_PILOTO_PSLOTS	5

typedef struct {		// Elemento de piloto: presion, hora.
	st_time_t hhmm;
	float presion;
} st_piloto_data_t;

struct {
	bool start_presion_test;
	bool start_steppers_test;
	int16_t npulses;
	uint16_t pwidth;
	uint16_t startup_time;
	t_stepper_dir dir;
	int8_t slot;
	float pRef;
	float pB;
	float pA;
	float pError;
	int8_t pA_channel;
	int8_t pB_channel;
} spiloto;


typedef struct {
	aplicacion_t aplicacion;				// Modo de operacion del datalogger
	st_consigna_t consigna;
	st_perforacion_t perforacion;
	st_alarmas_t plantapot;
	st_caudalimetros_t caudalimetro;
	st_piloto_data_t pSlots [MAX_PILOTO_PSLOTS];

	// Estructura usada en común con la aplicacion de TANQUES y ALARMAS
	char l_sms[MAX_NRO_SMS][SMS_NRO_LENGTH];

} aplicacion_conf_t;

aplicacion_conf_t sVarsApp;

#define MASK_NORMAL 0xFF
#define MASK_EMERGENCIA 0xF6

void xAPP_sms_checkpoint(void);

// APP_OFF
void tkApp_off(void);
uint8_t xAPP_off_hash(void);

// CONSIGNA
void tkApp_consigna(void);
bool xAPP_consigna_config ( char *hhmm1, char *hhmm2 );
void xAPP_consigna_config_defaults(void);
bool xAPP_consigna_write( char *param0, char *param1, char *param2 );
uint8_t xAPP_consigna_hash(void);
void xAPP_reconfigure_consigna(void);

// PLANTAPOT
void tkApp_plantapot(void);
uint8_t xAPP_plantapot_hash(void);
bool xAPP_plantapot_config( char *param0,char *param1, char *param2, char *param3, char *param4 );
void xAPP_plantapot_config_defaults(void);
void xAPP_plantapot_servicio_tecnico( char * action, char * device );
void xAPP_plantapot_print_status( bool full );
void xAPP_plantapot_test(void);
void xAPP_plantapot_adjust_vars( st_dataRecord_t *dr);

// PERFORACION
void tkApp_perforacion(void);
uint8_t xAPP_perforacion_hash(void);
void xAPP_perforacion_print_status( void );
void xAPP_perforacion_adjust_x_douts(uint8_t dout);

// CAUDALIMETRO
void tkApp_caudalimetro(void);
bool xAPP_caudalimetro_config ( char *pwidth, char *factorQ);
uint8_t xAPP_caudalimetro_hash(void);

// EXTERNAL POLL
void tkApp_external_poll(void);
uint8_t xAPP_external_poll_hash(void);

// GENERAL
void xAPP_set_douts_remote( uint8_t dout );
void xAPP_set_douts( uint8_t dout, uint8_t mask );
void xAPP_set_douts_lazy( uint8_t dout, uint8_t mask );
void xAPP_set_douts_remote_lazy( uint8_t dout );

// PILOTO
void tkApp_piloto(void);
uint8_t xAPP_piloto_hash( void );
void xAPP_piloto_config_defaults(void);
void xAPP_piloto_print_status( void );
bool xAPP_piloto_config( char *param1, char *param2, char *param3, char *param4 );
void xAPP_piloto_stepper_test( char *s_dir, char *s_npulses, char *s_pwidth, char *s_startup_time );
void xAPP_piloto_presion_test( char *s_out_pres, char *s_out_error );
void run_piloto_presion_test( void );
void run_piloto_stepper_test( void );

// APP_MODBUS
void tkApp_modbus(void);
uint8_t xAPP_modbus_hash(void);

#endif /* SRC_SPX_TASKS_SPX_TKAPP_TKAPP_H_ */
