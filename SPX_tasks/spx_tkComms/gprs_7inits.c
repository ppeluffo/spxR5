/*
 * gprs_7ifup.c
 *
 *  Created on: 9 oct. 2019
 *      Author: pablo
 */

#include <spx_tkComms/gprs.h>
#include "spx.h"

// La tarea no puede demorar mas de 180s.
#define WDG_GPRS_INITS	180

bool f_send_init_base = false;
bool f_send_init_analog = false;
bool f_send_init_digital = false;
bool f_send_init_counters = false;
bool f_send_init_range = false;
bool f_send_init_psensor = false;
bool f_send_init_app = false;


//bool f_reset = false;

typedef enum { GLOBAL=0, BASE, ANALOG, DIGITAL, COUNTERS, RANGE, PSENSOR, APP_A, APP_B, APP_C } init_frames_t;

static bool pv_process_frame_init(init_frames_t tipo);
static bool pv_tx_frame_init(init_frames_t tipo);
static void pv_tx_init_payload(init_frames_t tipo);
static void pv_tx_init_payload_global(void);
static void pv_tx_init_payload_base(void);
static void pv_tx_init_payload_analog(void);
static void pv_tx_init_payload_digital(void);
static void pv_tx_init_payload_counters(void);
static void pv_tx_init_payload_range(void);
static void pv_tx_init_payload_psensor(void);
static void pv_tx_init_payload_app_A(void);
static void pv_tx_init_payload_app_B(void);
static void pv_tx_init_payload_app_C(void);

static t_frame_responses pv_init_process_response(void);
static void pv_init_reconfigure_params_global(void);
static void pv_init_reconfigure_params_base(void);
static void pv_init_reconfigure_params_analog(void);
static void pv_init_reconfigure_params_digital(void);
static void pv_init_reconfigure_params_counters(void);
static void pv_init_reconfigure_params_range(void);
static void pv_init_reconfigure_params_psensor(void);
static void pv_init_reconfigure_params_app_A(void);
static void pv_init_reconfigure_params_app_B(void);
static void pv_init_reconfigure_params_app_C(void);

bool pv_process_frame_init_GLOBAL(void);
bool pv_process_frame_init_BASE(void);
bool pv_process_frame_init_ANALOG(void);
bool pv_process_frame_init_DIGITAL(void);
bool pv_process_frame_init_COUNTERS(void);
bool pv_process_frame_init_RANGE(void);
bool pv_process_frame_init_PSENSOR(void);
bool pv_process_frame_init_APP(void);

static void pv_init_reconfigure_app_off(void);
static void pv_init_reconfigure_app_consigna(void);
static void pv_init_reconfigure_app_perforacion(void);
static void pv_init_reconfigure_app_tanque(void);
static void pv_init_reconfigure_app_plantapot(void);


