/*
 * ul_alarmas_ose.c
 *
 *  Created on: 5 nov. 2019
 *      Author: pablo
 */

#include "spx.h"
#include "gprs.h"

bool flash_luz_verde;
bool flash_luz_roja;
bool flash_luz_amarilla;
bool flash_luz_azul;
bool flash_luz_naranja;

static int16_t timer_en_standby;

static void pv_appalarma_TimerCallback( TimerHandle_t xTimer );
static void pv_appalarma_leer_entradas(void);
static void pv_appalarma_check_inband(void);
static bool pv_fire_alarmas(void);
static void pv_apagar_alarmas(void);
static void pv_resetear_timers(void);
static void pv_reset_almVars(void);

TimerHandle_t appalarma_timerAlarmas;
StaticTimer_t appalarma_xTimerAlarmas;

uint16_t din[IO8_DINPUTS_CHANNELS];

static t_appalm_states appalarma_state;

// Estados
void st_appalarma_normal(void);
void st_appalarma_alarmado(void);
void st_appalarma_mantenimiento(void);
void st_appalarma_standby(void);

// Acciones
static void ac_luz_verde( t_dev_action action);
static void ac_luz_roja( t_dev_action action);
static void ac_luz_amarilla( t_dev_action action);
static void ac_luz_naranja( t_dev_action action);
static void ac_luz_azul( t_dev_action action);
static void ac_sirena(t_dev_action action);

static uint8_t ac_read_status_sensor_puerta(void);
static uint8_t ac_read_status_mantenimiento(void);
static uint8_t ac_read_status_boton_alarma(void);

//------------------------------------------------------------------------------------
void appalarma_stk(void)
{

	if ( spx_io_board != SPX_IO8CH ) {
		xprintf_P(PSTR("ALARMAS: Init ERROR: Control Alarmas only in IO_8CH.\r\n\0"));
		systemVars.aplicacion = APP_OFF;
		u_save_params_in_NVMEE();
		return;
	}

	// Creo y arranco el timer de control del tiempo de alarmas
	appalarma_timerAlarmas = xTimerCreateStatic ("PPOT",
			pdMS_TO_TICKS( 1000 ),
			pdTRUE,
			( void * ) 0,
			pv_appalarma_TimerCallback,
			&appalarma_xTimerAlarmas
			);

	xTimerStart(appalarma_timerAlarmas, 10);

	for (;;) {

		switch(appalarma_state) {
		case st_NORMAL:
			st_appalarma_normal();
			break;
		case st_ALARMADO:
			st_appalarma_alarmado();
			break;
		case st_STANDBY:
			st_appalarma_standby();
			break;
		case st_MANTENIMIENTO:
			st_appalarma_mantenimiento();
			break;
		default:
			xprintf_P(PSTR("MODO OPERACION ERROR: Plantapot state unknown (%d)\r\n\0"), appalarma_state );
			systemVars.aplicacion = APP_OFF;
			u_save_params_in_NVMEE();
			return;
			break;
		}
	}
}
//------------------------------------------------------------------------------------
// ESTADOS
//------------------------------------------------------------------------------------
void st_appalarma_normal(void)
{

// Entry:
	if ( systemVars.debug == DEBUG_APLICACION ) {
		xprintf_P(PSTR("APP_PLANTAPOT: Ingreso en modo Normal.\r\n\0"));
	}

	appalarma_init();
	ac_luz_verde( act_ON );

// Loop:
	// Genero un ciclo por segundo !!!

	while ( appalarma_state == st_NORMAL ) {

		ctl_watchdog_kick( WDG_APP,  WDG_APP_TIMEOUT );
		// Espera de 1s
		vTaskDelay( ( TickType_t)( 1000 / portTICK_RATE_MS ) );

		// Leo los datos
		pv_appalarma_leer_entradas();
		// Veo si hay alguna medida en una region de alarma
		pv_appalarma_check_inband();

		// CAMBIO DE ESTADO.
		// Disparo alarmas si es necesario
		if ( pv_fire_alarmas() ) {
			// Si dispare una alarma, paso al estado de sistema alarmado
			appalarma_state = st_ALARMADO;
		}

		// Veo si se activo la llave de mantenimiento.
		if ( alarmVars.llave_mantenimiento_on == true ) {
			// Activaron la llave de mantenimiento. Salgo
			appalarma_state = st_MANTENIMIENTO;
		}
	}

// Exit
	ac_luz_verde( act_OFF );
	return;

}
//------------------------------------------------------------------------------------
void st_appalarma_alarmado(void)
{
	// En este estado tengo al menos una alarma disparada.
	// Sigo monitoreando pero ahora controlo que presiones el botón de reset
	// Entry:

uint8_t timer_boton_pressed = SECS_BOTON_PRESSED;

	if ( systemVars.debug == DEBUG_APLICACION ) {
		xprintf_P(PSTR("APP_PLANTAPOT: Ingreso en modo Alarmado.\r\n\0"));
	}

	ac_luz_verde( act_OFF );

	// Loop:
	// Genero un ciclo por segundo !!!

	while ( appalarma_state == st_ALARMADO ) {

		ctl_watchdog_kick( WDG_APP,  WDG_APP_TIMEOUT );
		// Espera de 1s
		vTaskDelay( ( TickType_t)( 1000 / portTICK_RATE_MS ) );

		// Leo los datos
		pv_appalarma_leer_entradas();
		// Veo si hay alguna medida en una region de alarma
		pv_appalarma_check_inband();
		// Disparo alarmas si es necesario
		pv_fire_alarmas();

		// CAMBIO DE ESTADO
		// Veo si presiono el boton de reset de alarma por mas de 5secs.
		if ( ! alarmVars.boton_alarma_pressed ) {
			timer_boton_pressed = SECS_BOTON_PRESSED;
		} else {
			timer_boton_pressed--;
		}

		if ( timer_boton_pressed == 0 ) {
			// Reconoci la alarma por tener apretado el boton de pressed mas de 5s
			appalarma_state = st_STANDBY;
		}

		// Veo si se activo la llave de mantenimiento.
		if ( alarmVars.llave_mantenimiento_on == true ) {
			// Activaron la llave de mantenimiento. Salgo
			appalarma_state = st_MANTENIMIENTO;
		}

	}

// Exit
	return;
}
//------------------------------------------------------------------------------------
void st_appalarma_mantenimiento(void)
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
	if ( systemVars.debug == DEBUG_APLICACION ) {
		xprintf_P(PSTR("APP_PLANTAPOT: Ingreso en modo Mantenimiento.\r\n\0"));
	}
	// Al entrar en mantenimiento apago todas las posibles señales activas.
	// Solo debe flashear la luz verde.
	appalarma_init();

	// Flasheo luz azul
	ac_luz_azul(act_FLASH);

