/*
 * tkComms_09_initframes.c
 *
 *  Created on: 6 mar. 2020
 *      Author: pablo
 */

#include <tkComms.h>

// La tarea no puede demorar mas de 180s.
#define WDG_COMMS_TO_INITFRAME	180

#define MAX_INTENTOS_ENVIAR_INIT_FRAME	4

typedef enum { INIT_AUTH = 0, INIT_GLOBAL, INIT_BASE, INIT_ANALOG, INIT_DIGITAL, INIT_COUNTERS, INIT_RANGE, INIT_PSENSOR, INIT_EXIT   } t_init_frame;
typedef enum { SST_ENTRY = 0, SST_ENVIAR_FRAME, SST_PROCESAR_RESPUESTA, SST_EXIT  } t_inits_sub_state;

static bool process_frame (t_init_frame tipo_init_frame );
static bool send_init_frame(t_init_frame tipo_init_frame );
static bool process_init_response(void);

static bool init_frame_auth(void);
static bool init_frame_global(void);
static bool init_frame_base(void);
static bool init_frame_analog(void);
static bool init_frame_digital(void);
static bool init_frame_counters(void);
static bool init_frame_range(void);
static bool init_frame_psensor(void);

static bool init_reconfigure_params_auth(void);
static bool init_reconfigure_params_global(void);
static bool init_reconfigure_params_base(void);
static bool init_reconfigure_params_analog(void);
static bool init_reconfigure_params_digital(void);
static bool init_reconfigure_params_counters(void);
static bool init_reconfigure_params_psensor(void);
static bool init_reconfigure_params_range(void);

static bool f_send_init_frame_base = false;
static bool f_send_init_frame_analog = false;
static bool f_send_init_frame_digital = false;
static bool f_send_init_frame_counters = false;
static bool f_send_init_frame_range = false;
static bool f_send_init_frame_psensor = false;