//------------------------------------------------------------------------------------
bool st_gprs_inits(void)
{

	// Send Init Frame
	// GET /cgi-bin/spx/SPYR2.pl?DLGID=TEST02&TYPE=INIT&VER=2.0.6&PLOAD=[SIMPWD:DEFAULT;IMEI:860585004367917;
	//                                                                 SIMID:895980161423091055;BASE:0x32;AN:0xCB;DG:0x1A;CNT:0x47;RG:0xF7;PSE:0x73;OUT:0xB2]
	// HTTP/1.1
	// Host: www.spymovil.com
	// Connection: close\r\r ( no mando el close )


	GPRS_stateVars.state = G_INITS;

	// El resultado del frame GLOBAL es el que prende las flags de que
	// otras inits deben enviarse.
	if ( ! pv_process_frame_init_GLOBAL() ) {
		return(bool_RESTART);
	}


	// Configuracion BASE
	if ( ! pv_process_frame_init_BASE() ) {
		return(bool_RESTART);
	}

	// Configuracion ANALOG
	if ( ! pv_process_frame_init_ANALOG() ) {
		return(bool_RESTART);
	}

	// Configuracion DIGITAL
	if ( ! pv_process_frame_init_DIGITAL() ) {
		return(bool_RESTART);
	}

	// Configuracion COUNTERS
	if ( ! pv_process_frame_init_COUNTERS() ) {
		return(bool_RESTART);
	}

	// Configuracion RANGE
	if ( ! pv_process_frame_init_RANGE() ) {
		return(bool_RESTART);
	}

	// Configuracion PSENSOR
	if ( ! pv_process_frame_init_PSENSOR() ) {
		return(bool_RESTART);
	}

	// APLICACION:
	if ( ! pv_process_frame_init_APP() ) {
		return(bool_RESTART);
	}

	// Alguna configuracion requiere que me resetee para tomarla.

//	if ( f_reset ) {
//		xprintf_P( PSTR("GPRS: reset for reload config...\r\n\0"));
//		vTaskDelay( ( TickType_t)( 1500 / portTICK_RATE_MS ) );
//		CCPWrite( &RST.CTRL, RST_SWRST_bm );   /* Issue a Software Reset to initilize the CPU */
//	}

	return(bool_CONTINUAR);
}
//------------------------------------------------------------------------------------
bool pv_process_frame_init_GLOBAL(void)
{

	// Pongo las flags de configuracion de todos los componentes en false
	// y la respuesta del servidor es quien las va a prender.

bool retS = false;

	f_send_init_base = false;
	f_send_init_analog = false;
	f_send_init_digital = false;
	f_send_init_counters = false;
	f_send_init_range = false;
	f_send_init_psensor = false;
	f_send_init_app = false;

	retS = pv_process_frame_init(GLOBAL);
	return(retS);

}
//------------------------------------------------------------------------------------
bool pv_process_frame_init_BASE(void)
{

bool retS = true;

	if ( f_send_init_base ) {
		f_send_init_base = false;
		retS = pv_process_frame_init(BASE);
	}
	return(retS);

}
//------------------------------------------------------------------------------------
bool pv_process_frame_init_ANALOG(void)
{

bool retS = true;

	if ( f_send_init_analog ) {
		f_send_init_analog = false;
		retS = pv_process_frame_init(ANALOG);
	}
	return(retS);

}
//------------------------------------------------------------------------------------
bool pv_process_frame_init_DIGITAL(void)
{

bool retS = true;

	if ( f_send_init_digital ) {
		f_send_init_digital = false;
		retS = pv_process_frame_init(DIGITAL);
	}
	return(retS);

}
//------------------------------------------------------------------------------------
bool pv_process_frame_init_COUNTERS(void)
{

bool retS = true;

	if ( f_send_init_counters ) {
		f_send_init_counters = false;
		retS = pv_process_frame_init(COUNTERS);
	}
	return(retS);

}
//------------------------------------------------------------------------------------
bool pv_process_frame_init_RANGE(void)
{

bool retS = true;

	if ( f_send_init_range ) {
		f_send_init_range = false;
		retS = pv_process_frame_init(RANGE);
	}
	return(retS);

}
//------------------------------------------------------------------------------------
bool pv_process_frame_init_PSENSOR(void)
{

bool retS = true;

	if ( f_send_init_psensor ) {
		f_send_init_psensor = false;
		retS = pv_process_frame_init(PSENSOR);
	}
	return(retS);

}
//------------------------------------------------------------------------------------
bool pv_process_frame_init_APP(void)
{
	// Proceso la reconfiguracio de la aplicacion activa.
	// Puede involucrar varios frames, dependiendo de la aplicacion.
	// En el primero es en el que el server indica la aplicacion.
	// Luego, viendo cual aplicacion fue activada es que enviamos las
	// solicitudes de los otros frames.

bool retS = true;

	if ( f_send_init_app ) {
		// El frame APP_A configura el tipo de aplicacion lo que determina
		// si requiero frames extra
		retS = pv_process_frame_init(APP_A);

		switch(systemVars.aplicacion) {
		case APP_OFF:
			f_send_init_app = false;
			break;
		case APP_CONSIGNA:
			f_send_init_app = false;
			break;
		case APP_PERFORACION:
			f_send_init_app = false;
			break;
		case APP_TANQUE:
			f_send_init_app = false;
			break;
		case APP_PLANTAPOT:
			// Requiere 2 frames mas:
			retS = pv_process_frame_init(APP_B);	// SMS
			retS = pv_process_frame_init(APP_C);	// Niveles
			f_send_init_app = false;
			//f_reset = true;
			break;
		}
	}
	return(retS);

}
//------------------------------------------------------------------------------------
static bool pv_process_frame_init(init_frames_t tipo)
{
	// Debo mandar el frame de init al server, esperar la respuesta, analizarla
	// y reconfigurarme.
	// Intento 3 veces antes de darme por vencido.
	// El socket puede estar abierto o cerrado. Lo debo determinar en c/caso y
	// si esta cerrado abrirlo.
	// Mientras espero la respuesta debo monitorear que el socket no se cierre

uint8_t intentos = 0;
bool exit_flag = bool_RESTART;

// Entry:

	GPRS_stateVars.state = G_INITS;

	// En open_socket uso la IP del GPRS_stateVars asi que antes debo copiarla.
	strcpy( GPRS_stateVars.server_ip_address, systemVars.gprs_conf.server_ip_address );

	ctl_watchdog_kick(WDG_GPRSTX, WDG_GPRS_INITS );

	xprintf_P( PSTR("GPRS: iniframe(%d)\r\n\0"),tipo);

	// Intenteo MAX_INIT_TRYES procesar correctamente el INIT
	for ( intentos = 0; intentos < MAX_INIT_TRYES; intentos++ ) {

		if ( pv_tx_frame_init( tipo ) ) {

			switch( pv_init_process_response() ) {

			case FRAME_ERROR:
				// Reintento
				break;
			case FRAME_SOCK_CLOSE:
				// Reintento
				break;
			case FRAME_RETRY:
				// Reintento
				break;
			case FRAME_OK:
				// Aqui es que anduvo todo bien y continuo.
				if ( systemVars.debug == DEBUG_GPRS ) {
					xprintf_P( PSTR("\r\nGPRS: Init frame OK.\r\n\0" ));
				}
				exit_flag = bool_CONTINUAR;
				goto EXIT;
				break;
			case FRAME_NOT_ALLOWED:
				// Respondio bien pero debo salir a apagarme
				exit_flag = bool_RESTART;
				goto EXIT;
				break;
			case FRAME_ERR404:
				// No existe el recurso en el server
				exit_flag = bool_RESTART;
				goto EXIT;
				break;
			case FRAME_SRV_ERR:
				// Problemas en el servidor
				exit_flag = bool_RESTART;
				goto EXIT;
				break;
			}

		} else {

			if ( systemVars.debug == DEBUG_GPRS ) {
				xprintf_P( PSTR("GPRS: iniframe retry(%d)\r\n\0"),intentos);
			}

			// Espero 3s antes de reintentar
			vTaskDelay( (portTickType)( 3000 / portTICK_RATE_MS ) );
		}
	}

	// Aqui es que no puede enviar/procesar el INIT correctamente
	if ( systemVars.debug == DEBUG_GPRS ) {
		xprintf_P( PSTR("GPRS: Init frame FAIL !!.\r\n\0" ));
	}

// Exit
EXIT:

	return(exit_flag);

}
//------------------------------------------------------------------------------------
static bool pv_tx_frame_init(init_frames_t tipo)
{
	//  Aqui es donde trasmito el frame.
	//	GET /cgi-bin/spx/SPYR2.pl?DLGID=TEST02&TYPE=INIT&VER=2.0.6&PLOAD=[....] HTTP/1.1
	//	Host: www.spymovil.com
	//  Connection: close\r\r ( no mando el close )

uint8_t intentos = 0;
t_socket_status socket_status = 0;

	if ( systemVars.debug == DEBUG_GPRS  ) {
		xprintf_P( PSTR("GPRS: initframe: Sent\r\n\0"));
	}

	for ( intentos = 0; intentos < MAX_TRYES_OPEN_SOCKET; intentos++ ) {

		socket_status = u_gprs_check_socket_status();

		// Si el socket esta abierto trasmito y salgo a esperar la respuesta.
		if (  socket_status == SOCK_OPEN ) {
			u_gprs_flush_RX_buffer();
			u_gprs_flush_TX_buffer();

			u_gprs_tx_header("INIT");
			pv_tx_init_payload(tipo);
			u_gprs_tx_tail();

			return(true);
		}

		// Si el socket dio falla, debo reiniciar la conexion.
		if ( socket_status == SOCK_FAIL ) {
			return(false);
		}

		// El socket esta cerrado: Doy el comando para abrirlo y espero
		u_gprs_open_socket();

	}

	return(false);

}
//------------------------------------------------------------------------------------
static void pv_tx_init_payload(init_frames_t tipo)
{

	switch(tipo) {
	case GLOBAL:
		pv_tx_init_payload_global();
		break;
	case BASE:
		pv_tx_init_payload_base();
		break;
	case ANALOG:
		pv_tx_init_payload_analog();
		break;
	case DIGITAL:
		pv_tx_init_payload_digital();
		break;
	case COUNTERS:
		pv_tx_init_payload_counters();
		break;
	case RANGE:
		pv_tx_init_payload_range();
		break;
	case PSENSOR:
		pv_tx_init_payload_psensor();
		break;
	case APP_A:
		pv_tx_init_payload_app_A();
		break;
	case APP_B:
		pv_tx_init_payload_app_B();
		break;
	case APP_C:
		pv_tx_init_payload_app_C();
		break;
	}
}
//------------------------------------------------------------------------------------
static void pv_tx_init_payload_global(void)
{
uint8_t base_cks, an_cks, dig_cks, cnt_cks, range_cks, psens_cks, app_cks;

	base_cks = u_base_checksum();
	an_cks = ainputs_checksum();
	dig_cks = dinputs_checksum();
	cnt_cks = counters_checksum();
	range_cks = range_checksum();
	psens_cks = psensor_checksum();
	app_cks = u_aplicacion_checksum();

	xCom_printf_P( fdGPRS,PSTR("&PLOAD=CLASS:GLOBAL;NACH:%d;NDCH:%d;NCNT:%d;" ),NRO_ANINPUTS,NRO_DINPUTS,NRO_COUNTERS );
	xCom_printf_P( fdGPRS,PSTR("SIMPWD:%s;IMEI:%s;UID:%s;" ), systemVars.gprs_conf.simpwd, &buff_gprs_imei, NVMEE_readID() );
	xCom_printf_P( fdGPRS,PSTR("SIMID:%s;CSQ:%d;WRST:%02X;" ), &buff_gprs_ccid,GPRS_stateVars.dbm, wdg_resetCause );
	xCom_printf_P( fdGPRS,PSTR("BASE:0x%02X;AN:0x%02X;DG:0x%02X;" ), base_cks,an_cks,dig_cks );
	xCom_printf_P( fdGPRS,PSTR("CNT:0x%02X;RG:0x%02X;" ),cnt_cks,range_cks );
	xCom_printf_P( fdGPRS,PSTR("PSE:0x%02X;" ), psens_cks );
	xCom_printf_P( fdGPRS,PSTR("APP:0x%02X;" ), app_cks );

	// DEBUG & LOG
	if ( systemVars.debug ==  DEBUG_GPRS ) {
		xprintf_P( PSTR("&PLOAD=CLASS:GLOBAL;NACH:%d;NDCH:%d;NCNT:%d;" ),NRO_ANINPUTS,NRO_DINPUTS,NRO_COUNTERS );
		xprintf_P( PSTR("SIMPWD:%s;IMEI:%s;UID:%s;" ), systemVars.gprs_conf.simpwd, &buff_gprs_imei, NVMEE_readID() );
		xprintf_P( PSTR("SIMID:%s;CSQ:%d;WRST:%02X;" ), &buff_gprs_ccid,GPRS_stateVars.dbm, wdg_resetCause );
		xprintf_P( PSTR("BASE:0x%02X;AN:0x%02X;DG:0x%02X;" ), base_cks,an_cks,dig_cks );
		xprintf_P( PSTR("CNT:0x%02X;RG:0x%02X;" ),cnt_cks,range_cks );
		xprintf_P( PSTR("PSE:0x%02X;" ), psens_cks );
		xprintf_P( PSTR("APP_A:0x%02X;" ), app_cks );
	}

}
//------------------------------------------------------------------------------------
static void pv_tx_init_payload_base(void)
{

	// PLOAD=CLASS:BASE;

	xCom_printf_P( fdGPRS,PSTR("&PLOAD=CLASS:CONF_BASE;"));
	// DEBUG & LOG
	if ( systemVars.debug == DEBUG_GPRS ) {
		xprintf_P( PSTR("&PLOAD=CLASS:CONF_BASE;"));
	}

}
//------------------------------------------------------------------------------------
static void pv_tx_init_payload_analog(void)
{
	// Mandamos todos los canales analogicos, esten o no configurados (X)

	//  PLOAD=CLASS:ANALOG;

	xCom_printf_P( fdGPRS,PSTR("&PLOAD=CLASS:CONF_ANALOG;"));
	if ( systemVars.debug ==  DEBUG_GPRS ) {
		xprintf_P( PSTR("&PLOAD=CLASS:CONF_ANALOG;"));
	}
}
//------------------------------------------------------------------------------------
static void pv_tx_init_payload_digital(void)
{

	// PLOAD=CLASS:DIGITAL;

	xCom_printf_P( fdGPRS,PSTR("&PLOAD=CLASS:CONF_DIGITAL;"));
	if ( systemVars.debug ==  DEBUG_GPRS ) {
		xprintf_P( PSTR("&PLOAD=CLASS:CONF_DIGITAL;"));
	}

}
//------------------------------------------------------------------------------------
static void pv_tx_init_payload_counters(void)
{

	// PLOAD=CLASS:COUNTER;

	xCom_printf_P( fdGPRS,PSTR("&PLOAD=CLASS:CONF_COUNTER;"));
	if ( systemVars.debug ==  DEBUG_GPRS ) {
		xprintf_P( PSTR("&PLOAD=CLASS:CONF_COUNTER;"));
	}

}
//------------------------------------------------------------------------------------
static void pv_tx_init_payload_range(void)
{

	// PLOAD=CLASS:RANGE;R0:DIST1

	xCom_printf_P( fdGPRS,PSTR("&PLOAD=CLASS:CONF_RANGE;"));
	if ( systemVars.debug ==  DEBUG_GPRS ) {
		xprintf_P( PSTR("&PLOAD=CLASS:CONF_RANGE;"));
	}
}
//------------------------------------------------------------------------------------
static void pv_tx_init_payload_psensor(void)
{

	// PLOAD=CLASS:PSENSOR;PS0:CLORO,0,70

	xCom_printf_P( fdGPRS,PSTR("&PLOAD=CLASS:CONF_PSENSOR;"));

	if ( systemVars.debug ==  DEBUG_GPRS ) {
		xprintf_P( PSTR("&PLOAD=CLASS:CONF_PSENSOR;"));
	}

}
//------------------------------------------------------------------------------------
static void pv_tx_init_payload_app_A(void)
{

	xCom_printf_P( fdGPRS,PSTR("&PLOAD=CLASS:CONF_APP;"));
	if ( systemVars.debug ==  DEBUG_GPRS ) {
		xprintf_P( PSTR("&PLOAD=CLASS:CONF_APP;"));
	}

}
//------------------------------------------------------------------------------------
static void pv_tx_init_payload_app_B(void)
{

	// En aplicacion PPOT pido la configuracion de los SMS
	if ( systemVars.aplicacion == APP_PLANTAPOT ) {
		xCom_printf_P( fdGPRS,PSTR("&PLOAD=CLASS:CONF_PPOT_SMS;"));
		if ( systemVars.debug ==  DEBUG_GPRS ) {
			xprintf_P( PSTR("&PLOAD=CLASS:CONF_PPOT_SMS;"));
		}
	}
}
//------------------------------------------------------------------------------------
static void pv_tx_init_payload_app_C(void)
{

	// En aplicacion PPOT pido la configuracion de los NIVELES
	if ( systemVars.aplicacion == APP_PLANTAPOT ) {
		xCom_printf_P( fdGPRS,PSTR("&PLOAD=CLASS:CONF_PPOT_LEVELS;"));
		if ( systemVars.debug ==  DEBUG_GPRS ) {
			xprintf_P( PSTR("&PLOAD=CLASS:CONF_PPOT_LEVELS;"));
		}
	}

}
//------------------------------------------------------------------------------------
static t_frame_responses pv_init_process_response(void)
{

	// Espero la respuesta al frame de INIT.
	// Si la recibo la proceso.
	// Salgo por timeout 10s o por socket closed.

uint8_t timeout = 0;
uint8_t exit_code = FRAME_ERROR;

	for ( timeout = 0; timeout < 10; timeout++) {

		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );	// Espero 1s

		if ( u_gprs_check_socket_status() != SOCK_OPEN ) {		// El socket se cerro
			exit_code = FRAME_SOCK_CLOSE;
			goto EXIT;
		}

		if ( u_gprs_check_response("ERROR") ) {	// Recibi un ERROR de respuesta
			u_gprs_print_RX_Buffer();
			exit_code = FRAME_ERROR;
			goto EXIT;
		}

		if ( u_gprs_check_response("</h1>") ) {	// Respuesta completa del server

			if ( systemVars.debug == DEBUG_GPRS  ) {
				u_gprs_print_RX_Buffer();
			} else {
				u_gprs_print_RX_response();
			}

			// Analizo las respuestas.
			if ( u_gprs_check_response("GLOBAL") ) {	// Respuesta correcta:
				// Borro la causa del reset
				wdg_resetCause = 0x00;
				pv_init_reconfigure_params_global();
				exit_code = FRAME_OK;
				goto EXIT;
			}

			if ( u_gprs_check_response("BASE") ) {	// Respuesta correcta: Reconfiguro parametros base.
				// Borro la causa del reset
				wdg_resetCause = 0x00;
				pv_init_reconfigure_params_base();
				exit_code = FRAME_OK;
				goto EXIT;
			}

			if ( u_gprs_check_response("ANALOG") ) {
				// Borro la causa del reset
				wdg_resetCause = 0x00;
				pv_init_reconfigure_params_analog();
				exit_code = FRAME_OK;
				goto EXIT;
			}

			if ( u_gprs_check_response("DIGITAL") ) {
				// Borro la causa del reset
				wdg_resetCause = 0x00;
				pv_init_reconfigure_params_digital();
				exit_code = FRAME_OK;
				goto EXIT;
			}

			if ( u_gprs_check_response("COUNTER") ) {
				// Borro la causa del reset
				wdg_resetCause = 0x00;
				pv_init_reconfigure_params_counters();
				exit_code = FRAME_OK;
				goto EXIT;
			}

			if ( u_gprs_check_response("RANGE") ) {
				// Borro la causa del reset
				wdg_resetCause = 0x00;
				pv_init_reconfigure_params_range();
				exit_code = FRAME_OK;
				goto EXIT;
			}

			if ( u_gprs_check_response("PSENSOR") ) {
				// Borro la causa del reset
				wdg_resetCause = 0x00;
				pv_init_reconfigure_params_psensor();
				exit_code = FRAME_OK;
				goto EXIT;
			}

			if ( u_gprs_check_response("APP_A") ) {
				// Borro la causa del reset
				wdg_resetCause = 0x00;
				pv_init_reconfigure_params_app_A();
				exit_code = FRAME_OK;
				goto EXIT;
			}

			if ( u_gprs_check_response("APP_B") ) {
				// Borro la causa del reset
				wdg_resetCause = 0x00;
				pv_init_reconfigure_params_app_B();
				exit_code = FRAME_OK;
				goto EXIT;
			}

			if ( u_gprs_check_response("APP_C") ) {
				// Borro la causa del reset
				wdg_resetCause = 0x00;
				pv_init_reconfigure_params_app_C();
				exit_code = FRAME_OK;
				goto EXIT;
			}

			if ( u_gprs_check_response("SRV_ERR") ) {	// El servidor no pudo procesar el frame. Problema del server
				// Borro la causa del reset
				wdg_resetCause = 0x00;
				exit_code = FRAME_SRV_ERR;
				xprintf_P( PSTR("GPRS: SERVER ERROR !!.\r\n\0" ));
				goto EXIT;
			}

			if ( u_gprs_check_response("NOT_ALLOWED") ) {	// Datalogger esta usando un script incorrecto
				xprintf_P( PSTR("GPRS: SCRIPT ERROR !!.\r\n\0" ));
				exit_code = FRAME_NOT_ALLOWED;
				goto EXIT;
			}
		}
	}

