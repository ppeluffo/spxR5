/*
 * gprs_6scan_frame.c
 *
 *  Created on: 12 mar. 2019
 *      Author: pablo
 *
 *  En este estado entro solo si la IP que hay en el systemVars es DEFAULT.
 *  Si no, no hago nada y salgo
 *
 */

#include "gprs.h"

static bool pv_scan_try_ipservers(void);
static t_frame_responses pv_scan_process_response(void);
static void pv_scan_config_dlgid(void);

// La tarea no puede demorar mas de 180s.
#define WDG_GPRS_TO_INIT	180

const char server_ip_0[] PROGMEM = "192.168.0.9\0";		// SPYMOVIL
const char server_ip_1[] PROGMEM = "172.27.0.26\0";		// OSE
const char server_ip_2[] PROGMEM = "192.168.1.9\0";		// UTE

const char * const server_ip_list[] PROGMEM = { server_ip_0, server_ip_1, server_ip_2 };

#define MAXIP	3

//------------------------------------------------------------------------------------
bool st_gprs_scan_frame(void)
{
	// Si el systemVars.server_ip_address es DEFAULT, debo intentar conectarme
	// a algunos servidores a ver si descubro a cual correspondo.
	// Si la IP no es DEFAULT, salgo normalmente.

bool exit_flag = bool_RESTART;

// Entry:

	GPRS_stateVars.state = G_SCAN_FRAME;

	ctl_watchdog_kick(WDG_GPRSTX, WDG_GPRS_TO_INIT );

	// Veo si es necesario hacer un SCAN de la IP del server
	if ( strcmp_P( systemVars.gprs_conf.server_ip_address, PSTR("DEFAULT\0")) ) {
		// No es necesario.
		// Copio la IP en el GPRS_stateVars y salgo
		strcpy( GPRS_stateVars.server_ip_address, systemVars.gprs_conf.server_ip_address );
		exit_flag = bool_CONTINUAR;
		goto EXIT;

	} else {
		// La IP del systemVars es DEFAULT: empiezo un scan.
		exit_flag = pv_scan_try_ipservers();
		goto EXIT;
	}

// Exit
EXIT:

	return(exit_flag);

}
//------------------------------------------------------------------------------------
// FUNCIONES PRIVADAS
//------------------------------------------------------------------------------------
static bool pv_scan_try_ipservers(void)
{
	// El tema es que vimos que si nos intentamos conectar a una IP de otro APN, el socket
	// queda colgado y es casi imposible de salir lo que tranca el seguir con otra IP.
	// La solucion es que en esta instancia, se apaga y prende el modem, se configura
	// el APN y luego se intenta abrir el socket.

uint8_t ip_id = 0;

	xprintf_P( PSTR("GPRS_SCAN: starting ip scan frame.\r\n\0" ));

	// Tanteo IP's
	// Las voy a usar en u_gprs_open_socket
	for ( ip_id = 0; ip_id < MAXIP; ip_id++  ) {

		// Pass 1: Tomo una IP de la lista de posibles.
		strcpy_P( GPRS_stateVars.server_ip_address , (PGM_P)pgm_read_word(&(server_ip_list[ip_id])));
		xprintf_P( PSTR("GPRS_SCAN: fishing SERVER IP %d: %s\r\n\0"), ip_id, GPRS_stateVars.server_ip_address );

		// Apago
		xprintf_P( PSTR("GPRS_SCAN: Apago modem\r\n\0"));
		u_gprs_modem_pwr_off();
		vTaskDelay( (portTickType)( 10000 / portTICK_RATE_MS ) );

		// Prendo
		if ( st_gprs_prender() != bool_CONTINUAR )  {	// Intento prenderlo y si no salgo con false
			return(bool_RESTART);						// No intento con mas IPs
		}

		// Configuro
		if ( st_gprs_configurar() != bool_CONTINUAR ) {	// Intento configurarlo y si no puedo
			return(bool_RESTART);						// salgo con false y vuelvo a APAGAR
		}

		// Pido una IP
		if ( st_gprs_get_ip() != bool_CONTINUAR  ) {	// Si no logro una IP debo reiniciarme. Salgo en este caso
			return(bool_RESTART);						// con false.
		}

		// Aqui estoy con una IP listo para abrir un socket.
		// Solo mando 1 frame de scan
		if ( u_gprs_send_frame( SCAN_FRAME ) ) {

			if ( pv_scan_process_response() == FRAME_OK ) {

				// Aqui es que anduvo todo bien y debo salir para pasar al modo DATA
				xprintf_P( PSTR("\r\nGPRS: Scan frame OK.\r\n\0" ));
				return(bool_CONTINUAR);
			} else {
				// Reintento con el siguiente IP
				continue;
			}
		}
	}

	// Despues de reintentar con toda la lista de ip no pude conectarme a ninguna
	xprintf_P( PSTR("GPRS_SCAN: FAIL !!.\r\n\0" ));
	// Configuro para esperar 1 hora
	systemVars.gprs_conf.timerDial = 3600;

	return(bool_RESTART);

}
//------------------------------------------------------------------------------------
static t_frame_responses pv_scan_process_response(void)
{
	// Espero una respuesta del server y la proceso
	// Salgo por timeout 10s o por socket closed.

uint8_t timeout;
uint8_t exit_code = FRAME_ERROR;

	xprintf_P( PSTR("GPRS_SCAN:: try to check response\r\n\0"));

	for ( timeout = 0; timeout < 10; timeout++) {

		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );	// Espero 1s

		if ( u_gprs_check_socket_status() != SOCK_OPEN ) {		// El socket se cerro
			exit_code = FRAME_SOCK_CLOSE;
			goto EXIT;
		}

		// El socket esta abierto: chequeo la respuesta del server
		if ( u_gprs_check_response("404 Not Found") ) {	// Recibi un ERROR http 401: No existe el recurso
			exit_code = FRAME_ERR404;
			goto EXIT;
		}

		if ( u_gprs_check_response("ERROR") ) {	// Recibi un ERROR de respuesta
			exit_code = FRAME_ERROR;
			goto EXIT;
		}

		if ( u_gprs_check_response("</h1>") ) {	// Respuesta completa del server

			if ( u_gprs_check_response("DLGID") ) {	// Respuesta correcta
				pv_scan_config_dlgid();             // Configure el DLGID correcto y la SERVER_IP usada es la correcta.
				exit_code = FRAME_OK;
				goto EXIT;
			}

			if ( u_gprs_check_response("NOT_ALLOWED") ) {	// Datalogger esta usando un script incorrecto
				xprintf_P( PSTR("GPRS_SCAN: SCRIPT ERROR !!.\r\n\0" ));
				exit_code = FRAME_NOT_ALLOWED;
				goto EXIT;
			}

			if ( u_gprs_check_response("NOTDEFINED") ) {	// Datalogger esta usando un script incorrecto
				xprintf_P( PSTR("GPRS_SCAN: dlg not defined in BD\r\n\0" ));
				exit_code = FRAME_ERROR;
				goto EXIT;
			}

			if ( u_gprs_check_response("SCAN_ERR") ) {	// Datalogger no esta definido en el servidor usado
				xprintf_P( PSTR("GPRS_SCAN: dlg not defined in BD\r\n\0" ));
				exit_code = FRAME_ERROR;
				goto EXIT;
			}

		}

	}

