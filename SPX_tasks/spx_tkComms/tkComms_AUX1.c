/*
 * tkComms_AUX1.c
 *
 *  Created on: 5 mar. 2020
 *      Author: pablo
 */
#include "tkComms.h"

//------------------------------------------------------------------------------------

struct {
	char buffer[AUX1_RXBUFFER_LEN];
	uint16_t ptr;
} aux1RxBuffer;

//------------------------------------------------------------------------------------
void aux1_init(void)
{

}
//------------------------------------------------------------------------------------
void aux1_rxBuffer_fill(char c)
{
	/*
	 * Guarda el dato en el buffer LINEAL de operacion del GPRS
	 * Si hay lugar meto el dato.
	 */

	if ( aux1RxBuffer.ptr < AUX1_RXBUFFER_LEN )
		aux1RxBuffer.buffer[ aux1RxBuffer.ptr++ ] = c;
}
//------------------------------------------------------------------------------------
void aux1_apagar(void)
{
	/*
	 * Apaga el dispositivo quitando la energia del mismo
	 *
	 */
	IO_clr_AUX1_PWR();		// Apago el modulo
	// Espero 1s de settle time.
	vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );

}
//------------------------------------------------------------------------------------
bool aux1_prender( bool debug, uint8_t delay_factor )
{
	// Prendo el dispositivo AUX1.

	IO_clr_AUX1_PWR();		// Apago el modulo
	IO_set_AUX1_PWR();		// Prendo el modulo

	vTaskDelay( (portTickType)( ( 2000 + 2000 * delay_factor) / portTICK_RATE_MS ) );	// Espero que se estabilize la fuente.

	xCOMMS_stateVars.dispositivo_prendido = true;

	return(true);
}
//------------------------------------------------------------------------------------
void aux1_flush_RX_buffer(void)
{

	frtos_ioctl( fdAUX1,ioctl_UART_CLEAR_RX_BUFFER, NULL);
	memset( aux1RxBuffer.buffer, '\0', AUX1_RXBUFFER_LEN);
	aux1RxBuffer.ptr = 0;
}
//------------------------------------------------------------------------------------
void aux1_flush_TX_buffer(void)
{
	frtos_ioctl( fdAUX1,ioctl_UART_CLEAR_TX_BUFFER, NULL);

}
//------------------------------------------------------------------------------------
void aux1_print_RX_buffer(void)
{

	// Imprimo todo el buffer local de RX. Sale por \0.
	xprintf_P( PSTR ("AUX1: rxbuff>\r\n\0"));

	// Uso esta funcion para imprimir un buffer largo, mayor al que utiliza xprintf_P. !!!
	xnprint( aux1RxBuffer.buffer, AUX1_RXBUFFER_LEN );
	xprintf_P( PSTR ("\r\n[%d]\r\n\0"), aux1RxBuffer.ptr );
}
//------------------------------------------------------------------------------------
bool aux1_check_response( const char *rsp )
{
bool retS = false;

	if ( strstr( aux1RxBuffer.buffer, rsp) != NULL ) {
		retS = true;
	}
	return(retS);
}
//------------------------------------------------------------------------------------
bool aux1_configurar_dispositivo(uint8_t *err_code)
{
	/*
	 * Los dispositivos AUX1 no llevan configuracion.
	 * Al prenderlos ya son un cable
	 */
	*err_code = ERR_NONE;
	return(true);
}
//------------------------------------------------------------------------------------
void aux1_mon_sqe( void )
{
	return;
}
//------------------------------------------------------------------------------------
bool aux1_scan( t_scan_struct scan_boundle )
{
	return(true);
}
//------------------------------------------------------------------------------------
bool aux1_need_scan( t_scan_struct scan_boundle )
{
	return(true);
}
//------------------------------------------------------------------------------------
bool aux1_ip( void )
{
	return(true);
}
//------------------------------------------------------------------------------------
t_link_status aux1_check_socket_status(bool f_debug)
{
	/*
	 * En AUX1 el enlace P2P por lo tanto no hay sockets y el
	 * canal esta siempre abierto al estar el dispositivo prendido
	 */
	return(LINK_OPEN);
}
//------------------------------------------------------------------------------------
t_link_status aux1_open_socket(bool f_debug, char *ip, char *port)
{
	return(LINK_OPEN);
}
//------------------------------------------------------------------------------------
char *aux1_get_pattern_in_buffer( char *pattern)
{

	return( strstr( aux1RxBuffer.buffer, pattern) );
}
//------------------------------------------------------------------------------------
char *aux1_get_buffer_ptr(void)
{
	return(aux1RxBuffer.buffer);
}
//------------------------------------------------------------------------------------
