/*
 * tkComms_AUX1.c
 *
 *  Created on: 22 may. 2020
 *      Author: pablo
 */

#include "tkComms.h"

//------------------------------------------------------------------------------------
struct {
	char buffer[AUX1_RXBUFFER_LEN];
	uint16_t ptr;
} aux1RxBuffer;

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

	xCOMMS_stateVars.aux1_prendido = false;
	xCOMMS_stateVars.aux1_inicializado = false;
}
//------------------------------------------------------------------------------------
bool aux1_prender(void)
{
	// Prendo el dispositivo XBEE.

	IO_clr_AUX1_PWR();		// Apago el modulo
	IO_set_AUX1_PWR();		// Prendo el modulo

	xCOMMS_stateVars.aux1_prendido = true;
	xCOMMS_stateVars.aux1_inicializado = true;

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
char *aux1_get_buffer_ptr( char *pattern)
{
	return( strstr( aux1RxBuffer.buffer, pattern) );
}
//------------------------------------------------------------------------------------
