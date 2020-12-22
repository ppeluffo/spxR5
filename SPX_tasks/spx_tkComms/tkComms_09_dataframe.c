/*
 * tkComms_10_dataframe.c
 *
 *  Created on: 10 mar. 2020
 *      Author: pablo
 */

#include <tkComms.h>
#include <../spx_tkApp/tkApp.h>

// La tarea no puede demorar mas de 300s.

static void data_process_response_RESET(void);
static void data_process_response_MEMFORMAT(void);
static void data_process_response_DOUTS(void);
static void data_process_response_CLOCK(void);
static uint8_t data_process_response_OK(void);
static void data_process_response_MBUS(void);

//------------------------------------------------------------------------------------
t_comms_states tkComms_st_dataframe(void)
{
	/* Estado en que procesa los frames de datos, los transmite y procesa
	 * las respuestas
	 * Si no hay datos para transmitir sale
	 */

	xprintf_PD( DF_COMMS, PSTR("COMMS: IN st_dataframe.\r\n"));

	xCOMMS_process_frame(DATA, sVarsComms.server_ip_address, sVarsComms.server_tcp_port );

	// Si llego la senal, la reseteo ya que transmiti todos los frames.
	xCOMMS_SGN_FRAME_READY();

	xprintf_PD( DF_COMMS, PSTR("COMMS: OUT st_dataframe.\r\n\0"));
	return(ST_ENTRY);

}
//------------------------------------------------------------------------------------
t_send_status xDATA_FRAME_send(void)
{

uint8_t registros_trasmitidos = 0;

	if( xCOMMS_datos_para_transmitir() == 0 ) {
		return(SEND_NODATA);
	}

	// Envio un window frame
	registros_trasmitidos = 0;
	FF_rewind();

	// Header
	xCOMMS_flush_RX();
	xCOMMS_flush_TX();
	xCOMMS_xbuffer_init();
	xfprintf_P(fdFILE, PSTR("GET %s?DLGID=%s&TYPE=DATA&VER=%s&PLOAD=" ), sVarsComms.serverScript, sVarsComms.dlgId, SPX_FW_REV );
	if ( xCOMMS_xbuffer_send(DF_COMMS) < 0 ) {
		return(SEND_FAIL);
	}

	while ( ( xCOMMS_datos_para_transmitir() > 0 ) && ( registros_trasmitidos < MAX_RCDS_WINDOW_SIZE ) ) {

		xCOMMS_xbuffer_load_dataRecord();
		if ( xCOMMS_xbuffer_send(DF_COMMS) < 0 ) {
			return(SEND_FAIL);
		}
		registros_trasmitidos++;
		// Espero 250ms entre records
		vTaskDelay( (portTickType)( INTER_FRAMES_DELAY / portTICK_RATE_MS ) );
	}

	// Tail();
	xCOMMS_flush_RX();
	xCOMMS_flush_TX();
	xCOMMS_xbuffer_init();
	xfprintf_P(fdFILE, PSTR(" HTTP/1.1\r\nHost: www.spymovil.com\r\n\r\n\r\n") );
	if ( xCOMMS_xbuffer_send(DF_COMMS) < 0 ) {
		return(SEND_FAIL);
	}

	return(SEND_OK);
}
//------------------------------------------------------------------------------------
t_responses xDATA_FRAME_process_response(void)
{
	// Las respuestas pueden venir enganchadas por lo que hay que procesarlas todas !!

t_responses rsp = rsp_NONE;

	// Recibi un ERROR de respuesta
	if ( xCOMMS_check_response(0, "ERROR") > 0 ) {
		xCOMMS_print_RX_buffer();
		rsp = rsp_ERROR;
		return(rsp);
	}

	// Respuesta completa del server
	if ( xCOMMS_check_response(0, "</h1>") > 0 ) {

		xCOMMS_print_RX_buffer();

		if ( xCOMMS_check_response (0, "ERROR\0") > 0) {
			// ERROR del server: salgo inmediatamente
			rsp = rsp_ERROR;
			return(rsp);
		}

		if ( xCOMMS_check_response (0, "RESET\0") > 0) {
			// El sever mando la orden de resetearse inmediatamente
			data_process_response_RESET();
			rsp = rsp_OK;
			return(rsp);
		}

		if ( xCOMMS_check_response (0, "MFORMAT\0") > 0) {
			// El sever mando la orden de formatear la memoria y resetearse
			data_process_response_MEMFORMAT();
			rsp = rsp_OK;
			return(rsp);
		}

		// El resto de las respuestas pueden venir enganchadas: hay que procesarlas todas.

		if ( xCOMMS_check_response (0, "DOUTS\0") > 0) {
			// El sever mando actualizacion de las salidas
			data_process_response_DOUTS();
			// rsp = rsp_OK;
			// return(rsp);
		}

		if ( xCOMMS_check_response (0, "CLOCK\0") > 0) {
			// El sever mando actualizacion del rtc
			data_process_response_CLOCK();
			// rsp = rsp_OK;
			// return(rsp);
		}

		if ( xCOMMS_check_response (0, "MBUS\0") > 0) {
			data_process_response_MBUS();
			// rsp = rsp_OK;
			// return(rsp);
		}

		/*
		 * Lo ultimo que debo procesar es el OK !!!
		 */
		if ( xCOMMS_check_response (0, "RX_OK\0") > 0) {
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

int p1,p2;
char localStr[32] = { 0 };
char *stringp = NULL;
char *tk_douts = NULL;
char *delim = ",;:=><";
uint8_t douts;

	p1 = xCOMMS_check_response(0, "<h1>");
	if ( p1 == -1 ) {
		return;
	}
	p2 = xCOMMS_check_response( p1, "DOUTS");

	if ( p2 >= 0 ) {
		memset( &localStr, '\0', sizeof(localStr) );
		xCOMMS_rxbuffer_copy_to(localStr, p2, sizeof(localStr));

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
	 * <h1>TYPE=DATA&PLOAD=RX_OK:848;CLOCK:2012210626;DOUTS=5:</h1>
	 * <h1>TYPE=DATA&PLOAD=RX_OK:845;CLOCK:2012210625;</h1>
	 * Extraigo el valor, veo que tan desfasado estoy del RTC
	 * Si estoy mas de 60s, actualizo el RTC
	 *
	 * Alerta:
	 * Por error puede venir algo como <h1>TYPE=DATA&PLOAD=RX_OK:844;CLOCK:201221DOUTS=13:</h1>.
	 * En este caso debo descartar el ajuste de hora !!!
	 */

int p1,p2;
char localStr[32] = { 0 };
char *stringp = NULL;
char *token = NULL;
char *delim = ",;:=><";
char rtcStr[12];
uint8_t i = 0;
char c = '\0';
RtcTimeType_t rtc;
int8_t xBytes = 0;

	// CLOCK
	p1 = xCOMMS_check_response(0, "<h1>");
	if ( p1 == -1 ) {
		return;
	}
	p2 = xCOMMS_check_response( p1, "CLOCK");

	if ( p2 >= 0 ) {
		// Copio el mensaje enviado a un buffer local porque la funcion strsep lo modifica.
		memset( &localStr, '\0', sizeof(localStr) );
		xCOMMS_rxbuffer_copy_to(localStr, p2, sizeof(localStr));
		// Estraigo el string con la hora del server y lo dejo en una estructura RtcTimeType
		stringp = localStr;
		token = strsep(&stringp,delim);			// CLOCK
		token = strsep(&stringp,delim);			// 1910120345

		memset(rtcStr, '\0', sizeof(rtcStr));
		//memcpy(rtcStr,token, sizeof(rtcStr));	// token apunta al comienzo del string con la hora
		// Controlo que todos los caracteres sean numericos. Sino indica un error de parseo y salgo.
		for ( i = 0; i<10; i++) {
			c = *token;
			if ( ! isdigit(c)) {
				// Catch:
				xprintf_P(PSTR("COMMS: ERROR01 rspCLOCK RTCstring [%s]. Exit !!.\r\n\0"), rtcStr );
				return;
			}
			rtcStr[i] = c;
			c = *(++token);
			if ( c == '\0' )
				break;
		}

		if ( strlen(rtcStr) < 10 ) {
			// Hay un error en el string que tiene la fecha.
			// No lo reconfiguro
			xprintf_P(PSTR("COMMS: ERROR02 rspCLOCK RTCstring [%s]\r\n\0"), rtcStr );
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
static void data_process_response_MBUS(void)
{
	/*
	 * Recibo una respuesta que me dice que valores enviar por modbus
	 * para escribir un holding register
	 * Recibi algo del estilo MBUS:0xABCD,0x1234
	 * 0xABCD es la direccion del holding register
	 * 0x1234 es el valor
	 */

int p1,p2;
char localStr[32] = { 0 };
char *stringp = NULL;
char *tk_hr_address = NULL;
char *tk_hr_value = NULL;
char *delim = ",;:=><";


	p1 = xCOMMS_check_response(0, "<h1>");
	if ( p1 == -1 ) {
		return;
	}
	p2 = xCOMMS_check_response( p1, "MBUS");

	if ( p2 >= 0 ) {
		memset( &localStr, '\0', sizeof(localStr) );
		xCOMMS_rxbuffer_copy_to(localStr, p2, sizeof(localStr));

		stringp = localStr;
		tk_hr_address = strsep(&stringp,delim);	// MBUS
		tk_hr_address = strsep(&stringp,delim);	// Str. con la direccion del holding register
		tk_hr_value = strsep(&stringp,delim);	// Str. con el valor del holding register

		modbus_set_hr( DF_COMMS , systemVars.modbus_conf.modbus_slave_address ,"0x06", tk_hr_address, tk_hr_value);

		xprintf_PD( DF_COMMS, PSTR("COMMS: MBUS set HR[%s]=%s\r\n\0"), tk_hr_address, tk_hr_value );

	}
}
//------------------------------------------------------------------------------------


