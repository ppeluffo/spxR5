/*
 * spx_tkComms_init.c
 *
 *  Created on: 26 feb. 2020
 *      Author: pablo
 */

#include <comms.h>

//------------------------------------------------------------------------------------
t_comms_states tkComms_st_init(void)
{

	return(ST_DATA);
}
//------------------------------------------------------------------------------------
typedef enum { ST_INITS_AUTH = 0, ST_INITS_GLOBAL , ST_INITS_BASE, ST_INITS_ANALOG,ST_INITS_DIGITAL,ST_INITS_COUNTERS,ST_INITS_RANGE,ST_INITS_PSENSOR, ST_INITS_EXIT   } t_inits_state;
typedef enum { SST_EXIT = 0, SST_INIT , SST_ENVIAR_FRAME, SST_PROCESAR_RESPUESTA } t_inits_sub_state;

static t_inits_state state;
static t_inits_sub_state sub_state;

static uint8_t intentos;

static bool f_send_init_base = false;
static bool f_send_init_analog = false;
static bool f_send_init_digital = false;
static bool f_send_init_counters = false;
static bool f_send_init_range = false;
static bool f_send_init_psensor = false;
static bool f_send_init_app = false;

#define MAX_INTENTOS_ENVIAR_INIT_FRAME	4

#define AC_init_for_tx() ( intentos =  MAX_INTENTOS_ENVIAR_INIT_FRAME )

t_inits_state tkComms_sst_enviar_frame( t_inits_state init_code );

static t_inits_sub_state tkComms_sst_init( t_inits_state init_code );
static t_inits_sub_state tkComms_sst_send( t_inits_state init_code );
static t_inits_sub_state tkComms_sst_response( t_inits_state init_code );

static bool EV_hay_datos_para_transmitir( t_inits_state init_code);
static bool EV_envio_frame_OK( t_inits_state init_code);
static bool EV_procesar_respuesta_OK(void);

static bool ac_process_response_AUTH(void);
static void ac_process_response_GLOBAL(void);
static void ac_process_response_BASE(void);
static void ac_process_response_ANALOG(void);
static void ac_process_response_DIGITAL(void);
static void ac_process_response_COUNTER(void);
static void ac_process_response_RANGE(void);
static void ac_process_response_PSENSOR(void);

