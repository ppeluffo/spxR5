/*
 * tkComms_07_scan.c
 *
 *  Created on: 6 mar. 2020
 *      Author: pablo
 */

#include <tkComms.h>
#include "spx_tkApp/tkApp.h"

// La tarea no puede demorar mas de 600s.
#define WDG_COMMS_TO_SCAN	WDG_TO600

static void scan_process_response_RECONF(void);

//------------------------------------------------------------------------------------
t_comms_states tkComms_st_scan(void)
{
	/*
	 * El SCAN significa que si alguno de los parametros ( APN / IPserver / DLGID ) esta
	 * en DEFAULT, se procede a tantear hasta configurarlos.
	 * En caso que no se pueda, no se puede seguir por lo que se da un mensaje de error y
	 * se pasa a espera 1H para reintentar.
	 * El concepto de APN e IPserver solo se aplica a GPRS.
	 */

t_comms_states next_state = ST_ENTRY;

#ifdef BETA_TEST
	xprintf_PD( DF_COMMS, PSTR("COMMS: IN st_scan.[%d,%d,%d]\r\n\0"),xCOMMS_stateVars.gprs_prendido, xCOMMS_stateVars.gprs_inicializado,xCOMMS_stateVars.errores_comms);
#else
	xprintf_PD( DF_COMMS, PSTR("COMMS: IN st_scan.\r\n\0"));
#endif

#ifdef MONITOR_STACK
	debug_print_stack_watermarks("7");
#endif

	ctl_watchdog_kick( WDG_COMMS, WDG_COMMS_TO_SCAN );

	if ( xCOMMS_need_scan() == true ) {
		// Necesito descubir los parametros.
		// Puedo demorar hasta 10 minutos  por lo que ajusto el watchdog !!!
		ctl_watchdog_kick( WDG_COMMS, WDG_TO600 );

		// Estoy en un scan: pongo a default el server/port/cpin.
		strncpy_P( sVarsComms.serverScript, PSTR("/cgi-bin/SPY/spy.py\0"),SCRIPT_LENGTH);
		strncpy_P( sVarsComms.server_tcp_port, PSTR("80\0"),PORT_LENGTH	);
		snprintf_P( sVarsComms.simpwd, sizeof( sVarsComms.simpwd), PSTR("%s\0"), SIMPIN_DEFAULT );

		if ( xCOMMS_scan() == true ) {
			// Descubri los parametros. Ya estan en el sVarsComms.
			// Los salvo y salgo a reiniciarme con estos.
			u_save_params_in_NVMEE();

			xprintf_P( PSTR("COMMS: SCAN APN=[%s]\r\n\0"), sVarsComms.apn );
			xprintf_P( PSTR("COMMS: SCAN IP=[%s]\r\n\0"), sVarsComms.server_ip_address );
			xprintf_P( PSTR("COMMS: SCAN DLGID=[%s]\r\n\0"), sVarsComms.dlgId );

			xprintf_P( PSTR("COMMS: Reset to load new configuration...\r\n\0"));
			vTaskDelay( (portTickType)( 2000 / portTICK_RATE_MS ) );
			CCPWrite( &RST.CTRL, RST_SWRST_bm );   /* Issue a Software Reset to initilize the CPU */

		} else {
			// No pude descubrir los parametros. Espero 1H para reintentar.
			sVarsComms.timerDial = 3600;
			next_state = ST_ENTRY;
		}


	} else {
		// No need scan: go ahead to ST_INITFRAME
		next_state = ST_INITFRAME;
	}

	// Checkpoint de SMS's
	xAPP_sms_checkpoint();

#ifdef BETA_TEST
	xprintf_PD( DF_COMMS, PSTR("COMMS: OUT st_scan.[%d,%d,%d](%d)\r\n\0"),xCOMMS_stateVars.gprs_prendido, xCOMMS_stateVars.gprs_inicializado,xCOMMS_stateVars.errores_comms, next_state);
#else
	xprintf_PD( DF_COMMS, PSTR("COMMS: OUT st_scan.\r\n"));
#endif

	return(next_state);

}
//------------------------------------------------------------------------------------
t_send_status xSCAN_FRAME_send(void)
{
	xCOMMS_flush_RX();
	xCOMMS_flush_TX();
	xCOMMS_xbuffer_init();
	// Header, payload
	xfprintf_P(fdFILE, PSTR("GET %s?DLGID=DEFAULT&TYPE=CTL&VER=%s&PLOAD=CLASS:SCAN;UID:%s\0" ), sVarsComms.serverScript, SPX_FW_REV,  NVMEE_readID() );
	// Tail
	xfprintf_P(fdFILE,  PSTR(" HTTP/1.1\r\nHost: www.spymovil.com\r\n\r\n\r\n") );

	if ( xCOMMS_xbuffer_send(DF_COMMS) < 0 ) {
		return(SEND_FAIL);
	}

	return(SEND_OK);

}
//------------------------------------------------------------------------------------
t_responses xSCAN_FRAME_process_response(void)
{

t_responses rsp = rsp_NONE;

	// Recibi un ERROR de respuesta
	if ( xCOMMS_check_response(0, "ERROR") > 0 ) {
		xCOMMS_print_RX_buffer();
		rsp = rsp_ERROR;
		return(rsp);
	}

	if ( xCOMMS_check_response(0, "404 Not Found") > 0 ) {
		xCOMMS_print_RX_buffer();
		rsp = rsp_ERROR;
		return(rsp);
	}

	if ( xCOMMS_check_response(0, "Internal Server Error") > 0 ) {
		xCOMMS_print_RX_buffer();
		rsp = rsp_ERROR;
		return(rsp);
	}

	// Respuesta completa del server
	if ( xCOMMS_check_response(0, "</h1>") > 0 ) {

		xCOMMS_print_RX_buffer();

		if ( xCOMMS_check_response (0, "STATUS:OK") > 0) {
			// Respuesta correcta. El dlgid esta definido en la BD
			rsp = rsp_OK;
			return(rsp);
		}

		if ( xCOMMS_check_response (0, "STATUS:RECONF") > 0) {
			// Respuesta correcta
			// Configure el DLGID correcto y la SERVER_IP usada es la correcta.
			scan_process_response_RECONF();
			rsp = rsp_OK;
			return(rsp);
		}

		if ( xCOMMS_check_response (0, "STATUS:UNDEF") > 0) {
			// Datalogger esta usando un script incorrecto
			xprintf_P( PSTR("COMMS: ERROR SCAN SCRIPT !!.\r\n\0" ));
			rsp = rsp_ERROR;
			return(rsp);
		}

		if ( xCOMMS_check_response (0, "NOTDEFINED") > 0) {
			// Datalogger no definido en la base GDA
			xprintf_P( PSTR("COMMS: ERROR SCAN dlg not defined in BD !!!\r\n\0" ));
			rsp = rsp_ERROR;
			return(rsp);
		}

	}

// Exit:
	// No tuve respuesta aun
	return(rsp);
}
//------------------------------------------------------------------------------------
static void scan_process_response_RECONF(void)
{
	// La linea recibida es del tipo: <h1>TYPE=CTL&PLOAD=CLASS:SCAN;STATUS:RECONF;DLGID:TEST01</h1>
	// Es la respuesta del server a un frame de SCAN.
	// Indica 2 cosas: - El server es el correcto por lo que debo salvar la IP
	//                 - Me pasa el DLGID correcto.


int p1;
char localStr[32] = { 0 };
char *stringp = NULL;
char *token = NULL;
char *delim = ",;:=><";

	p1 = xCOMMS_check_response(0, "DLGID");
	if ( p1 == -1 ) {
		return;
	}

	// Copio el mensaje enviado a un buffer local porque la funcion strsep lo modifica.
	memset(localStr,'\0',32);
	xCOMMS_rxbuffer_copy_to(localStr, p1, sizeof(localStr));

	stringp = localStr;
	token = strsep(&stringp,delim);	// DLGID
	token = strsep(&stringp,delim);	// TH001

	// Copio el dlgid recibido al systemVars.dlgid que esta en el scan_boundle
	memset( sVarsComms.dlgId,'\0', DLGID_LENGTH );
	strncpy(sVarsComms.dlgId, token, DLGID_LENGTH);
	xprintf_P( PSTR("COMMS: SCAN discover DLGID to %s\r\n\0"), sVarsComms.dlgId );
}
//------------------------------------------------------------------------------------



