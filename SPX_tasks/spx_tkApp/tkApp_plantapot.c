/*
 * ul_alarmas_ose.c
 *
 *  Created on: 5 nov. 2019
 *      Author: pablo
 */

#include "tkApp.h"

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
#define SECS_ALM_LEVEL_2	240
#define SECS_ALM_LEVEL_3	120

#define SECS_BOTON_PRESSED	5

typedef enum {st_NORMAL = 0, st_ALARMADO, st_STANDBY, st_MANTENIMIENTO } t_appalm_states;
typedef enum { act_OFF = 0, act_ON,act_FLASH } t_dev_action;
typedef enum { alm_NOT_PRESENT = -1, alm_NOT_FIRED = 0 , alm_FIRED_L1, alm_FIRED_L2, alm_FIRED_L3   } t_alarm_fired;

// Tiempo dentro del estado standby ( luego de haber reconocido las alarmas )
#define TIME_IN_STANDBY			1800

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

bool flash_luz_verde;
bool flash_luz_roja;
bool flash_luz_amarilla;
bool flash_luz_azul;
bool flash_luz_naranja;

static int16_t timer_en_standby;

static void xAPP_plantapot_TimerCallback( TimerHandle_t xTimer );
static void xAPP_plantapot_leer_entradas(void);
static void xAPP_plantapot_check_inband(void);
static bool xAPP_plantapot_fire_alarmas(void);
static bool xAPP_plantapot_init(void);

TimerHandle_t plantapot_timerAlarmas;
StaticTimer_t plantapot_xTimerAlarmas;

uint16_t din[IO8_DINPUTS_CHANNELS];

static t_appalm_states plantapot_state;

// Estados
void sst_app_plantapot_normal(void);
void sst_app_plantapot_alarmado(void);
void sst_app_plantapot_mantenimiento(void);
void sst_app_plantapot_standby(void);

// Acciones
static void ac_luz_verde( t_dev_action action);
static void ac_luz_roja( t_dev_action action);
static void ac_luz_amarilla( t_dev_action action);
static void ac_luz_naranja( t_dev_action action);
static void ac_luz_azul( t_dev_action action);
static void ac_sirena(t_dev_action action);
static void ac_send_smsAlarm( int8_t level, uint8_t channel );

