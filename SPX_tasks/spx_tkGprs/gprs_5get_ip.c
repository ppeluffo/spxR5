/*
 * sp5KV5_tkGprs_ask_ip.c
 *
 *  Created on: 27 de abr. de 2017
 *      Author: pablo
 */

#include "gprs.h"

static bool pv_gprs_netopen(void);
static void pv_gprs_read_ip_assigned(void);
static void pg_gprs_APN(void);
static bool pv_scan_apn( void );

// La tarea no puede demorar mas de 180s.
#define WDG_GPRS_TO_IP	180

const char apn_0[] PROGMEM = "SPYMOVIL.VPNANTEL";	// SPYMOVIL | UTE | TAHONA
const char apn_1[] PROGMEM = "STG1.VPNANTEL";		// OSE
const char * const apn_names[] PROGMEM = { apn_1, apn_0 };

#define MAXAPN	2

//------------------------------------------------------------------------------------
bool st_gprs_get_ip(void)
{
	// El modem esta prendido y configurado.
	// Intento hasta 3 veces pedir la IP.
	// WATCHDOG: En el peor caso demoro 2 mins.
	// La asignacion de la direccion IP es al activar el contexto con el comando AT+CGACT

bool exit_flag = bool_RESTART;

// Entry:
	GPRS_stateVars.state = G_GET_IP;
	ctl_watchdog_kick(WDG_GPRSTX, WDG_GPRS_TO_IP);

	// Si el APN esta en DEFAULT debo arrancar un scan de APN
	if (!strcmp_P( systemVars.gprs_conf.apn, PSTR("DEFAULT\0")) ) {

		// El scan_apn usa pv_gprs_netopen por lo que si es existoso
		// el datalogger queda con una ip asignada.
		exit_flag = pv_scan_apn();

	} else {

		// El APN no esta en default por lo tanto lo activo.
		pg_gprs_APN();

		if ( pv_gprs_netopen() ) {
			exit_flag = bool_CONTINUAR;
		} else {
			// Aqui es que luego de tantos reintentos no consegui la IP.
			exit_flag = bool_RESTART;
			xprintf_P( PSTR("GPRS: ERROR: ip no asignada !!.\r\n\0") );
		}
	}

	return(exit_flag);

}
//------------------------------------------------------------------------------------
// FUNCIONES PRIVADAS
//------------------------------------------------------------------------------------
static bool pv_gprs_netopen(void)
{
	// Doy el comando para atachearme a la red
	// Puede demorar unos segundos por lo que espero para chequear el resultado
	// y reintento varias veces.

uint8_t reintentos = MAX_TRYES_NET_ATTCH;
uint8_t checks;

	xprintf_P( PSTR("GPRS: netopen (get IP).\r\n\0") );
	// Espero 2s para dar el comando
	vTaskDelay( ( TickType_t)( 2000 / portTICK_RATE_MS ) );

	while ( reintentos-- > 0 ) {

		if ( systemVars.debug == DEBUG_GPRS ) {
			xprintf_P( PSTR("GPRS: send NETOPEN cmd (%d)\r\n\0"),reintentos );
		}

		// Envio el comando.
		// AT+NETOPEN
		u_gprs_flush_RX_buffer();
		xCom_printf_P( fdGPRS,PSTR("AT+NETOPEN\r\0"));
		vTaskDelay( ( TickType_t)( 2000 / portTICK_RATE_MS ) );

		// Intento 5 veces ver si respondio correctamente.
		for ( checks = 0; checks < 5; checks++) {

			if ( systemVars.debug == DEBUG_GPRS ) {
				xprintf_P( PSTR("GPRS: netopen check.(%d)\r\n\0"),checks );
			}

			if ( systemVars.debug == DEBUG_GPRS ) {
				u_gprs_print_RX_Buffer();
			}

			// Evaluo las respuestas del modem.
			if ( u_gprs_check_response("+NETOPEN: 0")) {
				xprintf_P( PSTR("GPRS: NETOPEN OK !.\r\n\0") );
				pv_gprs_read_ip_assigned();
				return(true);

			} else if ( u_gprs_check_response("Network opened")) {
				xprintf_P( PSTR("GPRS: NETOPEN OK !.\r\n\0") );
				pv_gprs_read_ip_assigned();
				return(true);

			} else if ( u_gprs_check_response("+IP ERROR: Network is already opened")) {
				pv_gprs_read_ip_assigned();
				return(true);

			} else if ( u_gprs_check_response("+NETOPEN: 1")) {
				xprintf_P( PSTR("GPRS: NETOPEN FAIL !!.\r\n\0") );
				return(false);

			} else if ( u_gprs_check_response("ERROR")) {
				break;	// Salgo de la espera

			}
			// Aun no tengo ninguna respuesta esperada.
			// espero 5s para re-evaluar la respuesta.
			vTaskDelay( (portTickType)( 5000 / portTICK_RATE_MS ) );

		}

		// No pude atachearme. Debo mandar de nuevo el comando
		// Pasaron 25s.
	}

	// Luego de varios reintentos no pude conectarme a la red.
	xprintf_P( PSTR("GPRS: NETOPEN FAIL !!.\r\n\0"));
	return(false);

}
//------------------------------------------------------------------------------------
static void pv_gprs_read_ip_assigned(void)
{
	// Tengo la IP asignada: la leo para actualizar systemVars.ipaddress

char *ts = NULL;
int i=0;
char c;

	// AT+CGPADDR para leer la IP
	u_gprs_flush_RX_buffer();
	//xCom_printf_P( fdGPRS,PSTR("AT+CGPADDR\r\0"));
	xCom_printf_P( fdGPRS,PSTR("AT+IPADDR\r\0"));
	vTaskDelay( (portTickType)( 2000 / portTICK_RATE_MS ) );
	if ( systemVars.debug == DEBUG_GPRS ) {
		u_gprs_print_RX_Buffer();
	}

	// Extraigo la IP del token.
	ts = strchr( pv_gprsRxCbuffer.buffer, ':');
	ts++;
	while ( (c= *ts) != '\r') {
		GPRS_stateVars.dlg_ip_address[i++] = c;
		ts++;
	}
	GPRS_stateVars.dlg_ip_address[i++] = '\0';

	xprintf_P( PSTR("GPRS: ip address=[%s]\r\n\0"), GPRS_stateVars.dlg_ip_address);

}
//------------------------------------------------------------------------------------
static void pg_gprs_APN(void)
{
	//Defino el PDP indicando cual es el APN.

	xprintf_P( PSTR("GPRS: Set APN\r\n\0") );

	// AT+CGDCONT
	u_gprs_flush_RX_buffer();
	xCom_printf_P( fdGPRS, PSTR("AT+CGSOCKCONT=1,\"IP\",\"%s\"\r\0"), systemVars.gprs_conf.apn);
	vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
	if ( systemVars.debug == DEBUG_GPRS ) {
		u_gprs_print_RX_Buffer();
	}

	// Como puedo tener varios PDP definidos, indico cual va a ser el que se deba activar
	// al usar el comando NETOPEN.
	u_gprs_flush_RX_buffer();
	xCom_printf_P( fdGPRS,PSTR("AT+CSOCKSETPN=1\0"));
	vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
	if ( systemVars.debug == DEBUG_GPRS ) {
		u_gprs_print_RX_Buffer();
	}


}
//------------------------------------------------------------------------------------
static bool pv_scan_apn( void )
{
	// Toma APNs de una lista e intenta conectarse a la red para detectar
	// cual sim es compatible con el APN.
	// Cuando lo encuentra, lo graba en el systemVars.

uint8_t apn_id = 0;
char apn[APN_LENGTH];

	while ( apn_id < MAXAPN ) {
		strcpy_P( apn, (PGM_P)pgm_read_word(&(apn_names[apn_id++])));			// Tomo un APN de la lista de posibles.
		xprintf_P( PSTR("GPRS: scan APN: %s\r\n\0"), apn);

		u_gprs_flush_RX_buffer();
		xCom_printf_P( fdGPRS,PSTR("AT+CGSOCKCONT=1,\"IP\",\"%s\"\r\0"), apn);	// Lo configuro en el modem
		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
		if ( systemVars.debug == DEBUG_GPRS ) {
			u_gprs_print_RX_Buffer();
		}

		u_gprs_flush_RX_buffer();
		xCom_printf_P( fdGPRS,PSTR("AT+CSOCKSETPN=1\0"));						// Lo marco como actuvo
		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
		if ( systemVars.debug == DEBUG_GPRS ) {
			u_gprs_print_RX_Buffer();
		}

		// Pruebo abrir el socket con el APN dado.
		if ( pv_gprs_netopen() ) {
			// El APN funciono. Lo grabo en el systemVars y salgo porque tengo la IP asignada
			xprintf_P( PSTR("GPRS:APN SCAN SUCCESS !!.\r\n\0") );
			strcpy(systemVars.gprs_conf.apn, apn );
			u_save_params_in_NVMEE();
			pv_gprs_read_ip_assigned();
			return(bool_CONTINUAR);
		}
	}

	// Ya probe con todos los APN y ninguno funciono.
	xprintf_P( PSTR("GPRS:APN SCAN Failed !!.\r\n\0") );
	// No tengo un APN que me sirva.
	// Espero 1 hora para lo que reconfiguro el timerDial
	systemVars.gprs_conf.timerDial = 3600;

	return( bool_RESTART);

}
//------------------------------------------------------------------------------------
