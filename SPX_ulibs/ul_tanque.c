/*
 * ul_tanque.c
 *
 *  Created on: 13 nov. 2019
 *      Author: pablo
 */


#include "spx.h"
#include "gprs.h"

typedef enum { ST_TQ_NORMAL = 0, ST_TQ_ALARMA_L, ST_TQ_ALARMA_H } t_tanque_states;

t_tanque_states  tq_state;

bool tq_sms_enabled;
uint16_t perf_links_status;


void st_tq_normal(void);
void st_tq_alarmaL(void);
void st_tq_alarmaH(void);
float pv_tq_get_level(void);
void pv_fire_tq_alarm( uint8_t alarm_type );

//------------------------------------------------------------------------------------
void tanque_stk(void)
{

	tq_state = ST_TQ_NORMAL;
	// Flag que indica que aplico o no el envio de sms.
	tq_sms_enabled = false;

	for (;;) {

		switch(tq_state) {
		case ST_TQ_NORMAL:
			st_tq_normal();
			break;
		case ST_TQ_ALARMA_L:
			st_tq_alarmaL();
			break;
		case ST_TQ_ALARMA_H:
			st_tq_alarmaH();
			break;
		default:
			xprintf_P(PSTR("MODO OPERACION ERROR: Tanque state unknown (%d)\r\n\0"), tq_state );
			systemVars.aplicacion = APP_OFF;
			u_save_params_in_NVMEE();
			return;
			break;
		}
	}
}
//------------------------------------------------------------------------------------
void st_tq_normal(void)
{

float nivel_tanque;

// Entry:
	if ( systemVars.debug == DEBUG_APLICACION ) {
		xprintf_P(PSTR("APP_TANQUE: Ingreso en modo Normal.\r\n\0"));
	}

// Loop:
	while ( tq_state == ST_TQ_NORMAL ) {

		ctl_watchdog_kick( WDG_APP,  WDG_APP_TIMEOUT );
		// Espera de 1s
		vTaskDelay( ( TickType_t)( 1000 / portTICK_RATE_MS ) );

		nivel_tanque = pv_tq_get_level();

		// Veo si disparo por alarma_H
		if ( nivel_tanque > systemVars.aplicacion_conf.tanque.high_level + 0.05 ) {
			// Condicion de disparo
			tq_state = ST_TQ_ALARMA_H;
		}

		// Veo si disparo por alarma_L
		if ( nivel_tanque < systemVars.aplicacion_conf.tanque.low_level - 0.05 ) {
			// Condicion de disparo
			tq_state = ST_TQ_ALARMA_L;
		}

	}

// Exit
	return;

}
//------------------------------------------------------------------------------------
void st_tq_alarmaL(void)
{

float nivel_tanque;

// Entry:
	if ( systemVars.debug == DEBUG_APLICACION ) {
		xprintf_P(PSTR("APP_TANQUE: Ingreso en modo Alarma_L.\r\n\0"));
	}

	// Acciones por disparo de alarma_L
	pv_fire_tq_alarm(ST_TQ_ALARMA_L);

// Loop:
	while ( tq_state == ST_TQ_ALARMA_L ) {

		ctl_watchdog_kick( WDG_APP,  WDG_APP_TIMEOUT );
		// Espera de 1s
		vTaskDelay( ( TickType_t)( 1000 / portTICK_RATE_MS ) );

		nivel_tanque = pv_tq_get_level();

		// Veo si retorno al estado normal
		if ( nivel_tanque < systemVars.aplicacion_conf.tanque.high_level - 0.05 ) {
			// Condicion de disparo
			tq_state = ST_TQ_NORMAL;
		}

	}

// Exit
	return;

}
//------------------------------------------------------------------------------------
void st_tq_alarmaH(void)
{

float nivel_tanque;

// Entry:
	if ( systemVars.debug == DEBUG_APLICACION ) {
		xprintf_P(PSTR("APP_TANQUE: Ingreso en modo Alarma_H.\r\n\0"));
	}

	// Acciones por disparo de alarma_H
	pv_fire_tq_alarm(ST_TQ_ALARMA_H);

// Loop:
	while ( tq_state == ST_TQ_ALARMA_H ) {

		ctl_watchdog_kick( WDG_APP,  WDG_APP_TIMEOUT );
		// Espera de 1s
		vTaskDelay( ( TickType_t)( 1000 / portTICK_RATE_MS ) );

		nivel_tanque = pv_tq_get_level();

		// Veo si retorno al estado normal
		if ( nivel_tanque > systemVars.aplicacion_conf.tanque.low_level + 0.05 ) {
			// Condicion de disparo
			tq_state = ST_TQ_NORMAL;
		}
	}

// Exit
	return;

}
//------------------------------------------------------------------------------------
float pv_tq_get_level(void)
{
	// Lee el nivel del tanque

st_dataRecord_t dr;
float nivel_tanque;

	// Leo en modo 'copy', sin poleo real.
	data_read_inputs(&dr, true );

	nivel_tanque = dr.df.io8.ainputs[0];

	return(nivel_tanque);
}
//------------------------------------------------------------------------------------
bool tanque_config ( char *param1, char *param2, char *param3 )
{
	// Podemos configurar los niveles o los sms.

uint8_t sms_id;

	//Nivel bajo
	if (!strcmp_P( strupr(param1), PSTR("NIVELB")) ) {
		systemVars.aplicacion_conf.tanque.low_level = atof(param2);
		return(true);
	}

	// Nivel alto
	if (!strcmp_P( strupr(param1), PSTR("NIVELA")) ) {
		systemVars.aplicacion_conf.tanque.high_level = atof(param2);
		return(true);
	}

	// SMS
	if (!strcmp_P( strupr(param1), PSTR("SMS")) ) {
		sms_id = atoi(param2);
		if ( sms_id < NRO_PERFXTANQUE ) {
			memcpy( systemVars.aplicacion_conf.tanque.sms_perforaciones[sms_id], param3, SMS_NRO_LENGTH );
			systemVars.aplicacion_conf.tanque.sms_perforaciones[sms_id][SMS_NRO_LENGTH - 1] = '\0';
			return(true);
		}
	}

	return(false);

}
//------------------------------------------------------------------------------------
void tanque_config_defaults(void)
{
uint8_t i;

	systemVars.aplicacion_conf.tanque.high_level = 1.0;
	systemVars.aplicacion_conf.tanque.low_level = 0.2;
	for ( i = 0; i < NRO_PERFXTANQUE; i++ ) {
		memset( systemVars.aplicacion_conf.tanque.sms_perforaciones[i], '\0', SMS_NRO_LENGTH );
	}

}
//------------------------------------------------------------------------------------
uint8_t tanque_checksum(void)
{

uint8_t checksum = 0;
char dst[32];
char *p;
uint8_t i = 0;

	// calculate own checksum
	// Vacio el buffer temoral
	memset(dst,'\0', sizeof(dst));

	return(checksum);

}
//------------------------------------------------------------------------------------
void pv_fire_tq_alarm( uint8_t alarm_type )
{
	// Disparo de alarmas.
	// Si no tengo enlace debo mandar GPRS a todos las perforaciones remotas.
	// Si algun enlace de perforacion esta caido, le mando a esa perforacion un sms.
	// Todo esto siempre que la flag de sms_enabled este activa.

uint16_t link_mask;
uint8_t i;
uint16_t mask;
char msg[15];

	// No tengo enlace GPRS.
	if ( GPRS_stateVars.state < G_DATA ) {
		link_mask = 0x00;
	} else {
		link_mask = perf_links_status;
	}

	for ( i = 0; i < NRO_PERFXTANQUE; i++ ) {
		mask = 1 << i;
		if ( ( (link_mask & mask) != 0 ) && ( systemVars.aplicacion_conf.tanque.sms_perforaciones[i] != NULL ) ) {
			// El bit del link esta prendido. Link caido. Mando SMS
			switch(alarm_type) {
			case ST_TQ_ALARMA_L:
				strcpy(msg,"PERF_OUTS:15");
				break;
			case ST_TQ_ALARMA_H:
				strcpy(msg,"PERF_OUTS:5");
				break;
			}
			u_sms_send( systemVars.aplicacion_conf.tanque.sms_perforaciones[i], msg );
		}
	}
}
//------------------------------------------------------------------------------------
void tanque_set_params_from_gprs( char *tk_sms, char *tk_link )
{
	// Recibo de una respuesta del server si habilito o no los SMS y el estado
	// de todos los enlaces de las perforaciones.


	if ( systemVars.debug == DEBUG_APLICACION ) {
		xprintf_P(PSTR("APP_TANQUE: SEt param GPRS: [%s][%s]\r\n\0"), tk_sms, tk_link );
	}

	if ( atoi(tk_sms) == 0 ) {
		tq_sms_enabled = false;
	} else if ( atoi(tk_sms) == 1 ) {
		tq_sms_enabled = true;
	}

	perf_links_status = atoi(tk_link);

}
//------------------------------------------------------------------------------------
bool tanque_read_sms_enable_flag(void)
{
	// La utiliza el modo cmd para ver el estado de esta flag.
	return(tq_sms_enabled);
}
//------------------------------------------------------------------------------------
uint16_t tanque_perf_link_status(void)
{
	return(perf_links_status);
}
//------------------------------------------------------------------------------------

