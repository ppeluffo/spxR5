/*
 * spx_tkGprs_utils.c
 *
 *  Created on: 25 de oct. de 2017
 *      Author: pablo
 */

#include "gprs.h"

//------------------------------------------------------------------------------------
void u_gprs_open_socket(void)
{
	// Envio el comando AT para abrir el socket

	if ( systemVars.debug == DEBUG_GPRS ) {
		xprintf_P( PSTR("GPRS: try to open socket\r\n\0"));
	}

	u_gprs_flush_RX_buffer();
	xCom_printf_P( fdGPRS, PSTR("AT+CIPOPEN=0,\"TCP\",\"%s\",%s\r\n\0"),systemVars.server_ip_address,systemVars.server_tcp_port);
	vTaskDelay( (portTickType)( 1500 / portTICK_RATE_MS ) );

	if ( systemVars.debug == DEBUG_GPRS ) {
		u_gprs_print_RX_Buffer();
	}
}
//------------------------------------------------------------------------------------
t_socket_status u_gprs_check_socket_status(void)
{
	// El socket esta abierto si el modem esta prendido y
	// el DCD esta en 0.
	// Cuando el modem esta apagado pin_dcd = 0
	// Cuando el modem esta prendido y el socket cerrado pin_dcd = 1
	// Cuando el modem esta prendido y el socket abierto pin_dcd = 0.

uint8_t pin_dcd;
t_socket_status socket_status = SOCK_CLOSED;

	pin_dcd = IO_read_DCD();

	if ( ( GPRS_stateVars.modem_prendido == true ) && ( pin_dcd == 0 ) ){
		socket_status = SOCK_OPEN;
		if ( systemVars.debug == DEBUG_GPRS ) {
			xprintf_P( PSTR("GPRS: sckt is open (dcd=%d)\r\n\0"),pin_dcd);
		}

	} else if ( u_gprs_check_response("Operation not supported")) {
		socket_status = SOCK_ERROR;
		if ( systemVars.debug == DEBUG_GPRS ) {
			xprintf_P( PSTR("GPRS: sckt ERROR\r\n\0"));
		}

	} else if ( u_gprs_check_response("ERROR")) {
		socket_status = SOCK_ERROR;
		if ( systemVars.debug == DEBUG_GPRS ) {
			xprintf_P( PSTR("GPRS: sckt ERROR\r\n\0"));
		}

	} else {
		socket_status = SOCK_CLOSED;
		if ( systemVars.debug == DEBUG_GPRS ) {
			xprintf_P( PSTR("GPRS: sckt is close (dcd=%d)\r\n\0"),pin_dcd);
		}

	}

	return(socket_status);

}
//------------------------------------------------------------------------------------
void u_gprs_load_defaults(void)
{

	if ( spx_io_board == SPX_IO8CH ) {
		systemVars.timerDial = 0;
	} else if ( spx_io_board == SPX_IO5CH ) {
		systemVars.timerDial = 900;
	}

	snprintf_P( systemVars.apn, APN_LENGTH, PSTR("SPYMOVIL.VPNANTEL\0") );
	strncpy_P(systemVars.server_ip_address, PSTR("192.168.0.9\0"),16);
	strncpy_P(systemVars.serverScript, PSTR("/cgi-bin/sp5K/spxR3.pl\0"),SCRIPT_LENGTH);
	strncpy_P(systemVars.server_tcp_port, PSTR("80\0"),PORT_LENGTH	);
	strncpy_P(systemVars.simpwd, PSTR("spymovil123\0"),PASSWD_LENGTH);

/*
#ifdef APP_SP5K_SPYMOVIL
		snprintf_P( systemVars.apn, APN_LENGTH, PSTR("SPYMOVIL.VPNANTEL\0") );
		strncpy_P(systemVars.server_ip_address, PSTR("192.168.0.9\0"),16);
		strncpy_P(systemVars.serverScript, PSTR("/cgi-bin/sp5K/sp5K.pl\0"),SCRIPT_LENGTH);
#endif
#ifdef APP_SP5K_OSE
		snprintf_P( systemVars.apn, APN_LENGTH, PSTR("STG1.VPNANTEL\0") );
		strncpy_P(systemVars.server_ip_address, PSTR("172.27.0.26\0"),16);
		strncpy_P(systemVars.serverScript, PSTR("/cgi-bin/sp5K/sp5K.pl\0"),SCRIPT_LENGTH);
#endif

#ifdef APP_SPX_LATAHONA
		snprintf_P( systemVars.apn, APN_LENGTH, PSTR("SPYMOVIL.VPNANTEL\0") );
		strncpy_P(systemVars.server_ip_address, PSTR("192.168.0.9\0"),16);
		strncpy_P(systemVars.serverScript, PSTR("/cgi-bin/spx/spx_th.pl\0"),SCRIPT_LENGTH);
#endif
#ifdef APP_SPX_SPYMOVIL
		snprintf_P( systemVars.apn, APN_LENGTH, PSTR("SPYMOVIL.VPNANTEL\0") );
		strncpy_P(systemVars.server_ip_address, PSTR("192.168.0.9\0"),16);
		strncpy_P(systemVars.serverScript, PSTR("/cgi-bin/spx/spxR3.pl\0"),SCRIPT_LENGTH);
#endif
*/

}
//------------------------------------------------------------------------------------
void u_gprs_rxbuffer_flush(void)
{
	memset( pv_gprsRxCbuffer.buffer, '\0', UART_GPRS_RXBUFFER_LEN);
	pv_gprsRxCbuffer.ptr = 0;
}
//------------------------------------------------------------------------------------
bool u_gprs_check_response( const char *rsp )
{
bool retS = false;

	if ( strstr( pv_gprsRxCbuffer.buffer, rsp) != NULL ) {
		retS = true;
	}
	return(retS);
}
//------------------------------------------------------------------------------------
void u_gprs_print_RX_response(void)
{
	// Imprime la respuesta del server.
	// Utiliza el buffer de RX.
	// Solo muestra el payload, es decir lo que esta entre <h1> y </h1>
	// Todas las respuestas el server las encierra entre ambos tags excepto los errores del apache.

	char *start_tag, *end_tag;

	start_tag = strstr(pv_gprsRxCbuffer.buffer,"<h1>");
	end_tag = strstr(pv_gprsRxCbuffer.buffer, "</h1>");

	if ( ( start_tag != NULL ) && ( end_tag != NULL) ) {
		*end_tag = '\0';	// Para que frene el xprintf_P
		start_tag += 4;
		//xprintf_P ( PSTR("GPRS: rsp>%s\r\n\0"), start_tag );
		xprintf_P( PSTR ("GPRS: rxbuff>\r\n\0"));
		xnprint( start_tag, sizeof(pv_gprsRxCbuffer.buffer) );
		xprintf_P( PSTR ("\r\n[%d]\r\n\0"), pv_gprsRxCbuffer.ptr );
	}
}
//------------------------------------------------------------------------------------
void u_gprs_print_RX_Buffer(void)
{

	// Imprimo todo el buffer local de RX. Sale por \0.
	xprintf_P( PSTR ("GPRS: rxbuff>\r\n\0"));

	// Uso esta funcion para imprimir un buffer largo, mayor al que utiliza xprintf_P. !!!
	xnprint( pv_gprsRxCbuffer.buffer, sizeof(pv_gprsRxCbuffer.buffer) );
	xprintf_P( PSTR ("\r\n[%d]\r\n\0"), pv_gprsRxCbuffer.ptr );
}
//------------------------------------------------------------------------------------
void u_gprs_flush_RX_buffer(void)
{

	frtos_ioctl( fdGPRS,ioctl_UART_CLEAR_RX_BUFFER, NULL);
	u_gprs_rxbuffer_flush();

}
//------------------------------------------------------------------------------------
void u_gprs_flush_TX_buffer(void)
{
	frtos_ioctl( fdGPRS,ioctl_UART_CLEAR_TX_BUFFER, NULL);

}
//------------------------------------------------------------------------------------
void u_gprs_ask_sqe(void)
{
	// Veo la calidad de senal que estoy recibiendo

char csqBuffer[32];
char *ts = NULL;

	// AT+CSQ
	u_gprs_flush_RX_buffer();
	xCom_printf_P( fdGPRS, PSTR("AT+CSQ\r\0"));
	vTaskDelay( (portTickType)( 500 / portTICK_RATE_MS ) );

	if ( systemVars.debug == DEBUG_GPRS ) {
		u_gprs_print_RX_Buffer();
	}

	memcpy(csqBuffer, &pv_gprsRxCbuffer.buffer[0], sizeof(csqBuffer) );
	if ( (ts = strchr(csqBuffer, ':')) ) {
		ts++;
		GPRS_stateVars.csq = atoi(ts);
		GPRS_stateVars.dbm = 113 - 2 * GPRS_stateVars.csq;
	}

	// LOG & DEBUG
	xprintf_P ( PSTR("GPRS: signalQ CSQ=%d,DBM=%d\r\n\0"), GPRS_stateVars.csq, GPRS_stateVars.dbm );

}
//------------------------------------------------------------------------------------
bool u_gprs_modem_prendido(void)
{
	return(GPRS_stateVars.modem_prendido);
}
//------------------------------------------------------------------------------------
int32_t u_gprs_readTimeToNextDial(void)
{
	return(waiting_time);
}
//----------------------------------------------------------------------------------------
void u_gprs_redial(void)
{
	GPRS_stateVars.signal_redial = true;

}
//----------------------------------------------------------------------------------------
void u_gprs_config_timerdial ( char *s_timerdial )
{
	// El timer dial puede ser 0 si vamos a trabajar en modo continuo o mayor a
	// 15 minutos.
	// Es una variable de 32 bits para almacenar los segundos de 24hs.

	while ( xSemaphoreTake( sem_SYSVars, ( TickType_t ) 5 ) != pdTRUE )
		taskYIELD();

	systemVars.timerDial = atoi(s_timerdial);

	// Controlo que este en los rangos permitidos
	if ( (systemVars.timerDial > 0) && (systemVars.timerDial < 900) ) {
		systemVars.timerDial = 0;
		xprintf_P( PSTR("TDIAL warn !! Default to 0. ( continuo TDIAL=0, discreto TDIAL > 900)\r\n\0"));
	}

	xSemaphoreGive( sem_SYSVars );
	return;
}
//------------------------------------------------------------------------------------
void u_gprs_modem_pwr_off(void)
{
	// Apago la fuente del modem

	IO_clr_GPRS_SW();	// Es un FET que lo dejo cortado
	vTaskDelay( (portTickType)( 100 / portTICK_RATE_MS ) );
	IO_clr_GPRS_PWR();	// Apago la fuente.
	vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );

}
//------------------------------------------------------------------------------------
void u_gprs_modem_pwr_on(uint8_t pwr_time )
{
	// Prendo la fuente del modem

	IO_clr_GPRS_SW();	// GPRS=0V, PWR_ON pullup 1.8V )
	IO_set_GPRS_PWR();											// Prendo la fuente ( alimento al modem ) HW
	vTaskDelay( (portTickType)( ( 2000 + 2000 * pwr_time) / portTICK_RATE_MS ) );	// Espero 2s que se estabilize la fuente.


}
//------------------------------------------------------------------------------------
void u_gprs_modem_pwr_sw(void)
{
	// Genera un pulso en la linea PWR_SW. Como tiene un FET la senal se invierte.
	// En reposo debe la linea estar en 0 para que el fet flote y por un pull-up del modem
	// la entrada PWR_SW esta en 1.
	// El PWR_ON se pulsa a 0 saturando el fet.
	IO_set_GPRS_SW();	// GPRS_SW = 3V, PWR_ON = 0V.
	vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
	IO_clr_GPRS_SW();	// GPRS_SW = 0V, PWR_ON = pullup, 1.8V

}
//------------------------------------------------------------------------------------
void u_gprs_configPwrSave( char *s_modo, char *s_startTime, char *s_endTime)
{
	// Recibe como parametros el modo ( 0,1) y punteros a string con las horas de inicio y fin del pwrsave
	// expresadas en minutos.

	while ( xSemaphoreTake( sem_SYSVars, ( TickType_t ) 5 ) != pdTRUE )
		taskYIELD();

	if (!strcmp_P( strupr(s_modo), PSTR( "OFF"))) {
		systemVars.pwrSave.modo = modoPWRSAVE_OFF;
		goto quit;
	}

	if (!strcmp_P( strupr(s_modo), PSTR( "ON"))) {
		systemVars.pwrSave.modo = modoPWRSAVE_ON;
		if ( s_startTime != NULL ) { u_convert_str_to_time_t( s_startTime, &systemVars.pwrSave.hora_start); }
		if ( s_endTime != NULL ) { u_convert_str_to_time_t( s_endTime, &systemVars.pwrSave.hora_fin); }
		goto quit;
	}

quit:

	xSemaphoreGive( sem_SYSVars );

}
//------------------------------------------------------------------------------------

