/*
 * tkAux.c
 *
 *  Created on: 2 set. 2020
 *      Author: pablo
 */

#include "tkComms.h"

#define WDG_AUXRX_TIMEOUT WDG_TO60

struct {
	char buffer[AUX_RXBUFFER_LEN];
	uint16_t ptr;
} auxRxBuffer;

//------------------------------------------------------------------------------------
void tkAuxRX(void * pvParameters)
{
	// Esta tarea lee y procesa las respuestas del GPRS. Lee c/caracter recibido y lo va
	// metiendo en un buffer circular propio del GPRS que permite luego su impresion,
	// analisis, etc.

( void ) pvParameters;
char c;
uint32_t ulNotifiedValue;


	aux_init();

	// Espero la notificacion para arrancar
	while ( !startTask )
		vTaskDelay( ( TickType_t)( 100 / portTICK_RATE_MS ) );

	xprintf_P( PSTR("starting tkAuxRX..\r\n\0"));

	for( ;; )	{

		ctl_watchdog_kick(WDG_AUXRX, WDG_AUXRX_TIMEOUT );

		if ( systemVars.modbus_conf.modbus_slave_address != 0x00 ) {
			// Leo el UART de AUX1
			if ( frtos_read( fdAUX1, &c, 1 ) == 1 ) {
				aux_rxBuffer_fill(c);
			}

		} else {

			xTaskNotifyWait( 0x00, ULONG_MAX, &ulNotifiedValue, ((TickType_t) 10000 / portTICK_RATE_MS ) );

		}
	}
}
//------------------------------------------------------------------------------------
void aux_init(void)
{

	IO_config_AUX_PWR();
	IO_config_AUX_RTS();

	IO_clr_AUX_PWR();
	IO_clr_AUX_RTS();

}
//------------------------------------------------------------------------------------
void aux_rxBuffer_fill(char c)
{
	/*
	 * Guarda el dato en el buffer LINEAL de operacion del GPRS
	 * Si hay lugar meto el dato.
	 */

	if ( auxRxBuffer.ptr < AUX_RXBUFFER_LEN )
		auxRxBuffer.buffer[ auxRxBuffer.ptr++ ] = c;
}
//------------------------------------------------------------------------------------
char *aux_get_buffer( void )
{

	return( auxRxBuffer.buffer );
}
//------------------------------------------------------------------------------------
uint16_t aux_get_buffer_ptr( void )
{

	return( auxRxBuffer.ptr );
}
//------------------------------------------------------------------------------------
void aux_flush_RX_buffer(void)
{

	frtos_ioctl( fdAUX1,ioctl_UART_CLEAR_RX_BUFFER, NULL);
	memset( auxRxBuffer.buffer, '\0', AUX_RXBUFFER_LEN);
	auxRxBuffer.ptr = 0;
}
//------------------------------------------------------------------------------------
void aux_flush_TX_buffer(void)
{
	frtos_ioctl( fdAUX1,ioctl_UART_CLEAR_TX_BUFFER, NULL);
}
//------------------------------------------------------------------------------------
void aux_prender(void)
{
	// Prendo la fuente del aux.
	IO_set_AUX_PWR();
	vTaskDelay( 1000 / portTICK_RATE_MS );
}
//------------------------------------------------------------------------------------
void aux_apagar(void)
{
	IO_clr_AUX_PWR();
}
//------------------------------------------------------------------------------------
void aux_rts_on(void)
{
	IO_set_AUX_RTS();
}
//------------------------------------------------------------------------------------
void aux_rts_off(void)
{
	IO_clr_AUX_RTS();
}
//------------------------------------------------------------------------------------
void aux_print_RX_buffer( bool ascii_mode )
{
	// Imprimo en hexadecimal el buffer.

char *p;

	if ( DF_COMMS ) {
		// Imprimo todo el buffer local de RX. Sale por \0.
		xprintf_P( PSTR ("AUX: rxbuff>\r\n\0"));

		p = auxRxBuffer.buffer;
		while(*p) {
			if ( ascii_mode ) {
				xprintf_P( PSTR("%c"),(*p));
			} else {
				xprintf_P( PSTR("[0x%02x]"),(*p));
			}
			*p++;
		}

		xprintf_P( PSTR ("\r\n[%d]\r\n\0"), auxRxBuffer.ptr );
	}
}
//------------------------------------------------------------------------------------
