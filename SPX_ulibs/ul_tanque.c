/*
 * ul_tanque.c
 *
 *  Created on: 13 nov. 2019
 *      Author: pablo
 */

/* PENDIENTE:
 * Recepcion del ACK: Hay que ver de que numero estan mandando el sms
 * Antes de enviar un SMS verificar que halla lugar en la cola
 * Tamanios de cola de SMS
 * Testing:
 * operacion
 * configuracion local
 * configuracion remota
 * condiciones de alarma:
 * envio_sms
 * 	no ack
 * 	si ack
 */

#include <comms.h>
#include "spx.h"

typedef enum { ST_TQ_NORMAL = 0, ST_TQ_LOWLEVEL, ST_TQ_HIGHLEVEL } t_tanque_states;
typedef enum { TQ_NO_LINK = 1, TQ_LINK_DOWN = 0, TQ_LINK_UP } t_tanques_link_status;

#define MAX_NRO_REM_PERF			SMS_NRO_LENGTH
#define MAX_NRO_REINTENTOS_SMS_TQ	3
#define LOCAL_LINK_TIMEOUT			180

t_tanque_states  tq_state;

/* Estructura que tiene el estado de los enlaces a las perforaciones
 * y la cantidad de veces que mandamos un SMS.
 * Si el enlace esta caido debemos mandar un SMS reintentando hasta 3 veces.
 * Cuando se genera una condicion de alarma, si el enlace esta down ponemos un
 * 3 en sms_tries.
 * Cada 1 minuto intentamos enviarlo
 * Cuando recibimos un SMS-ACK, ponemos el contador en 0.
 */
typedef struct {
	t_tanques_link_status link_status;
	uint8_t reintentos_sms;
} t_perforacion_remota;

t_perforacion_remota lista_de_perforaciones[MAX_NRO_REM_PERF];

void st_tq_normal(void);
void st_tq_lowlevel(void);
void st_tq_highlevel(void);

float pv_tq_get_level(void);
void pv_tq_check_links(void);
void pv_tq_check_sms(void);
void pv_tq_fire_sms(void);
void pv_tq_send_sms(uint8_t perf_id );

float nivel_tanque;
#define NIVEL_TANQUE_HIGH() ( nivel_tanque > ( systemVars.aplicacion_conf.tanque.high_level + 0.05) )
#define NIVEL_TANQUE_LOW()  ( nivel_tanque < ( systemVars.aplicacion_conf.tanque.low_level - 0.05 ) )
#define NIVEL_TANQUE_NORMAL()  (  ! NIVEL_TANQUE_HIGH() && ! NIVEL_TANQUE_LOW() )

uint16_t server_response_timeout;

