/*
 * spx_tkComms_ul_gprs.c
 *
 *  Created on: 27 feb. 2020
 *      Author: pablo
 */

#include <comms.h>

//------------------------------------------------------------------------------------
bool gprs_check_modem_is_on(void)
{
	return(true);
}
//------------------------------------------------------------------------------------
void gprs_flush_RX_buffer(void)
{
	frtos_ioctl( fdGPRS,ioctl_UART_CLEAR_RX_BUFFER, NULL);

}
//------------------------------------------------------------------------------------
void gprs_flush_TX_buffer(void)
{
	frtos_ioctl( fdGPRS,ioctl_UART_CLEAR_TX_BUFFER, NULL);

}
//------------------------------------------------------------------------------------
t_link_status gprs_check_socket_status(void)
{
	/* En el caso de un link GPRS, el socket puede estar abierto
	 * o no.
	 * El socket esta abierto si el modem esta prendido y
	 * el DCD esta en 0.
	 * Cuando el modem esta apagado pin_dcd = 0
	 * Cuando el modem esta prendido y el socket cerrado pin_dcd = 1
	 * Cuando el modem esta prendido y el socket abierto pin_dcd = 0.
	 */


uint8_t pin_dcd = 0;
t_link_status socket_status = LINK_CLOSED;

	pin_dcd = IO_read_DCD();

	if ( ( gprs_check_modem_is_on() ) && ( pin_dcd == 0 ) ){
		socket_status = LINK_OPEN;
		u_debug_printf_P( DEBUG_COMMS, PSTR("COMMS: gprs sckt is open (dcd=%d)\r\n\0"),pin_dcd);

	} else if ( u_gprs_check_response("Operation not supported")) {
		socket_status = LINK_ERROR;
		u_debug_printf_P( DEBUG_COMMS, PSTR("COMMS: gprs sckt ERROR\r\n\0"));

	} else if ( u_gprs_check_response("ERROR")) {
		socket_status = LINK_ERROR;
		u_debug_printf_P( DEBUG_COMMS, PSTR("GPRS: sckt ERROR\r\n\0"));

	} else if ( u_gprs_check_response("+CIPEVENT:")) {
		// El socket no se va a recuperar. Hay que cerrar el net y volver a abrirlo.
		socket_status = LINK_FAIL;
		u_debug_printf_P( DEBUG_COMMS, PSTR("COMMS: gprs sckt FAIL +CIPEVENT:\r\n\0"));

	} else {
		socket_status = LINK_CLOSED;
		u_debug_printf_P( DEBUG_COMMS, PSTR("COMMS: gprs sckt is close (dcd=%d)\r\n\0"),pin_dcd);

	}

	return(socket_status);

}
//------------------------------------------------------------------------------------
t_link_status gprs_open_socket(void)
{

uint8_t timeout = 0;
uint8_t await_loops = 0;
t_link_status link_status = LINK_FAIL;


	u_debug_printf_P( DEBUG_COMMS, PSTR("COMMS: try to open gprs socket\r\n\0"));

	/* Mando el comando y espero */
	xfprintf_P( fdGPRS, PSTR("AT+CIPOPEN=0,\"TCP\",\"%s\",%s\r\n\0"), GPRS_stateVars.server_ip_address, systemVars.gprs_conf.server_tcp_port);
	vTaskDelay( (portTickType)( 1500 / portTICK_RATE_MS ) );

	if ( systemVars.debug == DEBUG_GPRS ) {
		u_gprs_print_RX_Buffer();
	}

	await_loops = ( 10 * 1000 / 3000 ) + 1;
	// Y espero hasta 30s que abra.
	for ( timeout = 0; timeout < await_loops; timeout++) {

		vTaskDelay( (portTickType)( 3000 / portTICK_RATE_MS ) );

		link_status = gprs_check_socket_status();

		// Si el socket abrio, salgo para trasmitir el frame de init.
		if ( link_status == LINK_OPEN ) {
			break;
		}

		// Si el socket dio error, salgo para enviar de nuevo el comando.
		if ( link_status == LINK_ERROR ) {
			break;
		}

		// Si el socket dio falla, debo reiniciar la conexion.
		if ( link_status == LINK_FAIL ) {
			break;
		}
	}

	return(link_status);

}
//------------------------------------------------------------------------------------
bool gprs_rxbuffer_poke(char c, t_comms_rx_buffer *comms_buff )
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
bool gprs_check_response( const char *rsp, t_comms_rx_buffer *comms_buff  )
{
bool retS = false;

	if ( strstr( comms_buff->buff, rsp) != NULL ) {
		retS = true;
	}
	return(retS);
}
//------------------------------------------------------------------------------------
void gprs_print_RX_Buffer(t_comms_rx_buffer *comms_buff)
{

	// Uso esta funcion para imprimir un buffer largo, mayor al que utiliza xprintf_P. !!!
	xnprint( comms_buff->buff, sizeof(comms_buff->size) );
	xprintf_P( PSTR ("\r\n[%d]\r\n\0"), comms_buff->ptr );
}
//------------------------------------------------------------------------------------