EXIT:

	u_gprs_print_RX_Buffer();
	return(exit_code);

}
//------------------------------------------------------------------------------------
static void pv_scan_config_dlgid(void)
{
	// La linea recibida es del tipo: <h1>DLGID=TH001</h1>
	// Es la respuesta del server a un frame de SCAN.
	// Indica 2 cosas: - El server es el correcto por lo que debo salvar la IP
	//                 - Me pasa el DLGID correcto.


char *p;
char localStr[32];
char *stringp;
char *token;
char *delim = ",=:><";

	p = strstr( (const char *)&pv_gprsRxCbuffer.buffer, "DLGID");
	if ( p == NULL ) {
		return;
	}

	// Copio el mensaje enviado a un buffer local porque la funcion strsep lo modifica.
	memset(localStr,'\0',32);
	memcpy(localStr,p,sizeof(localStr));

	stringp = localStr;
	token = strsep(&stringp,delim);	// DLGID
	token = strsep(&stringp,delim);	// TH001

	// La IP usada en el tanteo es la correcta. Copio al SystemVars.
	memset(systemVars.gprs_conf.server_ip_address,'\0', sizeof(systemVars.gprs_conf.server_ip_address ) );
	strncpy(systemVars.gprs_conf.server_ip_address, GPRS_stateVars.server_ip_address, IP_LENGTH );
	xprintf_P( PSTR("GPRS_SCAN: IP server OK [%s]\r\n\0"), systemVars.gprs_conf.server_ip_address );

	// Copio el dlgid recibido al systemVars.
	memset(systemVars.gprs_conf.dlgId,'\0', sizeof(systemVars.gprs_conf.dlgId) );
	strncpy(systemVars.gprs_conf.dlgId, token, DLGID_LENGTH);
	xprintf_P( PSTR("GPRS_SCAN: Reconfig DLGID to %s\r\n\0"), systemVars.gprs_conf.dlgId );

	u_save_params_in_NVMEE();

}
//------------------------------------------------------------------------------------



