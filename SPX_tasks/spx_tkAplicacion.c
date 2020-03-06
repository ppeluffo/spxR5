/*
 * spx_tkSistema.c
 *
 *  Created on: 23 oct. 2019
 *      Author: pablo
 */

#include <SPX_ulibs/ul_consigna.h>
#include "spx.h"

void aplicacion_off_stk(void);

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
		aplicacion_off_stk();
		break;
	case APP_CONSIGNA:
		consigna_stk();
		break;
	case APP_PERFORACION:
		perforacion_stk();
		break;
	case APP_TANQUE:
		// Es el caso en que no debo hacer nada con las salidas.
		// Duermo 25s para entrar en pwrdown.
		tanque_stk();
		break;
	case APP_PLANTAPOT:
		appalarma_stk();
		break;
	}

	aplicacion_off_stk();
}
//------------------------------------------------------------------------------------
void aplicacion_off_stk(void)
{
	// Cuando no hay una funcion especifica habilidada en el datalogger
	// ( solo monitoreo ), debemos dormir para que pueda entrar en
	// tickless

	for( ;; )
	{
		ctl_watchdog_kick( WDG_APP,  WDG_APP_TIMEOUT );
		vTaskDelay( ( TickType_t)( 25000 / portTICK_RATE_MS ) );
	}
}
//------------------------------------------------------------------------------------

