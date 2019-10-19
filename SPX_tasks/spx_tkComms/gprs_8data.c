/*
 * gprs_8data.c
 *
 *  Created on: 9 oct. 2019
 *      Author: pablo
 */

#include <spx_tkComms/gprs.h>

// La tarea no puede demorar mas de 180s.
#define WDG_GPRS_DATA	180

//------------------------------------------------------------------------------------
bool st_gprs_data(void)
{

	GPRS_stateVars.state = G_DATA;

	// Con la interfase levantada, espero.
	while (1) {
		ctl_watchdog_kick(WDG_GPRSTX, WDG_GPRS_DATA );
		vTaskDelay( ( TickType_t)( 1000 / portTICK_RATE_MS ) );
	}

	return(bool_CONTINUAR);
}
//------------------------------------------------------------------------------------

