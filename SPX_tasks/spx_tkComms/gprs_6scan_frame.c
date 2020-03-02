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

#include <comms.h>

// La tarea no puede demorar mas de 180s.
#define WDG_GPRS_TO_INIT	180

#define MAXAPN	3
const char apn_0[] PROGMEM = "SPYMOVIL.VPNANTEL";	// SPYMOVIL | UTE | TAHONA
const char apn_1[] PROGMEM = "STG1.VPNANTEL";		// OSE
const char apn_2[] PROGMEM = "ipgrs.claro.com.uy";	// CLARO
const char * const apn_list[] PROGMEM = { apn_0, apn_1, apn_2 };

#define MAXIP	4
const char server_ip_0[] PROGMEM = "192.168.0.9\0";		// SPYMOVIL
const char server_ip_1[] PROGMEM = "172.27.0.26\0";		// OSE
const char server_ip_2[] PROGMEM = "192.168.1.9\0";		// UTE
const char server_ip_3[] PROGMEM = "190.64.69.34\0";	// SPYMOVIL PUBLICA (CLARO)
const char * const server_ip_list[] PROGMEM = { server_ip_0, server_ip_1, server_ip_2, server_ip_3 };

char dlgId[DLGID_LENGTH];

typedef enum { MODEM_FAIL = 0, NETOPEN_ERROR, IP_FAIL, IP_GOOD } t_scan_errors;

t_scan_errors pv_scan_try( uint8_t apn_id, uint8_t ip_id );
static bool  pv_scan_send_frame( uint8_t ip_id );
static t_frame_responses pv_scan_process_response(void);
static void pv_scan_get_dlgid(void);


