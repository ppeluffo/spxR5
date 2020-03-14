/*
 * tkApp_off.c
 *
 *  Created on: 12 mar. 2020
 *      Author: pablo
 */

#include "spx.h"
#include "tkApp.h"

//------------------------------------------------------------------------------------

void tkApp_off(void)
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


