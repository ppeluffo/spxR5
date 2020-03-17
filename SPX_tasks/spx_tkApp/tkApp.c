/*
 * spx_tkSistema.c
 *
 *  Created on: 23 oct. 2019
 *      Author: pablo
 */


#include "spx.h"
#include "tkApp.h"

//------------------------------------------------------------------------------------
void tkAplicacion(void * pvParameters)
{

( void ) pvParameters;

	// Espero la notificacion para arrancar
	while ( !startTask )
		vTaskDelay( ( TickType_t)( 100 / portTICK_RATE_MS ) );

	xprintf_P( PSTR("starting tkAplicacion..\r\n\0"));

	// Inicializo los dispositivos
	switch (spx_io_board ) {
	case SPX_IO5CH:
		DRV8814_init();
		break;
	case SPX_IO8CH:
		MCP_init();
		break;
	}

	// De acuedo al modo de operacion disparo la tarea que realiza la
	// funcion especializada del datalogger.

	switch( systemVars.aplicacion ) {
	case APP_OFF:
		// Es el caso en que no debo hacer nada con las salidas.
		// Duermo 25s para entrar en pwrdown.
		tkApp_off();
		break;
	case APP_CONSIGNA:
		tkApp_consigna();
		break;
	case APP_PERFORACION:
		tkApp_perforacion();
		break;
	case APP_PLANTAPOT:
		tkApp_plantapot();
		break;
	default:
		break;
	}

	tkApp_off();
}
//------------------------------------------------------------------------------------