// Loop:
	while ( appalarma_state == st_MANTENIMIENTO ) {

		ctl_watchdog_kick( WDG_APP,  WDG_APP_TIMEOUT );
		// Espera de 1s
		vTaskDelay( ( TickType_t)( 1000 / portTICK_RATE_MS ) );

		// Veo si apagaron la llave de mantenimiento.
		if ( alarmVars.llave_mantenimiento_on == false ) {
			// Apagaron la llave de mantenimiento. Salgo
			appalarma_state = st_NORMAL;
		}

	}

// Exit
	ac_luz_azul(act_OFF);
	return;

}
//------------------------------------------------------------------------------------
void st_appalarma_standby(void)
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
	if ( systemVars.debug == DEBUG_APLICACION ) {
		xprintf_P(PSTR("APP_PLANTAPOT: Ingreso en modo Standby.\r\n\0"));
	}
	// Debo permancer 30 minutos.
	timer_en_standby = TIME_IN_STANDBY;

	// Apago todas las alarmas
	appalarma_init();
	ac_luz_verde(act_FLASH);

// Loop:
	while ( appalarma_state == st_STANDBY ) {

		ctl_watchdog_kick( WDG_APP,  WDG_APP_TIMEOUT );
		// Espera de 1s
		vTaskDelay( ( TickType_t)( 1000 / portTICK_RATE_MS ) );

		// CAMBIO DE ESTADO
		// Veo si activaron la llave de mantenimiento.
		if ( alarmVars.llave_mantenimiento_on == true ) {
			appalarma_state = st_MANTENIMIENTO;
		}

		// Veo si expiro el tiempo para salir y volver al estado NORMAL
		if ( timer_en_standby-- <= 1 ) {
			appalarma_state = st_NORMAL;
		}

	}

