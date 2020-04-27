/*
 * tkComms_XBEE.c
 *
 *  Created on: 5 mar. 2020
 *      Author: pablo
 */
#include "tkComms.h"

//------------------------------------------------------------------------------------

struct {
	char buffer[XBEE_RXBUFFER_LEN];
	uint16_t ptr;
} xbeeRxBuffer;

//------------------------------------------------------------------------------------
void xbee_init(void)
{

}
//------------------------------------------------------------------------------------
void xbee_rxBuffer_fill(char c)
{
	/*
	 * Guarda el dato en el buffer LINEAL de operacion del GPRS
	 * Si hay lugar meto el dato.
	 */

	if ( xbeeRxBuffer.ptr < XBEE_RXBUFFER_LEN )
		xbeeRxBuffer.buffer[ xbeeRxBuffer.ptr++ ] = c;
}
//------------------------------------------------------------------------------------
void xbee_apagar(void)
{
	/*
	 * Apaga el dispositivo quitando la energia del mismo
	 *
	 */
	IO_clr_XBEE_PWR();		// Apago el modulo
	// Espero 1s de settle time.
	vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );

}
//------------------------------------------------------------------------------------
bool xbee_prender( bool debug, uint8_t delay_factor )
{
	// Prendo el dispositivo XBEE.

	IO_clr_XBEE_PWR();		// Apago el modulo
	IO_set_XBEE_PWR();		// Prendo el modulo

	vTaskDelay( (portTickType)( ( 2000 + 2000 * delay_factor) / portTICK_RATE_MS ) );	// Espero que se estabilize la fuente.

	xCOMMS_stateVars.dispositivo_prendido = true;

	return(true);
}
//------------------------------------------------------------------------------------
void xbee_flush_RX_buffer(void)
{

	frtos_ioctl( fdAUX1,ioctl_UART_CLEAR_RX_BUFFER, NULL);
	memset( xbeeRxBuffer.buffer, '\0', XBEE_RXBUFFER_LEN);
	xbeeRxBuffer.ptr = 0;
}
//------------------------------------------------------------------------------------
void xbee_flush_TX_buffer(void)
{
	frtos_ioctl( fdAUX1,ioctl_UART_CLEAR_TX_BUFFER, NULL);

}
//------------------------------------------------------------------------------------
void xbee_print_RX_buffer(void)
{

	// Imprimo todo el buffer local de RX. Sale por \0.
	xprintf_P( PSTR ("XBEE: rxbuff>\r\n\0"));

	// Uso esta funcion para imprimir un buffer largo, mayor al que utiliza xprintf_P. !!!
	xnprint( xbeeRxBuffer.buffer, XBEE_RXBUFFER_LEN );
	xprintf_P( PSTR ("\r\n[%d]\r\n\0"), xbeeRxBuffer.ptr );
}
//------------------------------------------------------------------------------------
bool xbee_check_response( const char *rsp )
{
bool retS = false;

	if ( strstr( xbeeRxBuffer.buffer, rsp) != NULL ) {
		retS = true;
	}
	return(retS);
}
//------------------------------------------------------------------------------------
bool xbee_configurar_dispositivo(uint8_t *err_code)
{
	/*
	 * Los dispositivos XBEE no llevan configuracion.
	 * Al prenderlos ya son un cable
	 */
	*err_code = ERR_NONE;
	return(true);
}
//------------------------------------------------------------------------------------
void xbee_mon_sqe( void )
{
	return;
}
//------------------------------------------------------------------------------------
bool xbee_scan( t_scan_struct scan_boundle )
{
	return(true);
}
//------------------------------------------------------------------------------------
bool xbee_need_scan( t_scan_struct scan_boundle )
{
	return(true);
}
//------------------------------------------------------------------------------------
bool xbee_ip( void )
{
	return(true);
}
//------------------------------------------------------------------------------------
t_link_status xbee_check_socket_status(bool f_debug)
{
	/*
	 * En XBEE el enlace P2P por lo tanto no hay sockets y el
	 * canal esta siempre abierto al estar el dispositivo prendido
	 */
	return(LINK_OPEN);
}
//------------------------------------------------------------------------------------
t_link_status xbee_open_socket(bool f_debug, char *ip, char *port)
{
	return(LINK_OPEN);
}
//------------------------------------------------------------------------------------
char *xbee_get_buffer_ptr( char *pattern)
{

	return( strstr( xbeeRxBuffer.buffer, pattern) );
}
//------------------------------------------------------------------------------------

