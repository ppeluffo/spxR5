/*
 * spx_tkComms_utils.c
 *
 *  Created on: 26 feb. 2020
 *      Author: pablo
 */


#include <comms.h>

//------------------------------------------------------------------------------------
void u_comms_signal (t_comms_signals signal)
{
	/* Envia la señal dada.
	 * La escribe en una variable publica para que quede disponible a otras
	 * partes del codigo que la van a atender.
	 */

}
//------------------------------------------------------------------------------------
void xCOMMS_init( t_comms_rx_buffer *comms_buff, char *data_storage, uint16_t max_size )
{
	comms_buff->buff = data_storage;
	comms_buff->size = max_size;
	xCOMMS_flush_commsRxBuffer();
}
//------------------------------------------------------------------------------------
void xCOMMS_send_header(char *type)
{
	xCOMMS_send_P( PSTR("GET %s?DLGID=%s&TYPE=%s&VER=%s\0" ), systemVars.gprs_conf.serverScript, systemVars.gprs_conf.dlgId, type, SPX_FW_REV );
}
//------------------------------------------------------------------------------------
void xCOMMS_send_tail(void)
{

	// ( No mando el close ya que espero la respuesta y no quiero que el socket se cierre )
	xCOMMS_send_P( PSTR(" HTTP/1.1\r\nHost: www.spymovil.com\r\n\r\n\r\n\0") );

	vTaskDelay( (portTickType)( 250 / portTICK_RATE_MS ) );
}
//------------------------------------------------------------------------------------
int xCOMMS_send_P( PGM_P fmt, ...)
{
	/*
	 * Funcion que imprime formateado en el canal de comunicaciones activo y
	 * genera el debug
	 *
	 * http://c-faq.com/varargs/handoff.html
	 *
	 * Esta funcion actua como un wrapper de xfprintf_P.
	 *
	 */

va_list argp;
int i;

	va_start(argp, fmt);

	if ( systemVars.comms_channel == COMMS_CHANNEL_XBEE ) {
		i = xfprintf_V( fdXBEE, fmt, argp );
	} else if ( systemVars.comms_channel == COMMS_CHANNEL_GPRS ) {
		i = xfprintf_V( fdGPRS, fmt, argp );
	} else {
		return(-1);
	}

	if ( systemVars.debug == DEBUG_GPRS ) {
		xfprintf_V( fdTERM, fmt, argp );
	}

	return(i);
}
//------------------------------------------------------------------------------------
void xCOMMS_send_dr(st_dataRecord_t *dr)
{
	/*
	 * Imprime un datarecord en un file descriptor dado.
	 * En caso de debug, lo muestra en xTERM.
	 */

	if ( systemVars.comms_channel == COMMS_CHANNEL_XBEE ) {
		data_print_inputs(fdXBEE, dr);
	} else if ( systemVars.comms_channel == COMMS_CHANNEL_GPRS ) {
		data_print_inputs(fdGPRS, dr);
	} else {
		return;
	}

	if ( systemVars.debug == DEBUG_GPRS ) {
		data_print_inputs(fdTERM, dr);
	}

}
//------------------------------------------------------------------------------------
t_link_status xCOMMS_open_link(void)
{
	/*
	 * Intenta abrir el link hacia el servidor
	 * En caso de XBEE no hay que hacer nada
	 * En caso de GPRS se debe intentar abrir el socket
	 *
	 */

t_link_status lstatus = LINK_CLOSED;

	xCOMMS_flush_RX();

	if ( systemVars.comms_channel == COMMS_CHANNEL_XBEE ) {
		lstatus = xbee_open_socket();
	} else if ( systemVars.comms_channel == COMMS_CHANNEL_GPRS ) {
		lstatus = gprs_open_socket();
	}

	return(lstatus);

}
//------------------------------------------------------------------------------------
t_link_status xCOMMS_link_status(void)
{

t_link_status lstatus = LINK_CLOSED;

	if ( systemVars.comms_channel == COMMS_CHANNEL_XBEE ) {
		lstatus = xbee_check_socket_status();
	} else if ( systemVars.comms_channel == COMMS_CHANNEL_GPRS ) {
		lstatus = gprs_check_socket_status();
	}

	return(lstatus);
}
//------------------------------------------------------------------------------------
int xCOMMS_read(void)
{

	/* Lee de la uart mientras halla datos y los almacena en
	 * el commsRxBuffer.
	 * El poner los datos en commsRxBuffer es lineal !! o sea
	 * debemos cuidar que no este lleno.
	 *
	 */

char c;
int chars_leidos = 0;

	if ( systemVars.comms_channel == COMMS_CHANNEL_XBEE ) {

		while ( frtos_read( fdXBEE, &c, 1 ) == 1 ) {
			xbee_rxbuffer_poke(c, &xCOMMS_RX_buffer);
			chars_leidos++;
		}

	} else if ( systemVars.comms_channel == COMMS_CHANNEL_GPRS ) {

		while ( frtos_read( fdGPRS, &c, 1 ) == 1 ) {
			gprs_rxbuffer_poke(c, &xCOMMS_RX_buffer);
			chars_leidos++;
		}
	}

	return(chars_leidos);
}
//------------------------------------------------------------------------------------
bool xCOMMS_check_response( const char *rsp )
{
	/* Busca en el commsRxBuffer es string pasado en rsp.
	 * rsp es un puntero por lo tanto al invocar la funcion debemos
	 * poner un \0 al final de string.
	 *
	 * Antes debemos tener datos en el commsRxBuffer, cosa que se
	 * hace llamando a xCOMMS_read()
	 */

bool retS = false;

	if ( systemVars.comms_channel == COMMS_CHANNEL_XBEE ) {
		retS =  xbee_check_response( rsp, &xCOMMS_RX_buffer );
	} else if ( systemVars.comms_channel == COMMS_CHANNEL_GPRS ) {
		retS = gprs_check_response( rsp, &xCOMMS_RX_buffer );
	}

	return(retS);
}
//------------------------------------------------------------------------------------
void xCOMMS_print_RX_buffer(void)
{
	// OJO que debo imprimir el buffer o la response dependiendo del debugGprs
	// Pasar a debug COMM

	// Imprime la respuesta del server.
	// Utiliza el buffer de RX.
	// Solo muestra el payload, es decir lo que esta entre <h1> y </h1>
	// Todas las respuestas el server las encierra entre ambos tags excepto los errores del apache.

	if ( systemVars.comms_channel == COMMS_CHANNEL_XBEE ) {
		xbee_print_RX_Buffer( &xCOMMS_RX_buffer);
		xCOMMS_flush_commsRxBuffer();
	} else	if ( systemVars.comms_channel == COMMS_CHANNEL_GPRS ) {
		gprs_print_RX_Buffer( &xCOMMS_RX_buffer);
	}
}
//------------------------------------------------------------------------------------
void xCOMMS_flush_RX(void)
{
	/*
	 * Inicializa todos los buffers de recepcion para el canal activo.
	 * Reinicia el buffer que recibe de la uart del dispositivo
	 * de comunicaciones, y el buffer comun commsRxBuffer
	 */

	if ( systemVars.comms_channel == COMMS_CHANNEL_XBEE ) {
		xbee_flush_RX_buffer();
		xCOMMS_flush_commsRxBuffer();
	} else	if ( systemVars.comms_channel == COMMS_CHANNEL_GPRS ) {
		gprs_flush_RX_buffer();
		xCOMMS_flush_commsRxBuffer();
	}
}
//------------------------------------------------------------------------------------
void xCOMMS_flush_commsRxBuffer(void)
{
	/*
	 * Reinicia el commsRxBuffer
	 */
	memset( xCOMMS_RX_buffer.buff, '\0', xCOMMS_RX_buffer.size);
	xCOMMS_RX_buffer.ptr = 0;
}
//------------------------------------------------------------------------------------
void xCOMMS_flush_TX(void)
{
	/*
	 * Inicializa todos los buffers de trasmision para el canal activo.
	 * Reinicia el buffer que transmite en la uart del dispositivo
	 * de comunicaciones
	 */

	if ( systemVars.comms_channel == COMMS_CHANNEL_XBEE ) {
		xbee_flush_TX_buffer();
	} else	if ( systemVars.comms_channel == COMMS_CHANNEL_GPRS ) {
		gprs_flush_TX_buffer();
	}
}
//------------------------------------------------------------------------------------
char *xCOMMS_strstr( const char *pattern )
{
	/*
	 * Busca el string pattern en el buffer de recepcion.
	 * Retorna un puntero al comienzo del pattern o NULL
	 * si no lo encuentra
	 */
	return( strstr( (const char *)&xCOMMS_RX_buffer.buff, pattern )) ;
}
//------------------------------------------------------------------------------------