// Exit:
EXIT:

	return(exit_code);

}
//------------------------------------------------------------------------------------
static void pv_init_reconfigure_params_global(void)
{
	// Recibimos un frame que trae la fecha y hora y parametros que indican
	// que otras flags de configuraciones debemos prender.
	// GLOBAL;CLOCK:1910120345;BASE;ANALOG;DIGITAL;COUNTERS;RANGE;PSENSOR;APP_A;APP_B;APP_C

char *p = NULL;
char localStr[32] = { 0 };
char *stringp = NULL;
char *token = NULL;
char *delim = ":;><";
char rtcStr[12];
uint8_t i = 0;
char c = '\0';
RtcTimeType_t rtc;
int8_t xBytes = 0;


	// CLOCK
	p = strstr( (const char *)&pv_gprsRxCbuffer.buffer, "CLOCK");
	if ( p != NULL ) {
		// Copio el mensaje enviado a un buffer local porque la funcion strsep lo modifica.
		memset( &localStr, '\0', sizeof(localStr) );
		memcpy(localStr,p,sizeof(localStr));

		stringp = localStr;
		token = strsep(&stringp,delim);			// CLOCK

		token = strsep(&stringp,delim);			// 1910120345
		memset(rtcStr, '\0', sizeof(rtcStr));
		memcpy(rtcStr,token, sizeof(rtcStr));	// token apunta al comienzo del string con la hora
		for ( i = 0; i<12; i++) {
			c = *token;
			rtcStr[i] = c;
			c = *(++token);
			if ( c == '\0' )
				break;

		}

		memset( &rtc, '\0', sizeof(rtc) );
		RTC_str2rtc(rtcStr, &rtc);	// Convierto el string YYMMDDHHMM a RTC.
		xBytes = RTC_write_dtime(&rtc);		// Grabo el RTC
		if ( xBytes == -1 )
			xprintf_P(PSTR("ERROR: I2C:RTC:pv_process_server_clock\r\n\0"));

		if ( systemVars.debug == DEBUG_GPRS ) {
			xprintf_P( PSTR("GPRS: Update rtc to: %s\r\n\0"), rtcStr );
		}

	}

	// Flags de configuraciones particulares: BASE;ANALOG;DIGITAL;COUNTERS;RANGE;PSENSOR;OUTS
	p = strstr( (const char *)&pv_gprsRxCbuffer.buffer, "BASE");
	if ( p != NULL ) {
		f_send_init_base = true;
		if ( systemVars.debug == DEBUG_GPRS ) {
			xprintf_P( PSTR("GPRS: Reconfig BASE\r\n\0"));
		}
	}

	p = strstr( (const char *)&pv_gprsRxCbuffer.buffer, "ANALOG");
	if ( p != NULL ) {
		f_send_init_analog = true;
		if ( systemVars.debug == DEBUG_GPRS ) {
			xprintf_P( PSTR("GPRS: Reconfig ANALOG\r\n\0"));
		}
	}

	p = strstr( (const char *)&pv_gprsRxCbuffer.buffer, "DIGITAL");
	if ( p != NULL ) {
		f_send_init_digital = true;
		if ( systemVars.debug == DEBUG_GPRS ) {
			xprintf_P( PSTR("GPRS: Reconfig DIGITAL\r\n\0"));
		}
	}

	p = strstr( (const char *)&pv_gprsRxCbuffer.buffer, "COUNTERS");
	if ( p != NULL ) {
		f_send_init_counters = true;
		if ( systemVars.debug == DEBUG_GPRS ) {
			xprintf_P( PSTR("GPRS: Reconfig COUNTERS\r\n\0"));
		}
	}

	p = strstr( (const char *)&pv_gprsRxCbuffer.buffer, "RANGE");
	if ( p != NULL ) {
		f_send_init_range = true;
		if ( systemVars.debug == DEBUG_GPRS ) {
			xprintf_P( PSTR("GPRS: Reconfig RANGE\r\n\0"));
		}
	}

	p = strstr( (const char *)&pv_gprsRxCbuffer.buffer, "PSENSOR");
	if ( p != NULL ) {
		f_send_init_psensor = true;
		if ( systemVars.debug == DEBUG_GPRS ) {
			xprintf_P( PSTR("GPRS: Reconfig PSENSOR\r\n\0"));
		}
	}

	p = strstr( (const char *)&pv_gprsRxCbuffer.buffer, "APLICACION");
	if ( p != NULL ) {
		f_send_init_app = true;
		if ( systemVars.debug == DEBUG_GPRS ) {
			xprintf_P( PSTR("GPRS: Reconfig APLICACION\r\n\0"));
		}
	}


}
//------------------------------------------------------------------------------------
static void pv_init_reconfigure_params_base(void)
{

	//	TYPE=INIT&PLOAD=CLASS:BASE;TPOLL:60;TDIAL:60;PWST:5;PWRS:ON,2330,630

char *p = NULL;
char localStr[32] = { 0 };
char *stringp = NULL;
char *token = NULL;
char *tk_pws_modo = NULL;
char *tk_pws_start = NULL;
char *tk_pws_end = NULL;
char *delim = ",:;><";
bool save_flag = false;

	// TDIAL
	p = strstr( (const char *)&pv_gprsRxCbuffer.buffer, "TDIAL");
	if ( p != NULL ) {

		// Copio el mensaje enviado a un buffer local porque la funcion strsep lo modifica.
		memset( &localStr, '\0', sizeof(localStr) );
		memcpy(localStr,p,sizeof(localStr));

		stringp = localStr;
		token = strsep(&stringp,delim);		// TDIAL
		token = strsep(&stringp,delim);		// timerDial
		u_gprs_config_timerdial(token);
		save_flag = true;

		if ( systemVars.debug == DEBUG_GPRS ) {
			xprintf_P( PSTR("GPRS: Reconfig TDIAL\r\n\0"));
		}
	}

	// TPOLL
	p = strstr( (const char *)&pv_gprsRxCbuffer.buffer, "TPOLL");
	if ( p != NULL ) {

		memset( &localStr, '\0', sizeof(localStr) );
		memcpy(localStr,p,sizeof(localStr));

		stringp = localStr;
		token = strsep(&stringp,delim);		// TPOLL
		token = strsep(&stringp,delim);		// timerPoll
		u_config_timerpoll(token);
		save_flag = true;

		if ( systemVars.debug == DEBUG_GPRS ) {
			xprintf_P( PSTR("GPRS: Reconfig TPOLL\r\n\0"));
		}
	}

	// PWST
	p = strstr( (const char *)&pv_gprsRxCbuffer.buffer, "PWST");
	if ( p != NULL ) {

		// Copio el mensaje enviado a un buffer local porque la funcion strsep lo modifica.
		memset( &localStr, '\0', sizeof(localStr) );
		memcpy(localStr,p,sizeof(localStr));

		stringp = localStr;
		token = strsep(&stringp,delim);		// PWST
		token = strsep(&stringp,delim);		// timePwrSensor
		ainputs_config_timepwrsensor(token);
		save_flag = true;

		if ( systemVars.debug == DEBUG_GPRS ) {
			xprintf_P( PSTR("GPRS: Reconfig PWRSTIME\r\n\0"));
		}
	}

	// PWRS
	p = strstr( (const char *)&pv_gprsRxCbuffer.buffer, "PWRS");
	if ( p != NULL ) {
		memset( &localStr, '\0', sizeof(localStr) );
		memcpy(localStr,p,sizeof(localStr));

		stringp = localStr;
		tk_pws_modo = strsep(&stringp,delim);		//PWRS
		tk_pws_modo = strsep(&stringp,delim);		// modo ( ON / OFF ).
		tk_pws_start = strsep(&stringp,delim);		// startTime
		tk_pws_end  = strsep(&stringp,delim); 		// endTime
		u_gprs_configPwrSave(tk_pws_modo, tk_pws_start, tk_pws_end );
		save_flag = true;

		if ( systemVars.debug == DEBUG_GPRS ) {
			xprintf_P( PSTR("GPRS: Reconfig PWRSAVE\r\n\0"));
		}
	}

	if ( save_flag ) {
		u_save_params_in_NVMEE();
	}

}
//------------------------------------------------------------------------------------
static void pv_init_reconfigure_params_analog(void)
{
	//	CLASS:ANALOG;A0:PA,4,20,0,100;A1:X,0,0,0,0;A2:OPAC,0,20,0,100,A3:NOX,4,20,0,2000;A4:SO2,4,20,0,1000;A5:CO,4,20,0,25;A6:NO2,4,20,0,000;A7:CAU,4,20,0,1000
	// 	PLOAD=CLASS:ANALOG;A0:PA,4,20,0.0,10.0;A1:X,4,20,0.0,10.0;A3:X,4,20,0.0,10.0;
	//  TYPE=INIT&PLOAD=CLASS:ANALOG;A0:PA,4,20,0.0,10.0;A1:X,4,20,0.0,10.0;A2:X,4,20,0.0,10.0;A3:X,4,20,0.0,10.0;A4:X,4,20,0.0,10.0;

char *p = NULL;
char localStr[32] = { 0 };
char *stringp = NULL;
char *tk_name= NULL;
char *tk_iMin= NULL;
char *tk_iMax = NULL;
char *tk_mMin = NULL;
char *tk_mMax = NULL;
char *tk_offset = NULL;
char *delim = ",=:;><";
bool save_flag = false;
uint8_t ch;
char str_base[8];

	// A?
	for (ch=0; ch < NRO_ANINPUTS; ch++ ) {
		memset( &str_base, '\0', sizeof(str_base) );
		snprintf_P( str_base, sizeof(str_base), PSTR("A%d\0"), ch );
		//xprintf_P( PSTR("DEBUG str_base: %s\r\n\0"), str_base);

		p = strstr( (const char *)&pv_gprsRxCbuffer.buffer, str_base);
		//xprintf_P( PSTR("DEBUG str_p: %s\r\n\0"), p);
		if ( p != NULL ) {
			memset(localStr,'\0',sizeof(localStr));
			memcpy(localStr,p,sizeof(localStr));
			stringp = localStr;
			//xprintf_P( PSTR("DEBUG local_str: %s\r\n\0"), localStr );
			tk_name = strsep(&stringp,delim);		//A0
			tk_name = strsep(&stringp,delim);		//name
			tk_iMin = strsep(&stringp,delim);		//iMin
			tk_iMax = strsep(&stringp,delim);		//iMax
			tk_mMin = strsep(&stringp,delim);		//mMin
			tk_mMax = strsep(&stringp,delim);		//mMax
			tk_offset = strsep(&stringp,delim);		//offset

			ainputs_config_channel( ch, tk_name ,tk_iMin, tk_iMax, tk_mMin, tk_mMax, tk_offset );

			if ( systemVars.debug == DEBUG_GPRS ) {
				xprintf_P( PSTR("GPRS: Reconfig A%d\r\n\0"), ch);
			}

			save_flag = true;
		}
	}

	if ( save_flag ) {
		u_save_params_in_NVMEE();
	}

}
//------------------------------------------------------------------------------------
static void pv_init_reconfigure_params_digital(void)
{

	//	PLOAD=CLASS:DIGITAL;D0:DIN0,NORMAL;D1:DIN1,TIMER;

char *p = NULL;
char localStr[32] = { 0 };
char *stringp = NULL;
char *tk_name= NULL;
char *tk_type= NULL;
char *delim = ",=:;><";
bool save_flag = false;
uint8_t ch;
char str_base[8];

	// A?
	for (ch=0; ch < NRO_DINPUTS; ch++ ) {
		memset( &str_base, '\0', sizeof(str_base) );
		snprintf_P( str_base, sizeof(str_base), PSTR("D%d\0"), ch );
		p = strstr( (const char *)&pv_gprsRxCbuffer.buffer, str_base);
		if ( p != NULL ) {
			memset(localStr,'\0',sizeof(localStr));
			memcpy(localStr,p,sizeof(localStr));
			stringp = localStr;
			tk_name = strsep(&stringp,delim);		//D0
			tk_name = strsep(&stringp,delim);		//DIN0
			tk_type = strsep(&stringp,delim);		//NORMAL

			dinputs_config_channel( ch,tk_name ,tk_type );

			if ( systemVars.debug == DEBUG_GPRS ) {
				xprintf_P( PSTR("GPRS: Reconfig D%d\r\n\0"), ch);
			}
			save_flag = true;
		}
	}

	if ( save_flag ) {
		u_save_params_in_NVMEE();
	}

}
//------------------------------------------------------------------------------------
static void pv_init_reconfigure_params_counters(void)
{
	//	PLOAD=CLASS:COUNTER;C0:CNT0,1.0,15,1000,0;C1:X,1.0,10,100,1;
	//  PLOAD=CLASS:COUNTER;C0:CNT0,1.0,15,1000,0;C1:X,1.0,10,100,1;

char *p = NULL;
char localStr[32] = { 0 };
char *stringp = NULL;
char *tk_name = NULL;
char *tk_magpp = NULL;
char *tk_pwidth = NULL;
char *tk_period = NULL;
char *tk_speed = NULL;
char *delim = ",=:;><";
bool save_flag = false;
uint8_t ch;
char str_base[8];

	// C?
	for (ch=0; ch < NRO_COUNTERS; ch++ ) {
		memset( &str_base, '\0', sizeof(str_base) );
		snprintf_P( str_base, sizeof(str_base), PSTR("C%d\0"), ch );
		p = strstr( (const char *)&pv_gprsRxCbuffer.buffer, str_base);
		if ( p != NULL ) {
			memset(localStr,'\0',sizeof(localStr));
			memcpy(localStr,p,sizeof(localStr));
			stringp = localStr;
			tk_name = strsep(&stringp,delim);		//C0
			tk_name = strsep(&stringp,delim);		//name
			tk_magpp = strsep(&stringp,delim);		//magpp
			tk_pwidth = strsep(&stringp,delim);
			tk_period = strsep(&stringp,delim);
			tk_speed = strsep(&stringp,delim);

			counters_config_channel( ch,tk_name ,tk_magpp, tk_pwidth, tk_period, tk_speed );

			if ( systemVars.debug == DEBUG_GPRS ) {
				xprintf_P( PSTR("GPRS: Reconfig C%d\r\n\0"), ch);
			}
			save_flag = true;
		}
	}

	if ( save_flag ) {
		u_save_params_in_NVMEE();
	}

	// Necesito resetearme para tomar la nueva configuracion.
	//f_reset = true;

}
//------------------------------------------------------------------------------------
static void pv_init_reconfigure_params_range(void)
{
	// TYPE=INIT&PLOAD=CLASS:RANGE;R0:DIST;

char *p = NULL;
char localStr[32] = { 0 };
char *stringp = NULL;
char *token = NULL;
char *delim = ",=:;><";

	// RANGE
	p = strstr( (const char *)&pv_gprsRxCbuffer.buffer, "R0");
	if ( p != NULL ) {

		// Copio el mensaje enviado a un buffer local porque la funcion strsep lo modifica.
		memset( &localStr, '\0', sizeof(localStr) );
		memcpy(localStr,p,sizeof(localStr));
		stringp = localStr;
		token = strsep(&stringp,delim);		// R0
		token = strsep(&stringp,delim);		// Range name
		range_config(token);

		if ( systemVars.debug == DEBUG_GPRS ) {
			xprintf_P( PSTR("GPRS: Reconfig RANGE\r\n\0"));
		}

		u_save_params_in_NVMEE();
	}

}
//------------------------------------------------------------------------------------
static void pv_init_reconfigure_params_psensor(void)
{

	//	La linea recibida trae: PLOAD=CLASS:COUNTER;PS0:PSENS,1480,6200,0.0,28.5,0.0:

char *p = NULL;
char localStr[48] = { 0 };
char *stringp = NULL;
char *tk_name = NULL;
char *tk_countMin = NULL;
char *tk_countMax = NULL;
char *tk_pMin = NULL;
char *tk_pMax = NULL;
char *tk_offset = NULL;
char *delim = ",=:;><";

	if ( systemVars.debug == DEBUG_GPRS ) {
		xprintf_P( PSTR("GPRS: Reconfig PSENSOR\r\n\0"));
	}

	p = strstr( (const char *)&pv_gprsRxCbuffer.buffer, "PS0:");
	if ( p != NULL ) {
		memset( &localStr, '\0', sizeof(localStr) );
		memcpy(localStr,p,sizeof(localStr));

		stringp = localStr;
		tk_name = strsep(&stringp,delim);		// PS0:
		tk_name = strsep(&stringp,delim);		// PSENS
		tk_countMin = strsep(&stringp,delim);	// 1480
		tk_countMax  = strsep(&stringp,delim);	// 6200
		tk_pMin  = strsep(&stringp,delim);		// 0.0
		tk_pMax  = strsep(&stringp,delim);		// 28.5
		tk_offset  = strsep(&stringp,delim);	// 0.0

		psensor_config(tk_name, tk_countMin, tk_countMax, tk_pMin, tk_pMax, tk_offset );

		if ( systemVars.debug == DEBUG_GPRS ) {
			xprintf_P( PSTR("GPRS: Reconfig PSENSOR\r\n\0"));
		}

		u_save_params_in_NVMEE();

	}

}
//------------------------------------------------------------------------------------
static void pv_init_reconfigure_params_app_A(void)
{

	// Aplicacion ALARMAS
#ifdef APLICACION_ALARMAS_PPOT
	pv_init_reconfigure_app_plantapot();
	return;
#endif

	// TYPE=INIT&PLOAD=CLASS:APP;AP0:OFF;
	if ( strstr( (const char *)&pv_gprsRxCbuffer.buffer, "AP0:OFF") != NULL ) {
		pv_init_reconfigure_app_off();
		return;
	}

	// TYPE=INIT&PLOAD=CLASS:APP;AP0:CONSIGNA,1230,0940;
	if ( strstr( (const char *)&pv_gprsRxCbuffer.buffer, "AP0:CONSIGNA") != NULL ) {
		pv_init_reconfigure_app_consigna();
		return;
	}

	// // TYPE=INIT&PLOAD=CLASS:APP;AP0:PERFORACION;
	if ( strstr( (const char *)&pv_gprsRxCbuffer.buffer, "AP0:PERFORACION") != NULL ) {
		pv_init_reconfigure_app_perforacion();
		return;
	}

	// TYPE=INIT&PLOAD=CLASS:APP;AP0:TANQUE
	if ( strstr( (const char *)&pv_gprsRxCbuffer.buffer, "AP0:TANQUE") != NULL ) {
		pv_init_reconfigure_app_tanque();
		return;
	}

	// AP0:PLANTAPOT
	if ( strstr( (const char *)&pv_gprsRxCbuffer.buffer, "AP0:PLANTAPOT") != NULL ) {
		pv_init_reconfigure_app_plantapot();
	}

}
//------------------------------------------------------------------------------------
static void pv_init_reconfigure_params_app_B(void)
{

	// TYPE=INIT&PLOAD=CLASS:APP_B;SMS01:111111,1;SMS02:2222222,2;SMS03:3333333,3;SMS04:4444444,1;...SMS09:9999999,3

char *p = NULL;
char localStr[32] = { 0 };
char *stringp = NULL;
char *tk_nro= NULL;
char *tk_level= NULL;
char *delim = ",=:;><";
uint8_t i;
char id[2];
char str_base[8];

	// SMS?
	for (i=0; i < MAX_NRO_SMS_ALARMAS; i++ ) {

		memset( &str_base, '\0', sizeof(str_base) );
		snprintf_P( str_base, sizeof(str_base), PSTR("SMS0%d\0"), i );
		//xprintf_P( PSTR("DEBUG str_base: %s\r\n\0"), str_base);
		p = strstr( (const char *)&pv_gprsRxCbuffer.buffer, str_base);
		//xprintf_P( PSTR("DEBUG str_p: %s\r\n\0"), p);
		if ( p != NULL ) {
			memset(localStr,'\0',sizeof(localStr));
			memcpy(localStr,p,sizeof(localStr));
			stringp = localStr;
			//xprintf_P( PSTR("DEBUG local_str: %s\r\n\0"), localStr );
			tk_nro = strsep(&stringp,delim);		//SMS0x
			tk_nro = strsep(&stringp,delim);		//09111111
			tk_level = strsep(&stringp,delim);		//1

			id[0] = '0' + i;
			id[1] = '\0';

			//xprintf_P( PSTR("DEBUG SMS: ID:%s, NRO=%s, LEVEL=%s\r\n\0"), id, tk_nro,tk_level);
			appalarma_config("SMS", id, tk_nro, tk_level, NULL );

			if ( systemVars.debug == DEBUG_APLICACION ) {
				xprintf_P( PSTR("GPRS: Reconfig SMS0%d\r\n\0"), i);
			}
		}
	}

	u_save_params_in_NVMEE();

}
//------------------------------------------------------------------------------------
static void pv_init_reconfigure_params_app_C(void)
{
	// TYPE=INIT&PLOAD=CLASS:APP_C;CH00:V1_INF,V1_SUP,V1_INF,V2_SUP,V3_INF,V3_SUP;CH01:V1_INF,V1_SUP,V1_INF,V2_SUP,V3_INF,V3_SUP;..

char *p = NULL;
char localStr[32] = { 0 };
char *stringp = NULL;
char *tk_V1_INF = NULL;
char *tk_V1_SUP = NULL;
char *tk_V2_INF = NULL;
char *tk_V2_SUP = NULL;
char *tk_V3_INF = NULL;
char *tk_V3_SUP = NULL;
char *delim = ",=:;><";
uint8_t i;
char id[2];
char str_base[8];

	// LEVELS?
	for (i=0; i < NRO_CANALES_ALM; i++ ) {

		memset( &str_base, '\0', sizeof(str_base) );
		snprintf_P( str_base, sizeof(str_base), PSTR("CH%d\0"), i );

		//xprintf_P( PSTR("DEBUG str_base: %s\r\n\0"), str_base);

		p = strstr( (const char *)&pv_gprsRxCbuffer.buffer, str_base);

		//xprintf_P( PSTR("DEBUG str_p: %s\r\n\0"), p);
		if ( p != NULL ) {
			memset(localStr,'\0',sizeof(localStr));
			memcpy(localStr,p,sizeof(localStr));
			stringp = localStr;
			//xprintf_P( PSTR("DEBUG local_str: %s\r\n\0"), localStr );
			tk_V1_INF = strsep(&stringp,delim);		//CH0x

			tk_V1_INF = strsep(&stringp,delim);
			tk_V1_SUP = strsep(&stringp,delim);
			tk_V2_INF = strsep(&stringp,delim);
			tk_V2_SUP = strsep(&stringp,delim);
			tk_V3_INF = strsep(&stringp,delim);
			tk_V3_SUP = strsep(&stringp,delim);

			id[0] = '0' + i;
			id[1] = '\0';

			//xprintf_P( PSTR("DEBUG LEVELS: ID:%s\r\n\0"), id );

			appalarma_config("NIVEL", id, "1", "INF", tk_V1_INF );
			appalarma_config("NIVEL", id, "1", "SUP", tk_V1_SUP );
			appalarma_config("NIVEL", id, "2", "INF", tk_V2_INF );
			appalarma_config("NIVEL", id, "2", "SUP", tk_V2_SUP );
			appalarma_config("NIVEL", id, "3", "INF", tk_V3_INF );
			appalarma_config("NIVEL", id, "3", "SUP", tk_V3_SUP );

			if ( systemVars.debug == DEBUG_APLICACION ) {
				xprintf_P( PSTR("GPRS: Reconfig ALM_CH 0%d\r\n\0"), i);
			}
		}
	}

	u_save_params_in_NVMEE();
}
//------------------------------------------------------------------------------------
static void pv_init_reconfigure_app_off(void)
{
	// TYPE=INIT&PLOAD=CLASS:APP;AP0:OFF;

	systemVars.aplicacion = APP_OFF;
	u_save_params_in_NVMEE();
	//f_reset = true;

	if ( systemVars.debug == DEBUG_GPRS ) {
		xprintf_P( PSTR("GPRS: Reconfig APLICACION:OFF\r\n\0"));
	}
}
//------------------------------------------------------------------------------------
static void pv_init_reconfigure_app_consigna(void)
{
	// TYPE=INIT&PLOAD=CLASS:APP;AP0:CONSIGNA,1230,0940;

char *p = NULL;
char localStr[32] = { 0 };
char *stringp = NULL;
char *tk_cons_dia = NULL;
char *tk_cons_noche = NULL;
char *delim = ",=:;><";

	systemVars.aplicacion = APP_CONSIGNA;

	memset( &localStr, '\0', sizeof(localStr) );
	p = strstr( (const char *)&pv_gprsRxCbuffer.buffer, "AP0:CONSIGNA");
	memcpy(localStr,p,sizeof(localStr));

	stringp = localStr;
	tk_cons_dia = strsep(&stringp,delim);	// AP0
	tk_cons_dia = strsep(&stringp,delim);	// CONSIGNA
	tk_cons_dia = strsep(&stringp,delim);	// 1230
	tk_cons_noche = strsep(&stringp,delim); // 0940
	consigna_config(tk_cons_dia, tk_cons_noche );
	u_save_params_in_NVMEE();
	//f_reset = true;

	if ( systemVars.debug == DEBUG_GPRS ) {
		xprintf_P( PSTR("GPRS: Reconfig APLICACION:CONSIGNA (%s,%s)\r\n\0"),tk_cons_dia, tk_cons_noche);
	}

}
//------------------------------------------------------------------------------------
static void pv_init_reconfigure_app_perforacion(void)
{
	// TYPE=INIT&PLOAD=CLASS:APP;AP0:PERFORACION;

	systemVars.aplicacion = APP_PERFORACION;
	u_save_params_in_NVMEE();
	//f_reset = true;

	if ( systemVars.debug == DEBUG_GPRS ) {
		xprintf_P( PSTR("GPRS: Reconfig APLICACION:PERFORACION\r\n\0"));
	}

}
//------------------------------------------------------------------------------------
static void pv_init_reconfigure_app_tanque(void)
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
static void pv_init_reconfigure_app_plantapot(void)
{

	// Como quedan aun 2 frames, no reseteo.
	systemVars.aplicacion = APP_PLANTAPOT;
	u_save_params_in_NVMEE();

	if ( systemVars.debug == DEBUG_GPRS ) {
		xprintf_P( PSTR("GPRS: Reconfig APLICACION:PLANTAPOT\r\n\0"));
	}
}
//------------------------------------------------------------------------------------


