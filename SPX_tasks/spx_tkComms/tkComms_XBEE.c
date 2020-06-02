/*
 * tkComms_XBEE.c
 *
 *  Created on: 5 mar. 2020
 *      Author: pablo
 */
#include "tkComms.h"

//------------------------------------------------------------------------------------
void xbee_init(void)
{
	XCOMMS_to_timer_start();
	xCOMMS_stateVars.aux1_prendido = false;
	xCOMMS_stateVars.aux1_inicializado = false;
}
//------------------------------------------------------------------------------------
void xbee_apagar(void)
{
	aux1_apagar();
}
//------------------------------------------------------------------------------------
bool xbee_prender( bool debug, uint8_t delay_factor )
{
	// Prendo el dispositivo XBEE.
	aux1_prender();
	vTaskDelay( (portTickType)( ( 2000 + 2000 * delay_factor) / portTICK_RATE_MS ) );	// Espero que se estabilize la fuente.
	return(true);
}
//------------------------------------------------------------------------------------
void xbee_flush_RX_buffer(void)
{
	aux1_flush_RX_buffer();
}
//------------------------------------------------------------------------------------
void xbee_flush_TX_buffer(void)
{
	aux1_flush_TX_buffer();
}
//------------------------------------------------------------------------------------
void xbee_print_RX_buffer(void)
{

	// Imprimo todo el buffer local de RX. Sale por \0.
	xprintf_P( PSTR ("XBEE: rxbuff>\r\n\0"));
	aux1_print_RX_buffer();
}
//------------------------------------------------------------------------------------
bool xbee_check_response( const char *rsp )
{
	return( xbee_check_response(rsp) );
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

	return( aux1_get_buffer_ptr(pattern) );
}
//------------------------------------------------------------------------------------

