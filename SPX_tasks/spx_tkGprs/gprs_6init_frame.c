/*
 * sp5KV5_tkGprs_init_frame.c
 *
 *  Created on: 27 de abr. de 2017
 *      Author: pablo
 */

#include "gprs.h"

static t_frame_responses pv_init_process_response(void);
static void pv_init_process_server_clock(void);
static void pv_init_reconfigure_params(void);
static uint8_t pv_init_config_dlg_id(void);
static uint8_t pv_init_config_pwrSave(void);
static uint8_t pv_init_config_timerPoll(void);
static uint8_t pv_init_config_timerDial(void);
static uint8_t pv_init_config_digitalCh(uint8_t channel);
static uint8_t pv_init_config_analogCh(uint8_t channel);
static uint8_t pv_init_config_counterCh(uint8_t channel);
static uint8_t pv_init_config_rangeMeter(void);
static uint8_t pv_init_config_doutputs(void);
static uint8_t pv_init_config_consignas(void);
static uint8_t pv_init_config_piloto(void);
static uint8_t pv_init_config_default(void);

// La tarea no puede demorar mas de 180s.
#define WDG_GPRS_TO_INIT	180

//------------------------------------------------------------------------------------
bool st_gprs_init_frame(void)
{
	// Debo mandar el frame de init al server, esperar la respuesta, analizarla
	// y reconfigurarme.
	// Intento 3 veces antes de darme por vencido.
	// El socket puede estar abierto o cerrado. Lo debo determinar en c/caso y
	// si esta cerrado abrirlo.
	// Mientras espero la respuesta debo monitorear que el socket no se cierre

uint8_t intentos;
bool exit_flag = bool_RESTART;

// Entry:

	GPRS_stateVars.state = G_INIT_FRAME;

	// En open_socket uso la IP del GPRS_stateVars asi que antes debo copiarla.
	strcpy( GPRS_stateVars.server_ip_address, systemVars.gprs_conf.server_ip_address );

	ctl_watchdog_kick(WDG_GPRSTX, WDG_GPRS_TO_INIT );

	xprintf_P( PSTR("GPRS: iniframe.\r\n\0" ));

	// Intenteo MAX_INIT_TRYES procesar correctamente el INIT
	for ( intentos = 0; intentos < MAX_INIT_TRYES; intentos++ ) {

		if ( u_gprs_send_frame( INIT_FRAME ) ) {

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
				// Aqui es que anduvo todo bien y debo salir para pasar al modo DATA
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
// FUNCIONES PRIVADAS
//------------------------------------------------------------------------------------
static t_frame_responses pv_init_process_response(void)
{

	// Espero la respuesta al frame de INIT.
	// Si la recibo la proceso.
	// Salgo por timeout 10s o por socket closed.

uint8_t timeout;
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

			if ( u_gprs_check_response("INIT_OK") ) {	// Respuesta correcta
				// Borro la causa del reset
				wdg_resetCause = 0x00;
				pv_init_reconfigure_params();
				exit_code = FRAME_OK;
				goto EXIT;
			}

			if ( u_gprs_check_response("NOT_ALLOWED") ) {	// Datalogger esta usando un script incorrecto
				xprintf_P( PSTR("GPRS: SCRIPT ERROR !!.\r\n\0" ));
				exit_code = FRAME_NOT_ALLOWED;
				goto EXIT;
			}

			if ( u_gprs_check_response("DLGID") ) {	// Reconfiguro el DLGID.
				pv_init_config_dlg_id();
				u_save_params_in_NVMEE();
				exit_code = FRAME_RETRY;
				goto EXIT;
			}
		}

	}

// Exit:
EXIT:

	return(exit_code);

}
//------------------------------------------------------------------------------------
static void pv_init_reconfigure_params(void)
{

uint8_t saveFlag = 0;

	// Proceso la respuesta del INIT para reconfigurar los parametros
	pv_init_process_server_clock();

	// En el protocolo nuevo, el dlgid se reconfigura en SCAN
	//saveFlag += pv_init_config_dlg_id();

	saveFlag += pv_init_config_timerPoll();
	saveFlag += pv_init_config_timerDial();
	saveFlag += pv_init_config_pwrSave();

	// RangeMeter
	saveFlag += pv_init_config_rangeMeter();

	// Doutputs
	saveFlag += pv_init_config_doutputs();
	// Consignas
	saveFlag += pv_init_config_consignas();
	// Perforaciones: No se configuran
	// Piloto
	saveFlag += pv_init_config_piloto();

	// Canales analogicos.
	saveFlag += pv_init_config_analogCh(0);
	saveFlag += pv_init_config_analogCh(1);
	saveFlag += pv_init_config_analogCh(2);
	saveFlag += pv_init_config_analogCh(3);
	saveFlag += pv_init_config_analogCh(4);
	if ( spx_io_board == SPX_IO8CH ) {
		saveFlag += pv_init_config_analogCh(5);
		saveFlag += pv_init_config_analogCh(6);
		saveFlag += pv_init_config_analogCh(7);
	}

	// Canales digitales
	saveFlag += pv_init_config_digitalCh(0);
	saveFlag += pv_init_config_digitalCh(1);
	if ( spx_io_board == SPX_IO8CH ) {
		saveFlag += pv_init_config_digitalCh(2);
		saveFlag += pv_init_config_digitalCh(3);
		saveFlag += pv_init_config_digitalCh(4);
		saveFlag += pv_init_config_digitalCh(5);
		saveFlag += pv_init_config_digitalCh(6);
		saveFlag += pv_init_config_digitalCh(7);
	}

	// Canales de contadores
	saveFlag += pv_init_config_counterCh(0);
	saveFlag += pv_init_config_counterCh(1);

	// DEFAULT=dlgid,NONE|SPY|UTE|OSE
	saveFlag += pv_init_config_default();

	if ( saveFlag > 0 ) {

		u_save_params_in_NVMEE();

		// DEBUG & LOG
		if ( systemVars.debug ==  DEBUG_GPRS ) {
			xprintf_P( PSTR("GPRS: Save params OK\r\n\0"));
		}
	}

}
//------------------------------------------------------------------------------------
static void pv_init_process_server_clock(void)
{
/* Extraigo el srv clock del string mandado por el server y si el drift con la hora loca
 * es mayor a 5 minutos, ajusto la hora local
 * La linea recibida es del tipo: <h1>INIT_OK:CLOCK=1402251122:PWRM=DISC:</h1>
 *
 */

char *p;
char localStr[32];
char *stringp;
char *token;
char *delim = ",=:><";
char rtcStr[12];
uint8_t i;
char c;
RtcTimeType_t rtc;
int8_t xBytes;

	p = strstr( (const char *)&pv_gprsRxCbuffer.buffer, "CLOCK");
	if ( p  == NULL ) {
		return;
	}

	// Copio el mensaje enviado a un buffer local porque la funcion strsep lo modifica.
	memset(localStr,'\0',32);
	memcpy(localStr,p,sizeof(localStr));

	stringp = localStr;
	token = strsep(&stringp,delim);			// CLOCK

	token = strsep(&stringp,delim);			// rtc
	memset(rtcStr, '\0', sizeof(rtcStr));
	memcpy(rtcStr,token, sizeof(rtcStr));	// token apunta al comienzo del string con la hora
	for ( i = 0; i<12; i++) {
		c = *token;
		rtcStr[i] = c;
		c = *(++token);
		if ( c == '\0' )
			break;

	}

	RTC_str2rtc(rtcStr, &rtc);	// Convierto el string YYMMDDHHMM a RTC.
	xBytes = RTC_write_dtime(&rtc);		// Grabo el RTC
	if ( xBytes == -1 )
		xprintf_P(PSTR("ERROR: I2C:RTC:pv_process_server_clock\r\n\0"));

	if ( systemVars.debug == DEBUG_GPRS ) {
		xprintf_P( PSTR("GPRS: Update rtc to: %s\r\n\0"), rtcStr );
	}

}
//------------------------------------------------------------------------------------
static uint8_t pv_init_config_dlg_id(void)
{
	//	La linea recibida es del tipo: <h1>INIT_OK:CLOCK=1402251122:DLGID=TH001:PWRM=DISC:</h1>

char *p;
uint8_t ret = 0;
char localStr[32];
char *stringp;
char *token;
char *delim = ",=:><";

	p = strstr( (const char *)&pv_gprsRxCbuffer.buffer, "DLGID");
	if ( p == NULL ) {
		goto quit;
	}

	// Copio el mensaje enviado a un buffer local porque la funcion strsep lo modifica.
	memset(localStr,'\0',32);
	memcpy(localStr,p,sizeof(localStr));

	stringp = localStr;
	token = strsep(&stringp,delim);	// DLGID
	token = strsep(&stringp,delim);	// TH001

	if ( token == NULL ) {
		xprintf_P( PSTR("GPRS: ERROR Reconfig DLGID !!\r\n\0"));
		goto quit;
	}

	memset(systemVars.gprs_conf.dlgId,'\0', sizeof(systemVars.gprs_conf.dlgId) );
	strncpy(systemVars.gprs_conf.dlgId, token, DLGID_LENGTH);

	ret = 1;
	if ( systemVars.debug == DEBUG_GPRS ) {
		xprintf_P( PSTR("GPRS: Reconfig DLGID\r\n\0"));
	}

quit:

	return(ret);
}
//------------------------------------------------------------------------------------
static uint8_t pv_init_config_timerPoll(void)
{
//	La linea recibida es del tipo: <h1>INIT_OK:CLOCK=1402251122:TPOLL=600:</h1>

char *p;
char localStr[32];
char *stringp;
char *tk_timerPoll;
char *delim = ",=:><";

	p = strstr( (const char *)&pv_gprsRxCbuffer.buffer, "TPOLL");
	if ( p == NULL ) {
		return(0);
	}

	// Copio el mensaje enviado a un buffer local porque la funcion strsep lo modifica.
	memset(localStr,'\0',32);
	memcpy(localStr,p,sizeof(localStr));

	stringp = localStr;
	tk_timerPoll = strsep(&stringp,delim);	// TPOLL
	tk_timerPoll = strsep(&stringp,delim);	// timerPoll
	u_config_timerpoll(tk_timerPoll);

	if ( systemVars.debug == DEBUG_GPRS ) {
		xprintf_P( PSTR("GPRS: Reconfig TPOLL\r\n\0"));
	}

	return(1);
}
//------------------------------------------------------------------------------------
static uint8_t pv_init_config_timerDial(void)
{
	//	La linea recibida es del tipo: <h1>INIT_OK:CLOCK=1402251122:TPOLL=600:TDIAL=10300:PWRM=DISC:CD=1230:CN=0530</h1>

char *p;
char localStr[32];
char *stringp;
char *tk_timerDial;
char *delim = ",=:><";

	p = strstr( (const char *)&pv_gprsRxCbuffer.buffer, "TDIAL");
	if ( p == NULL ) {
		return(0);
	}

	// Copio el mensaje enviado a un buffer local porque la funcion strsep lo modifica.
	memset(localStr,'\0',32);
	memcpy(localStr,p,sizeof(localStr));

	stringp = localStr;
	strsep(&stringp,delim);	// TDIAL
	tk_timerDial = strsep(&stringp,delim);	// timerDial
	u_gprs_config_timerdial(tk_timerDial);

	if ( systemVars.debug == DEBUG_GPRS ) {
		xprintf_P( PSTR("GPRS: Reconfig TDIAL\r\n\0"));
	}

	return(1);
}
//------------------------------------------------------------------------------------
static uint8_t pv_init_config_pwrSave(void)
{
//	La linea recibida trae: PWRS=ON,2230,0600:
//  Las horas estan en formato HHMM.

char localStr[32];
char *stringp;
char *tk_pws_modo, *tk_pws_start, *tk_pws_end;
char *delim = ",=:><";
char *p;

	p = strstr( (const char *)&pv_gprsRxCbuffer.buffer, "PWRS");
	if ( p == NULL ) {
		return(0);
	}

	// Copio el mensaje enviado a un buffer local porque la funcion strsep lo modifica.
	memset(localStr,'\0',32);
	memcpy(localStr,p,sizeof(localStr));

	stringp = localStr;
	tk_pws_modo = strsep(&stringp,delim);		//PWRS
	tk_pws_modo = strsep(&stringp,delim);		// modo ( ON / OFF ).
	tk_pws_start = strsep(&stringp,delim);		// startTime
	tk_pws_end  = strsep(&stringp,delim); 		// endTime
	u_gprs_configPwrSave(tk_pws_modo, tk_pws_start, tk_pws_end );
	if ( systemVars.debug == DEBUG_GPRS ) {
		xprintf_P( PSTR("GPRS: Reconfig PWRSAVE\r\n\0"));
	}

	return(1);
}
//--------------------------------------------------------------------------------------
static uint8_t pv_init_config_rangeMeter(void)
{
	// ?DIST=ON{OFF}

char *p;
uint8_t ret = 0;

	p = strstr( (const char *)&pv_gprsRxCbuffer.buffer, "DIST");
	// No vino el parametro
	if ( p == NULL ) {
		goto quit;
	}

	// Si vino el parametro
	if ( strstr( (const char *)&pv_gprsRxCbuffer.buffer, "DIST=ON") ) {
		range_config("ON");
		ret = 1;
	} else 	if ( strstr( (const char *)&pv_gprsRxCbuffer.buffer, "DIST=OFF") ) {
		range_config("OFF");
		ret = 1;
	}

	if ( ( systemVars.debug == DEBUG_GPRS ) && ( ret == 1 ) ) {
		xprintf_P( PSTR("GPRS: Reconfig RANGEMETER\r\n\0"));
	}

quit:

	return(ret);

}
//--------------------------------------------------------------------------------------
static uint8_t pv_init_config_analogCh(uint8_t channel)
{
//	La linea recibida es del tipo:
//	<h1>INIT_OK:CLOCK=1402251122:TPOLL=600:TDIAL=10300:PWRM=DISC:A0=pA,0,20,0,6:A1=pB,0,20,0,10:A2=pC,0,20,0,10:D0=q0,1.00:D1=q1,1.00</h1>

char localStr[32];
char *stringp;
char *delim = ",=:><";
char *tk_id, *tk_name,*tk_iMin,*tk_iMax,*tk_mMin,*tk_mMax;

	switch (channel) {
	case 0:
		stringp = strstr((const char *)&pv_gprsRxCbuffer.buffer, "A0=");
		break;
	case 1:
		stringp = strstr((const char *)&pv_gprsRxCbuffer.buffer, "A1=");
		break;
	case 2:
		stringp = strstr((const char *)&pv_gprsRxCbuffer.buffer, "A2=");
		break;
	case 3:
		stringp = strstr((const char *)&pv_gprsRxCbuffer.buffer, "A3=");
		break;
	case 4:
		stringp = strstr((const char *)&pv_gprsRxCbuffer.buffer, "A4=");
		break;
	case 5:
		stringp = strstr((const char *)&pv_gprsRxCbuffer.buffer, "A5=");
		break;
	case 6:
		stringp = strstr((const char *)&pv_gprsRxCbuffer.buffer, "A6=");
		break;
	case 7:
		stringp = strstr((const char *)&pv_gprsRxCbuffer.buffer, "A7=");
		break;
	default:
		return(0);
		break;
	}

	if ( stringp == NULL ) {
		return(0);
	}

	// Copio el mensaje enviado a un buffer local porque la funcion strsep lo modifica.
	memset(localStr,'\0',32);
	memcpy(localStr,stringp,31);

	stringp = localStr;
	tk_id = strsep(&stringp,delim);			//A0
	tk_name = strsep(&stringp,delim);		//name
	tk_iMin = strsep(&stringp,delim);		//iMin
	tk_iMax = strsep(&stringp,delim);		//iMax
	tk_mMin = strsep(&stringp,delim);		//mMin
	tk_mMax = strsep(&stringp,delim);		//mMax

	ainputs_config_channel( channel, tk_name ,tk_iMin, tk_iMax, tk_mMin, tk_mMax );
	if ( systemVars.debug == DEBUG_GPRS ) {
		xprintf_P( PSTR("GPRS: Reconfig A%d\r\n\0"), channel);
	}

	return(1);
}
//--------------------------------------------------------------------------------------
static uint8_t pv_init_config_digitalCh(uint8_t channel)
{

//	La linea recibida es del tipo:
//	<h1>INIT_OK:CLOCK=1402251122:TPOLL=600:TDIAL=10300:PWRM=DISC:A0=pA,0,20,0,6:A1=pB,0,20,0,10:A2=pC,0,20,0,10:D0=C,q0,1.00:D1=L,q1</h1>
//
// D0=C,q0,1.00:D1=L,q1
//
char localStr[32];
char *stringp;
char *delim = ",=:><";
char *tk_id, *tk_name;

	switch (channel) {
	case 0:
		stringp = strstr( (const char *)&pv_gprsRxCbuffer.buffer, "D0=");
		break;
	case 1:
		stringp = strstr( (const char *)&pv_gprsRxCbuffer.buffer, "D1=");
		break;
	case 2:
		stringp = strstr( (const char *)&pv_gprsRxCbuffer.buffer, "D2=");
		break;
	case 3:
		stringp = strstr( (const char *)&pv_gprsRxCbuffer.buffer, "D3=");
		break;
	case 4:
		stringp = strstr( (const char *)&pv_gprsRxCbuffer.buffer, "D4=");
		break;
	case 5:
		stringp = strstr( (const char *)&pv_gprsRxCbuffer.buffer, "D5=");
		break;
	case 6:
		stringp = strstr( (const char *)&pv_gprsRxCbuffer.buffer, "D6=");
		break;
	case 7:
		stringp = strstr( (const char *)&pv_gprsRxCbuffer.buffer, "D7=");
		break;
	default:
		return(0);
		break;
	}

	if ( stringp == NULL ) {
		return(0);
	}

	// Copio el mensaje enviado ( solo 32 bytes ) a un buffer local porque la funcion strsep lo modifica.
	memset(localStr,'\0',32);
	memcpy(localStr,stringp, 31);

	stringp = localStr;
	tk_id = strsep(&stringp,delim);	//D0
	tk_name = strsep(&stringp,delim);	//name

	dinputs_config_channel( channel, tk_name );

	if ( systemVars.debug == DEBUG_GPRS ) {
		xprintf_P( PSTR("GPRS: Reconfig D%d\r\n\0"), channel);
	}
	return(1);

}
//--------------------------------------------------------------------------------------
static uint8_t pv_init_config_counterCh(uint8_t channel)
{

//	La linea recibida es del tipo:
//	<h1>INIT_OK:CLOCK=1402251122:TPOLL=600:TDIAL=10300:PWRM=DISC:A0=pA,0,20,0,6:A1=pB,0,20,0,10:A2=pC,0,20,0,10:D0=C,q0,1.00:D1=L,q1</h1>
//
//  C0=q0,1.00:C1=q1,1.45
//
char localStr[32];
char *stringp;
char *delim = ",=:><";
char *tk_id = NULL;
char *tk_name = NULL;
char *tk_magPP = NULL;
char *tk_pwidth = NULL;
char *tk_period = NULL;
char *tk_speed = NULL;

	switch (channel) {
	case 0:
		stringp = strstr( (const char *)&pv_gprsRxCbuffer.buffer, "C0=");
		break;
	case 1:
		stringp = strstr( (const char *)&pv_gprsRxCbuffer.buffer, "C1=");
		break;
	default:
		return(0);
		break;
	}

	if ( stringp == NULL ) {
		return(0);
	}

	// Copio el mensaje enviado ( solo 32 bytes ) a un buffer local porque la funcion strsep lo modifica.
	memset(localStr,'\0',32);
	memcpy(localStr,stringp, 31);

	stringp = localStr;
	tk_id = strsep(&stringp,delim);		// C0
	tk_name = strsep(&stringp,delim);	//name
	tk_magPP = strsep(&stringp,delim);	//magPP
	tk_pwidth = strsep(&stringp,delim);	//pulse width
	tk_period = strsep(&stringp,delim);	//period
	tk_speed = strsep(&stringp,delim);	//speed

	counters_config_channel ( channel, tk_name, tk_magPP, tk_pwidth, tk_period, tk_speed );

	if ( systemVars.debug == DEBUG_GPRS ) {
		xprintf_P( PSTR("GPRS: Reconfig C%d\r\n\0"), channel);
	}

	return(1);

}
//--------------------------------------------------------------------------------------
static uint8_t pv_init_config_doutputs(void)
{
	//	La linea recibida es del tipo: <h1>INIT_OK:DOUTMODE=doutmode</h1>

char *p;
char localStr[32];
char *stringp;
char *tk_doutmode;
char *delim = ",=:><";
uint8_t ret = 0;

	p = strstr( (const char *)&pv_gprsRxCbuffer.buffer, "DOUTMODE");
	if ( p == NULL ) {
		goto quit;
	}

	// Copio el mensaje enviado a un buffer local porque la funcion strsep lo modifica.
	memset(localStr,'\0',32);
	memcpy(localStr,p,sizeof(localStr));

	stringp = localStr;
	strsep(&stringp,delim);					// DOUTMODE
	tk_doutmode = strsep(&stringp,delim);	// doutmode
	if ( doutputs_config_mode( tk_doutmode ) == true ) {
		ret = 1;
	}

	if ( ( systemVars.debug == DEBUG_GPRS ) && ( ret == 1 ) ) {
		xprintf_P( PSTR("GPRS: Reconfig DOUTMODE\r\n\0"));
	}

quit:

	return(ret);
}
//--------------------------------------------------------------------------------------
static uint8_t pv_init_config_consignas(void)
{
	// La linea recibida es del tipo:
	// <h1>INIT_OK:CONS=param1,param2:</h1>
	// param1, param2 son las horas de la consigna diurna y la nocturna
	// Las horas estan en formato HHMM.

char localStr[32];
char *stringp;
char *tk_hhmm1, *tk_hhmm2;
char *delim = ",=:><";
char *p;
uint8_t ret = 0;


	p = strstr( (const char *)&pv_gprsRxCbuffer.buffer, "CONS");

	// No vino el parametro
	if ( p == NULL )
		goto quit;

	// Si vino el parametro
	// Copio el mensaje enviado a un buffer local porque la funcion strsep lo modifica.
	memset(localStr,'\0',32);
	memcpy(localStr,p,sizeof(localStr));

	stringp = localStr;
	tk_hhmm1 = strsep(&stringp,delim);	// CONS
	tk_hhmm1 = strsep(&stringp,delim);	// shhmm consigna_diurna
	tk_hhmm2 = strsep(&stringp,delim); 	// shhmm consigna_nocturna

	if ( doutputs_config_consignas(tk_hhmm1, tk_hhmm2) ) {
		ret = 1;
	}

	if ( ( systemVars.debug == DEBUG_GPRS ) && ( ret == 1 ) ) {
		xprintf_P( PSTR("GPRS: Reconfig CONSIGNAS\r\n\0"));
	}

quit:

	return(ret);
}
//--------------------------------------------------------------------------------------
static uint8_t pv_init_config_piloto(void)
{
	// La linea recibida es del tipo:
	// <h1>INIT_OK:PILOTO=param1,param2,param3:</h1>
	// param1, param2, param3 son: pout,pband,max_steps

char localStr[32];
char *stringp;
char *tk_pout, *tk_pband, *tk_psteps;
char *delim = ",=:><";
char *p;
uint8_t ret = 0;


	p = strstr( (const char *)&pv_gprsRxCbuffer.buffer, "PILOTO");

	// No vino el parametro
	if ( p == NULL )
		goto quit;

	// Si vino el parametro
	// Copio el mensaje enviado a un buffer local porque la funcion strsep lo modifica.
	memset(localStr,'\0',32);
	memcpy(localStr,p,sizeof(localStr));

	stringp = localStr;
	tk_pout = strsep(&stringp,delim);	// PILOTO
	tk_pout = strsep(&stringp,delim);	// pout
	tk_pband = strsep(&stringp,delim); 	// pband
	tk_psteps = strsep(&stringp,delim); 	// psteps

	if ( doutputs_config_piloto(tk_pout, tk_pband, tk_psteps) ) {
		ret = 1;
	}

	if ( ( systemVars.debug == DEBUG_GPRS ) && ( ret == 1 ) ) {
		xprintf_P( PSTR("GPRS: Reconfig PILOTOS\r\n\0"));
	}

quit:

	return(ret);
}
//--------------------------------------------------------------------------------------
static uint8_t pv_init_config_default(void)
{
	// Permite hacer una configuracion por defecto desde el servidor
	//	La linea recibida es del tipo:
	//	<h1>INIT_OK:CLOCK=1402251122:DEFAULT=dlgid,SPY</h1>
	//
char localStr[32];
char *stringp;
char *delim = ",=:><";
char *tk_id, *tk_dlgid, *tk_modo;

	stringp = strstr( (const char *)&pv_gprsRxCbuffer.buffer, "DEFAULT=");
	if ( stringp == NULL ) {
		return(0);
	}

	// Copio el mensaje enviado ( solo 32 bytes ) a un buffer local porque la funcion strsep lo modifica.
	memset(localStr,'\0',32);
	memcpy(localStr,stringp, 31);

	stringp = localStr;
	tk_id = strsep(&stringp,delim);		// DEFAULT
	tk_dlgid = strsep(&stringp,delim);	// DLGID
	tk_modo = strsep(&stringp,delim);	// (NONE|SPY|OSE|UTE)

	u_load_defaults( tk_modo );
	memset(systemVars.gprs_conf.dlgId,'\0', sizeof(systemVars.gprs_conf.dlgId) );
	strncpy(systemVars.gprs_conf.dlgId, tk_dlgid, DLGID_LENGTH);

	xprintf_P( PSTR("GPRS: Reconfig to DEFAULT: dlgid->%s, modo->%s\r\n\0"), tk_dlgid, tk_modo );

	u_save_params_in_NVMEE();

	xprintf_P( PSTR("GPRS: Reset...\r\n\0") );
	vTaskDelay( ( TickType_t)( 2000 / portTICK_RATE_MS ) );
	CCPWrite( &RST.CTRL, RST_SWRST_bm );
	return(0);

}
//--------------------------------------------------------------------------------------