//------------------------------------------------------------------------------------
t_comms_states tkComms_st_inits(void)
{
/* Estado en que procesa los frames de inits, los transmite y procesa
 * las respuestas
 * Si no hay datos para transmitir sale
 * Si luego de varios reintentos no pudo, sale
 */

// ENTRY

	state = ST_INITS_AUTH;
	u_debug_printf_P( DEBUG_COMMS, PSTR("COMMS:Inits.\r\n\0"));

// LOOP
	for( ;; )
	{

		//ctl_watchdog_kick(WDG_GPRSTX, WDG_GPRS_DATA );

		switch ( state ) {
		case ST_INITS_AUTH:
			state = tkComms_sst_enviar_frame(ST_INITS_AUTH);
			break;
		case ST_INITS_GLOBAL:
			state = tkComms_sst_enviar_frame(ST_INITS_GLOBAL);
			break;
		case ST_INITS_BASE:
			state = tkComms_sst_enviar_frame(ST_INITS_BASE);
			break;
		case ST_INITS_ANALOG:
			state = tkComms_sst_enviar_frame(ST_INITS_ANALOG);
			break;
		case ST_INITS_DIGITAL:
			state = tkComms_sst_enviar_frame(ST_INITS_DIGITAL);
			break;
		case ST_INITS_COUNTERS:
			state = tkComms_sst_enviar_frame(ST_INITS_COUNTERS);
			break;
		case ST_INITS_RANGE:
			state = tkComms_sst_enviar_frame(ST_INITS_RANGE);
			break;
		case ST_INITS_PSENSOR:
			state = tkComms_sst_enviar_frame(ST_INITS_PSENSOR);
			break;
		case ST_INITS_EXIT:
			// Salgo del estado.
			return(ST_STANDBY);
			break;
		default:
			xprintf_P( PSTR("COMMS: inits state ERROR !!.\r\n\0"));
			return(ST_STANDBY);
		}
	}

// EXIT:
	return(ST_STANDBY);
}
//------------------------------------------------------------------------------------
// ESTADOS LOCALES
//------------------------------------------------------------------------------------
t_inits_state tkComms_sst_enviar_frame( t_inits_state init_code )
{
/*
 * Envia un frame de init que depende del init_code.
 * El proceso implica enviarlo y esperar la respuesta.
 * Si todo es correcto, sale con
 * Estado en que procesa los frames de inits, los transmite y procesa
 * las respuestas
 * Si luego de varios reintentos no pudo, sale
 */

// ENTRY

	sub_state = SST_INIT;
	u_debug_printf_P( DEBUG_COMMS, PSTR("COMMS:sst_inits.\r\n\0"));

// LOOP
	for( ;; )
	{

		//ctl_watchdog_kick(WDG_GPRSTX, WDG_GPRS_DATA );

		switch ( sub_state ) {
		case SST_INIT:
			// Determina si hay datos o no para transmitir
			sub_state = tkComms_sst_init(init_code);
			break;
		case  SST_ENVIAR_FRAME:
			sub_state = tkComms_sst_send(init_code);
			break;
		case SST_PROCESAR_RESPUESTA:
			sub_state = tkComms_sst_response(init_code);
			break;
		case SST_EXIT:
			// Salgo del estado.
			// OJO; EL CODIGO DE SALIDA DEPENDE DEL INIT_CODE
			return(ST_STANDBY);
			break;
		default:
			xprintf_P( PSTR("COMMS: data state ERROR !!.\r\n\0"));
			return(ST_STANDBY);
		}
	}

// EXIT:
	return(ST_STANDBY);
}
//------------------------------------------------------------------------------------
static t_inits_sub_state tkComms_sst_init( t_inits_state init_code )
{

	/* subEstado en que se determina si hay que transmitir el frame o no
	 * Si hay, inicializa las variables de intentos.
	 * So no hay, sale y pasa al estado superior de standby
	 */

	if ( EV_hay_datos_para_transmitir(init_code) ) {
		/* Inicializo el contador de errores [AC_init_for_tx()] y paso
		 * al estado que intento trasmitir los frames
		 */
		AC_init_for_tx();
		return(SST_ENVIAR_FRAME);

	} else {
		/* Al no haber datos, voy al estado local de EXIT
		 * y actualizo la variable de salida que voy a retornar al nivel superior
		 */
		return(SST_EXIT);
	}

	return(SST_EXIT);
}
//------------------------------------------------------------------------------------
static t_inits_sub_state tkComms_sst_send( t_inits_state init_code )
{

t_inits_state next_state;

	if ( EV_envio_frame_OK(init_code) ) {

		next_state = SST_PROCESAR_RESPUESTA;

	} else {

		/* Despues de varios reintentos no pude trasmitir el bloque
		 * Debo mandar apagar y prender el dispositivo
		 * Salgo y fijo el estado del nivel superior
		 */
		u_comms_signal(SGN_RESET_DEVICE);
		next_state = SST_EXIT;
	}

	return(next_state);
}
//------------------------------------------------------------------------------------
static t_inits_sub_state tkComms_sst_response( t_inits_state init_code )
{

t_inits_state next_state;

		if ( EV_procesar_respuesta_OK() ) {
			// Procese correctamente la respuesta. Veo si hay mas datos para transmitir.
			next_state = SST_EXIT;

		} else {
			// Error al procesar: reintento enviar el frame
			next_state = SST_ENVIAR_FRAME;
		}

		return(next_state);
}
//------------------------------------------------------------------------------------
// FUNCIONES AUXILIARES
//------------------------------------------------------------------------------------
static bool EV_hay_datos_para_transmitir( t_inits_state init_code)
{
bool retS = false;

	switch(init_code) {
	case ST_INITS_AUTH:
		// Siempre mando el AUTH.
		retS = true;
		break;
	case ST_INITS_GLOBAL:
		// Siempre mando el GLOBAL. Este me prende el resto de las flags.
		retS = true;
		break;
	case ST_INITS_BASE:
		retS = f_send_init_base;
		break;
	case ST_INITS_ANALOG:
		retS = f_send_init_analog;
		break;
	case ST_INITS_DIGITAL:
		retS = f_send_init_digital;
		break;
	case ST_INITS_COUNTERS:
		retS = f_send_init_counters;
		break;
	case ST_INITS_RANGE:
		retS = f_send_init_range;
		break;
	case ST_INITS_PSENSOR:
		retS = f_send_init_psensor;
		break;
	}

	return(retS);

}
//------------------------------------------------------------------------------------
static bool EV_envio_frame_OK( t_inits_state init_code)
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

			switch(init_code) {
			case ST_INITS_AUTH:
				xCOMMS_send_P( PSTR("&PLOAD=CLASS:AUTH;UID:%s;" ),NVMEE_readID() );
				break;

			case ST_INITS_GLOBAL:
				base_cks = u_base_checksum();
				an_cks = ainputs_checksum();
				dig_cks = dinputs_checksum();
				cnt_cks = counters_checksum();
				range_cks = range_checksum();
				psens_cks = psensor_checksum();
				app_cks = u_aplicacion_checksum();

				xCOMMS_send_P( PSTR("&PLOAD=CLASS:GLOBAL;NACH:%d;NDCH:%d;NCNT:%d;" ),NRO_ANINPUTS,NRO_DINPUTS,NRO_COUNTERS );
				xCOMMS_send_P( PSTR("IMEI:%s;" ), &buff_gprs_imei );
				xCOMMS_send_P( PSTR("SIMID:%s;CSQ:%d;WRST:%02X;" ), &buff_gprs_ccid,GPRS_stateVars.dbm, wdg_resetCause );
				xCOMMS_send_P( PSTR("BASE:0x%02X;AN:0x%02X;DG:0x%02X;" ), base_cks,an_cks,dig_cks );
				xCOMMS_send_P( PSTR("CNT:0x%02X;RG:0x%02X;" ),cnt_cks,range_cks );
				xCOMMS_send_P( PSTR("PSE:0x%02X;" ), psens_cks );
				xCOMMS_send_P( PSTR("APP:0x%02X;" ), app_cks );
				break;

			case ST_INITS_BASE:
				xCOMMS_send_P( PSTR("&PLOAD=CLASS:CONF_BASE;"));
				break;
			case ST_INITS_ANALOG:
				xCOMMS_send_P( PSTR("&PLOAD=CLASS:CONF_ANALOG;"));
				break;
			case ST_INITS_DIGITAL:
				xCOMMS_send_P( PSTR("&PLOAD=CLASS:CONF_DIGITAL;"));
				break;
			case ST_INITS_COUNTERS:
				xCOMMS_send_P( PSTR("&PLOAD=CLASS:CONF_COUNTER;"));
				break;
			case ST_INITS_RANGE:
				xCOMMS_send_P( PSTR("&PLOAD=CLASS:CONF_RANGE;"));
				break;
			case ST_INITS_PSENSOR:
				xCOMMS_send_P( PSTR("&PLOAD=CLASS:CONF_PSENSOR;"));
				break;

			}

			xCOMMS_send_tail();
			//  El bloque se trasmition OK. Paso a esperar la respuesta
			//
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
static bool EV_procesar_respuesta_OK(void)
{
	/*
	 * Me quedo hasta 10s esperando la respuesta del server al frame de init.
	 * Salgo por timeout, socket cerrado, error del server o respuesta correcta
	 * Si la respuesta es correcta, ejecuto la misma
	 */

uint8_t timeout = 0;

	for ( timeout = 0; timeout < 10; timeout++) {

		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );	// Espero 1s
		/*
		 * Si no hay enlace al servidor ( socket cerrado ) no voy
		 * a recibir respuesta nunca. Salgo a reintentar
		 */
		if ( xCOMMS_link_status() != LINK_OPEN ) {
			return(false);
		}

		// Voy leyendo datos
		xCOMMS_read();

		/* Hay enlace: veo si ya termino de llegar la respuesta */
		if ( xCOMMS_check_response ("</h1>\0")) {	// Recibi una respuesta del server.
			// Log.

			xCOMMS_print_RX_buffer();

			// Analizo las respuestas.

			if ( xCOMMS_check_response ("SRV_ERR") ) {
				// El servidor no pudo procesar el frame. Problema del server
				return(false);
			}

			if ( u_gprs_check_response("NOT_ALLOWED") ) {
				// Datalogger esta usando un script incorrecto
				return(false);
			}

			if ( xCOMMS_check_response ("AUTH\0")) {
				// El sever mando la orden de resetearse inmediatamente
				if ( ac_process_response_AUTH() == true ) {
					return(true);
				} else {
					return(false);
				}
			}

			if ( xCOMMS_check_response ("GLOBAL\0")) {
				ac_process_response_GLOBAL();
				return(true);
			}

			if ( xCOMMS_check_response ("BASE\0")) {
				ac_process_response_BASE();
				return(true);
			}

			if ( xCOMMS_check_response ("ANALOG\0")) {
				ac_process_response_ANALOG();
				return(true);
			}

			if ( xCOMMS_check_response ("DIGITAL\0")) {
				ac_process_response_DIGITAL();
				return(true);
			}

			if ( xCOMMS_check_response ("COUNTER\0")) {
				ac_process_response_COUNTER();
				return(true);
			}

			if ( xCOMMS_check_response ("RANGE\0")) {
				ac_process_response_RANGE();
				return(true);
			}

			if ( xCOMMS_check_response ("PSENSOR\0")) {
				ac_process_response_PSENSOR();
				return(true);
			}

			return(false);

		}
	}

