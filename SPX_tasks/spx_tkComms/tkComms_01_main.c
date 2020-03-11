/*
 * tkComms_01_main.c
 *
 *  Created on: 5 mar. 2020
 *      Author: pablo
 */

#include <tkComms.h>

//------------------------------------------------------------------------------------
void tkComms(void * pvParameters)
{

( void ) pvParameters;

	// Espero la notificacion para arrancar
	while ( !startTask )
		vTaskDelay( ( TickType_t)( 100 / portTICK_RATE_MS ) );

	xCOMMS_init();

	tkComms_state = ST_ENTRY;
	xprintf_P( PSTR("starting tkComms..\r\n\0"));

	// loop
	for( ;; )
	{

		switch ( tkComms_state ) {
		case ST_ENTRY:
			tkComms_state = tkComms_st_entry();
			break;
		case ST_ESPERA_APAGADO:
			tkComms_state = tkComms_st_espera_apagado();
			break;
		case ST_ESPERA_PRENDIDO:
			tkComms_state = tkComms_st_espera_prendido();
			break;
		case ST_PRENDER:
			tkComms_state = tkComms_st_prender();
			break;
		case ST_CONFIGURAR:
			tkComms_state = tkComms_st_configurar();
			break;
		case ST_MON_SQE:
			tkComms_state = tkComms_st_mon_sqe();
			break;
		case ST_SCAN:
			tkComms_state = tkComms_st_scan();
			break;
		case ST_IP:
			tkComms_state = tkComms_st_ip();
			break;
		case ST_INITFRAME:
			tkComms_state = tkComms_st_initframe();
			break;
		case ST_DATAFRAME:
			tkComms_state = tkComms_st_dataframe();
			break;
		default:
			tkComms_state = ST_ENTRY;
			xprintf_P( PSTR("COMMS: state ERROR !!.\r\n\0"));
		}

	}
}
//------------------------------------------------------------------------------------
void tkCommsRX(void * pvParameters)
{
	// Esta tarea lee y procesa las respuestas del GPRS. Lee c/caracter recibido y lo va
	// metiendo en un buffer circular propio del GPRS que permite luego su impresion,
	// analisis, etc.

( void ) pvParameters;
char c;

	// Espero la notificacion para arrancar
	while ( !startTask )
		vTaskDelay( ( TickType_t)( 100 / portTICK_RATE_MS ) );

	xprintf_P( PSTR("starting tkCommsRX..\r\n\0"));

	// loop
	for( ;; )
	{

		// Leo el UART de GPRS
		if ( frtos_read( fdGPRS, &c, 1 ) == 1 ) {
			gprs_rxBuffer_fill(c);
		}

		// Leo el UART de XBEE
		if ( frtos_read( fdXBEE, &c, 1 ) == 1 ) {
			xbee_rxBuffer_fill(c);
		}


	}
}
//------------------------------------------------------------------------------------