//------------------------------------------------------------------------------------
t_comms_states tkComms_st_initframe(void)
{
	/* Estado en que procesa los frames de inits, los transmite y procesa
	 * las respuestas
	 * Si no hay datos para transmitir sale
	 * Si luego de varios reintentos no pudo, sale
	 */

t_comms_states next_state = ST_ENTRY;

	xprintf_PD( DF_COMMS, PSTR("COMMS: IN st_initframe.\r\n\0"));
	xprintf_P( PSTR("COMMS: initframe.\r\n\0"));

	ctl_watchdog_kick(WDG_COMMS, WDG_COMMS_TO_INITFRAME );

	if ( ! init_frame_auth() ) {
		next_state = ST_ENTRY;
		goto EXIT;
	}

	if ( ! init_frame_global() ) {
		next_state = ST_ENTRY;
		goto EXIT;
	}

	if ( ! init_frame_base() ) {
		next_state = ST_ENTRY;
		goto EXIT;
	}

	if ( ! init_frame_analog() )  {
		next_state = ST_ENTRY;
		goto EXIT;
	}

	if ( ! init_frame_digital() ) {
		next_state = ST_ENTRY;
		goto EXIT;
	}

	if ( ! init_frame_counters() ) {
		next_state = ST_ENTRY;
		goto EXIT;
	}

	if ( ! init_frame_range() ) {
		next_state = ST_ENTRY;
		goto EXIT;
	}

	if ( ! init_frame_psensor() ) {
		next_state = ST_ENTRY;
		goto EXIT;
	}

	next_state = ST_DATAFRAME;

	// Proceso las señales:
	if ( xCOMMS_procesar_senales( ST_INITFRAME , &next_state ) )
		goto EXIT;

EXIT:

	xprintf_PD( DF_COMMS, PSTR("COMMS: OUT st_initframe.\r\n\0"));
	return(next_state);
}
//------------------------------------------------------------------------------------
static bool process_frame (t_init_frame tipo_init_frame )
{
	/*
	 * Esta el la funcion que hace el trabajo de mandar un frame , esperar
	 * la respuesta y procesarla.
	 */

uint8_t intentos;

	for( intentos= 0; intentos < MAX_INTENTOS_ENVIAR_INIT_FRAME ; intentos++ )
	{

		if ( send_init_frame(tipo_init_frame) ) {

			if ( process_init_response()) {
				return(true);
			}

		}
	}

	return(false);

}
//------------------------------------------------------------------------------------
static bool send_init_frame(t_init_frame tipo_init_frame )
{
	/*
	 * Intenta enviar un frame de init
	 * Si se genera algun problema debo esperar 3secs antes de reintentar
	 */

uint8_t i = 0;
uint8_t base_cks, an_cks, dig_cks, cnt_cks, range_cks, psens_cks, app_cks;


	// Loop
	for ( i = 0; i < MAX_TRYES_OPEN_COMMLINK; i++ ) {

		if (  xCOMMS_link_status() == LINK_OPEN ) {

			xCOMMS_flush_RX();
			xCOMMS_flush_TX();
			xCOMMS_send_header("INIT");

			switch(tipo_init_frame) {
			case INIT_AUTH:
				xprintf_PVD(  xCOMMS_get_fd(), DF_COMMS, PSTR("&PLOAD=CLASS:AUTH;UID:%s;" ),NVMEE_readID() );
				break;
			case INIT_GLOBAL:
				base_cks = u_base_checksum();
				an_cks = ainputs_checksum();
				dig_cks = dinputs_checksum();
				cnt_cks = counters_checksum();
				range_cks = range_checksum();
				psens_cks = psensor_checksum();
				app_cks = u_aplicacion_checksum();
				//
				xprintf_PVD(  xCOMMS_get_fd(), DF_COMMS, PSTR("&PLOAD=CLASS:GLOBAL;NACH:%d;NDCH:%d;NCNT:%d;" ),NRO_ANINPUTS,NRO_DINPUTS,NRO_COUNTERS );
				xCOMM_send_global_params();
				//
				xprintf_PVD(  xCOMMS_get_fd(), DF_COMMS, PSTR("BASE:0x%02X;AN:0x%02X;DG:0x%02X;" ), base_cks,an_cks,dig_cks );
				xprintf_PVD(  xCOMMS_get_fd(), DF_COMMS, PSTR("CNT:0x%02X;RG:0x%02X;" ),cnt_cks,range_cks );
				xprintf_PVD(  xCOMMS_get_fd(), DF_COMMS, PSTR("PSE:0x%02X;" ), psens_cks );
				xprintf_PVD(  xCOMMS_get_fd(), DF_COMMS, PSTR("APP:0x%02X;" ), app_cks );
				break;
			case INIT_BASE:
				xprintf_PVD(  xCOMMS_get_fd(), DF_COMMS, PSTR("&PLOAD=CLASS:CONF_BASE;"));
				break;
			case INIT_ANALOG:
				xprintf_PVD(  xCOMMS_get_fd(), DF_COMMS, PSTR("&PLOAD=CLASS:CONF_ANALOG;"));
				break;
			case INIT_DIGITAL:
				xprintf_PVD(  xCOMMS_get_fd(), DF_COMMS, PSTR("&PLOAD=CLASS:CONF_DIGITAL;"));
				break;
			case INIT_COUNTERS:
				xprintf_PVD(  xCOMMS_get_fd(), DF_COMMS, PSTR("&PLOAD=CLASS:CONF_COUNTER;"));
				break;
			case INIT_RANGE:
				xprintf_PVD(  xCOMMS_get_fd(), DF_COMMS, PSTR("&PLOAD=CLASS:CONF_RANGE;"));
				break;
			case INIT_PSENSOR:
				xprintf_PVD(  xCOMMS_get_fd(), DF_COMMS, PSTR("&PLOAD=CLASS:CONF_PSENSOR;"));
				break;

			default:
				break;
			}

			xCOMMS_send_tail();
			//  El bloque se trasmition OK. Paso a esperar la respuesta
			return(true);

		} else {
			// No tengo enlace al server. Intento abrirlo
			vTaskDelay( (portTickType)( 3000 / portTICK_RATE_MS ) );
			xCOMMS_open_link();
		}
	}

	/*
	 * Despues de varios reintentos no logre enviar el frame
	 */
	return(false);

}
//------------------------------------------------------------------------------------
static bool process_init_response(void)
{

	// Espero la respuesta al frame de INIT.
	// Si la recibo la proceso.
	// Salgo por timeout 10s o por socket closed.

uint8_t timeout = 0;
bool retS = false;

	for ( timeout = 0; timeout < 10; timeout++) {

		vTaskDelay( (portTickType)( 2000 / portTICK_RATE_MS ) );	// Espero 1s

		// El socket se cerro
		if ( xCOMMS_link_status() != LINK_OPEN ) {
			return(false);
		}

		//xCOMMS_print_RX_buffer(true);

		// Recibi un ERROR de respuesta
		if ( xCOMMS_check_response("ERROR") ) {
			xCOMMS_print_RX_buffer(true);
			return(false);
		}

		// Respuesta completa del server
		if ( xCOMMS_check_response("</h1>") ) {

			xCOMMS_print_RX_buffer( DF_COMMS );

			// Analizo las respuestas.
			if ( xCOMMS_check_response("CLASS:AUTH") ) {	// Respuesta correcta:
				// Borro la causa del reset
				wdg_resetCause = 0x00;
				retS = init_reconfigure_params_auth();
				return(retS);
			}

			// Analizo las respuestas.
			if ( xCOMMS_check_response("CLASS:GLOBAL") ) {
				// Borro la causa del reset
				wdg_resetCause = 0x00;
				retS = init_reconfigure_params_global();
				return(retS);
			}

			if ( xCOMMS_check_response("BASE") ) {
				// Borro la causa del reset
				wdg_resetCause = 0x00;
				retS = init_reconfigure_params_base();
				return(retS);
			}

			if ( xCOMMS_check_response("ANALOG") ) {
				// Borro la causa del reset
				wdg_resetCause = 0x00;
				retS = init_reconfigure_params_analog();
				return(retS);
			}

			if ( xCOMMS_check_response("DIGITAL") ) {
				// Borro la causa del reset
				wdg_resetCause = 0x00;
				retS = init_reconfigure_params_digital();
				return(retS);
			}

			if ( xCOMMS_check_response("COUNTER") ) {
				// Borro la causa del reset
				wdg_resetCause = 0x00;
				retS = init_reconfigure_params_counters();
				return(retS);
			}

			if ( xCOMMS_check_response("PSENSOR") ) {
				// Borro la causa del reset
				wdg_resetCause = 0x00;
				retS = init_reconfigure_params_psensor();
				return(retS);
			}

			if ( xCOMMS_check_response("RANGE") ) {
				// Borro la causa del reset
				wdg_resetCause = 0x00;
				retS = init_reconfigure_params_range();
				return(retS);
			}

			// El servidor no pudo procesar el frame. Problema del server
			if ( xCOMMS_check_response("SRV_ERR") ) {
				// Borro la causa del reset
				wdg_resetCause = 0x00;
				xprintf_P( PSTR("COMMS: SERVER ERROR !!.\r\n\0" ));
				return(false);
			}

			// Datalogger esta usando un script incorrecto
			if ( xCOMMS_check_response("NOT_ALLOWED") ) {
				xprintf_P( PSTR("COMMS: SCRIPT ERROR !!.\r\n\0" ));
				return(false);
			}
		}
	}

// Exit:

	return(false);
}
//------------------------------------------------------------------------------------
// FUNCIONES AUXILIARES ENVIO FRAMES
//------------------------------------------------------------------------------------
static bool init_frame_auth(void)
{
	/*
	 * Este frame es el que determina si el datalogger puede transmitir
	 * al server o no.
	 */

	if ( process_frame( INIT_AUTH) )
		return(true);

	return(false);
}
//------------------------------------------------------------------------------------
static bool init_frame_global(void)
{

	/*
	 * Este estado es el que prende las flags para indicar que otros grupos de
	 * parámetros deben ser configurados.
	 */

bool retS = false;

	f_send_init_frame_base = false;
	f_send_init_frame_analog = false;
	f_send_init_frame_digital = false;
	f_send_init_frame_counters = false;
	f_send_init_frame_range = false;
	f_send_init_frame_psensor = false;

	retS = process_frame( INIT_GLOBAL);
	return(retS);

}
//------------------------------------------------------------------------------------
static bool init_frame_base(void)
{
	/*
	 * Configura los parametros base. ( tdial, tpoll, pwrsave, etc)
	 */

bool retS = true;

	if ( f_send_init_frame_base ) {
		f_send_init_frame_base = false;
		retS = process_frame( INIT_BASE);
	}
	return(retS);

}
//------------------------------------------------------------------------------------
static bool init_frame_analog(void)
{

bool retS = true;

	if ( f_send_init_frame_analog ) {
		f_send_init_frame_analog = false;
		retS = process_frame( INIT_ANALOG);
	}
	return(retS);
}
//------------------------------------------------------------------------------------
static bool init_frame_digital(void)
{

bool retS = true;

	if ( f_send_init_frame_digital ) {
		f_send_init_frame_digital = false;
		retS = process_frame( INIT_DIGITAL);
	}
	return(retS);
}
//------------------------------------------------------------------------------------
static bool init_frame_counters(void)
{

bool retS = true;

	if ( f_send_init_frame_counters ) {
		f_send_init_frame_counters = false;
		retS = process_frame( INIT_COUNTERS);
	}
	return(retS);
}
//------------------------------------------------------------------------------------
static bool init_frame_psensor(void)
{

bool retS = true;

	if ( f_send_init_frame_psensor ) {
		f_send_init_frame_psensor = false;
		retS = process_frame( INIT_PSENSOR);
	}
	return(retS);
}
//------------------------------------------------------------------------------------
static bool init_frame_range(void)
{

bool retS = true;

	if ( f_send_init_frame_range ) {
		f_send_init_frame_range = false;
		retS = process_frame( INIT_RANGE);
	}
	return(retS);
}
//------------------------------------------------------------------------------------
// FUNCIONES AUXILIARES RECONFIGURACION
//------------------------------------------------------------------------------------
static bool init_reconfigure_params_auth(void)
{
	// Recibimos un frame de autorizacion que indica si los parametros UID/DLGID son
	// correctos o debe reconfigurar el DLGID.
	// TYPE=INIT&PLOAD=CLASS:AUTH;STATUS:OK
	// TYPE=INIT&PLOAD=CLASS:AUTH;STATUS:RECONF;DLGID:TEST01
	// TYPE=INIT&PLOAD=CLASS:AUTH;STATUS:ERROR_DS

char *p = NULL;
char localStr[32] = { 0 };
char *stringp = NULL;
char *token = NULL;
char *tk_action = NULL;
char *delim = ",=:><";
char dlgId[DLGID_LENGTH];

	xprintf_PD( DF_COMMS, PSTR("COMMS_INIT_AUTH\r\n\0"));

	p = xCOMM_get_buffer_ptr("AUTH");
	if ( p == NULL ) {
		return(false);
	}

	// Copio el mensaje enviado a un buffer local porque la funcion strsep lo modifica.
	memset(localStr,'\0',32);
	memcpy(localStr,p,sizeof(localStr));

	stringp = localStr;
	token = strsep(&stringp,delim);	    // AUTH
	token = strsep(&stringp,delim);	    // STATUS
	tk_action = strsep(&stringp,delim);	// Action

	if ( strcmp_P(tk_action, PSTR("OK") == 0 )) {
		// Autorizado por el server.
		return(true);
	} else if ( strcmp_P (tk_action, PSTR("ERROR_DS"))) {
		// No autorizado
		return(false);
	} else if ( strcmp_P (tk_action, PSTR("RECONF"))) {
		// Autorizado. Debo reconfigurar el DLGID
		token = strsep(&stringp,delim);	 // DLGID
		token = strsep(&stringp,delim);	 // TEST01
		// Copio el dlgid recibido al systemVars.
		memset(dlgId,'\0', sizeof(dlgId) );
		strncpy(dlgId, token, DLGID_LENGTH);
		u_save_params_in_NVMEE();
		xprintf_P( PSTR("COMMS_INIT_AUTH: reconfig DLGID to %s\r\n\0"), dlgId );
		return(true);
	}

	return(false);
}
//------------------------------------------------------------------------------------
static bool init_reconfigure_params_global(void)
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

	xprintf_PD( DF_COMMS, PSTR("COMMS_INIT_GLOBAL\r\n\0"));

	// CLOCK
	p = xCOMM_get_buffer_ptr("CLOCK");
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

		//xprintf_P(PSTR("DEBUG_RTC: [%s]\r\n\0"), rtcStr);

		memset( &rtc, '\0', sizeof(rtc) );
		RTC_str2rtc(rtcStr, &rtc);			// Convierto el string YYMMDDHHMM a RTC.
		xBytes = RTC_write_dtime(&rtc);		// Grabo el RTC
		if ( xBytes == -1 )
			xprintf_P(PSTR("ERROR: I2C:RTC:pv_process_server_clock\r\n\0"));

		xprintf_PD( DF_COMMS, PSTR("COMMS: Update rtc to: %s\r\n\0"), rtcStr );


	}

	// Flags de configuraciones particulares: BASE;ANALOG;DIGITAL;COUNTERS;RANGE;PSENSOR;OUTS

	p = xCOMM_get_buffer_ptr("BASE");
	if ( p != NULL ) {
		f_send_init_frame_base = true;
	}

	p = xCOMM_get_buffer_ptr("ANALOG");
	if ( p != NULL ) {
		f_send_init_frame_analog = true;
	}

	p = xCOMM_get_buffer_ptr("DIGITAL");
	if ( p != NULL ) {
		f_send_init_frame_digital = true;
	}

	p = xCOMM_get_buffer_ptr("COUNTERS");
	if ( p != NULL ) {
		f_send_init_frame_counters = true;
	}

	p = xCOMM_get_buffer_ptr("RANGE");
	if ( p != NULL ) {
		f_send_init_frame_range = true;
	}

	p = xCOMM_get_buffer_ptr("PSENSOR");
	if ( p != NULL ) {
		f_send_init_frame_psensor = true;
	}

	return(true);
}
//------------------------------------------------------------------------------------
static bool init_reconfigure_params_base(void)
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

	xprintf_PD( DF_COMMS, PSTR("COMMS_INIT_BASE\r\n\0"));

	// TDIAL
	p = xCOMM_get_buffer_ptr("TDIAL");
	if ( p != NULL ) {

		// Copio el mensaje enviado a un buffer local porque la funcion strsep lo modifica.
		memset( &localStr, '\0', sizeof(localStr) );
		memcpy(localStr,p,sizeof(localStr));

		stringp = localStr;
		token = strsep(&stringp,delim);		// TDIAL
		token = strsep(&stringp,delim);		// timerDial

		//xprintf_P(PSTR("DEBUG_TDIAL: [%s]\r\n\0"), token);

		u_config_timerdial(token);
		save_flag = true;
		xprintf_PD( DF_COMMS, PSTR("COMMS: Reconfig TDIAL\r\n\0"));
	}

	// TPOLL
	p = xCOMM_get_buffer_ptr("TPOLL");
	if ( p != NULL ) {

		memset( &localStr, '\0', sizeof(localStr) );
		memcpy(localStr,p,sizeof(localStr));

		stringp = localStr;
		token = strsep(&stringp,delim);		// TPOLL
		token = strsep(&stringp,delim);		// timerPoll

		//xprintf_P(PSTR("DEBUG_TPOLL: [%s]\r\n\0"), token);

		u_config_timerpoll(token);
		save_flag = true;
		xprintf_PD( DF_COMMS, PSTR("COMMS: Reconfig TPOLL\r\n\0"));
	}

	// PWST
	p = xCOMM_get_buffer_ptr("PWST");
	if ( p != NULL ) {

		// Copio el mensaje enviado a un buffer local porque la funcion strsep lo modifica.
		memset( &localStr, '\0', sizeof(localStr) );
		memcpy(localStr,p,sizeof(localStr));

		stringp = localStr;
		token = strsep(&stringp,delim);		// PWST
		token = strsep(&stringp,delim);		// timePwrSensor

		//xprintf_P(PSTR("DEBUG_TPWRSENS: [%s]\r\n\0"), token);

		ainputs_config_timepwrsensor(token);
		save_flag = true;
		xprintf_PD( DF_COMMS, PSTR("COMMS: Reconfig PWRSTIME\r\n\0"));
	}

	// PWRS
	p = xCOMM_get_buffer_ptr("PWRS");
	if ( p != NULL ) {
		memset( &localStr, '\0', sizeof(localStr) );
		memcpy(localStr,p,sizeof(localStr));

		stringp = localStr;
		tk_pws_modo = strsep(&stringp,delim);		//PWRS
		tk_pws_modo = strsep(&stringp,delim);		// modo ( ON / OFF ).
		tk_pws_start = strsep(&stringp,delim);		// startTime
		tk_pws_end  = strsep(&stringp,delim); 		// endTime

		//xprintf_P(PSTR("DEBUG_PWRS: [%s][%s][%s]\r\n\0"), tk_pws_modo, tk_pws_start, tk_pws_end );

		u_configPwrSave(tk_pws_modo, tk_pws_start, tk_pws_end );
		save_flag = true;
		xprintf_PD( DF_COMMS, PSTR("COMMS: Reconfig PWRSAVE\r\n\0"));
	}

	if ( save_flag ) {
		u_save_params_in_NVMEE();
	}

	return(true);
}
//------------------------------------------------------------------------------------
static bool init_reconfigure_params_analog(void)
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

	xprintf_PD( DF_COMMS, PSTR("COMMS_INIT_ANALOG\r\n\0"));

	// A?
	for (ch=0; ch < NRO_ANINPUTS; ch++ ) {
		memset( &str_base, '\0', sizeof(str_base) );
		snprintf_P( str_base, sizeof(str_base), PSTR("A%d\0"), ch );
		//xprintf_P( PSTR("DEBUG str_base: %s\r\n\0"), str_base);

		p = xCOMM_get_buffer_ptr(str_base);
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

			xprintf_PD( DF_COMMS, PSTR("COMMS: Reconfig A%d\r\n\0"), ch);

			save_flag = true;
		}
	}

	if ( save_flag ) {
		u_save_params_in_NVMEE();
	}

	return(true);
}
//------------------------------------------------------------------------------------
static bool init_reconfigure_params_digital(void)
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

	xprintf_PD( DF_COMMS, PSTR("COMMS_INIT_DIGITAL\r\n\0"));

	// D?
	for (ch=0; ch < NRO_DINPUTS; ch++ ) {
		memset( &str_base, '\0', sizeof(str_base) );
		snprintf_P( str_base, sizeof(str_base), PSTR("D%d\0"), ch );
		p = xCOMM_get_buffer_ptr(str_base);
		if ( p != NULL ) {
			memset(localStr,'\0',sizeof(localStr));
			memcpy(localStr,p,sizeof(localStr));
			stringp = localStr;
			tk_name = strsep(&stringp,delim);		//D0
			tk_name = strsep(&stringp,delim);		//DIN0
			tk_type = strsep(&stringp,delim);		//NORMAL

			dinputs_config_channel( ch,tk_name ,tk_type );

			xprintf_PD( DF_COMMS, PSTR("COMMS: Reconfig D%d\r\n\0"), ch);

			save_flag = true;
		}
	}

	if ( save_flag ) {
		u_save_params_in_NVMEE();
	}

	return(true);
}
//------------------------------------------------------------------------------------
static bool init_reconfigure_params_counters(void)
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

	xprintf_PD( DF_COMMS, PSTR("COMMS_INIT_COUNTERS\r\n\0"));

	// C?
	for (ch=0; ch < NRO_COUNTERS; ch++ ) {
		memset( &str_base, '\0', sizeof(str_base) );
		snprintf_P( str_base, sizeof(str_base), PSTR("C%d\0"), ch );
		p = xCOMM_get_buffer_ptr(str_base);
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

			xprintf_PD( DF_COMMS, PSTR("COMMS: Reconfig C%d\r\n\0"), ch);

			save_flag = true;
		}
	}

	if ( save_flag ) {
		u_save_params_in_NVMEE();
	}

	// Necesito resetearme para tomar la nueva configuracion.
	//f_reset = true;
	return(true);

}
//------------------------------------------------------------------------------------
static bool init_reconfigure_params_range(void)
{
	// TYPE=INIT&PLOAD=CLASS:RANGE;R0:DIST;

char *p = NULL;
char localStr[32] = { 0 };
char *stringp = NULL;
char *token = NULL;
char *delim = ",=:;><";

	xprintf_PD( DF_COMMS, PSTR("COMMS_INIT_RANGE\r\n\0"));

	// RANGE
	p = xCOMM_get_buffer_ptr("R0");
	if ( p != NULL ) {

		// Copio el mensaje enviado a un buffer local porque la funcion strsep lo modifica.
		memset( &localStr, '\0', sizeof(localStr) );
		memcpy(localStr,p,sizeof(localStr));
		stringp = localStr;
		token = strsep(&stringp,delim);		// R0
		token = strsep(&stringp,delim);		// Range name
		range_config(token);

		xprintf_PD( DF_COMMS, PSTR("COMMS: Reconfig RANGE\r\n\0"));

		u_save_params_in_NVMEE();
	}

	return(true);

}
//------------------------------------------------------------------------------------
static bool init_reconfigure_params_psensor(void)
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

	xprintf_PD( DF_COMMS, PSTR("COMMS_INIT_PSENSOR\r\n\0"));

	p = xCOMM_get_buffer_ptr("PS0:");
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

		xprintf_PD( DF_COMMS, PSTR("COMMS: Reconfig PSENSOR\r\n\0"));

		u_save_params_in_NVMEE();

	}
	return(true);

}
//------------------------------------------------------------------------------------