//------------------------------------------------------------------------------------
void tkApp_plantapot(void)
{

	if ( spx_io_board != SPX_IO8CH ) {
		xprintf_P(PSTR("APP: PLANTAPOT Init ERROR: run only in IO_8CH.\r\n\0"));
		sVarsApp.aplicacion = APP_OFF;
		u_save_params_in_NVMEE();
		return;
	}

	// Creo y arranco el timer de control del tiempo de alarmas
	plantapot_timerAlarmas = xTimerCreateStatic ("PPOT",
			pdMS_TO_TICKS( 1000 ),
			pdTRUE,
			( void * ) 0,
			xAPP_plantapot_TimerCallback,
			&plantapot_xTimerAlarmas
			);

	xTimerStart(plantapot_timerAlarmas, 10);

	for (;;) {

		switch(plantapot_state) {
		case st_NORMAL:
			sst_app_plantapot_normal();
			break;
		case st_ALARMADO:
			sst_app_plantapot_alarmado();
			break;
		case st_STANDBY:
			sst_app_plantapot_standby();
			break;
		case st_MANTENIMIENTO:
			sst_app_plantapot_mantenimiento();
			break;
		default:
			xprintf_P(PSTR("APP: PLANTAPOT state unknown (%d)\r\n\0"), plantapot_state );
			sVarsApp.aplicacion = APP_OFF;
			u_save_params_in_NVMEE();
			return;
			break;
		}
	}
}
//------------------------------------------------------------------------------------
// ESTADOS
//------------------------------------------------------------------------------------
void sst_app_plantapot_normal(void)
{

// Entry:
	xprintf_PD( DF_APP, PSTR("APP: PLANTAPOT modo Normal in\r\n\0"));

	xAPP_plantapot_init();
	ac_luz_verde( act_ON );

// Loop:
	// Genero un ciclo por segundo !!!

	while ( plantapot_state == st_NORMAL ) {

		ctl_watchdog_kick( WDG_APP,  WDG_APP_TIMEOUT );
		// Espera de 1s
		vTaskDelay( ( TickType_t)( 1000 / portTICK_RATE_MS ) );

		// Leo los datos
		xAPP_plantapot_leer_entradas();
		// Veo si hay alguna medida en una region de alarma
		xAPP_plantapot_check_inband();

		// CAMBIO DE ESTADO.
		// Disparo alarmas si es necesario
		if ( xAPP_plantapot_fire_alarmas() ) {
			// Si dispare una alarma, paso al estado de sistema alarmado
			plantapot_state = st_ALARMADO;
			break;
		}

		// Veo si se activo la llave de mantenimiento.
		if ( alarmVars.llave_mantenimiento_on.master == true ) {
			// Activaron la llave de mantenimiento. Salgo
			plantapot_state = st_MANTENIMIENTO;
			break;
		}
	}

// Exit
	ac_luz_verde( act_OFF );
	xprintf_PD( DF_APP, PSTR("APP: PLANTAPOT modo Normal out\r\n\0"));
	return;

}
//------------------------------------------------------------------------------------
void sst_app_plantapot_alarmado(void)
{
	// En este estado tengo al menos una alarma disparada.
	// Sigo monitoreando pero ahora controlo que presiones el botón de reset
	// Entry:

uint8_t timer_boton_pressed = SECS_BOTON_PRESSED;

	xprintf_PD( DF_APP, PSTR("APP: PLANTAPOT modo Alarmado in\r\n\0"));

	ac_luz_verde( act_OFF );

	// Loop:
	// Genero un ciclo por segundo !!!

	while ( plantapot_state == st_ALARMADO ) {

		ctl_watchdog_kick( WDG_APP,  WDG_APP_TIMEOUT );
		// Espera de 1s
		vTaskDelay( ( TickType_t)( 1000 / portTICK_RATE_MS ) );

		// Leo los datos
		xAPP_plantapot_leer_entradas();
		// Veo si hay alguna medida en una region de alarma
		xAPP_plantapot_check_inband();
		// Disparo alarmas si es necesario
		xAPP_plantapot_fire_alarmas();

		// CAMBIO DE ESTADO
		// Veo si presiono el boton de reset de alarma por mas de 5secs.
		if ( ! alarmVars.boton_alarma_pressed.master ) {
			timer_boton_pressed = SECS_BOTON_PRESSED;
		} else {
			timer_boton_pressed--;
		}

		if ( timer_boton_pressed == 0 ) {
			// Reconoci la alarma por tener apretado el boton de pressed mas de 5s
			plantapot_state = st_STANDBY;
			break;
		}

		// Veo si se activo la llave de mantenimiento.
		if ( alarmVars.llave_mantenimiento_on.master == true ) {
			// Activaron la llave de mantenimiento. Salgo
			plantapot_state = st_MANTENIMIENTO;
			break;
		}

	}

// Exit
	xprintf_PD( DF_APP, PSTR("APP: PLANTAPOT modo Alarmado out\r\n\0"));
	return;
}
//------------------------------------------------------------------------------------
void sst_app_plantapot_mantenimiento(void)
{
	// Este estado es en mantenimiento ( llave de mantenimiento en on)
	// Solo se polean los datos cada 30s y se guardan en la base de datos pero no
	// se generan alarmas.
	// Los datos se polean y guardan en la tarea spx_tkInputs y se mandan al server con
	// spx_tkComms por lo tanto no hacemos nada.
	// - No se activan las alarmas
	// - No se disparan las sirenas
	// - No se generan SMS
	// - Solo flashea la luz azul.
	// Cuando salgo solo voy al estado NORMAL


// Entry:
	xprintf_PD( DF_APP, PSTR("APP: PLANTAPOT modo Mantenimiento in\r\n\0"));

	// Al entrar en mantenimiento apago todas las posibles señales activas.
	// Solo debe flashear la luz verde.
	xAPP_plantapot_init();

	// Flasheo luz azul
	ac_luz_azul(act_FLASH);

// Loop:
	while ( plantapot_state == st_MANTENIMIENTO ) {

		ctl_watchdog_kick( WDG_APP,  WDG_APP_TIMEOUT );
		// Espera de 1s
		vTaskDelay( ( TickType_t)( 1000 / portTICK_RATE_MS ) );

		// Veo si apagaron la llave de mantenimiento.
		if ( alarmVars.llave_mantenimiento_on.master == false ) {
			// Apagaron la llave de mantenimiento. Salgo
			plantapot_state = st_NORMAL;
		}

	}

// Exit
	ac_luz_azul(act_OFF);
	xprintf_PD( DF_APP, PSTR("APP: PLANTAPOT modo Mantenimiento out\r\n\0"));
	return;

}
//------------------------------------------------------------------------------------
void sst_app_plantapot_standby(void)
{
	// Este estado es en standby. Entro luego de estar alarmado y que se presiono
	// el boton de reset.
	// Solo se polean los datos cada 30s y se guardan en la base de datos pero no
	// se generan alarmas.
	// Debo permanecer en este estado hasta que transcurran 30 minutos
	// Los datos se polean y guardan en la tarea spx_tkInputs y se mandan al server con
	// spx_tkComms por lo tanto no hacemos nada.
	// - No se activan las alarmas
	// - No se disparan las sirenas
	// - No se generan SMS
	// Cuando salgo solo voy al estado NORMAL o al estado MANTENIMIENTO.
	// El timer que uso es una variable estática externa para poder mostrarlo en el status.


// Entry:
	xprintf_PD( DF_APP, PSTR("APP: PLANTAPOT modo Standby in\r\n\0"));

	// Debo permancer 30 minutos.
	timer_en_standby = TIME_IN_STANDBY;

	// Apago todas las alarmas
	xAPP_plantapot_init();
	ac_luz_verde(act_FLASH);

// Loop:
	while ( plantapot_state == st_STANDBY ) {

		ctl_watchdog_kick( WDG_APP,  WDG_APP_TIMEOUT );
		// Espera de 1s
		vTaskDelay( ( TickType_t)( 1000 / portTICK_RATE_MS ) );

		// CAMBIO DE ESTADO
		// Veo si activaron la llave de mantenimiento.
		if ( alarmVars.llave_mantenimiento_on.master == true ) {
			plantapot_state = st_MANTENIMIENTO;
			break;
		}

		// Veo si expiro el tiempo para salir y volver al estado NORMAL
		if ( timer_en_standby-- <= 1 ) {
			plantapot_state = st_NORMAL;
			break;
		}

	}

// Exit
	timer_en_standby = 0;
	ac_luz_verde(act_OFF);
	xprintf_PD( DF_APP, PSTR("APP: PLANTAPOT modo Standby out\r\n\0"));

}
//------------------------------------------------------------------------------------
// FUNCIONES GENERALES PRIVADAS
//------------------------------------------------------------------------------------
static bool xAPP_plantapot_init(void)
{

	// Inicializa las salidas y las variables de trabajo.

uint8_t i;

	// 1- Apagar alarmas
	ac_luz_verde( act_OFF );
	ac_luz_roja( act_OFF );
	ac_luz_amarilla( act_OFF );
	ac_luz_naranja( act_OFF );
	ac_luz_azul(act_OFF);
	ac_sirena( act_OFF );

	// 2- Reset AlmVars
	alarmVars.boton_alarma_pressed.master = false;
	alarmVars.llave_mantenimiento_on.master = false;
	alarmVars.sensor_puerta_1_open.master = false;
	alarmVars.sensor_puerta_2_open.master = false;

	// 3- Reset Timers
	// Inicializo los timers por canal y por nivel de alamra
	for ( i = 0; i < NRO_CANALES_MONITOREO; i++) {
		alm_sysVars[i].enabled = false;

		alm_sysVars[i].L1_timer = SECS_ALM_LEVEL_1;
		alm_sysVars[i].L2_timer = SECS_ALM_LEVEL_2;
		alm_sysVars[i].L3_timer = SECS_ALM_LEVEL_3;
		//alm_sysVars[i].alm_fired = alm_NOT_FIRED;
	}

	// Borro los SMS de alarmas pendientes
	xSMS_init();

	return(true);
}
//------------------------------------------------------------------------------------
static void xAPP_plantapot_TimerCallback( TimerHandle_t xTimer )
{
	// Se ejecuta cada 1s como callback del timer.

uint8_t i;

	// Flash de luces
	if ( flash_luz_verde ) ac_luz_verde(act_FLASH);
	if ( flash_luz_roja) ac_luz_roja(act_FLASH);
	if ( flash_luz_azul ) ac_luz_azul(act_FLASH);
	if ( flash_luz_amarilla ) ac_luz_amarilla(act_FLASH);
	if ( flash_luz_naranja ) ac_luz_naranja(act_FLASH);

	/*
	 * Leo las entradas digitales
	 * Las leo aqui porque en caso de activarse no debo esperar 1 minuto !!!
	 * La llave de mantenimiento t el boton de alarma corresponden a entradas
	 * digitales normales ( 6 y 7 ) por lo tanto deben activarse contra VCC
	 * La entrada del sensor de puerta se conecta a CNT0 que tiene un pull-up
	 * por lo tanto leo 1.
	 * La idea es que en reposo, el estado que midamos sea el 'activo' de modo
	 * que en caso de falla, se detecte como una condición de alarma.
	 * - Llave de mantenimiento / boton de alarma: normalmente conectados a 12V
	 *   Se activan 'abriendo' el circuito
	 *
	 */

	dinputs_read( din );

	// Actualizo las entradas digitales de los canales habilitados
	for (i=0; i < NRO_CANALES_MONITOREO; i++) {
		// Los switches ( canales digitales ) indican si el canal esta
		// habilitado si o no.
		if ( din[i] == 0 ) {
			alm_sysVars[i].enabled = false;
		} else {
			alm_sysVars[i].enabled = true;
		}
	}

	if ( din[IPIN_LLAVE_MANTENIMIENTO] == 1 ) {
		alarmVars.llave_mantenimiento_on.master = true;
		alarmVars.llave_mantenimiento_on.slave = true;
	} else {
		alarmVars.llave_mantenimiento_on.master = false;
	}

	if ( din[IPIN_BOTON_ALARMA] == 	1 ) {
		alarmVars.boton_alarma_pressed.master = true;
		alarmVars.boton_alarma_pressed.slave = true;
	} else {
		alarmVars.boton_alarma_pressed.master = false;
	}

	// La entrada del sensor de puerta 1 la leo directamente de la entrda del CNT0
	if ( CNT_read_CNT0() == 0 ) {
		alarmVars.sensor_puerta_1_open.master = true;
		alarmVars.sensor_puerta_1_open.slave = true;
	} else {
		alarmVars.sensor_puerta_1_open.master = false;
	}

	// La entrada del sensor de puerta 2 la leo directamente de la entrda del CNT0
	if ( CNT_read_CNT1() == 0 ) {
		alarmVars.sensor_puerta_2_open.master = true;
		alarmVars.sensor_puerta_2_open.slave = true;
	} else {
		alarmVars.sensor_puerta_2_open.master = false;
	}

}
//------------------------------------------------------------------------------------
static void xAPP_plantapot_leer_entradas(void)
{
	// El poleo de las entradas lo hace la tarea tkInputs. Esta debe tener un timerpoll de 30s
	// Aqui c/1s leo las entradas en modo 'copy' ( sin poleo ).
	// Por eso no tengo que monitorear los 30s sino que lo hago c/1s. y esto me permite tener
	// permantente control de la tarea.
	// Leo las entradas analogicas
	// Los sensores externos: llave_mantenimiento, sensor_puerta, boton_alarma
	// los leo en la funcion de callback del timer cada 1s.

st_dataRecord_t dr;
uint8_t i;

	// Leo todas las entradas analogicas, digitales, contadores.
	// Es un DLG8CH
	// Leo en modo 'copy', sin poleo real.
	data_read_inputs(&dr, true );

	for (i=0; i < NRO_CANALES_MONITOREO; i++) {

		// Los switches ( canales digitales ) indican si el canal esta
		// habilitado si o no.
		// Los canales digitales los leo en la funcion de callback por lo tanto
		// no los sobreescribo aqui. !!!

		/*
		if ( dr.df.io8.dinputs[i] == 0 ) {
			alm_sysVars[i].enabled = false;
		} else {
			alm_sysVars[i].enabled = true;
		}
		*/

		// Leo el valor de todos los canales ( habilitados o no )
		alm_sysVars[i].value = dr.df.io8.ainputs[i];

	}

}
//------------------------------------------------------------------------------------
static void xAPP_plantapot_check_inband(void)
{
	// Para cada canal, veo si el nivel actual esta en alguna banda de alarma.
	// En caso afirmativo decremento el contador hasta 0.
	// En caso que el valor este en el valor normal, reseteo los contadores de c/banda
	// Los timers son decrementados a 0 si son mayores a 0. !!!


uint8_t i;
float value;

	for (i=0; i < NRO_CANALES_MONITOREO; i++) {
		// Si el canal esta apagado lo salteo
		if ( alm_sysVars[i].enabled == false )
			continue;

		// Canal habilitado:
		value = alm_sysVars[i].value;

		// Countdown para ALARMA NIVEL_3
		if (alm_sysVars[i].L3_timer > 0 ) {
			if ( value > sVarsApp.plantapot.l_niveles_alarma[i].alarma3.lim_sup ) {
				alm_sysVars[i].L3_timer--;
				continue;
			}

			if ( value < sVarsApp.plantapot.l_niveles_alarma[i].alarma3.lim_inf ) {
				alm_sysVars[i].L3_timer--;
				continue;
			}
		}

		// Countdown para ALARMA NIVEL_2
		if (alm_sysVars[i].L2_timer > 0 ) {
			if ( ( value > sVarsApp.plantapot.l_niveles_alarma[i].alarma2.lim_sup ) &&
				( value < sVarsApp.plantapot.l_niveles_alarma[i].alarma3.lim_sup ) ) {
				alm_sysVars[i].L2_timer--;
				continue;
			}

			if ( ( value < sVarsApp.plantapot.l_niveles_alarma[i].alarma2.lim_inf ) &&
				( value > sVarsApp.plantapot.l_niveles_alarma[i].alarma3.lim_inf ) ) {
				alm_sysVars[i].L2_timer--;
				continue;
			}
		}

		// Countdown para ALARMA NIVEL_1
		if (alm_sysVars[i].L1_timer > 0 ) {

			if ( ( value > sVarsApp.plantapot.l_niveles_alarma[i].alarma1.lim_sup ) &&
				( value < sVarsApp.plantapot.l_niveles_alarma[i].alarma2.lim_sup ) ) {
				alm_sysVars[i].L1_timer--;
				continue;
			}
			if ( ( value < sVarsApp.plantapot.l_niveles_alarma[i].alarma1.lim_inf ) &&
				( value > sVarsApp.plantapot.l_niveles_alarma[i].alarma2.lim_inf ) ) {
				alm_sysVars[i].L1_timer--;
				continue;
			}
		}

		// Reset a NIVELES NORMALES
		if ( ( value <  sVarsApp.plantapot.l_niveles_alarma[i].alarma1.lim_sup ) &&
				( value >= sVarsApp.plantapot.l_niveles_alarma[i].alarma1.lim_inf ) ) {

			// Estoy en la banda normal. Reseteo los timers
			alm_sysVars[i].L1_timer = SECS_ALM_LEVEL_1;
			alm_sysVars[i].L3_timer = SECS_ALM_LEVEL_3;
		}

	}
}
//------------------------------------------------------------------------------------
static bool xAPP_plantapot_fire_alarmas(void)
{
	// Revisa si hay algún canal que deba disparar alguna alarma.
	// En caso afirmativo la dispara.

uint8_t i;

bool alm_fired = false;	// Variable de retorno para el cambio de estado

	for (i=0; i < NRO_CANALES_MONITOREO; i++) {

		// Si el canal esta apagado lo salteo
		if ( alm_sysVars[i].enabled == false ) {
			continue;
		}

		// Disparo alarma LEVEL_1
		if ( alm_sysVars[i].L1_timer == 1 ) {
			// Luces
			ac_luz_verde(act_OFF);
			ac_luz_amarilla(act_FLASH);
			// Sirena
			ac_sirena(act_ON);
			// Envio Sms
			ac_send_smsAlarm( alm_FIRED_L1, i );

			alm_sysVars[i].alm_fired = alm_FIRED_L1;
			alm_fired = true;
			continue;
		}

		// Disparo alarma LEVEL_2
		if ( alm_sysVars[i].L2_timer == 1 ) {
			ac_luz_verde(act_OFF);
			ac_luz_naranja(act_FLASH);
			// Sirena
			ac_sirena(act_ON);
			// Envio Sms
			ac_send_smsAlarm( alm_FIRED_L2, i );

			alm_sysVars[i].alm_fired = alm_FIRED_L2;
			alm_fired = true;
			continue;
		}

		// Disparo alarma LEVEL_3
		if ( alm_sysVars[i].L3_timer == 1 ) {
			ac_luz_verde(act_OFF);
			ac_luz_roja(act_FLASH);
			// Sirena
			ac_sirena(act_ON);
			// Envio Sms
			ac_send_smsAlarm( alm_FIRED_L3, i );

			alm_sysVars[i].alm_fired = alm_FIRED_L3;
			alm_fired = true;
			continue;
		}

	}

	return(alm_fired);

}
//------------------------------------------------------------------------------------
// ACCIONES
//------------------------------------------------------------------------------------
static void ac_luz_verde( t_dev_action action)
{
	/*
	 *  En reposo quiero que el led este apagado.
	 *  El led esta conectado al polo NO por lo tanto
	 *  el transistor debe estar cortado o sea que el
	 *  MCP debe poner un 0 y en la salida aparecera un 1
	 */

static uint8_t l_state = 0;

	switch(action) {
	case act_OFF:
		if ( flash_luz_verde ) {
			flash_luz_verde = false;
			vTaskDelay( ( TickType_t)( 1500 / portTICK_RATE_MS ) );
		}
		u_write_output_pins( OPIN_LUZ_VERDE, 1 );
		l_state = 1;
		break;
	case act_ON:
		u_write_output_pins( OPIN_LUZ_VERDE, 0 );
		l_state = 0;
		break;
	case act_FLASH:
		flash_luz_verde = true;
		if ( l_state == 0 ) {
			l_state = 1;
			u_write_output_pins( OPIN_LUZ_VERDE, 1 );
		} else {
			l_state = 0;
			u_write_output_pins( OPIN_LUZ_VERDE, 0 );
		}
		break;
	default:
		break;
	}
}
//------------------------------------------------------------------------------------
static void ac_luz_roja( t_dev_action action)
{

	static uint8_t l_state = 0;

		switch(action) {
		case act_OFF:
			if ( flash_luz_roja ) {
				flash_luz_roja = false;
				vTaskDelay( ( TickType_t)( 1500 / portTICK_RATE_MS ) );
			}
			u_write_output_pins( OPIN_LUZ_ROJA, 1 );
			l_state = 1;
			break;
		case act_ON:
			u_write_output_pins( OPIN_LUZ_ROJA, 0 );
			l_state = 0;
			break;
		case act_FLASH:
			flash_luz_roja = true;
			if ( l_state == 0 ) {
				l_state = 1;
				u_write_output_pins( OPIN_LUZ_ROJA, 1 );
			} else {
				l_state = 0;
				u_write_output_pins( OPIN_LUZ_ROJA, 0 );
			}
			break;
		default:
			break;
		}
}
//------------------------------------------------------------------------------------
static void ac_luz_amarilla( t_dev_action action)
{

	static uint8_t l_state = 0;

		switch(action) {
		case act_OFF:
			if ( flash_luz_amarilla ) {
				flash_luz_amarilla = false;
				vTaskDelay( ( TickType_t)( 1500 / portTICK_RATE_MS ) );
			}
			u_write_output_pins( OPIN_LUZ_AMARILLA, 1 );
			l_state = 1;
			break;
		case act_ON:
			u_write_output_pins( OPIN_LUZ_AMARILLA, 0 );
			l_state = 0;
			break;
		case act_FLASH:
			flash_luz_amarilla = true;
			if ( l_state == 0 ) {
				l_state = 1;
				u_write_output_pins( OPIN_LUZ_AMARILLA, 1 );
			} else {
				l_state = 0;
				u_write_output_pins( OPIN_LUZ_AMARILLA, 0 );
			}
			break;
		default:
			break;
		}
}
//------------------------------------------------------------------------------------
static void ac_luz_naranja( t_dev_action action)
{

	static uint8_t l_state = 0;

		switch(action) {
		case act_OFF:
			if ( flash_luz_naranja ) {
				flash_luz_naranja = false;
				vTaskDelay( ( TickType_t)( 1500 / portTICK_RATE_MS ) );
			}
			u_write_output_pins( OPIN_LUZ_NARANJA, 1 );
			l_state = 1;
			break;
		case act_ON:
			u_write_output_pins( OPIN_LUZ_NARANJA, 0 );
			l_state = 0;
			break;
		case act_FLASH:
			flash_luz_naranja = true;
			if ( l_state == 0 ) {
				l_state = 1;
				u_write_output_pins( OPIN_LUZ_NARANJA, 1 );
			} else {
				l_state = 0;
				u_write_output_pins( OPIN_LUZ_NARANJA, 0 );
			}
			break;
		default:
			break;
		}
}
//------------------------------------------------------------------------------------
static void ac_luz_azul( t_dev_action action)
{

	static uint8_t l_state = 0;

		switch(action) {
		case act_OFF:
			if ( flash_luz_azul ) {
				flash_luz_azul = false;
				vTaskDelay( ( TickType_t)( 1500 / portTICK_RATE_MS ) );
			}
			u_write_output_pins( OPIN_LUZ_AZUL, 1 );
			l_state = 1;
			break;
		case act_ON:
			u_write_output_pins( OPIN_LUZ_AZUL, 0 );
			l_state = 0;
			break;
		case act_FLASH:
			flash_luz_azul = true;
			if ( l_state == 0 ) {
				l_state = 1;
				u_write_output_pins( OPIN_LUZ_AZUL, 1 );
			} else {
				l_state = 0;
				u_write_output_pins( OPIN_LUZ_AZUL, 0 );
			}
			break;
		default:
			break;
		}
}
//------------------------------------------------------------------------------------
static void ac_sirena(t_dev_action action)
{

	switch(action) {
	case act_OFF:
		u_write_output_pins( OPIN_SIRENA, 1 );
		break;
	case act_ON:
		u_write_output_pins( OPIN_SIRENA, 0 );
		break;
	case act_FLASH:
		break;
	default:
		break;
	}
}
//------------------------------------------------------------------------------------
static void ac_send_smsAlarm( int8_t level, uint8_t channel )
{
	// Envio el mensaje de alarma por SMS.
	// Veo en la lista de niveles de alarma de c/SMS, cual nro. tiene el nivel adecuado para ser dispararado.

char sms_msg[SMS_MSG_LENGTH];
uint8_t pos;

	// Preparo el mensaje
	snprintf_P( sms_msg, SMS_MSG_LENGTH, PSTR("ALARMA Nivel %d activada por: %s\0"), level, systemVars.ainputs_conf.name[channel] );

	for (pos = 0; pos < MAX_NRO_SMS; pos++) {

		// Envio a todos los numeros configurados para nivel level.
		if ( sVarsApp.plantapot.alm_level[pos] != level ) {
			// Salto las posiciones que no tienen igual level
			//xprintf_P(PSTR("ALARMA L%d: pos=%d, Level(%d)\r\n"), level, pos, sVarsApp.plantapot.l_sms[pos].alm_level );
			continue;
		}


		if ( strcmp ( sVarsApp.l_sms[pos] , "X" ) == 0 ) {
			// Salto las posiciones que no tienen un nro.configurado
			//xprintf_P(PSTR("ALARMA L%d: pos=%d, Name(%s)\r\n"), level, pos, sVarsApp.plantapot.l_sms[pos].sms_nro );
			continue;
		}

		xprintf_P( PSTR("ALARMA L%d: pos=%d smsLevel=%d, SMSnro=%s, MSG=%s !!\r\n"), level, pos, level, sVarsApp.l_sms[pos] , sms_msg );

		if ( ! xSMS_send( sVarsApp.l_sms[pos] , (char *) xSMS_format(sms_msg) ) ) {
			xprintf_P( PSTR("ERROR: ALARMA SMS NIVEL %d NO PUEDE SER ENVIADA !!!\r\n"),level );
		}
	}
}
//------------------------------------------------------------------------------------
// GENERAL
//------------------------------------------------------------------------------------
uint8_t xAPP_plantapot_hash( void )
{

	// En app_A_cks ponemos el checksum de los SMS y en app_B_cks los niveles de alarma

uint8_t hash = 0;
//char dst[48];
char *p;
uint8_t i;
uint8_t j;
int16_t free_size = sizeof(hash_buffer);

	memset(hash_buffer,'\0', sizeof(hash_buffer));

	j = 0;
	j += snprintf_P( hash_buffer, free_size, PSTR("PPOT;"));
	free_size = (  sizeof(hash_buffer) - j );
	if ( free_size < 0 ) goto exit_error;

	// Apunto al comienzo para recorrer el buffer
	p = hash_buffer;
	while (*p != '\0') {
		hash = u_hash(hash, *p++);
	}

	// Numeros de SMS
	for (i=0; i < MAX_NRO_SMS;i++) {
		// Vacio el buffer temoral
		memset(hash_buffer,'\0', sizeof(hash_buffer));
		free_size = sizeof(hash_buffer);
		// Copio sobe el buffer una vista ascii ( imprimible ) de c/registro.
		j += snprintf_P( hash_buffer, free_size, PSTR("SMS%02d:%s,%d;"), i, sVarsApp.l_sms[i], sVarsApp.plantapot.alm_level[i]);
		free_size = (  sizeof(hash_buffer) - j );
		if ( free_size < 0 ) goto exit_error;

		// Apunto al comienzo para recorrer el buffer
		p = hash_buffer;
		while (*p != '\0') {
			hash = u_hash(hash, *p++);
		}
	}

	// Niveles
	for (i=0; i<NRO_CANALES_ALM;i++) {
		// Vacio el buffer temoral

		memset(hash_buffer,'\0', sizeof(hash_buffer));
		free_size = sizeof(hash_buffer);
		j += snprintf_P( hash_buffer, free_size, PSTR("LCH%d:%.02f,%.02f,%.02f,%.02f,%.02f,%.02f;"), i,
			sVarsApp.plantapot.l_niveles_alarma[i].alarma1.lim_inf,sVarsApp.plantapot.l_niveles_alarma[i].alarma1.lim_sup,
			sVarsApp.plantapot.l_niveles_alarma[i].alarma2.lim_inf,sVarsApp.plantapot.l_niveles_alarma[i].alarma2.lim_sup,
			sVarsApp.plantapot.l_niveles_alarma[i].alarma3.lim_inf,sVarsApp.plantapot.l_niveles_alarma[i].alarma3.lim_sup );

		free_size = (  sizeof(hash_buffer) - j );
		if ( free_size < 0 ) goto exit_error;

		p = hash_buffer;
		while (*p != '\0') {
			hash = u_hash(hash, *p++);
		}
	}

	return(hash);

exit_error:
	xprintf_P( PSTR("COMMS: ppot_hash ERROR !!!\r\n\0"));
	return(0x00);

}
//------------------------------------------------------------------------------------
bool xAPP_plantapot_config( char *param0,char *param1, char *param2, char *param3, char *param4 )
{
	// config appalarm sms {id} {nro} {almlevel}\r\n\0"));
	// config appalarm nivel {chid} {alerta} {inf|sup} val\r\n\0"));

nivel_alarma_t nivel_alarma;
float valor;
uint8_t limite;
uint8_t pos;

	//xprintf_P(PSTR("p0=%s, p1=%s, p2=%s, p3=%s, p4=%s\r\n"), param0, param1, param2,param3, param4);

	if ( param1 == NULL ) {
		return(false);
	}

	if ( param2 == NULL ) {
		return(false);
	}

	if ( param3 == NULL ) {
		return(false);
	}

	// config appalarm sms {id} {nro} {almlevel}\r\n\0"));
	if (strcmp_P( strupr(param0), PSTR("SMS\0")) == 0 ) {

		pos = atoi(param1);
		if (pos >= MAX_NRO_SMS ){
			return false;
		}

		nivel_alarma = atoi(param3);
		if ( ( nivel_alarma <= 0 ) || ( nivel_alarma > ALARMA_NIVEL_3) ) {
			return(false);
		}

		memcpy( sVarsApp.l_sms[pos], param2, SMS_NRO_LENGTH );
		sVarsApp.plantapot.alm_level[pos] = nivel_alarma;
		return(true);

	}

	// config appalarm nivel {chid} {alerta} {inf|sup} val\r\n\0"));
	if ( strcmp_P( strupr(param0), PSTR("NIVEL\0")) == 0 ) {

		if ( param4 == NULL ) {
			return(false);
		}

		pos = atoi(param1);
		if ( pos > NRO_CANALES_ALM ) {
			return (false);
		}

		switch(atoi(param2)) {
		case 1:
			nivel_alarma = ALARMA_NIVEL_1;
			break;
		case 2:
			nivel_alarma = ALARMA_NIVEL_2;
			break;
		case 3:
			nivel_alarma = ALARMA_NIVEL_3;
			break;
		default:
			return(false);
		}

		if ( strcmp_P( strupr(param3), PSTR("INF\0")) == 0 ) {
			limite = 0;
		} else 	if ( strcmp_P( strupr(param3), PSTR("SUP\0")) == 0 ) {
			limite = 1;
		} else {
			return(false);
		}

		valor = atof(param4);

		// Limite INF/SUP
		if ( strcmp_P( strupr(param3), PSTR("INF\0")) == 0 ) {
			limite = 0;
		} else 	if ( strcmp_P( strupr(param3), PSTR("SUP\0")) == 0 ) {
			limite = 1;
		} else {
			return(false);
		}

		// Configuro
		switch ( nivel_alarma ) {
		case ALARMA_NIVEL_0:
			break;
		case ALARMA_NIVEL_1:
			if ( limite == 0 ) {
				sVarsApp.plantapot.l_niveles_alarma[pos].alarma1.lim_inf = valor;
			} else 	if ( limite == 1 ) {
				sVarsApp.plantapot.l_niveles_alarma[pos].alarma1.lim_sup = valor;
			}
			break;
		case ALARMA_NIVEL_2:
			if ( limite == 0 ) {
				sVarsApp.plantapot.l_niveles_alarma[pos].alarma2.lim_inf = valor;
			} else 	if ( limite == 1 ) {
				sVarsApp.plantapot.l_niveles_alarma[pos].alarma2.lim_sup = valor;
			}
			break;
		case ALARMA_NIVEL_3:
			if ( limite == 0 ) {
				sVarsApp.plantapot.l_niveles_alarma[pos].alarma3.lim_inf = valor;
			} else 	if ( limite == 1 ) {
				sVarsApp.plantapot.l_niveles_alarma[pos].alarma3.lim_sup = valor;
			}
			break;
		}

		return(true);
	}

	return(false);

}
//------------------------------------------------------------------------------------
void xAPP_plantapot_config_defaults(void)
{

uint8_t i;

	// Canal 0: PH
	sVarsApp.plantapot.l_niveles_alarma[0].alarma1.lim_inf = 6.7;
	sVarsApp.plantapot.l_niveles_alarma[0].alarma1.lim_sup = 8.3;
	sVarsApp.plantapot.l_niveles_alarma[0].alarma2.lim_inf = 6.0;
	sVarsApp.plantapot.l_niveles_alarma[0].alarma2.lim_sup = 9.0;
	sVarsApp.plantapot.l_niveles_alarma[0].alarma3.lim_inf = 5.5;
	sVarsApp.plantapot.l_niveles_alarma[0].alarma3.lim_sup = 9.5;

	// Canal 1: TURBIDEZ
	sVarsApp.plantapot.l_niveles_alarma[1].alarma1.lim_inf = -1.0;
	sVarsApp.plantapot.l_niveles_alarma[1].alarma1.lim_sup = 0.9;
	sVarsApp.plantapot.l_niveles_alarma[1].alarma2.lim_inf = -1.0;
	sVarsApp.plantapot.l_niveles_alarma[1].alarma2.lim_sup = 4.0;
	sVarsApp.plantapot.l_niveles_alarma[1].alarma3.lim_inf = -1.0;
	sVarsApp.plantapot.l_niveles_alarma[1].alarma3.lim_sup = 9.0;

	// Canal 2: CLORO
	sVarsApp.plantapot.l_niveles_alarma[2].alarma1.lim_inf = 0.7;
	sVarsApp.plantapot.l_niveles_alarma[2].alarma1.lim_sup = 2.3;
	sVarsApp.plantapot.l_niveles_alarma[2].alarma2.lim_inf = 0.5;
	sVarsApp.plantapot.l_niveles_alarma[2].alarma2.lim_sup = 3.5;
	sVarsApp.plantapot.l_niveles_alarma[2].alarma3.lim_inf = -1.0;
	sVarsApp.plantapot.l_niveles_alarma[2].alarma3.lim_sup = 5.0;

	for ( i = 3; i < NRO_CANALES_ALM; i++) {
		sVarsApp.plantapot.l_niveles_alarma[i].alarma1.lim_inf = 4.1;
		sVarsApp.plantapot.l_niveles_alarma[i].alarma1.lim_sup = 6.1;
		sVarsApp.plantapot.l_niveles_alarma[i].alarma2.lim_inf = 3.1;
		sVarsApp.plantapot.l_niveles_alarma[i].alarma2.lim_sup = 7.1;
		sVarsApp.plantapot.l_niveles_alarma[i].alarma3.lim_inf = 2.1;
		sVarsApp.plantapot.l_niveles_alarma[i].alarma3.lim_sup = 8.1;
	}

	for ( i = 0; i < MAX_NRO_SMS; i++) {
		memcpy( sVarsApp.l_sms[i], "X", SMS_NRO_LENGTH );
		if ( i < 3 ) {
			sVarsApp.plantapot.alm_level[i] = 1;
		} else if ( ( i >= 3) && ( i < 6 )) {
			sVarsApp.plantapot.alm_level[i] = 2;
		} else {
			sVarsApp.plantapot.alm_level[i] = 3;
		}
	}

}
//------------------------------------------------------------------------------------
void xAPP_plantapot_servicio_tecnico( char *action, char *device )
{
	// action: (prender/apagar/flash,rtimers )
	// device: (lroja,lverde,lamarilla,lnaranja, sirena)

//uint8_t i;

	/*
	if ( strcmp_P( strupr(action), PSTR("RTIMERS\0")) == 0) {
		for (i=0; i<NRO_CANALES_MONITOREO; i++) {
			xprintf_P(PSTR("DEBUG: timer[%d] status[%d] value[%d]\r\n"), i, l_ppot_input_channels[i].enabled, l_ppot_input_channels[i].timer );
		}
		return;
	}
	**/

	if ( strcmp_P( strupr(device), PSTR("LROJA\0")) == 0 ) {
		if ( strcmp_P( strupr(action), PSTR("PRENDER\0")) == 0 ) {
			ac_luz_roja(act_ON);
			return;
		} else 	if ( strcmp_P( strupr(action), PSTR("APAGAR\0")) == 0 ) {
			ac_luz_roja(act_OFF);
			return;
		}  else if ( strcmp_P( strupr(action), PSTR("FLASH\0")) == 0 ) {
			ac_luz_roja(act_FLASH);
			return;
		}
		xprintf_P(PSTR("ERROR\r\n\0"));
		return;
	}

	if ( strcmp_P( strupr(device), PSTR("LVERDE\0")) == 0 ) {
		if ( strcmp_P( strupr(action), PSTR("PRENDER\0")) == 0 ) {
			ac_luz_verde(act_ON);
			return;
		} else 	if ( strcmp_P( strupr(action), PSTR("APAGAR\0")) == 0) {
			ac_luz_verde(act_OFF);
			return;
		}  else if ( strcmp_P( strupr(action), PSTR("FLASH\0")) == 0) {
			ac_luz_verde(act_FLASH);
			return;
		}
		xprintf_P(PSTR("ERROR\r\n\0"));
		return;
	}

	if ( strcmp_P( strupr(device), PSTR("LAMARILLA\0")) == 0) {
		if ( strcmp_P( strupr(action), PSTR("PRENDER\0")) == 0) {
			ac_luz_amarilla(act_ON);
			return;
		} else 	if ( strcmp_P( strupr(action), PSTR("APAGAR\0")) == 0) {
			ac_luz_amarilla(act_OFF);
			return;
		}  else if ( strcmp_P( strupr(action), PSTR("FLASH\0")) == 0) {
			ac_luz_amarilla(act_FLASH);
			return;
		}
		xprintf_P(PSTR("ERROR\r\n\0"));
		return;
	}

	if ( strcmp_P( strupr(device), PSTR("LNARANJA\0")) == 0) {
		if ( strcmp_P( strupr(action), PSTR("PRENDER\0")) == 0) {
			ac_luz_naranja(act_ON);
			return;
		} else 	if ( strcmp_P( strupr(action), PSTR("APAGAR\0")) == 0) {
			ac_luz_naranja(act_OFF);
			return;
		} else if ( strcmp_P( strupr(action), PSTR("FLASH\0")) == 0) {
			ac_luz_naranja(act_FLASH);
			return;
		}
		xprintf_P(PSTR("ERROR\r\n\0"));
		return;
	}

	if ( strcmp_P( strupr(device), PSTR("LAZUL\0")) == 0) {
		if ( strcmp_P( strupr(action), PSTR("PRENDER\0")) == 0) {
			ac_luz_azul(act_ON);
			return;
		} else 	if ( strcmp_P( strupr(action), PSTR("APAGAR\0")) == 0) {
			ac_luz_azul(act_OFF);
			return;
		}  else if ( strcmp_P( strupr(action), PSTR("FLASH\0")) == 0) {
			ac_luz_azul(act_FLASH);
			return;
		}
		xprintf_P(PSTR("ERROR\r\n\0"));
		return;
	}

	if ( strcmp_P( strupr(device), PSTR("SIRENA\0")) == 0) {
		if ( strcmp_P( strupr(action), PSTR("PRENDER\0")) == 0) {
			ac_sirena(act_ON);
			return;
		} else 	if ( strcmp_P( strupr(action), PSTR("APAGAR\0")) == 0) {
			ac_sirena(act_OFF);
			return;
		}
		xprintf_P(PSTR("ERROR\r\n\0"));
		return;
	}

	xprintf_P(PSTR("ERROR\r\n\0"));
	return;
}
//------------------------------------------------------------------------------------
void xAPP_plantapot_print_status( bool full )
{

uint8_t pos;

	xprintf_P( PSTR("  modo: PLANTAPOT\r\n\0"));
	// Configuracion de los SMS
	xprintf_P( PSTR("  Nros.SMS configurados:\r\n"));
	for (pos = 0; pos < MAX_NRO_SMS; pos++) {
		xprintf_P( PSTR("   (%02d): %s, Nivel_%d\r\n"), pos, sVarsApp.l_sms[pos], sVarsApp.plantapot.alm_level[pos] );
	}

	// Configuracion de los canales y niveles de alarma configurados
	xprintf_P( PSTR("  Niveles de alarma:\r\n\0"));
	for ( pos=0; pos<NRO_CANALES_ALM; pos++) {
		xprintf_P( PSTR("    ch%d: ALM_L1:(%.02f,%.02f),"),pos, sVarsApp.plantapot.l_niveles_alarma[pos].alarma1.lim_inf,  sVarsApp.plantapot.l_niveles_alarma[pos].alarma1.lim_sup);
		xprintf_P( PSTR(" ALM_L2:(%.02f,%.02f),"), sVarsApp.plantapot.l_niveles_alarma[pos].alarma2.lim_inf,  sVarsApp.plantapot.l_niveles_alarma[pos].alarma2.lim_sup);
		xprintf_P( PSTR(" ALM_L3:(%.02f,%.02f) \r\n\0"),  sVarsApp.plantapot.l_niveles_alarma[pos].alarma3.lim_inf,  sVarsApp.plantapot.l_niveles_alarma[pos].alarma3.lim_sup);
	}

	// Entradas digitales
	xprintf_P( PSTR("  Entradas de control:\r\n\0"));
	xprintf_P( PSTR("    Mantenimiento:"));
	if ( alarmVars.llave_mantenimiento_on.master == true ) {
		xprintf_P( PSTR("ON"));
	} else {
		xprintf_P( PSTR("OFF"));
	}
	if ( full ) {
		if ( alarmVars.llave_mantenimiento_on.slave == true ) {
			xprintf_P( PSTR("/ON"));
		} else {
			xprintf_P( PSTR("/OFF"));
		}
	}
	xprintf_P( PSTR("\r\n"));

	xprintf_P( PSTR("    Boton:"));
	if ( alarmVars.boton_alarma_pressed.master == true ) {
		xprintf_P( PSTR("PRESIONADO"));
	} else {
		xprintf_P( PSTR("NORMAL"));
	}
	if ( full ) {
		if ( alarmVars.boton_alarma_pressed.slave == true ) {
			xprintf_P( PSTR("/PRESIONADO"));
		} else {
			xprintf_P( PSTR("/NORMAL"));
		}
	}
	xprintf_P( PSTR("\r\n"));

	xprintf_P( PSTR("    Puerta 1:"));
	if ( alarmVars.sensor_puerta_1_open.master== true ) {
		xprintf_P( PSTR("ABIERTA"));
	} else {
		xprintf_P( PSTR("CERRADA"));
	}
	if ( full ) {
		if ( alarmVars.sensor_puerta_1_open.slave == true ) {
			xprintf_P( PSTR("/ABIERTA"));
		} else {
			xprintf_P( PSTR("/CERRADA"));
		}
	}
	xprintf_P( PSTR("\r\n"));

	xprintf_P( PSTR("    Puerta 2:"));
	if ( alarmVars.sensor_puerta_2_open.master== true ) {
		xprintf_P( PSTR("ABIERTA"));
	} else {
		xprintf_P( PSTR("CERRADA"));
	}
	if ( full ) {
		if ( alarmVars.sensor_puerta_2_open.slave == true ) {
			xprintf_P( PSTR("/ABIERTA"));
		} else {
			xprintf_P( PSTR("/CERRADA"));
		}
	}
	xprintf_P( PSTR("\r\n"));

	// Estado del programa
	xprintf_P( PSTR("  Estado:"));
	switch (plantapot_state) {
	case st_NORMAL:
		xprintf_P( PSTR(" NORMAL (0)\r\n"));
		break;
	case st_ALARMADO:
		xprintf_P( PSTR(" ALARMADO (1)\r\n"));
		break;
	case st_STANDBY:
		xprintf_P( PSTR(" STANDBY (2) (%d)\r\n"), timer_en_standby );
		break;
	case st_MANTENIMIENTO:
		xprintf_P( PSTR(" MANTENIMIENTO (3) \r\n"));
		break;

	}

}
//------------------------------------------------------------------------------------
void xAPP_plantapot_test(void)
{
	// Funcion invocada desde el modo comando para ver como evolucionan
	// las variables de la aplicacion

uint8_t i;

	xAPP_plantapot_print_status(true);

	dinputs_service_read();

	xprintf_P(PSTR("  Timers x canal x nivel:\r\n") );

	for (i=0; i < NRO_CANALES_MONITOREO; i++) {

		xprintf_P(PSTR("    ch%02d:"), i );

		if ( alm_sysVars[i].enabled ) {
			xprintf_P(PSTR("[ON ] "));
		} else {
			xprintf_P(PSTR("[OFF] "));
		}
		xprintf_P(PSTR("val=%.02f, L1=%d, L2=%d, L3=%d FIRED=%d\r\n\0"), alm_sysVars[i].value, alm_sysVars[i].L1_timer, alm_sysVars[i].L2_timer, alm_sysVars[i].L3_timer, alm_sysVars[i].alm_fired );

	}


}
//------------------------------------------------------------------------------------
void xAPP_plantapot_adjust_vars( st_dataRecord_t *dr)
{

uint8_t i;

	// La funcion de tkInputs usa esta funcion para ajustar el valor de las variables digitales
	// a transmitir al servidor ya que estas deben transmitir el estado.

	// Niveles de alarma disparados
	for ( i = 0; i < NRO_CANALES_MONITOREO; i++) {

		if ( alm_sysVars[i].enabled == false ) {
			dr->df.io8.dinputs[i] = 99;
		} else {
			dr->df.io8.dinputs[i] = alm_sysVars[i].alm_fired;
		}

		// Si estoy en modo standby, reseteo los nivels de disparo.
		if ( ( plantapot_state == st_STANDBY ) || ( plantapot_state == st_NORMAL ))  {
			alm_sysVars[i].alm_fired = alm_NOT_FIRED;
		}

	}

	// Las señales de puerta, boton, llave son las slave para tener el cambio en el periodo.
	// Luego se resetean para seguir el valor de la señal master.
	dr->df.io8.dinputs[6] = alarmVars.llave_mantenimiento_on.slave;
	alarmVars.llave_mantenimiento_on.slave = alarmVars.llave_mantenimiento_on.master;

	dr->df.io8.dinputs[7] = alarmVars.boton_alarma_pressed.slave;
	alarmVars.boton_alarma_pressed.slave = alarmVars.boton_alarma_pressed.master;

	dr->df.io8.counters[0] = alarmVars.sensor_puerta_1_open.slave;
	alarmVars.sensor_puerta_1_open.slave = alarmVars.sensor_puerta_1_open.master;

	dr->df.io8.counters[1] = alarmVars.sensor_puerta_2_open.master;
	alarmVars.sensor_puerta_2_open.slave = alarmVars.sensor_puerta_2_open.master;

	// Estado en que se encuentra el sistema de alarmas.
	dr->df.io8.ainputs[7] = plantapot_state;


}
//------------------------------------------------------------------------------------

