/*
 * tkComms_10_dataframe.c
 *
 *  Created on: 10 mar. 2020
 *      Author: pablo
 */

#include <tkComms.h>
#include <../spx_tkApp/tkApp.h>

// La tarea no puede demorar mas de 300s.

#define WDG_COMMS_TO_DATAFRAME	WDG_TO300

static st_dataRecord_t dataRecord;

static void data_send_record( void );
static void data_process_response_RESET(void);
static void data_process_response_MEMFORMAT(void);
static void data_process_response_DOUTS(void);
static void data_process_response_CLOCK(void);
static uint8_t data_process_response_OK(void);

//------------------------------------------------------------------------------------
t_comms_states tkComms_st_dataframe(void)
{
	/* Estado en que procesa los frames de datos, los transmite y procesa
	 * las respuestas
	 * Si no hay datos para transmitir sale
	 */

// ENTRY

#ifdef BETA_TEST
	xprintf_PD( DF_COMMS, PSTR("COMMS: IN st_dataframe.[%d,%d,%d]\r\n\0"),xCOMMS_stateVars.gprs_prendido, xCOMMS_stateVars.gprs_inicializado,xCOMMS_stateVars.errores_comms);
#else
	xprintf_PD( DF_COMMS, PSTR("COMMS: IN st_dataframe.\r\n"));
#endif

#ifdef MONITOR_STACK
	debug_monitor_stack_watermarks("13");
#endif

	xprintf_P( PSTR("COMMS: dataframe.\r\n\0"));
	ctl_watchdog_kick(WDG_COMMS, WDG_COMMS_TO_DATAFRAME );

	while ( xCOMMS_datos_para_transmitir() > 0 ) {
		xCOMMS_process_frame(DATA, sVarsComms.server_ip_address, sVarsComms.server_tcp_port );
	}

	// Si llego la senal, la reseteo ya que transmiti todos los frames.
	xCOMMS_SGN_FRAME_READY();

#ifdef BETA_TEST
	xprintf_PD( DF_COMMS, PSTR("COMMS: OUT st_dataframe.[%d,%d,%d]\r\n\0"),xCOMMS_stateVars.gprs_prendido, xCOMMS_stateVars.gprs_inicializado,xCOMMS_stateVars.errores_comms);
#else
	xprintf_PD( DF_COMMS, PSTR("COMMS: OUT st_dataframe.\r\n\0"));
#endif

	return(ST_ENTRY);

}
//------------------------------------------------------------------------------------
void xDATA_FRAME_send(void)
{

uint8_t registros_trasmitidos = 0;

	// Envio un window frame
	registros_trasmitidos = 0;
	FF_rewind();

	xCOMMS_flush_RX();
	xCOMMS_flush_TX();

	xCOMMS_send_header("DATA");
	xprintf_PVD(  xCOMMS_get_fd(), DF_COMMS, PSTR("&PLOAD=\0") );

	while ( ( xCOMMS_datos_para_transmitir() > 0 ) && ( registros_trasmitidos < MAX_RCDS_WINDOW_SIZE ) ) {
		data_send_record();
		registros_trasmitidos++;
		// Espero 250ms entre records
		vTaskDelay( (portTickType)( INTER_FRAMES_DELAY / portTICK_RATE_MS ) );
	}

	xCOMMS_send_tail();

}
//------------------------------------------------------------------------------------
t_responses xDATA_FRAME_process_response(void)
{
	// Las respuestas pueden venir enganchadas por lo que hay que procesarlas todas !!

t_responses rsp = rsp_NONE;

	// Recibi un ERROR de respuesta
	if ( xCOMMS_check_response("ERROR") ) {
		xCOMMS_print_RX_buffer();
		rsp = rsp_ERROR;
		return(rsp);
	}

	// Respuesta completa del server
	if ( xCOMMS_check_response("</h1>") ) {

		xCOMMS_print_RX_buffer();

		if ( xCOMMS_check_response ("ERROR\0")) {
			// ERROR del server: salgo inmediatamente
			rsp = rsp_ERROR;
			return(rsp);
		}

		if ( xCOMMS_check_response ("RESET\0")) {
			// El sever mando la orden de resetearse inmediatamente
			data_process_response_RESET();
			rsp = rsp_OK;
			return(rsp);
		}

		if ( xCOMMS_check_response ("MFORMAT\0")) {
			// El sever mando la orden de formatear la memoria y resetearse
			data_process_response_MEMFORMAT();
			rsp = rsp_OK;
			return(rsp);
		}

		// El resto de las respuestas pueden venir enganchadas: hay que procesarlas todas.

		if ( xCOMMS_check_response ("DOUTS\0")) {
			// El sever mando actualizacion de las salidas
			data_process_response_DOUTS();
			// rsp = rsp_OK;
			// return(rsp);
		}

		if ( xCOMMS_check_response ("CLOCK\0")) {
			// El sever mando actualizacion del rtc
			data_process_response_CLOCK();
			// rsp = rsp_OK;
			// return(rsp);
		}

		/*
		 * Lo ultimo que debo procesar es el OK !!!
		 */
		if ( xCOMMS_check_response ("RX_OK\0")) {
			// Datos procesados por el server.
			data_process_response_OK();
			rsp = rsp_OK;
			return(rsp);
		}
	}

// Exit:
	// No tuve respuesta aun
	return(rsp);
}
//------------------------------------------------------------------------------------
// FUNCIONES AUXILIARES
//------------------------------------------------------------------------------------
static void data_send_record( void )
{
	/* Leo un registro de la memoria haciendo el proceso inverso de
	 * cuando los grabe en spx_tkData::pv_data_guardar_en_BD y lo
	 * mando por el canal de comunicaciones
	 */

size_t bRead;
FAT_t fat;

	memset ( &fat, '\0', sizeof(FAT_t));
	memset ( &dataRecord, '\0', sizeof( st_dataRecord_t));

	// Paso1: Leo un registro de memoria
	bRead = FF_readRcd( &dataRecord, sizeof(st_dataRecord_t));
	FAT_read(&fat);

	if ( bRead == 0) {
		return;
	}

	xprintf_PVD(  xCOMMS_get_fd(), DF_COMMS, PSTR("CTL:%d;\0"),fat.rdPTR );
	xCOMMS_send_dr(DF_COMMS, &dataRecord);

}
//------------------------------------------------------------------------------------
static void data_process_response_RESET(void)
{
	// El server me pide que me resetee de modo de mandar un nuevo init y reconfigurarme

	xprintf_P( PSTR("COMMS: RESET...\r\n\0"));

	vTaskDelay( ( TickType_t)( 1000 / portTICK_RATE_MS ) );
	// RESET
	CCPWrite( &RST.CTRL, RST_SWRST_bm );   /* Issue a Software Reset to initilize the CPU */

}
//------------------------------------------------------------------------------------
static void data_process_response_MEMFORMAT(void)
{
	// El server me pide que me reformatee la memoria y me resetee

	xprintf_P( PSTR("COMMS: MFORMAT...\r\n\0"));

	vTaskDelay( ( TickType_t)( 1000 / portTICK_RATE_MS ) );
	//
	// Primero reseteo a default
	u_load_defaults(NULL);
	u_save_params_in_NVMEE();

	// Nadie debe usar la memoria !!!
	ctl_watchdog_kick(WDG_CMD, 0x8000 );

	vTaskSuspend( xHandle_tkAplicacion );
	ctl_watchdog_kick(WDG_APP, 0x8000 );

	vTaskSuspend( xHandle_tkInputs );
	ctl_watchdog_kick(WDG_DINPUTS, 0x8000 );

	//vTaskSuspend( xHandle_tkComms );
	//ctl_watchdog_kick(WDG_COMMSRX, 0x8000 );

	// No suspendo esta tarea porque estoy dentro de ella. !!!
	//vTaskSuspend( xHandle_tkGprsTx );
	//ctl_watchdog_kick(WDG_COMMSRX, 0x8000 );

	// Formateo
	FF_format(true);

	// Reset
	CCPWrite( &RST.CTRL, RST_SWRST_bm );   /* Issue a Software Reset to initilize the CPU */

}
//------------------------------------------------------------------------------------
static void data_process_response_DOUTS(void)
{
	/*
	 * Recibo una respuesta que me dice que valores poner en las salidas
	 * de modo de controlar las bombas de una perforacion u otra aplicacion
	 * Recibi algo del estilo DOUTS:245
	 * Extraigo el valor de las salidas y las seteo.
	 */


char localStr[32] = { 0 };
char *stringp = NULL;
char *tk_douts = NULL;
char *delim = ",;:=><";
char *p = NULL;
uint8_t douts;

	p = xCOMM_get_buffer_ptr("DOUTS");
	if ( p != NULL ) {
		memset( &localStr, '\0', sizeof(localStr) );
		memcpy(localStr,p,sizeof(localStr));

		stringp = localStr;
		tk_douts = strsep(&stringp,delim);	// DOUTS
		tk_douts = strsep(&stringp,delim);	// Str. con el valor de las salidas. 0..128
		douts = atoi( tk_douts );

		// Actualizo el status a travez de una funcion propia del modulo de outputs
		xAPP_set_douts_remote( douts);

		xprintf_PD( DF_COMMS, PSTR("COMMS: DOUTS [0x%0X]\r\n\0"), douts);

	}
}
//------------------------------------------------------------------------------------
static uint8_t data_process_response_OK(void)
{
	/*
	 * Retorno la cantidad de registros procesados ( y borrados )
	 * Recibi un OK del server y el ultimo ID de registro a borrar.
	 * Los borro de a uno.
	 */

uint8_t recds_borrados = 0;
FAT_t fat;

	memset ( &fat, '\0', sizeof( FAT_t));

	// Borro los registros.
	while (  u_check_more_Rcds4Del(&fat) ) {
		FF_deleteRcd();
		recds_borrados++;
		FAT_read(&fat);
		xprintf_PD( DF_COMMS, PSTR("COMMS: mem wrPtr=%d,rdPtr=%d,delPtr=%d,r4wr=%d,r4rd=%d,r4del=%d \r\n\0"), fat.wrPTR, fat.rdPTR, fat.delPTR, fat.rcds4wr, fat.rcds4rd, fat.rcds4del );
	}

	return(recds_borrados);
}
//------------------------------------------------------------------------------------
static void data_process_response_CLOCK(void)
{
	/*
	 * Recibi algo del estilo CLOCK:1910120345
	 * Extraigo el valor, veo que tan desfasado estoy del RTC
	 * Si estoy mas de 60s, actualizo el RTC
	 */


char localStr[32] = { 0 };
char *stringp = NULL;
char *token = NULL;
char *delim = ",;:=><";
char *p = NULL;
char rtcStr[12];
uint8_t i = 0;
char c = '\0';
RtcTimeType_t rtc;
int8_t xBytes = 0;

	// CLOCK
	p = xCOMM_get_buffer_ptr("CLOCK");
	if ( p != NULL ) {
		// Copio el mensaje enviado a un buffer local porque la funcion strsep lo modifica.
		memset( &localStr, '\0', sizeof(localStr) );
		memcpy(localStr,p,sizeof(localStr));
		// Estraigo el string con la hora del server y lo dejo en una estructura RtcTimeType
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
		if ( strlen(rtcStr) < 10 ) {
			// Hay un error en el string que tiene la fecha.
			// No lo reconfiguro
			xprintf_P(PSTR("COMMS: ERROR rspCLOCK RTCstring [%s]\r\n\0"), rtcStr );
			return;
		}

		memset( &rtc, '\0', sizeof(rtc) );
		RTC_str2rtc(rtcStr, &rtc);			// Convierto el string YYMMDDHHMM a RTC.

		// Vemos si es necesario ajustar el RTC del datalogger.
		if ( RTC_has_drift(&rtc, 60 ) ) {
			xBytes = RTC_write_dtime(&rtc);		// Grabo el RTC
			if ( xBytes == -1 )
				xprintf_P(PSTR("ERROR: I2C:RTC:data_process_response_CLOCK\r\n\0"));

			xprintf_PD( DF_COMMS, PSTR("COMMS: DATA ONLINE Update rtc to: %s\r\n\0"), rtcStr );
		}

	}
}
//------------------------------------------------------------------------------------