//------------------------------------------------------------------------------------
void tanque_stk(void)
{

uint8_t i;

	tq_state = ST_TQ_NORMAL;
	server_response_timeout = LOCAL_LINK_TIMEOUT;
	for ( i=0; i<MAX_NRO_REM_PERF; i++ ) {
		lista_de_perforaciones[i].link_status = TQ_NO_LINK;
	}

	for (;;) {

		switch(tq_state) {
		case ST_TQ_NORMAL:
			st_tq_normal();
			break;
		case ST_TQ_LOWLEVEL:
			st_tq_lowlevel();
			break;
		case ST_TQ_HIGHLEVEL:
			st_tq_highlevel();
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

// Entry:
	if ( systemVars.debug == DEBUG_APLICACION ) {
		xprintf_P(PSTR("APP_TANQUE: Ingreso en modo Normal.\r\n\0"));
	}

// Loop:
	while ( tq_state == ST_TQ_NORMAL ) {

		ctl_watchdog_kick( WDG_APP,  WDG_APP_TIMEOUT );
		// Espera de 1s
		vTaskDelay( ( TickType_t)( 1000 / portTICK_RATE_MS ) );

		pv_tq_check_links();
		pv_tq_check_sms();

		nivel_tanque = pv_tq_get_level();

		// Veo si disparo por alarma_H
		if ( NIVEL_TANQUE_HIGH() ) {
			// Condicion de disparo
			tq_state = ST_TQ_HIGHLEVEL;
			break;
		}

		// Veo si disparo por alarma_L
		if ( NIVEL_TANQUE_LOW() ) {
			// Condicion de disparo
			tq_state = ST_TQ_LOWLEVEL;
			break;
		}

	}

// Exit
	return;

}
//------------------------------------------------------------------------------------
void st_tq_lowlevel(void)
{

float nivel_tanque;

// Entry:
	if ( systemVars.debug == DEBUG_APLICACION ) {
		xprintf_P(PSTR("APP_TANQUE: Ingreso en modo LOW_LEVEL.\r\n\0"));
	}

	// Acciones por disparo de LOW_LEVEL
	pv_tq_fire_sms();

	// Loop:
	while ( tq_state == ST_TQ_LOWLEVEL ) {

		ctl_watchdog_kick( WDG_APP,  WDG_APP_TIMEOUT );
		// Espera de 1s
		vTaskDelay( ( TickType_t)( 1000 / portTICK_RATE_MS ) );

		pv_tq_check_links();
		pv_tq_check_sms();

		nivel_tanque = pv_tq_get_level();

		if ( NIVEL_TANQUE_HIGH() ) {
			// Condicion de disparo
			tq_state = ST_TQ_HIGHLEVEL;
			break;
		}

		if ( NIVEL_TANQUE_NORMAL() ) {
			// Condicion de disparo
			tq_state = ST_TQ_NORMAL;
			break;
		}
	}

// Exit
	return;

}
//------------------------------------------------------------------------------------
void st_tq_highlevel(void)
{

float nivel_tanque;

// Entry:
	if ( systemVars.debug == DEBUG_APLICACION ) {
		xprintf_P(PSTR("APP_TANQUE: Ingreso en modo HIGH_LEVEL.\r\n\0"));
	}

	// Acciones por disparo de HIGH_LEVEL
	pv_tq_fire_sms();

	// Loop:
	while ( tq_state == ST_TQ_HIGHLEVEL ) {

		ctl_watchdog_kick( WDG_APP,  WDG_APP_TIMEOUT );
		// Espera de 1s
		vTaskDelay( ( TickType_t)( 1000 / portTICK_RATE_MS ) );

		pv_tq_check_links();
		pv_tq_check_sms();

		nivel_tanque = pv_tq_get_level();

		if ( NIVEL_TANQUE_NORMAL() ) {
			// Condicion de disparo
			tq_state = ST_TQ_NORMAL;
			break;
		}

		if ( NIVEL_TANQUE_LOW() ) {
			// Condicion de disparo
			tq_state = ST_TQ_LOWLEVEL;
			break;
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
void pv_tq_check_links(void)
{
	// Los links se controlan por 2 caminos:
	// Uno es que el server envia el status de c/u.
	// Otro es local y monitorea que el servidor envie una respuesta en menos de 3 minutos.
	// Esta funcion debe controlar el tiempo entre las respuestas del server.
	// Si pasa mas de 3 minutos sin respuesta, marca todos los enlaces caidos
	// Es invocada una vez por segundo.
	// Existe una variable global ( server_response_timeout ) que cada vez que se recibe
	// una respuesta del server, se resetea a 3 minutos.
	// Aqui la vamos decrementando.
	// Si llego a 0 es que hace 3 minutos que no hay respuesta y entonces ponemos
	// todos los enlaces en DOWN

uint8_t i;

	if ( server_response_timeout > 0 ) {
		server_response_timeout--;
		return;
	}

	// Aqui es que server_response_timeout = 0.
	// Marco todos los enlaces como caidos.
	for ( i = 0; i < MAX_NRO_REM_PERF; i++ ) {
		lista_de_perforaciones[i].link_status = TQ_LINK_DOWN;
	}

}
//------------------------------------------------------------------------------------
void pv_tq_fire_sms(void)
{
	// Esta funcion inicializa la variable reintentos de todos las perforaciones
	// que esten caidas.
	// Luego la función pv_t_check_sms los envia.

uint8_t i;

	for ( i = 0; i < MAX_NRO_REM_PERF; i++ ) {
		if ( ( lista_de_perforaciones[i].link_status == TQ_LINK_DOWN ) &&
				( systemVars.aplicacion_conf.tanque.sms_enabled == true ) ) {
			lista_de_perforaciones[i].reintentos_sms = MAX_NRO_REINTENTOS_SMS_TQ;
		}
	}
}
//------------------------------------------------------------------------------------
void pv_tq_check_sms(void)
{
	// Una vez por minuto se fija si hay SMS para mandar.
	// Si los hay los manda y decrementa el contador de reintentos.
	// Esta funcion se ejecuta 1 vez por seg.

static uint8_t timeout = 60;
uint8_t i;

	// Cuento hasta 1 minuto
	if ( timeout > 0 ) {
		timeout--;
		return;
	}

	// Reviso los SMS
	for ( i = 0; i < MAX_NRO_REM_PERF; i++ ) {
		if ( lista_de_perforaciones[i].reintentos_sms > 0 ) {
			lista_de_perforaciones[i].reintentos_sms--;
			pv_tq_send_sms(i);
		}
	}
}
//------------------------------------------------------------------------------------
void pv_tq_send_sms(uint8_t perf_id )
{
	// Envia un sms al nro de la perforacion perf_id.
	// De acuerdo al tq_state ( LOW, HIGH ) es el texto del mensaje.
	// DEBEMOS VER COMO CHEQUAR QUE LA LISTA DE SMS PENDIENTES NO ESTE LLENA
	// La lista de SMSs esta en systemVars.aplicacion_conf.l_sms

char *sms_nro;
char msg[15];

	sms_nro = systemVars.aplicacion_conf.l_sms[perf_id];
	switch ( tq_state ) {
	case ST_TQ_LOWLEVEL:
		strcpy(msg,"PERF_OUTS:15");		// Mando prender la bomba
		break;
	case ST_TQ_HIGHLEVEL:
		strcpy(msg,"PERF_OUTS:5");		// Mando apagar la bomba
		break;
	case ST_TQ_NORMAL:
		return;
		break;
	}

	// Envio el SMS
	u_sms_send( sms_nro, msg);

}
//------------------------------------------------------------------------------------
// FUNCIONES DE CONFIGURACION
//------------------------------------------------------------------------------------
bool tanque_config ( char *param1, char *param2, char *param3 )
{
	// Podemos configurar los niveles o los sms.
	// sms {id} nro
	// nivel {BAJO|ALTO} valor

uint8_t sms_id;

	if (!strcmp_P( strupr(param1), PSTR("nivel")) ) {

		//Nivel bajo
		if (!strcmp_P( strupr(param2), PSTR("BAJO")) ) {
			systemVars.aplicacion_conf.tanque.low_level = atof(param3);
			return(true);
		}

		// Nivel alto
		if (!strcmp_P( strupr(param2), PSTR("ALTO")) ) {
			systemVars.aplicacion_conf.tanque.high_level = atof(param3);
			return(true);
		}

		return(false);
	}

	// SMS
	if (!strcmp_P( strupr(param1), PSTR("SMS")) ) {
		sms_id = atoi(param2);
		if ( sms_id < NRO_PERFXTANQUE ) {
			memcpy( systemVars.aplicacion_conf.l_sms[sms_id], param3, SMS_NRO_LENGTH );
			systemVars.aplicacion_conf.l_sms[sms_id][SMS_NRO_LENGTH - 1] = '\0';
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
	systemVars.aplicacion_conf.tanque.sms_enabled = true;
	for ( i = 0; i < MAX_NRO_SMS ; i++ ) {
		memset( systemVars.aplicacion_conf.l_sms[i], '\0', SMS_NRO_LENGTH );
	}

}
//------------------------------------------------------------------------------------
uint8_t tanque_checksum(void)
{

uint8_t checksum = 0;
char dst[48];
char *p;
uint8_t i;
uint8_t j = 0;

	// Niveles de altura
	memset(dst,'\0', sizeof(dst));
	j = 0;
	j += snprintf_P( &dst[j], sizeof(dst), PSTR("LEVELS:%.02f,%.02f;"),systemVars.aplicacion_conf.tanque.low_level, systemVars.aplicacion_conf.tanque.high_level );
	if ( systemVars.aplicacion_conf.tanque.sms_enabled == true ) {
		j += snprintf_P( &dst[j], sizeof(dst), PSTR("SMSEN:1;"));
	} else {
		j += snprintf_P( &dst[j], sizeof(dst), PSTR("SMSEN:0;"));
	}
	// Apunto al comienzo para recorrer el buffer
	p = dst;
	// Mientras no sea NULL calculo el checksum deol buffer
	while (*p != '\0') {
		checksum += *p++;
	}
	//xprintf_P( PSTR("DEBUG: PPOT_CKS = [%s]\r\n\0"), dst );
	//xprintf_P( PSTR("DEBUG: PPOT_CKS cks = [0x%02x]\r\n\0"), checksum );


	// NROS.DE SMS
	for (i=0; i < MAX_NRO_SMS;i++) {
		// Vacio el buffer temoral
		memset(dst,'\0', sizeof(dst));
		// Copio sobe el buffer una vista ascii ( imprimible ) de c/registro.
		snprintf_P( dst, sizeof(dst), PSTR("SMS%02d:%s;"), i, systemVars.aplicacion_conf.l_sms[i] );
		// Apunto al comienzo para recorrer el buffer
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
void tanque_set_params_from_gprs( char *tk_sms, char *tk_link )
{
	// Recibo de una respuesta del server (modo DATA) si habilito o no los SMS y el estado
	// de todos los enlaces de las perforaciones.

char *ch;
uint8_t link_id;

	if ( systemVars.debug == DEBUG_APLICACION ) {
		xprintf_P(PSTR("APP_TANQUE: Set param GPRS: [%s][%s]\r\n\0"), tk_sms, tk_link );
	}

	// Reinicio el timer de respuestas del servidor.
	server_response_timeout = LOCAL_LINK_TIMEOUT;

	// SMS's.
	if ( atoi(tk_sms) == 0 ) {
		systemVars.aplicacion_conf.tanque.sms_enabled = false;
	} else if ( atoi(tk_sms) == 1 ) {
		systemVars.aplicacion_conf.tanque.sms_enabled = true;
	}

	// Estados de los enlaces.
	// El string tk_link lo convierto a entero (110110010). Este trae un patron de 16 bits.
	// Veo c/bit que esta en 0 y corresponde a un link caido.
	ch = tk_link;
	link_id = 0;
	while ( ch != NULL ) {
		if ( atoi(ch) == 0 ) {
			lista_de_perforaciones[link_id].link_status = TQ_LINK_DOWN;
		} else {
			lista_de_perforaciones[link_id].link_status = TQ_LINK_UP;
		}
		ch++;
		link_id++;
		if ( link_id >= MAX_NRO_REM_PERF )
			return;
	}

}
//------------------------------------------------------------------------------------
void tanque_process_rxsms(char *sms_msg)
{
	// Cuando mando un SMS a una perforacion, espero un ACK para confirmar que lo recibio.
	// En este caso, pongo el contador de reintentos en 0.
	// Debo determinar de que perforacion llego. Lo hago con el numero origen.


}
//------------------------------------------------------------------------------------
void tanque_reconfigure_app(void)
{
	// TYPE=INIT&PLOAD=CLASS:APP;AP0:TANQUE

	systemVars.aplicacion = APP_TANQUE;
	u_save_params_in_NVMEE();
	//f_reset = true;

	if ( systemVars.debug == DEBUG_GPRS ) {
		xprintf_P( PSTR("GPRS: Reconfig APLICACION:TANQUE\r\n\0"));
	}

}
//------------------------------------------------------------------------------------
void tanque_process_gprs_response( const char *gprsbuff )
{
	// Recibi algo del estilo TQS:0,110110010
	// Es la respuesta del server a un frame de datos de un tanque.
	// El primer dato puede ser 0 o 1.
	// Si es 0 hay que deshabilitar el SMS
	// Si es 1 hay que habilitarlo
	// El segundo dato es un numero que indica el estado de los enlaces
	// de las perforaciones. 0 indica caido, 1 indica activo.
	//
	// Extraigo el valor de las salidas y las seteo.

char localStr[32] = { 0 };
char *stringp = NULL;
char *tk_sms = NULL;
char *tk_link = NULL;
char *delim = ",=:><";
char *p = NULL;

	//p = strstr( (const char *)&commsRxBuffer.buffer, "TQS");
	p = strstr( gprsbuff, "TQS");
	if ( p == NULL ) {
		return;
	}

	// Copio el mensaje enviado a un buffer local porque la funcion strsep lo modifica.
	memset(localStr,'\0',32);
	memcpy(localStr,p,sizeof(localStr));

	stringp = localStr;
	tk_sms = strsep(&stringp,delim);	// TQS
	tk_sms = strsep(&stringp,delim);	// Str. con el valor 0,1 del enable del sms
	tk_link = strsep(&stringp,delim);	// Str. con el valor del estado de los links de las perforaciones

	// Actualizo el status a travez de una funcion propia del modulo de outputs
	tanque_set_params_from_gprs( tk_sms, tk_link );

	if ( systemVars.debug == DEBUG_GPRS ) {
		xprintf_P( PSTR("GPRS: TQS\r\n\0"));
	}

}
//------------------------------------------------------------------------------------
void tanque_reconfigure_levels_by_gprsinit(const char *gprsbuff)
{
	// TANQUE:
	// TYPE=INIT&PLOAD=CLASS:APP_B;LOW:0.2;HIGH:1.34

char *p = NULL;
char localStr[32] = { 0 };
char *stringp = NULL;
char *tk_low_level = NULL;
char *tk_high_level = NULL;
char *delim = ",=:;><";


	memset( &localStr, '\0', sizeof(localStr) );
	//p = strstr( (const char *)&commsRxBuffer.buffer, "LOW");
	p = strstr( gprsbuff, "LOW");
	memcpy(localStr,p,sizeof(localStr));

	stringp = localStr;
	tk_low_level = strsep(&stringp,delim);	// LOW
	tk_low_level = strsep(&stringp,delim);	// 0.2
	tk_high_level = strsep(&stringp,delim); // HIGH
	tk_high_level = strsep(&stringp,delim); // 1.34
	tanque_config("NIVEL","BAJO", tk_low_level);
	tanque_config("NIVEL","ALTO", tk_high_level);

	if ( systemVars.debug == DEBUG_GPRS ) {
		xprintf_P( PSTR("GPRS: Reconfig TANQUE. Niveles (low=%s,high=%s)\r\n\0"),tk_low_level, tk_high_level);
	}

	u_save_params_in_NVMEE();

}
//------------------------------------------------------------------------------------
void tanque_reconfigure_sms_by_gprsinit(const char *gprsbuff)
{
	// TANQUE SMS
	// TYPE=INIT&PLOAD=CLASS:APP_C;SMS01:111111;SMS02:2222222;SMS03:3333333;SMS04:4444444;...SMS09:9999999

char *p = NULL;
char localStr[32] = { 0 };
char *stringp = NULL;
char *tk_nro= NULL;
char *delim = ",=:;><";
uint8_t i;
char id[2];
char str_base[8];

	// SMS?
	for (i=0; i < MAX_NRO_SMS; i++ ) {
		memset( &str_base, '\0', sizeof(str_base) );
		snprintf_P( str_base, sizeof(str_base), PSTR("SMS0%d\0"), i );
		//xprintf_P( PSTR("DEBUG str_base: %s\r\n\0"), str_base);
		//p = strstr( (const char *)&commsRxBuffer.buffer, str_base);
		p = strstr( gprsbuff, str_base);
		//xprintf_P( PSTR("DEBUG str_p: %s\r\n\0"), p);
		if ( p != NULL ) {
			memset(localStr,'\0',sizeof(localStr));
			memcpy(localStr,p,sizeof(localStr));
			stringp = localStr;
			//xprintf_P( PSTR("DEBUG local_str: %s\r\n\0"), localStr );
			tk_nro = strsep(&stringp,delim);		//SMS0x
			tk_nro = strsep(&stringp,delim);		//09111111

			id[0] = '0' + i;
			id[1] = '\0';

			//xprintf_P( PSTR("DEBUG SMS: ID:%s, NRO=%s, LEVEL=%s\r\n\0"), id, tk_nro,tk_level);
			tanque_config("SMS", id, tk_nro );

			if ( systemVars.debug == DEBUG_APLICACION ) {
				xprintf_P( PSTR("GPRS: Reconfig TANQUE SMS0%d\r\n\0"), i);
			}
		}
	}

	u_save_params_in_NVMEE();

}
//------------------------------------------------------------------------------------