// Exit:

	return(false);

}
//------------------------------------------------------------------------------------
static bool ac_process_response_AUTH(void)
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

	p = xCOMMS_strstr( "AUTH\0");
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

	if ( strcmp_P(tk_action, PSTR("OK\0"))) {
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
		xprintf_P( PSTR("COMMS: INIT_AUTH: reconfig DLGID to %s\r\n\0"), dlgId );
		return(true);
	}

	return(false);

}
//------------------------------------------------------------------------------------
static void ac_process_response_GLOBAL(void)
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
	p = xCOMMS_strstr( "CLOCK\0");
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

		xprintf_P( PSTR("COMMS: Update rtc to: %s\r\n\0"), rtcStr );

	}

	// Flags de configuraciones particulares: BASE;ANALOG;DIGITAL;COUNTERS;RANGE;PSENSOR;OUTS
	p = xCOMMS_strstr("BASE\0");
	if ( p != NULL ) {
		f_send_init_base = true;
		u_debug_printf_P( DEBUG_COMMS, PSTR("COMMS: Reconfig BASE\r\n\0"));
	}

	p = xCOMMS_strstr("ANALOG\0");
	if ( p != NULL ) {
		f_send_init_analog = true;
		u_debug_printf_P( DEBUG_COMMS, PSTR("COMMS: Reconfig ANALOG\r\n\0"));
	}

	p = xCOMMS_strstr("DIGITAL\0");
	if ( p != NULL ) {
		f_send_init_digital = true;
		u_debug_printf_P( DEBUG_COMMS, PSTR("COMMS: Reconfig DIGITAL\r\n\0"));
	}

	p = xCOMMS_strstr("COUNTERS\0");
	if ( p != NULL ) {
		f_send_init_counters = true;
		u_debug_printf_P( DEBUG_COMMS, PSTR("COMMS: Reconfig COUNTERS\r\n\0"));
	}

	p = xCOMMS_strstr("RANGE\0");
	if ( p != NULL ) {
		f_send_init_range = true;
		u_debug_printf_P( DEBUG_COMMS, PSTR("COMMS: Reconfig RANGE\r\n\0"));
	}

	p = xCOMMS_strstr("PSENSOR\0");
	if ( p != NULL ) {
		f_send_init_psensor = true;
		u_debug_printf_P( DEBUG_COMMS, PSTR("COMMS: Reconfig PSENSOR\r\n\0"));
	}

}
//------------------------------------------------------------------------------------
static void ac_process_response_BASE(void)
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
	p = xCOMMS_strstr( "TDIAL\0");
	if ( p != NULL ) {

		// Copio el mensaje enviado a un buffer local porque la funcion strsep lo modifica.
		memset( &localStr, '\0', sizeof(localStr) );
		memcpy(localStr,p,sizeof(localStr));

		stringp = localStr;
		token = strsep(&stringp,delim);		// TDIAL
		token = strsep(&stringp,delim);		// timerDial
		u_gprs_config_timerdial(token);
		save_flag = true;
		u_debug_printf_P( DEBUG_COMMS, PSTR("COMMS: Reconfig TDIAL\r\n\0"));
	}

	// TPOLL
	p = xCOMMS_strstr( "TPOLL\0");
	if ( p != NULL ) {

		memset( &localStr, '\0', sizeof(localStr) );
		memcpy(localStr,p,sizeof(localStr));

		stringp = localStr;
		token = strsep(&stringp,delim);		// TPOLL
		token = strsep(&stringp,delim);		// timerPoll
		u_config_timerpoll(token);
		save_flag = true;
		u_debug_printf_P( DEBUG_COMMS, PSTR("COMMS: Reconfig TPOLL\r\n\0"));
	}

	// PWST
	p = xCOMMS_strstr( "PWST\0");
	if ( p != NULL ) {

		// Copio el mensaje enviado a un buffer local porque la funcion strsep lo modifica.
		memset( &localStr, '\0', sizeof(localStr) );
		memcpy(localStr,p,sizeof(localStr));

		stringp = localStr;
		token = strsep(&stringp,delim);		// PWST
		token = strsep(&stringp,delim);		// timePwrSensor
		ainputs_config_timepwrsensor(token);
		save_flag = true;
		u_debug_printf_P( DEBUG_COMMS, PSTR("COMMS: Reconfig PWRSTIME\r\n\0"));
	}

	// PWRS
	p = xCOMMS_strstr( "PWRS\0");
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
		u_debug_printf_P( DEBUG_COMMS, PSTR("COMMS: Reconfig PWRSAVE\r\n\0"));
	}

	if ( save_flag ) {
		u_save_params_in_NVMEE();
	}

}
//------------------------------------------------------------------------------------
static void ac_process_response_ANALOG(void)
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

		p = xCOMMS_strstr(str_base);
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

			u_debug_printf_P( DEBUG_COMMS, PSTR("COMMS: Reconfig A%d\r\n\0"), ch);

			save_flag = true;
		}
	}

	if ( save_flag ) {
		u_save_params_in_NVMEE();
	}

}
//------------------------------------------------------------------------------------
static void ac_process_response_DIGITAL(void)
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
		p = xCOMMS_strstr( str_base);
		if ( p != NULL ) {
			memset(localStr,'\0',sizeof(localStr));
			memcpy(localStr,p,sizeof(localStr));
			stringp = localStr;
			tk_name = strsep(&stringp,delim);		//D0
			tk_name = strsep(&stringp,delim);		//DIN0
			tk_type = strsep(&stringp,delim);		//NORMAL

			dinputs_config_channel( ch,tk_name ,tk_type );

			u_debug_printf_P( DEBUG_COMMS, PSTR("COMMS: Reconfig D%d\r\n\0"), ch);

			save_flag = true;
		}
	}

	if ( save_flag ) {
		u_save_params_in_NVMEE();
	}

}
//------------------------------------------------------------------------------------
static void ac_process_response_COUNTER(void)
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
		p = xCOMMS_strstr( str_base);
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

			u_debug_printf_P( DEBUG_COMMS, PSTR("COMMS: Reconfig C%d\r\n\0"), ch);

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
static void ac_process_response_RANGE(void)
{
	// TYPE=INIT&PLOAD=CLASS:RANGE;R0:DIST;

char *p = NULL;
char localStr[32] = { 0 };
char *stringp = NULL;
char *token = NULL;
char *delim = ",=:;><";

	// RANGE
	p = xCOMMS_strstr( "R0\0");
	if ( p != NULL ) {

		// Copio el mensaje enviado a un buffer local porque la funcion strsep lo modifica.
		memset( &localStr, '\0', sizeof(localStr) );
		memcpy(localStr,p,sizeof(localStr));
		stringp = localStr;
		token = strsep(&stringp,delim);		// R0
		token = strsep(&stringp,delim);		// Range name
		range_config(token);

		u_debug_printf_P( DEBUG_COMMS, PSTR("COMMS: Reconfig RANGE\r\n\0"));

		u_save_params_in_NVMEE();
	}

}
//------------------------------------------------------------------------------------
static void ac_process_response_PSENSOR(void)
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

	p = xCOMMS_strstr( "PS0:\0");
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

		u_debug_printf_P( DEBUG_COMMS, PSTR("COMMS: Reconfig PSENSOR\r\n\0"));

		u_save_params_in_NVMEE();

	}

}
//------------------------------------------------------------------------------------
