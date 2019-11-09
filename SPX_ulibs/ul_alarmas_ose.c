/*
 * ul_alarmas_ose.c
 *
 *  Created on: 5 nov. 2019
 *      Author: pablo
 */

#include "ul_alarmas_ose.h"


static t_alarm_states state_tkalarm;

bool alarm_init(void);
// Estados
void st_alarm_normal(void);
void st_alarm_alarmado(void);
void st_alarm_mantenimiento(void);
void st_alarm_standby(void);

void pv_disparar_alm_nivel1(void);
void pv_disparar_alm_nivel2(void);
void pv_disparar_alm_nivel3(void);

static void pv_alarm_leer_entradas(void);
static void pv_alarm_set_levels(void);
static void pv_alarm_timers_update(void);
static bool pv_alarm_fire(void);

//------------------------------------------------------------------------------------
void alarmas_stk(void)
{

	if ( !alarm_init() )
		return;

	state_tkalarm = st_alm_NORMAL;

	for (;;) {

		switch(state_tkalarm) {
		case st_alm_NORMAL:
			st_alarm_normal();
			break;
		case st_alm_ALARMADO:
			st_alarm_alarmado();
			break;
		case st_alm_STANDBY:
			st_alarm_standby();
			break;
		case st_alm_MANTENIMIENTO:
			st_alarm_mantenimiento();
			break;
		default:
			xprintf_P(PSTR("MODO OPERACION ERROR: Alarm state unknown (%d)\r\n\0"), state_tkalarm );
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
void st_alarm_normal(void)
{

bool alarm_fired = false;

// Entry:
	if ( systemVars.debug == DEBUG_ALARMAS ) {
		xprintf_P(PSTR("ALARMAS: Ingreso en modo Normal.\r\n\0"));
	}

// Loop:
	while ( state_tkalarm == st_alm_NORMAL ) {

		ctl_watchdog_kick( WDG_APP,  WDG_APP_TIMEOUT );
		// Espera de 1s
		vTaskDelay( ( TickType_t)( 1000 / portTICK_RATE_MS ) );

		// Leo los datos
		pv_alarm_leer_entradas();
		// Calculo los niveles de alarmas
		pv_alarm_set_levels();
		// Actualizo los timers de las alarmas
		pv_alarm_timers_update();
		// Veo si hay que disparar alguna alarma
		alarm_fired = pv_alarm_fire();

		// Si dispare una alarma, paso al estado de sistema alarmado
		if ( alarm_fired == true ) {
			state_tkalarm = st_alm_ALARMADO;
		}
	}

// Exit
	return;

}
//------------------------------------------------------------------------------------
void st_alarm_alarmado(void)
{
	// En este estado tengo al menos una alarma disparada.
	// Sigo monitoreando pero ahora controlo que presiones el botón de reset

}
//------------------------------------------------------------------------------------
void st_alarm_mantenimiento(void)
{
	// Este estado es en mantenimiento ( llave de mantenimiento en on)
	// Solo se polean los datos cada 30s y se guardan en la base de datos pero no
	// se generan alarmas.
	// Los datos se polean y guardan en la tarea spx_tkInputs y se mandan al server con
	// spx_tkComms por lo tanto no hacemos nada.
	// - No se activan las alarmas
	// - No se disparan las sirenas
	// - No se generan SMS
	// - Solo flashea la luz verde.
	// Cuando salgo solo voy al estado NORMAL


// Entry:
	if ( systemVars.debug == DEBUG_ALARMAS ) {
		xprintf_P(PSTR("ALARMAS: Ingreso en modo Mantenimiento.\r\n\0"));
	}
	// Al entrar en mantenimiento apago todas las posibles señales activas.
	// Solo debe flashear la luz verde.
	alarma_luz_verde( act_OFF );
	alarma_luz_roja( act_OFF );
	alarma_luz_amarilla( act_OFF );
	alarma_luz_naranja( act_OFF );
	alarma_sirena( act_OFF );

// Loop:
	while ( state_tkalarm == st_alm_MANTENIMIENTO ) {

		ctl_watchdog_kick( WDG_APP,  WDG_APP_TIMEOUT );
		// Espera de 1s
		vTaskDelay( ( TickType_t)( 1000 / portTICK_RATE_MS ) );
		// Flasheo luz verde
		alarma_luz_verde(act_FLASH);
		// Veo si apagaron la llave de mantenimiento.
		pv_alarm_leer_entradas();
		if ( alarmVars.llave_mantenimiento == false ) {
			// Apagaron la llave de mantenimiento. Salgo
			state_tkalarm = st_alm_NORMAL;
		}

	}

// Exit

	alarm_init();
	return;

}
//------------------------------------------------------------------------------------
void st_alarm_standby(void)
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


int16_t timer_en_standby;

// Entry:
	if ( systemVars.debug == DEBUG_ALARMAS ) {
		xprintf_P(PSTR("ALARMAS: Ingreso en modo Standby.\r\n\0"));
	}
	// Debo permancer 30 minutos.
	timer_en_standby = TIME_IN_STANDBY;

// Loop:
	while ( state_tkalarm == st_alm_STANDBY ) {

		ctl_watchdog_kick( WDG_APP,  WDG_APP_TIMEOUT );
		// Espera de 1s
		vTaskDelay( ( TickType_t)( 1000 / portTICK_RATE_MS ) );

		// Veo si activaron la llave de mantenimiento.
		pv_alarm_leer_entradas();
		if ( alarmVars.llave_mantenimiento == true ) {
			state_tkalarm = st_alm_MANTENIMIENTO;
		}

		// Veo si expiro el tiempo para salir y volver al estado NORMAL
		if ( timer_en_standby-- == 0 ) {
			state_tkalarm = st_alm_NORMAL;
		}

	}

// Exit
	alarm_init();

}
//------------------------------------------------------------------------------------
// FUNCIONES GENERALES
//------------------------------------------------------------------------------------
bool alarm_init(void)
{
uint8_t i;

	// Inicializa las salidas y las variables de trabajo.

	if ( spx_io_board != SPX_IO8CH ) {
		xprintf_P(PSTR("ALARMAS: Init ERROR: Control Alarmas only in IO_8CH.\r\n\0"));
		systemVars.aplicacion = APP_OFF;
		u_save_params_in_NVMEE();
		return(false);
	}

	alarma_luz_verde( act_OFF );
	alarma_luz_roja( act_OFF );
	alarma_luz_amarilla( act_OFF );
	alarma_luz_naranja( act_OFF );
	alarma_sirena( act_OFF );

	for (i=0; i<NRO_CANALES_ALARMA; i++) {
		l_alarm_input_channels[i].enabled = false;					// Todo deshabilitado
		l_alarm_input_channels[i].value = 0.0;						// Valores en 0
		l_alarm_input_channels[i].timer = -1;						// Timers apagados
		l_alarm_input_channels[i].nivel_alarma = ALARMA_NIVEL_0;	// Nivel normal
	}

	alarmVars.llave_mantenimiento = false;
	alarmVars.sensor_puerta_open = false;

	return(true);

}
//------------------------------------------------------------------------------------
static void pv_alarm_leer_entradas(void)
{
	// El poleo de las entradas lo hace la tarea tkInputs. Esta debe tener un timerpoll de 30s
	// Aqui c/1s leo las entradas en modo 'copy' ( sin poleo ).
	// Por eso no tengo que monitorear los 30s sino que lo hago c/1s. y esto me permite tener
	// permantente control de la tarea.
	// Leo las entradas analogicas y digitales y seteo las correspondientes variables locales de c/canal.
	// Seteo el valor de la llave de mantenimiento.

st_dataRecord_t dr;
uint8_t i;

	// Leo todas las entradas analogicas, digitales, contadores.
	// Es un DLG8CH
	// Leo en modo 'copy', sin poleo real.
	data_read_inputs(&dr, true );

	for (i=0; i<NRO_CANALES_ALARMA; i++) {
		// Canal habilitado si o no.
		if ( dr.df.io8.dinputs[i] == 0 ) {
			l_alarm_input_channels[i].enabled = true;
		} else {
			l_alarm_input_channels[i].enabled = false;
		}
		// Valor.
		l_alarm_input_channels[i].value = dr.df.io8.ainputs[i];

	}

	// Llave de mantenimiento
	if ( dr.df.io8.dinputs[IPIN_LLAVE_MANTENIMIENTO] == 0 ) {
		alarmVars.llave_mantenimiento = false;
	} else {
		alarmVars.llave_mantenimiento = true;
	}

	// Sensor de puerta
	if ( dr.df.io8.dinputs[IPIN_SENSOR_PUERTA] == 0 ) {
		alarmVars.sensor_puerta_open = false;
	} else {
		alarmVars.sensor_puerta_open = true;
	}

}
//------------------------------------------------------------------------------------
static void pv_alarm_set_levels(void)
{

uint8_t i;
float valor;
nivel_alarma_t nivel_alarma;

	// Para cada entrada habilitada, determino en que nivel de alarma esta

	for ( i = 0; i < NRO_CANALES_ALARMA; i++) {

		// Salteo los canales no habilitados
		if ( l_alarm_input_channels[i].enabled == false )
			continue;

		// Proceso solo los canales habilitados.
		// Vemos en que franja de niveles de alarma estamos para c/canal.
		valor = l_alarm_input_channels[i].value;
		nivel_alarma = ALARMA_NIVEL_0;
		if ( valor > systemVars.aplicacion_conf.alarmas[i].alarma3.lim_sup ) {
			nivel_alarma = ALARMA_NIVEL_3;
		} else if  ( valor > systemVars.aplicacion_conf.alarmas[i].alarma2.lim_sup ) {
			nivel_alarma = ALARMA_NIVEL_2;
		} else if  ( valor > systemVars.aplicacion_conf.alarmas[i].alarma1.lim_sup ) {
			nivel_alarma = ALARMA_NIVEL_1;
		} else if  ( valor < systemVars.aplicacion_conf.alarmas[i].alarma3.lim_inf ) {
			nivel_alarma = ALARMA_NIVEL_3;
		} else if  ( valor  < systemVars.aplicacion_conf.alarmas[i].alarma2.lim_inf ) {
			nivel_alarma = ALARMA_NIVEL_2;
		} else if  ( valor < systemVars.aplicacion_conf.alarmas[i].alarma3.lim_inf ) {
			nivel_alarma = ALARMA_NIVEL_1;
		}

		// Si el nivel de alarma del canal cambio, activo el timer correspondiente.

		// 1 - Si es normal, apago el timer
		if ( nivel_alarma == ALARMA_NIVEL_0 ) {
			l_alarm_input_channels[i].nivel_alarma = ALARMA_NIVEL_0;
			l_alarm_input_channels[i].timer = -1;
			continue;
		}

		// Si el nivel de alerta cambio, arranco el timer ( !!! OJO que no considera oscilaciones
		// alrrededor de una fronter !!!! )
		if ( l_alarm_input_channels[i].nivel_alarma != nivel_alarma ) {
			l_alarm_input_channels[i].nivel_alarma = nivel_alarma;
			switch(nivel_alarma) {
			case ALARMA_NIVEL_0:
				l_alarm_input_channels[i].timer = TIMEOUT_ALARMA_NIVEL_0;
				break;
			case ALARMA_NIVEL_1:
				l_alarm_input_channels[i].timer = TIMEOUT_ALARMA_NIVEL_1;
				break;
			case ALARMA_NIVEL_2:
				l_alarm_input_channels[i].timer = TIMEOUT_ALARMA_NIVEL_2;
				break;
			case ALARMA_NIVEL_3:
				l_alarm_input_channels[i].timer = TIMEOUT_ALARMA_NIVEL_3;
				break;

			}
		}
	}
}
//------------------------------------------------------------------------------------
static void pv_alarm_timers_update(void)
{
	// Se ejecuta cada 1s.
	// Si algun timer de algun canal esta activo, lo decremento hasta llegar a 0

uint8_t i;

	for (i=0; i<NRO_CANALES_ALARMA; i++) {

		if ( ( l_alarm_input_channels[i].enabled == true ) && ( l_alarm_input_channels[i].timer > 0 ) ) {
			l_alarm_input_channels[i].timer--;
		}
	}
}
//------------------------------------------------------------------------------------
static bool pv_alarm_fire(void)
{
	// Reviso los timers de los canales activos si llegaron a 0.
	// En este caso expiraron por lo tanto se debe dispara una alarma
	// por dicho canal.

	// Ver si las alarmas se dispara por canal o por nivel !!!!
	// Hay que mantener un timer por alarma y por canal o solo por alarma ???

uint8_t i;

	for (i=0; i<NRO_CANALES_ALARMA; i++) {

		if ( ( l_alarm_input_channels[i].enabled == true ) && ( l_alarm_input_channels[i].timer == 0 ) ) {
			l_alarm_input_channels[i].timer = -1 ;
		}
	}
	return(false);

}
//------------------------------------------------------------------------------------
void pv_disparar_alm_nivel1(void)
{

}
//------------------------------------------------------------------------------------
void pv_disparar_alm_nivel2(void)
{

}
//------------------------------------------------------------------------------------
void pv_disparar_alm_nivel3(void)
{

}
//------------------------------------------------------------------------------------
// ACCIONES
//------------------------------------------------------------------------------------
void alarma_luz_verde( t_dev_action action)
{

static uint8_t l_state = 0;

	switch(action) {
	case act_OFF:
		u_write_output_pins( OPIN_LUZ_VERDE, 0 );
		l_state = 0;
		break;
	case act_ON:
		u_write_output_pins( OPIN_LUZ_VERDE, 1 );
		l_state = 1;
		break;
	case act_FLASH:
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
void alarma_luz_roja( t_dev_action action)
{

static uint8_t l_state = 0;

	switch(action) {
	case act_OFF:
		u_write_output_pins( OPIN_LUZ_ROJA, 0 );
		l_state = 0;
		break;
	case act_ON:
		u_write_output_pins( OPIN_LUZ_ROJA, 1 );
		l_state = 1;
		break;
	case act_FLASH:
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
void alarma_luz_amarilla( t_dev_action action)
{

static uint8_t l_state = 0;

	switch(action) {
	case act_OFF:
		u_write_output_pins( OPIN_LUZ_AMARILLA, 0 );
		l_state = 0;
		break;
	case act_ON:
		u_write_output_pins( OPIN_LUZ_AMARILLA, 1 );
		l_state = 1;
		break;
	case act_FLASH:
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
void alarma_luz_naranja( t_dev_action action)
{

static uint8_t l_state = 0;

	switch(action) {
	case act_OFF:
		u_write_output_pins( OPIN_LUZ_NARANJA, 0 );
		l_state = 0;
		break;
	case act_ON:
		u_write_output_pins( OPIN_LUZ_NARANJA, 1 );
		l_state = 1;
		break;
	case act_FLASH:
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
void alarma_sirena(t_dev_action action)
{

	switch(action) {
	case act_OFF:
		u_write_output_pins( OPIN_SIRENA, 0 );
		break;
	case act_ON:
		u_write_output_pins( OPIN_SIRENA, 1 );
		break;
	case act_FLASH:
		break;
	default:
		break;
	}
}
//------------------------------------------------------------------------------------
// GENERAL
//------------------------------------------------------------------------------------
uint8_t alarmas_checksum(void)
{

uint8_t checksum = 0;
char dst[32];
char *p;
uint8_t i = 0;

	// calculate own checksum
	// Vacio el buffer temoral
	memset(dst,'\0', sizeof(dst));

	i = snprintf_P( &dst[i], sizeof(dst), PSTR("ALARMAS,"));
	i += snprintf_P(&dst[i], sizeof(dst), PSTR("%02d%02d,"), systemVars.aplicacion_conf.consigna.hhmm_c_diurna.hour, systemVars.aplicacion_conf.consigna.hhmm_c_diurna.min );
	i += snprintf_P(&dst[i], sizeof(dst), PSTR("%02d%02d"), systemVars.aplicacion_conf.consigna.hhmm_c_nocturna.hour, systemVars.aplicacion_conf.consigna.hhmm_c_nocturna.min );

	//xprintf_P( PSTR("DEBUG: CONS = [%s]\r\n\0"), dst );
	// Apunto al comienzo para recorrer el buffer
	p = dst;
	// Mientras no sea NULL calculo el checksum deol buffer
	while (*p != '\0') {
		checksum += *p++;
	}
	//xprintf_P( PSTR("DEBUG: cks = [0x%02x]\r\n\0"), checksum );

	return(checksum);
}
//------------------------------------------------------------------------------------
bool alarma_config( char *canal_str,char *alarm_str, char *limit_str, char *value_str )
{
uint8_t ch;
nivel_alarma_t nivel_alarma;
float valor;
uint8_t limite;

	// Chequeo integridad
	if ( canal_str == NULL ) {
		return(false);
	}

	if ( alarm_str == NULL ) {
		return(false);
	}

	if ( limit_str == NULL ) {
		return(false);
	}

	if ( value_str == NULL ) {
		return(false);
	}

	// Converto a valores utiles
	ch = atoi(canal_str);

	if (!strcmp_P( strupr(alarm_str), PSTR("ALARMA1\0")) ) {
		nivel_alarma = ALARMA_NIVEL_1;
	} else 	if (!strcmp_P( strupr(alarm_str), PSTR("ALARMA2\0")) ) {
		nivel_alarma = ALARMA_NIVEL_2;
	} else 	if (!strcmp_P( strupr(alarm_str), PSTR("ALARMA3\0")) ) {
		nivel_alarma = ALARMA_NIVEL_3;
	} else {
		return(false);
	}

	if (!strcmp_P( strupr(limit_str), PSTR("INF\0")) ) {
		limite = 0;
	} else 	if (!strcmp_P( strupr(limit_str), PSTR("SUP\0")) ) {
		limite = 1;
	} else {
		return(false);
	}

	valor = atof(value_str);

	// Configuro
	switch ( nivel_alarma ) {
	case ALARMA_NIVEL_0:
		break;
	case ALARMA_NIVEL_1:
		if ( limite == 0 ) {
			systemVars.aplicacion_conf.alarmas[ch].alarma1.lim_inf = valor;
		} else 	if ( limite == 1 ) {
			systemVars.aplicacion_conf.alarmas[ch].alarma1.lim_sup = valor;
		}
		break;
	case ALARMA_NIVEL_2:
		if ( limite == 0 ) {
			systemVars.aplicacion_conf.alarmas[ch].alarma2.lim_inf = valor;
		} else 	if ( limite == 1 ) {
			systemVars.aplicacion_conf.alarmas[ch].alarma2.lim_sup = valor;
		}
		break;
	case ALARMA_NIVEL_3:
		if ( limite == 0 ) {
			systemVars.aplicacion_conf.alarmas[ch].alarma3.lim_inf = valor;
		} else 	if ( limite == 1 ) {
			systemVars.aplicacion_conf.alarmas[ch].alarma3.lim_sup = valor;
		}
		break;
	}

	return(true);

}
//------------------------------------------------------------------------------------
void alarma_config_defaults(void)
{

uint8_t i;

	for ( i = 0; i < NRO_CANALES_ALARMA; i++) {
		systemVars.aplicacion_conf.alarmas[i].alarma1.lim_inf = 1.0;
		systemVars.aplicacion_conf.alarmas[i].alarma1.lim_sup = 1.1;
		systemVars.aplicacion_conf.alarmas[i].alarma2.lim_inf = 2.0;
		systemVars.aplicacion_conf.alarmas[i].alarma2.lim_sup = 2.2;
		systemVars.aplicacion_conf.alarmas[i].alarma3.lim_inf = 3.0;
		systemVars.aplicacion_conf.alarmas[i].alarma3.lim_sup = 3.3;
	}
}
//------------------------------------------------------------------------------------
uint8_t alarma_read_status_sensor_puerta(void)
{
	if ( alarmVars.sensor_puerta_open == true ) {
		return(1);
	} else {
		return(0);
	}
}
//------------------------------------------------------------------------------------
uint8_t alarma_read_status_mantenimiento(void)
{
	if ( alarmVars.llave_mantenimiento == true ) {
		return(1);
	} else {
		return(0);
	}
}
//------------------------------------------------------------------------------------
void alarma_servicio_tecnico( char * action, char * device )
{
	// action: (prender/apagar)
	// device: (lroja,lverde,lamarilla,lnaranja, sirena)

	if ( !strcmp_P( strupr(device), PSTR("LROJA\0"))) {
		if ( !strcmp_P( strupr(action), PSTR("PRENDER\0"))) {
			alarma_luz_roja(act_ON);
		} else 	if ( !strcmp_P( strupr(action), PSTR("APAGAR\0"))) {
			alarma_luz_roja(act_OFF);
		}
		return;
	}

	if ( !strcmp_P( strupr(device), PSTR("LVERDE\0"))) {
		if ( !strcmp_P( strupr(action), PSTR("PRENDER\0"))) {
			alarma_luz_verde(act_ON);
		} else 	if ( !strcmp_P( strupr(action), PSTR("APAGAR\0"))) {
			alarma_luz_verde(act_OFF);
		}
		return;
	}

	if ( !strcmp_P( strupr(device), PSTR("LAMARILLA\0"))) {
		if ( !strcmp_P( strupr(action), PSTR("PRENDER\0"))) {
			alarma_luz_amarilla(act_ON);
		} else 	if ( !strcmp_P( strupr(action), PSTR("APAGAR\0"))) {
			alarma_luz_amarilla(act_OFF);
		}
		return;
	}

	if ( !strcmp_P( strupr(device), PSTR("LNARANJA\0"))) {
		if ( !strcmp_P( strupr(action), PSTR("PRENDER\0"))) {
			alarma_luz_naranja(act_ON);
		} else 	if ( !strcmp_P( strupr(action), PSTR("APAGAR\0"))) {
			alarma_luz_naranja(act_OFF);
		}
		return;
	}

	if ( !strcmp_P( strupr(device), PSTR("SIRENA\0"))) {
		if ( !strcmp_P( strupr(action), PSTR("PRENDER\0"))) {
			alarma_sirena(act_ON);
		} else 	if ( !strcmp_P( strupr(action), PSTR("APAGAR\0"))) {
			alarma_sirena(act_OFF);
		}
		return;
	}
}
//------------------------------------------------------------------------------------