//------------------------------------------------------------------------------------
bool st_gprs_scan_frame(void)
{
	// Si alguno de los parametros (apn,ip,dlid) esta en DEFAULT, debo hacer un scan
	// para determinar los correctos.
	// Una vez determinados debo grabarlos en el systemVars.

bool exit_flag = bool_RESTART;
uint8_t apn_id, ip_id;
uint8_t try_status;

// Entry:

	GPRS_stateVars.state = G_SCAN_FRAME;

	ctl_watchdog_kick(WDG_GPRSTX, WDG_GPRS_TO_INIT );

	// Veo si es necesario hacer un SCAN de la IP del server
	if ( ( strcmp_P( systemVars.gprs_conf.server_ip_address, PSTR("DEFAULT\0")) == 0 ) ||
		( strcmp_P( systemVars.gprs_conf.apn, PSTR("DEFAULT\0")) == 0 ) ||
			( strcmp_P( systemVars.gprs_conf.dlgId, PSTR("DEFAULT\0")) == 0 ) )
	{
		// Alguno de los parametros esta en DEFAULT.
		// Inicio un ciclo de SCAN
		xprintf_P( PSTR("GPRS_SCAN: starting to scan...\r\n\0" ));

		for ( apn_id = 0; apn_id < MAXAPN; apn_id++ ) {
			for (ip_id = 0; ip_id < MAXIP; ip_id++ ) {
				try_status =  pv_scan_try( apn_id, ip_id );

				if  ( try_status == MODEM_FAIL ) {
					systemVars.gprs_conf.timerDial = 3600;
					exit_flag = bool_RESTART;
					goto EXIT;

				} else if ( try_status == NETOPEN_ERROR ) {
					// Salto al siguiente apn
					break;

				} else if ( try_status == IP_GOOD ) {
					// Logre encontrar los parametros. Los salvo en EE
					strncpy(systemVars.gprs_conf.dlgId, dlgId, DLGID_LENGTH);
					strcpy_P( systemVars.gprs_conf.server_ip_address , (PGM_P)pgm_read_word(&(server_ip_list[ip_id])));
					strcpy_P( systemVars.gprs_conf.apn , (PGM_P)pgm_read_word(&(apn_list[apn_id])));
					u_save_params_in_NVMEE();
					exit_flag = bool_RESTART;
					// Debo rearrancar con los nuevos parametros.
					xprintf_P( PSTR("GPRS_SCAN: OK.\r\n\0" ));
					exit_flag = bool_RESTART;
					goto EXIT;
				}
			}
		}
		// Luego de todo el escaneo no encontre parametros utiles.
		// Espero 1 hora para repetir
		systemVars.gprs_conf.timerDial = 3600;
		exit_flag = bool_RESTART;

	}  else {

		// Los datos son correctos.
		xprintf_P( PSTR("GPRS_SCAN: Params.OK (no scan).\r\n\0" ));
		exit_flag = bool_CONTINUAR;
	}

EXIT:
	return(exit_flag);

}
//------------------------------------------------------------------------------------
// FUNCIONES PRIVADAS
//------------------------------------------------------------------------------------
t_scan_errors pv_scan_try( uint8_t apn_id, uint8_t ip_id )
{
	// Tomo un APN y la IP indicados por los indices y trato de abrir un socket.
	// Si lo logro mando un frame de SCAN y veo si tengo suerte.

char apn_tmp[APN_LENGTH];

	// Apago
	xprintf_P( PSTR("GPRS_SCAN: Restart modem (apn=%d,ip=%d)\r\n\0"),apn_id, ip_id);
	u_gprs_modem_pwr_off();
	vTaskDelay( (portTickType)( 10000 / portTICK_RATE_MS ) );

	// Prendo
	if ( st_gprs_prender() != bool_CONTINUAR )  {
		return(MODEM_FAIL);
	}

	// Configuro
	if ( st_gprs_configurar() != bool_CONTINUAR ) {
		return(MODEM_FAIL);
	}

	// Seteo el apn
	memset( &apn_tmp, '\0', APN_LENGTH );
	strcpy_P( apn_tmp, (PGM_P)pgm_read_word(&(apn_list[apn_id])));
	xprintf_P( PSTR("GPRS_SCAN: try APN #%d: %s\r\n\0"), apn_id, apn_tmp);

	gprs_set_apn( apn_tmp );

	// Intento pedir una IP.
	if ( ! gprs_netopen() ) {
		return(NETOPEN_ERROR);
	}

	// Me asigno una IP.
	gprs_read_ip_assigned();

	// Trato de abir la conexion al server.
	// Solo mando 1 frame de scan
	if ( pv_scan_send_frame( ip_id ) ) {

		if ( pv_scan_process_response() == FRAME_OK ) {

			// Aqui es que anduvo todo bien y debo salir para pasar al modo DATA
			xprintf_P( PSTR("\r\nGPRS: Scan frame OK.\r\n\0" ));
			return(IP_GOOD);
		}
	}

	return(IP_FAIL);

}
//------------------------------------------------------------------------------------
static bool pv_scan_send_frame( uint8_t ip_id )
{

	// El socket puede estar cerrado por lo que reintento abrirlo hasta 3 veces.
	// Una vez que envie el INIT, salgo.
	// Al entrar, veo que el socket este cerrado.
	// GET /cgi-bin/spx/SPY.pl?DLGID=TEST02&TYPE=CTL&VER=2.0.6&PLOAD=TYPE:SCAN;UID:304632333433180f000500 HTTP/1.1
	// Host: www.spymovil.com
	// Connection: close\r\r ( no mando el close )


uint8_t intentos = 0;
bool exit_flag = false;
uint8_t timeout = 0;
uint8_t await_loops = 0;
t_socket_status socket_status = 0;

	memset( &GPRS_stateVars.server_ip_address, '\0', IP_LENGTH );
	strcpy_P( GPRS_stateVars.server_ip_address , (PGM_P)pgm_read_word(&(server_ip_list[ip_id])));
	xprintf_P( PSTR("GPRS_SCAN: try SERVER IP #%d: %s\r\n\0"), ip_id, GPRS_stateVars.server_ip_address );

	// Me aseguro que siempre en el SCAN el dlgid es DEFAULT
	strncpy_P(systemVars.gprs_conf.dlgId, PSTR("DEFAULT") , DLGID_LENGTH);

	for ( intentos = 0; intentos < MAX_TRYES_OPEN_SOCKET; intentos++ ) {

		socket_status = u_gprs_check_socket_status();

		if (  socket_status == SOCK_OPEN ) {

			u_gprs_flush_RX_buffer();
			u_gprs_flush_TX_buffer();
			//
			u_gprs_tx_header("CTL");

			xfprintf_P( fdGPRS, PSTR("&PLOAD=CLASS:SCAN;UID:%s\0" ), NVMEE_readID() );
			// DEBUG & LOG
			if ( systemVars.debug ==  DEBUG_GPRS ) {
				xprintf_P(  PSTR("&PLOAD=CLASS:SCAN;UID:%s\0" ), NVMEE_readID() );
			}

			u_gprs_tx_tail();
			xprintf_P( PSTR("GPRS_SCAN:: scanframe: Sent\r\n\0"));
			vTaskDelay( (portTickType)( 250 / portTICK_RATE_MS ) );
			return(true);
		}

		// Doy el comando para abrirlo y espero
		u_gprs_open_socket();

		await_loops = ( 10 * 1000 / 3000 ) + 1;
		// Y espero hasta 30s que abra.
		for ( timeout = 0; timeout < await_loops; timeout++) {
			vTaskDelay( (portTickType)( 3000 / portTICK_RATE_MS ) );
			socket_status = u_gprs_check_socket_status();

			// Si el socket abrio, salgo para trasmitir el frame de init.
			if ( socket_status == SOCK_OPEN ) {
				break;
			}

			// Si el socket dio error, salgo para enviar de nuevo el comando.
			if ( socket_status == SOCK_ERROR ) {
				break;
			}

			// Si el socket dio falla, debo reiniciar la conexion.
			if ( socket_status == SOCK_FAIL ) {
				return(exit_flag);
				break;
			}
		}
	}

	return(exit_flag);
}
//------------------------------------------------------------------------------------
static t_frame_responses pv_scan_process_response(void)
{
	// Espero una respuesta del server y la proceso
	// Salgo por timeout 10s o por socket closed.

uint8_t timeout = 0;
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

			if ( u_gprs_check_response("STATUS:OK") ) {	// Respuesta correcta. El dlgid esta definido en la BD
				exit_code = FRAME_OK;
				goto EXIT;

			} else if ( u_gprs_check_response("STATUS:RECONF") ) {	// Respuesta correcta
				pv_scan_get_dlgid();             				    // Configure el DLGID correcto y la SERVER_IP usada es la correcta.
				exit_code = FRAME_OK;
				goto EXIT;

			} else if ( u_gprs_check_response("STATUS:UNDEF") ) {	// Datalogger esta usando un script incorrecto
				xprintf_P( PSTR("GPRS_SCAN: SCRIPT ERROR !!.\r\n\0" ));
				exit_code = FRAME_NOT_ALLOWED;
				goto EXIT;

			} else if ( u_gprs_check_response("NOTDEFINED") ) {	// Datalogger esta usando un script incorrecto
				xprintf_P( PSTR("GPRS_SCAN: dlg not defined in BD\r\n\0" ));
				exit_code = FRAME_ERROR;
				goto EXIT;

			} else {
				// Frame no reconocido !!!
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
static void pv_scan_get_dlgid(void)
{
	// La linea recibida es del tipo: <h1>TYPE=CTL&PLOAD=CLASS:SCAN;STATUS:RECONF;DLGID:TEST01</h1>
	// Es la respuesta del server a un frame de SCAN.
	// Indica 2 cosas: - El server es el correcto por lo que debo salvar la IP
	//                 - Me pasa el DLGID correcto.


char *p = NULL;
char localStr[32] = { 0 };
char *stringp = NULL;
char *token = NULL;
char *delim = ",=:><";

	p = strstr( (const char *)&commsRxBuffer.buffer, "DLGID");
	if ( p == NULL ) {
		return;
	}

	// Copio el mensaje enviado a un buffer local porque la funcion strsep lo modifica.
	memset(localStr,'\0',32);
	memcpy(localStr,p,sizeof(localStr));

	stringp = localStr;
	token = strsep(&stringp,delim);	// DLGID
	token = strsep(&stringp,delim);	// TH001

	// Copio el dlgid recibido al systemVars.
	memset(dlgId,'\0', sizeof(dlgId) );
	strncpy(dlgId, token, DLGID_LENGTH);
	xprintf_P( PSTR("GPRS_SCAN: discover DLGID to %s\r\n\0"), dlgId );

}
//------------------------------------------------------------------------------------



