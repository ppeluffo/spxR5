/*
 * spx_tkComms_ul_xbee.c
 *
 *  Created on: 27 feb. 2020
 *      Author: pablo
 */

#include <comms.h>

//------------------------------------------------------------------------------------
void xbee_flush_RX_buffer(void)
{
	frtos_ioctl( fdXBEE,ioctl_UART_CLEAR_RX_BUFFER, NULL);

}
//------------------------------------------------------------------------------------
void xbee_flush_TX_buffer(void)
{
	frtos_ioctl( fdXBEE,ioctl_UART_CLEAR_TX_BUFFER, NULL);

}
//------------------------------------------------------------------------------------
t_link_status xbee_check_socket_status(void)
{
	/*
	 * En XBEE el enlace P2P por lo tanto no hay sockets y el
	 * canal esta siempre abierto al estar el dispositivo prendido
	 */
	return(LINK_OPEN);
}
//------------------------------------------------------------------------------------
t_link_status xbee_open_socket(void)
{
	return(LINK_OPEN);
}
//------------------------------------------------------------------------------------
bool xbee_rxbuffer_poke(char c, t_comms_rx_buffer *comms_buff )
{

	/*
	 * Guardo el dato en el buffer en forma lineal
	 * NO ES UN BUFFER CIRCULAR !!!!
	 * Debo atender que halla lugar.
	 *
	 */
	if ( comms_buff->ptr < comms_buff->size ) {
		comms_buff->buff[ comms_buff->ptr++ ] = c;
		return(true);
	}

	return(false);
}
//------------------------------------------------------------------------------------
bool xbee_check_response( const char *rsp, t_comms_rx_buffer *comms_buff  )
{
bool retS = false;

	if ( strstr( comms_buff->buff, rsp) != NULL ) {
		retS = true;
	}
	return(retS);
}
//------------------------------------------------------------------------------------
void xbee_print_RX_Buffer(t_comms_rx_buffer *comms_buff)
{

	// Uso esta funcion para imprimir un buffer largo, mayor al que utiliza xprintf_P. !!!
	xnprint( comms_buff->buff, sizeof(comms_buff->size) );
	xprintf_P( PSTR ("\r\n[%d]\r\n\0"), comms_buff->ptr );
}
//------------------------------------------------------------------------------------
