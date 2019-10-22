/*
 * spx_tkGprs_utils.c
 *
 *  Created on: 25 de oct. de 2017
 *      Author: pablo
 */

#include <spx_tkComms/gprs.h>

//------------------------------------------------------------------------------------
t_socket_status u_gprs_open_socket(void)
{
	// Envio el comando AT para abrir el socket.

uint8_t timeout = 0;
uint8_t await_loops = 0;
t_socket_status socket_status = SOCK_FAIL;

	if ( systemVars.debug == DEBUG_GPRS ) {
		xprintf_P( PSTR("GPRS: try to open socket\r\n\0"));
	}

	u_gprs_flush_RX_buffer();
	xCom_printf_P( fdGPRS, PSTR("AT+CIPOPEN=0,\"TCP\",\"%s\",%s\r\n\0"), GPRS_stateVars.server_ip_address, systemVars.gprs_conf.server_tcp_port);
	vTaskDelay( (portTickType)( 1500 / portTICK_RATE_MS ) );

	if ( systemVars.debug == DEBUG_GPRS ) {
		u_gprs_print_RX_Buffer();
	}

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
			break;
		}
	}

	return(socket_status);
}
//------------------------------------------------------------------------------------
void u_gprs_close_socket(void)
{
	// Envio el comando AT para cerrar el socket.

	if ( systemVars.debug == DEBUG_GPRS ) {
		xprintf_P( PSTR("GPRS: try to close socket\r\n\0"));
	}

	u_gprs_flush_RX_buffer();
	// Envio el comando para ver si el socket esta cerrado
	xCom_printf_P( fdGPRS, PSTR("AT+CIPCLOSE=?\r\n\0"));
	vTaskDelay( (portTickType)( 500 / portTICK_RATE_MS ) );

	if ( u_gprs_check_response("+CIPCLOSE: 0,") ) {
		// Socket cerrado.
		return;
	} else {
		u_gprs_flush_RX_buffer();
		// Envio el comando para ver si el socket esta cerrado
		xCom_printf_P( fdGPRS, PSTR("AT+CIPCLOSE=0\r\n\0"));
		vTaskDelay( (portTickType)( 2000 / portTICK_RATE_MS ) );
		if ( systemVars.debug == DEBUG_GPRS ) {
			u_gprs_print_RX_Buffer();
		}
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

uint8_t pin_dcd = 0;
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

	} else if ( u_gprs_check_response("+CIPEVENT:")) {
		// El socket no se va a recuperar. Hay que cerrar el net y volver a abrirlo.
		socket_status = SOCK_FAIL;
		if ( systemVars.debug == DEBUG_GPRS ) {
			xprintf_P( PSTR("GPRS: sckt FAIL +CIPEVENT:\r\n\0"));
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
void u_gprs_load_defaults( char *opt )
{

char l_data[10] = { 0 };

	memcpy(l_data, opt, sizeof(l_data));
	strupr(l_data);

	if ( spx_io_board == SPX_IO8CH ) {
		systemVars.gprs_conf.timerDial = 0;
	} else if ( spx_io_board == SPX_IO5CH ) {
		systemVars.gprs_conf.timerDial = 900;
	}

	if (!strcmp_P( l_data, PSTR("SPY\0"))) {
		snprintf_P( systemVars.gprs_conf.apn, APN_LENGTH, PSTR("SPYMOVIL.VPNANTEL\0") );
		strncpy_P(systemVars.gprs_conf.server_ip_address, PSTR("192.168.0.9\0"),16);

	} else if (!strcmp_P( l_data, PSTR("UTE\0"))) {
		snprintf_P( systemVars.gprs_conf.apn, APN_LENGTH, PSTR("SPYMOVIL.VPNANTEL\0") );
		strncpy_P(systemVars.gprs_conf.server_ip_address, PSTR("192.168.8.13\0"),16);

	} else if (!strcmp_P( l_data, PSTR("OSE\0"))) {
		snprintf_P( systemVars.gprs_conf.apn, APN_LENGTH, PSTR("STG1.VPNANTEL\0") );
		strncpy_P(systemVars.gprs_conf.server_ip_address, PSTR("172.27.0.26\0"),16);

	} else {
		snprintf_P( systemVars.gprs_conf.apn, APN_LENGTH, PSTR("DEFAULT\0") );
		strncpy_P(systemVars.gprs_conf.server_ip_address, PSTR("DEFAULT\0"),16);
	}

	snprintf_P( systemVars.gprs_conf.dlgId, DLGID_LENGTH, PSTR("DEFAULT\0") );
	//strncpy_P(systemVars.gprs_conf.serverScript, PSTR("/cgi-bin/PY/spy.py\0"),SCRIPT_LENGTH);
	strncpy_P(systemVars.gprs_conf.serverScript, PSTR("/cgi-bin/PTMP01/spy.py\0"),SCRIPT_LENGTH);
	strncpy_P(systemVars.gprs_conf.server_tcp_port, PSTR("80\0"),PORT_LENGTH	);
    strncpy_P(systemVars.gprs_conf.simpwd, PSTR("DEFAULT\0"),PASSWD_LENGTH);

	// PWRSAVE
	if ( spx_io_board == SPX_IO5CH ) {
		systemVars.gprs_conf.pwrSave.pwrs_enabled = true;
	} else if ( spx_io_board == SPX_IO8CH ) {
		systemVars.gprs_conf.pwrSave.pwrs_enabled = false;
	}

	systemVars.gprs_conf.pwrSave.hora_start.hour = 23;
	systemVars.gprs_conf.pwrSave.hora_start.min = 30;
	systemVars.gprs_conf.pwrSave.hora_fin.hour = 5;
	systemVars.gprs_conf.pwrSave.hora_fin.min = 30;

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

char *start_tag = NULL;
char *end_tag = NULL;

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

char csqBuffer[32] = { 0 };
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


	//xprintf_P( PSTR("DEBUG_A TDIAL CONFIG: [%s]\r\n\0"), s_timerdial );

	while ( xSemaphoreTake( sem_SYSVars, ( TickType_t ) 5 ) != pdTRUE )
		taskYIELD();

	systemVars.gprs_conf.timerDial = atoi(s_timerdial);

	// Controlo que este en los rangos permitidos
	if ( (systemVars.gprs_conf.timerDial > 0) && (systemVars.gprs_conf.timerDial < TDIAL_MIN_DISCRETO ) ) {
		//systemVars.gprs_conf.timerDial = 0;
		//xprintf_P( PSTR("TDIAL warn !! Default to 0. ( continuo TDIAL=0, discreto TDIAL > 900)\r\n\0"));
		xprintf_P( PSTR("TDIAL warn: continuo TDIAL < 900, discreto TDIAL >= 900)\r\n\0"));
	}

	xSemaphoreGive( sem_SYSVars );
	//xprintf_P( PSTR("DEBUG_B TDIAL CONFIG: [%d]\r\n\0"), systemVars.gprs_conf.timerDial );
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

	//xprintf_P( PSTR("DEBUG_A: PWRS modo=%s, startitme=%s, endtime=%s\r\n\0"), s_modo, s_startTime, s_endTime );

	if (!strcmp_P( strupr(s_modo), PSTR( "OFF"))) {
		systemVars.gprs_conf.pwrSave.pwrs_enabled = false;
		//xprintf_P( PSTR("DEBUG: pwrsave off(%d)\r\n\0"), systemVars.gprs_conf.pwrSave.pwrs_enabled );
		goto quit;
	}

	if (!strcmp_P( strupr(s_modo), PSTR( "ON"))) {
		systemVars.gprs_conf.pwrSave.pwrs_enabled = true;
		//xprintf_P( PSTR("DEBUG: pwrsave ON(%d)\r\n\0"), systemVars.gprs_conf.pwrSave.pwrs_enabled );
		if ( s_startTime != NULL ) { u_convert_str_to_time_t( s_startTime, &systemVars.gprs_conf.pwrSave.hora_start); }
		if ( s_endTime != NULL ) { u_convert_str_to_time_t( s_endTime, &systemVars.gprs_conf.pwrSave.hora_fin); }
		//xprintf_P( PSTR("DEBUG: pwrsave start_time = %02d%02d\r\n\0"), systemVars.gprs_conf.pwrSave.hora_start.hour, systemVars.gprs_conf.pwrSave.hora_start.min);
		//xprintf_P( PSTR("DEBUG: pwrsave end_time = %02d%02d\r\n\0"), systemVars.gprs_conf.pwrSave.hora_fin.hour, systemVars.gprs_conf.pwrSave.hora_fin.min);
		goto quit;
	}

quit:

	xSemaphoreGive( sem_SYSVars );

	//xprintf_P( PSTR("DEBUG_B: PWRS modo=%d, startitme=%02d%02d, endtime=%02d%02d\r\n\0"), systemVars.gprs_conf.pwrSave.pwrs_enabled, systemVars.gprs_conf.pwrSave.hora_start.hour, systemVars.gprs_conf.pwrSave.hora_start.min, systemVars.gprs_conf.pwrSave.hora_fin.hour, systemVars.gprs_conf.pwrSave.hora_fin.min );

}
//------------------------------------------------------------------------------------
void u_gprs_init_pines(void)
{
	// GPRS
	IO_config_GPRS_SW();
	IO_config_GPRS_PWR();
	IO_config_GPRS_RTS();
	IO_config_GPRS_CTS();
	IO_config_GPRS_DCD();
	IO_config_GPRS_RI();
	IO_config_GPRS_RX();
	IO_config_GPRS_TX();

}
//------------------------------------------------------------------------------------
uint32_t u_gprs_read_timeToNextDial(void)
{
	return( ctl_read_timeToNextDial() );
}
//------------------------------------------------------------------------------------
void u_gprs_set_timeToNextDial( uint32_t time_to_dial )
{
	ctl_set_timeToNextDial( time_to_dial );
}
//------------------------------------------------------------------------------------
void u_gprs_tx_header(char *type)
{

	xCom_printf_P( fdGPRS,PSTR("GET %s?DLGID=%s&TYPE=%s&VER=%s\0" ), systemVars.gprs_conf.serverScript, systemVars.gprs_conf.dlgId, type, SPX_FW_REV );
	// DEBUG & LOG
	if ( systemVars.debug ==  DEBUG_GPRS ) {
		xprintf_P( PSTR("GET %s?DLGID=%s&TYPE=%s&VER=%s\0" ), systemVars.gprs_conf.serverScript, systemVars.gprs_conf.dlgId, type, SPX_FW_REV );
	}

}
//------------------------------------------------------------------------------------
void u_gprs_tx_tail(void)
{

	// ( No mando el close ya que espero la respuesta y no quiero que el socket se cierre )
	xCom_printf_P( fdGPRS, PSTR(" HTTP/1.1\r\nHost: www.spymovil.com\r\n\r\n\r\n\0") );

	// DEBUG & LOG
	if ( systemVars.debug ==  DEBUG_GPRS ) {
		xprintf_P( PSTR(" HTTP/1.1\r\nHost: www.spymovil.com\r\n\r\n\r\n\0") );
	}

	vTaskDelay( (portTickType)( 250 / portTICK_RATE_MS ) );
}
//------------------------------------------------------------------------------------

