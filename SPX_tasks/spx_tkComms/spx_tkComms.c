/*
 * spx_tkComms.c
 *
 *  Created on: 26 feb. 2020
 *      Author: pablo
 */

#include <comms.h>

//------------------------------------------------------------------------------------
void tkComms(void * pvParameters)
{

( void ) pvParameters;
t_comms_states tkComms_state;

	// Espero la notificacion para arrancar
	while ( !startTask )
		vTaskDelay( ( TickType_t)( 100 / portTICK_RATE_MS ) );

	tkComms_state = ST_STANDBY;
	xprintf_P( PSTR("starting tkComms..\r\n\0"));
	xCOMMS_init( &xCOMMS_RX_buffer, xcomms_rx_buffer, COMMS_RXBUFFER_LEN );

	// loop
	for( ;; )
	{

		switch ( tkComms_state ) {
		case ST_STANDBY:
			/* Estado de espera prendido o apagado
			 * Al salir el dispositivo esta configurado y listo
			 * para transmitir.
			 */
			tkComms_state = tkComms_st_standby();
			break;
		case  ST_INIT:
			/* Estado en que se envian los frames de init y se
			 * procesan las respuestas para configurar el datalogger
			 */
			tkComms_state = tkComms_st_init();
			break;
		case ST_DATA:
			/* Estado en que trasmite frames de datos y procesa
			 * las respuestas
			 */
			tkComms_state = tkComms_st_data();
			break;
		default:
			tkComms_state = ST_STANDBY;
			xprintf_P( PSTR("COMMS: state ERROR !!.\r\n\0"));
		}

	}
}
//------------------------------------------------------------------------------------