// Exit
	timer_en_standby = 0;
	ac_luz_verde(act_OFF);

}
//------------------------------------------------------------------------------------
// FUNCIONES GENERALES PRIVADAS
//------------------------------------------------------------------------------------
bool appalarma_init(void)
{

	// Inicializa las salidas y las variables de trabajo.
	pv_apagar_alarmas();
	pv_reset_almVars();
	pv_resetear_timers();
	//
	// Borro los SMS de alarmas pendientes
	u_sms_init();

	return(true);
}
//------------------------------------------------------------------------------------
static void pv_appalarma_TimerCallback( TimerHandle_t xTimer )
{
	// Se ejecuta cada 1s como callback del timer.

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

	if ( din[IPIN_LLAVE_MANTENIMIENTO] == 1 ) {
		alarmVars.llave_mantenimiento_on = true;
	} else {
		alarmVars.llave_mantenimiento_on = false;
	}

	if ( din[IPIN_BOTON_ALARMA] == 	1 ) {
		alarmVars.boton_alarma_pressed = true;
	} else {
		alarmVars.boton_alarma_pressed = false;
	}

	// La entrada del sensor de puerta la leo directamente de la entrda del CNT0
	if ( CNT_read_CNT0() == 0 ) {
		alarmVars.sensor_puerta_open = true;
	} else {
		alarmVars.sensor_puerta_open = false;
	}

}
//------------------------------------------------------------------------------------
static void pv_appalarma_leer_entradas(void)
{
	// El poleo de las entradas lo hace la tarea tkInputs. Esta debe tener un timerpoll de 30s
	// Aqui c/1s leo las entradas en modo 'copy' ( sin poleo ).
	// Por eso no tengo que monitorear los 30s sino que lo hago c/1s. y esto me permite tener
	// permantente control de la tarea.
	// Leo las entradas analogicas
	// Los sensores externos: llave_mantenimiento, sensor_puerta, boton_alarma
	// los leo en la funcion de callback del timer.

st_dataRecord_t dr;
uint8_t i;

	// Leo todas las entradas analogicas, digitales, contadores.
	// Es un DLG8CH
	// Leo en modo 'copy', sin poleo real.
	data_read_inputs(&dr, true );

	for (i=0; i < NRO_CANALES_MONITOREO; i++) {
		// Los switches ( canales digitales ) indican si el canal esta
		// habilitado si o no.
		if ( dr.df.io8.dinputs[i] == 0 ) {
			alm_sysVars[i].enabled = false;
		} else {
			alm_sysVars[i].enabled = true;
		}

		// Leo el valor de todos los canales ( habilitados o no )
		alm_sysVars[i].value = dr.df.io8.ainputs[i];

	}

}
//------------------------------------------------------------------------------------
static void pv_appalarma_check_inband(void)
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

		value = alm_sysVars[i].value;

		// ALARMA NIVEL_3
		if (alm_sysVars[i].L3_timer > 0 ) {
			if ( value > systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[i].alarma3.lim_sup ) {
				alm_sysVars[i].L3_timer--;
				continue;
			}

			if ( value < systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[i].alarma3.lim_inf ) {
				alm_sysVars[i].L3_timer--;
				continue;
			}
		}

		// ALARMA NIVEL_2
		if (alm_sysVars[i].L2_timer > 0 ) {
			if ( ( value > systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[i].alarma2.lim_sup ) &&
				( value < systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[i].alarma3.lim_sup ) ) {
				alm_sysVars[i].L2_timer--;
				continue;
			}

			if ( ( value < systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[i].alarma2.lim_inf ) &&
				( value > systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[i].alarma3.lim_inf ) ) {
				alm_sysVars[i].L2_timer--;
				continue;
			}
		}

		// ALARMA NIVEL_1
		if (alm_sysVars[i].L1_timer > 0 ) {

			if ( ( value > systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[i].alarma1.lim_sup ) &&
				( value < systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[i].alarma2.lim_sup ) ) {
				alm_sysVars[i].L1_timer--;
				continue;
			}
			if ( ( value < systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[i].alarma1.lim_inf ) &&
				( value > systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[i].alarma2.lim_inf ) ) {
				alm_sysVars[i].L1_timer--;
				continue;
			}
		}

		// NIVELES NORMALES
		if ( ( value <  systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[i].alarma1.lim_sup ) &&
				( value >= systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[i].alarma1.lim_inf ) ) {

			// Estoy en la banda normal. Reseteo los timers
			alm_sysVars[i].L3_timer = SECS_ALM_LEVEL_3;
			alm_sysVars[i].L2_timer = SECS_ALM_LEVEL_2;
			alm_sysVars[i].L1_timer = SECS_ALM_LEVEL_1;
		}

	}
}
//------------------------------------------------------------------------------------
static bool pv_fire_alarmas(void)
{
	// Revisa si hay algún canal que deba disparar alguna alarma.
	// En caso afirmativo la dispara.

uint8_t i;
uint8_t pos;
bool alm_fired = false;
char sms_msg[SMS_MSG_LENGTH];


	for (i=0; i < NRO_CANALES_MONITOREO; i++) {

		// Si el canal esta apagado lo salteo
		if ( alm_sysVars[i].enabled == false )
			continue;

		// Disparo alarma LEVEL_1
		if ( alm_sysVars[i].L1_timer == 1 ) {
			// Luces
			ac_luz_verde(act_OFF);
			ac_luz_amarilla(act_FLASH);
			// Envio Sms
			snprintf_P( sms_msg, SMS_MSG_LENGTH, PSTR("ALARMA Nivel 1 activada por: %s\0"), systemVars.ainputs_conf.name[i] );
			for (pos = 0; pos < MAX_NRO_SMS_ALARMAS; pos++) {
				// Envio a todos los numeros configurados para nivel 1.
				if ( ( systemVars.aplicacion_conf.alarma_ppot.l_sms[pos].alm_level == 1) && ( strcmp ( systemVars.ainputs_conf.name[i], "X" ) != 0 )) {
					if ( ! u_sms_send( systemVars.aplicacion_conf.alarma_ppot.l_sms[pos].sms_nro, sms_msg ) ) {
						xprintf_P( PSTR("ERROR: ALARMA SMS NIVEL 1 NO PUEDE SER ENVIADA !!!\r\n"));
					}
				}
				xprintf_P( PSTR("ALARMA L1 SMS: NRO:%s, MSG:%s\r\n"),systemVars.aplicacion_conf.alarma_ppot.l_sms[pos].sms_nro, sms_msg );
			}
			alm_fired = true;
		}

		// Disparo alarma LEVEL_2
		if ( alm_sysVars[i].L2_timer == 1 ) {
			ac_luz_verde(act_OFF);
			ac_luz_naranja(act_FLASH);
			// Envio Sms
			snprintf_P( sms_msg, SMS_MSG_LENGTH, PSTR("ALARMA Nivel 2 activada por: %s\0"), systemVars.ainputs_conf.name[i] );
			for (pos = 0; pos < MAX_NRO_SMS_ALARMAS; pos++) {
				// Envio a todos los numeros configurados para nivel 1.
				if ( ( systemVars.aplicacion_conf.alarma_ppot.l_sms[pos].alm_level == 2) && ( strcmp ( systemVars.ainputs_conf.name[i], "X" ) != 0 )) {
					if ( ! u_sms_send( systemVars.aplicacion_conf.alarma_ppot.l_sms[pos].sms_nro, sms_msg ) ) {
						xprintf_P( PSTR("ERROR: ALARMA SMS NIVEL 2 NO PUEDE SER ENVIADA !!!\r\n"));
					}
				}
				xprintf_P( PSTR("ALARMA L2 SMS: NRO:%s, MSG:%s\r\n"),systemVars.aplicacion_conf.alarma_ppot.l_sms[pos].sms_nro, sms_msg );
			}
			alm_fired = true;
		}

		// Disparo alarma LEVEL_3
		if ( alm_sysVars[i].L3_timer == 1 ) {
			ac_luz_verde(act_OFF);
			ac_luz_roja(act_FLASH);
			// Envio Sms
			snprintf_P( sms_msg, SMS_MSG_LENGTH, PSTR("ALARMA Nivel 3 activada por: %s\0"), systemVars.ainputs_conf.name[i] );
			for (pos = 0; pos < MAX_NRO_SMS_ALARMAS; pos++) {
				// Envio a todos los numeros configurados para nivel 1.
				if ( ( systemVars.aplicacion_conf.alarma_ppot.l_sms[pos].alm_level == 3) && ( strcmp ( systemVars.ainputs_conf.name[i], "X" ) != 0 )) {
					if ( ! u_sms_send( systemVars.aplicacion_conf.alarma_ppot.l_sms[pos].sms_nro, sms_msg ) ) {
						xprintf_P( PSTR("ERROR: ALARMA SMS NIVEL 3 NO PUEDE SER ENVIADA !!!\r\n"));
					}
				}
				xprintf_P( PSTR("ALARMA L3 SMS: NRO:%s, MSG:%s\r\n"),systemVars.aplicacion_conf.alarma_ppot.l_sms[pos].sms_nro, sms_msg );
			}
			alm_fired = true;
		}

	}

	return(alm_fired);

}
//------------------------------------------------------------------------------------
static void pv_apagar_alarmas(void)
{
	ac_luz_verde( act_OFF );
	ac_luz_roja( act_OFF );
	ac_luz_amarilla( act_OFF );
	ac_luz_naranja( act_OFF );
	ac_luz_azul(act_OFF);
	ac_sirena( act_OFF );
}
//------------------------------------------------------------------------------------
static void pv_reset_almVars(void)
{
	alarmVars.boton_alarma_pressed = false;
	alarmVars.llave_mantenimiento_on= false;
	alarmVars.sensor_puerta_open = false;

}
//------------------------------------------------------------------------------------
static void pv_resetear_timers(void)
{

uint8_t i;

	// Inicializo los timers por canal y por nivel de alamra
	for ( i = 0; i < NRO_CANALES_MONITOREO; i++) {
		alm_sysVars[i].enabled = false;
		alm_sysVars[i].L1_timer = SECS_ALM_LEVEL_1;
		alm_sysVars[i].L2_timer = SECS_ALM_LEVEL_2;
		alm_sysVars[i].L3_timer = SECS_ALM_LEVEL_3;
	}
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
static uint8_t ac_read_status_sensor_puerta(void)
{
	if ( alarmVars.sensor_puerta_open == true ) {
		return(1);
	} else {
		return(0);
	}
}
//------------------------------------------------------------------------------------
static uint8_t ac_read_status_mantenimiento(void)
{
	if ( alarmVars.llave_mantenimiento_on ) {
		return(1);
	} else {
		return(0);
	}
}
//------------------------------------------------------------------------------------
static uint8_t ac_read_status_boton_alarma(void)
{

	if ( alarmVars.boton_alarma_pressed) {
		return(1);
	} else {
		return(0);
	}
}
//------------------------------------------------------------------------------------
// GENERAL
//------------------------------------------------------------------------------------
uint8_t appalarma_checksum( void )
{

	// En app_A_cks ponemos el checksum de los SMS y en app_B_cks los niveles de alarma

uint8_t checksum = 0;
char dst[48];
char *p;
uint8_t i;


	memset(dst,'\0', sizeof(dst));
	snprintf_P( dst, sizeof(dst), PSTR("PPOT;"));
	// Apunto al comienzo para recorrer el buffer
	p = dst;
	// Mientras no sea NULL calculo el checksum deol buffer
	while (*p != '\0') {
		checksum += *p++;
	}
	//xprintf_P( PSTR("DEBUG: PPOT_CKS = [%s]\r\n\0"), dst );
	//xprintf_P( PSTR("DEBUG: PPOT_CKS cks = [0x%02x]\r\n\0"), checksum );

	// Numeros de SMS
	for (i=0; i<MAX_NRO_SMS_ALARMAS;i++) {
		// Vacio el buffer temoral
		memset(dst,'\0', sizeof(dst));
		// Copio sobe el buffer una vista ascii ( imprimible ) de c/registro.
		snprintf_P( dst, sizeof(dst), PSTR("SMS%02d:%s,%d;"), i, systemVars.aplicacion_conf.alarma_ppot.l_sms[i].sms_nro, systemVars.aplicacion_conf.alarma_ppot.l_sms[i].alm_level);
		// Apunto al comienzo para recorrer el buffer
		p = dst;
		// Mientras no sea NULL calculo el checksum deol buffer
		while (*p != '\0') {
			checksum += *p++;
		}
		//xprintf_P( PSTR("DEBUG: PPOT_CKS = [%s]\r\n\0"), dst );
		//xprintf_P( PSTR("DEBUG: PPOT_CKS cks = [0x%02x]\r\n\0"), checksum );
	}

	// Niveles
	for (i=0; i<NRO_CANALES_ALM;i++) {
		// Vacio el buffer temoral

		memset(dst,'\0', sizeof(dst));
		snprintf_P( dst, sizeof(dst), PSTR("LCH%d:%.02f,%.02f,%.02f,%.02f,%.02f,%.02f;"), i,
			systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[i].alarma1.lim_inf,systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[i].alarma1.lim_sup,
			systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[i].alarma2.lim_inf,systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[i].alarma2.lim_sup,
			systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[i].alarma3.lim_inf,systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[i].alarma3.lim_sup );
		p = dst;
		// Mientras no sea NULL calculo el checksum deol buffer
		while (*p != '\0') {
			checksum += *p++;
		}
		//xprintf_P( PSTR("DEBUG: PPOT_CKS = [%s]\r\n\0"), dst );
		//xprintf_P( PSTR("DEBUG: PPOT_CKS cks = [0x%02x]\r\n\0"), checksum );
	}

	return(checksum);

}
//------------------------------------------------------------------------------------
bool appalarma_config( char *param0,char *param1, char *param2, char *param3, char *param4 )
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
	if (!strcmp_P( strupr(param0), PSTR("SMS\0")) ) {

		pos = atoi(param1);
		if (pos >= MAX_NRO_SMS_ALARMAS ){
			return false;
		}

		nivel_alarma = atoi(param3);
		if ( ( nivel_alarma <= 0 ) || ( nivel_alarma > ALARMA_NIVEL_3) ) {
			return(false);
		}

		memcpy( systemVars.aplicacion_conf.alarma_ppot.l_sms[pos].sms_nro, param2, SMS_NRO_LENGTH );
		systemVars.aplicacion_conf.alarma_ppot.l_sms[pos].alm_level = nivel_alarma;
		return(true);

	}

	// config appalarm nivel {chid} {alerta} {inf|sup} val\r\n\0"));
	if (!strcmp_P( strupr(param0), PSTR("NIVEL\0")) ) {

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

		if (!strcmp_P( strupr(param3), PSTR("INF\0")) ) {
			limite = 0;
		} else 	if (!strcmp_P( strupr(param3), PSTR("SUP\0")) ) {
			limite = 1;
		} else {
			return(false);
		}

		valor = atof(param4);

		// Limite INF/SUP
		if (!strcmp_P( strupr(param3), PSTR("INF\0")) ) {
			limite = 0;
		} else 	if (!strcmp_P( strupr(param3), PSTR("SUP\0")) ) {
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
				systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[pos].alarma1.lim_inf = valor;
			} else 	if ( limite == 1 ) {
				systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[pos].alarma1.lim_sup = valor;
			}
			break;
		case ALARMA_NIVEL_2:
			if ( limite == 0 ) {
				systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[pos].alarma2.lim_inf = valor;
			} else 	if ( limite == 1 ) {
				systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[pos].alarma2.lim_sup = valor;
			}
			break;
		case ALARMA_NIVEL_3:
			if ( limite == 0 ) {
				systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[pos].alarma3.lim_inf = valor;
			} else 	if ( limite == 1 ) {
				systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[pos].alarma3.lim_sup = valor;
			}
			break;
		}

		return(true);
	}

	return(false);

}
//------------------------------------------------------------------------------------
void appalarma_config_defaults(void)
{

uint8_t i;

	// Canal 0: PH
	systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[0].alarma1.lim_inf = 6.7;
	systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[0].alarma1.lim_sup = 8.3;
	systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[0].alarma2.lim_inf = 6.0;
	systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[0].alarma2.lim_sup = 9.0;
	systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[0].alarma3.lim_inf = 5.5;
	systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[0].alarma3.lim_sup = 9.5;

	// Canal 1: TURBIDEZ
	systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[1].alarma1.lim_inf = -1.0;
	systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[1].alarma1.lim_sup = 0.9;
	systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[1].alarma2.lim_inf = -1.0;
	systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[1].alarma2.lim_sup = 4.0;
	systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[1].alarma3.lim_inf = -1.0;
	systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[1].alarma3.lim_sup = 9.0;

	// Canal 2: CLORO
	systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[2].alarma1.lim_inf = 0.7;
	systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[2].alarma1.lim_sup = 2.3;
	systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[2].alarma2.lim_inf = 0.5;
	systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[2].alarma2.lim_sup = 3.5;
	systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[2].alarma3.lim_inf = -1.0;
	systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[2].alarma3.lim_sup = 5.0;

	for ( i = 3; i < NRO_CANALES_ALM; i++) {
		systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[i].alarma1.lim_inf = 4.1;
		systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[i].alarma1.lim_sup = 6.1;
		systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[i].alarma2.lim_inf = 3.1;
		systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[i].alarma2.lim_sup = 7.1;
		systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[i].alarma3.lim_inf = 2.1;
		systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[i].alarma3.lim_sup = 8.1;
	}

	for ( i = 0; i < MAX_NRO_SMS_ALARMAS; i++) {
		memcpy( systemVars.aplicacion_conf.alarma_ppot.l_sms[i].sms_nro, "X", SMS_NRO_LENGTH );
		if ( i < 3 ) {
			systemVars.aplicacion_conf.alarma_ppot.l_sms[i].alm_level = 1;
		} else if ( ( i >= 3) && ( i < 6 )) {
			systemVars.aplicacion_conf.alarma_ppot.l_sms[i].alm_level = 2;
		} else {
			systemVars.aplicacion_conf.alarma_ppot.l_sms[i].alm_level = 3;
		}
	}

}
//------------------------------------------------------------------------------------
void appalarma_servicio_tecnico( char *action, char *device )
{
	// action: (prender/apagar/flash,rtimers )
	// device: (lroja,lverde,lamarilla,lnaranja, sirena)

//uint8_t i;

	/*
	if ( !strcmp_P( strupr(action), PSTR("RTIMERS\0"))) {
		for (i=0; i<NRO_CANALES_MONITOREO; i++) {
			xprintf_P(PSTR("DEBUG: timer[%d] status[%d] value[%d]\r\n"), i, l_ppot_input_channels[i].enabled, l_ppot_input_channels[i].timer );
		}
		return;
	}
	**/

	if ( !strcmp_P( strupr(device), PSTR("LROJA\0"))) {
		if ( !strcmp_P( strupr(action), PSTR("PRENDER\0"))) {
			ac_luz_roja(act_ON);
			return;
		} else 	if ( !strcmp_P( strupr(action), PSTR("APAGAR\0"))) {
			ac_luz_roja(act_OFF);
			return;
		}  else if ( !strcmp_P( strupr(action), PSTR("FLASH\0"))) {
			ac_luz_roja(act_FLASH);
			return;
		}
		xprintf_P(PSTR("ERROR\r\n\0"));
		return;
	}

	if ( !strcmp_P( strupr(device), PSTR("LVERDE\0"))) {
		if ( !strcmp_P( strupr(action), PSTR("PRENDER\0"))) {
			ac_luz_verde(act_ON);
			return;
		} else 	if ( !strcmp_P( strupr(action), PSTR("APAGAR\0"))) {
			ac_luz_verde(act_OFF);
			return;
		}  else if ( !strcmp_P( strupr(action), PSTR("FLASH\0"))) {
			ac_luz_verde(act_FLASH);
			return;
		}
		xprintf_P(PSTR("ERROR\r\n\0"));
		return;
	}

	if ( !strcmp_P( strupr(device), PSTR("LAMARILLA\0"))) {
		if ( !strcmp_P( strupr(action), PSTR("PRENDER\0"))) {
			ac_luz_amarilla(act_ON);
			return;
		} else 	if ( !strcmp_P( strupr(action), PSTR("APAGAR\0"))) {
			ac_luz_amarilla(act_OFF);
			return;
		}  else if ( !strcmp_P( strupr(action), PSTR("FLASH\0"))) {
			ac_luz_amarilla(act_FLASH);
			return;
		}
		xprintf_P(PSTR("ERROR\r\n\0"));
		return;
	}

	if ( !strcmp_P( strupr(device), PSTR("LNARANJA\0"))) {
		if ( !strcmp_P( strupr(action), PSTR("PRENDER\0"))) {
			ac_luz_naranja(act_ON);
			return;
		} else 	if ( !strcmp_P( strupr(action), PSTR("APAGAR\0"))) {
			ac_luz_naranja(act_OFF);
			return;
		} else if ( !strcmp_P( strupr(action), PSTR("FLASH\0"))) {
			ac_luz_naranja(act_FLASH);
			return;
		}
		xprintf_P(PSTR("ERROR\r\n\0"));
		return;
	}

	if ( !strcmp_P( strupr(device), PSTR("LAZUL\0"))) {
		if ( !strcmp_P( strupr(action), PSTR("PRENDER\0"))) {
			ac_luz_azul(act_ON);
			return;
		} else 	if ( !strcmp_P( strupr(action), PSTR("APAGAR\0"))) {
			ac_luz_azul(act_OFF);
			return;
		}  else if ( !strcmp_P( strupr(action), PSTR("FLASH\0"))) {
			ac_luz_azul(act_FLASH);
			return;
		}
		xprintf_P(PSTR("ERROR\r\n\0"));
		return;
	}

	if ( !strcmp_P( strupr(device), PSTR("SIRENA\0"))) {
		if ( !strcmp_P( strupr(action), PSTR("PRENDER\0"))) {
			ac_sirena(act_ON);
			return;
		} else 	if ( !strcmp_P( strupr(action), PSTR("APAGAR\0"))) {
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
void appalarma_print_status(void)
{

uint8_t pos;

	xprintf_P( PSTR("  modo: PLANTAPOT\r\n\0"));
	// Configuracion de los SMS
	xprintf_P( PSTR("  Nros.SMS configurados:\r\n"));
	for (pos = 0; pos < MAX_NRO_SMS_ALARMAS; pos++) {
		xprintf_P( PSTR("   (%02d): %s, Nivel_%d\r\n"), pos, systemVars.aplicacion_conf.alarma_ppot.l_sms[pos].sms_nro, systemVars.aplicacion_conf.alarma_ppot.l_sms[pos].alm_level );
	}

	// Configuracion de los canales y niveles de alarma configurados
	xprintf_P( PSTR("  Niveles de alarma:\r\n\0"));
	for ( pos=0; pos<NRO_CANALES_ALM; pos++) {
		xprintf_P( PSTR("    ch%d: ALM_L1:(%.02f,%.02f),"),pos, systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[pos].alarma1.lim_inf,  systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[pos].alarma1.lim_sup);
		xprintf_P( PSTR(" ALM_L2:(%.02f,%.02f),"), systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[pos].alarma2.lim_inf,  systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[pos].alarma2.lim_sup);
		xprintf_P( PSTR(" ALM_L3:(%.02f,%.02f) \r\n\0"),  systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[pos].alarma3.lim_inf,  systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[pos].alarma3.lim_sup);
	}

	// Entradas digitales
	xprintf_P( PSTR("  Entradas de control:\r\n\0"));
	if ( ac_read_status_mantenimiento() ) {
		xprintf_P( PSTR("    mantenimiento: ON\r\n\0"));
	} else {
		xprintf_P( PSTR("    mantenimiento: OFF\r\n\0"));
	}

	if ( ac_read_status_sensor_puerta() ) {
		xprintf_P( PSTR("    Puerta: ABIERTA\r\n\0"));
	} else {
		xprintf_P( PSTR("    Puerta: CERRADA\r\n\0"));
	}

	if ( ac_read_status_boton_alarma() ) {
		xprintf_P( PSTR("    boton alarma: PRESIONADO\r\n\0"));
	} else {
		xprintf_P( PSTR("    boton alarma: NORMAL\r\n\0"));
	}

	// Estado del programa
	xprintf_P( PSTR("  Estado:"));
	switch (appalarma_state) {
	case st_NORMAL:
		xprintf_P( PSTR(" NORMAL\r\n"));
		break;
	case st_ALARMADO:
		xprintf_P( PSTR(" ALARMADO\r\n"));
		break;
	case st_STANDBY:
		xprintf_P( PSTR(" STANDBY(%d)\r\n"), timer_en_standby );
		break;
	case st_MANTENIMIENTO:
		xprintf_P( PSTR(" MANTENIMIENTO\r\n"));
		break;

	}

}
//------------------------------------------------------------------------------------
void appalarma_test(void)
{
	// Funcion invocada desde el modo comando para ver como evolucionan
	// las variables de la aplicacion

uint8_t i;

	appalarma_print_status();

	dinputs_service_read();

	xprintf_P(PSTR("  Timers x canal x nivel:\r\n") );

	for (i=0; i < NRO_CANALES_MONITOREO; i++) {
		xprintf_P(PSTR("    ch%02d: [%d] val=%.02f, L1=%d, L2=%d, L3=%d\r\n\0"), i, alm_sysVars[i].enabled, alm_sysVars[i].value, alm_sysVars[i].L1_timer, alm_sysVars[i].L2_timer, alm_sysVars[i].L3_timer );
	}


}
//------------------------------------------------------------------------------------

