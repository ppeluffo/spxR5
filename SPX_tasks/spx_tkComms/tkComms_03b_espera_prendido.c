/*
 * tkComms_03b_espera_prendido.c
 *
 *  Created on: 5 mar. 2020
 *      Author: pablo
 */

#include <tkComms.h>

// La tarea no puede demorar mas de 30s.
#define WDG_COMMS_TO_ESPERA_ON 30

//------------------------------------------------------------------------------------
t_comms_states tkComms_st_espera_prendido(void)
{
	/*
	 * Espero con el dispositivo prendido.
	 * Salgo solo al recibir una senal.
	 *
	 */

t_comms_states next_state = ST_ENTRY;
int8_t timer = 60;

// Entry:
	ctl_watchdog_kick(WDG_COMMS,WDG_COMMS_TO_ESPERA_ON );
	xprintf_PD( DF_COMMS, PSTR("COMMS: IN st_espera_prendido.\r\n\0"));
	xprintf_P( PSTR("COMMS: espera_prendido.\r\n\0"));

// Loop:
	while (true) {

		ctl_watchdog_kick(WDG_COMMS,WDG_COMMS_TO_ESPERA_ON );

		// Espero de a 10s para poder entrar en tickless.
		vTaskDelay( (portTickType)( 10000 / portTICK_RATE_MS ) );
		timer -= 10;

		// Cada 60s salgo.
		if ( timer <= 0) {
			next_state = ST_DATAFRAME;
			goto EXIT;
		}

		// Proceso las señales:
		if ( xCOMMS_procesar_senales( ST_ESPERA_PRENDIDO , &next_state ) )
			goto EXIT;

	}

EXIT:

	xprintf_PD( DF_COMMS, PSTR("COMMS: OUT st_espera_prendido.\r\n\0"));
	return(next_state);
}
//------------------------------------------------------------------------------------




