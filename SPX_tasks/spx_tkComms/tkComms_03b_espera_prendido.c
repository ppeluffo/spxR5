/*
 * tkComms_03b_espera_prendido.c
 *
 *  Created on: 5 mar. 2020
 *      Author: pablo
 */

#include <tkComms.h>
#include "spx_tkApp/tkApp.h"

// La tarea no puede demorar mas de 30s.
#define WDG_COMMS_TO_ESPERA_ON WDG_TO30

//------------------------------------------------------------------------------------
t_comms_states tkComms_st_espera_prendido(void)
{
	/*
	 * Espero con el dispositivo prendido.
	 * Salgo solo al recibir una senal.
	 *
	 */

t_comms_states next_state = ST_DATAFRAME;
int8_t timer = 60;

// Entry:
#ifdef BETA_TEST
	xprintf_PD( DF_COMMS, PSTR("COMMS: IN st_espera_prendido.[%d,%d,%d]\r\n"),xCOMMS_stateVars.gprs_prendido, xCOMMS_stateVars.gprs_inicializado,xCOMMS_stateVars.errores_comms);
#else
	xprintf_PD( DF_COMMS, PSTR("COMMS: IN st_espera_prendido.\r\n"));
#endif

#ifdef MONITOR_STACK
	debug_print_stack_watermarks("3");
#endif

	ctl_watchdog_kick(WDG_COMMS,WDG_COMMS_TO_ESPERA_ON );
// Loop:
	while (true) {

		ctl_watchdog_kick(WDG_COMMS,WDG_COMMS_TO_ESPERA_ON );

		// Espero de a 10s para poder entrar en tickless.
		vTaskDelay( (portTickType)( 10000 / portTICK_RATE_MS ) );
		timer -= 10;

		// Cada 60s salgo.
		if ( timer <= 0) {
			goto EXIT;
		}

		// Proceso las señales:
		if ( xCOMMS_SGN_FRAME_READY()) {
			goto EXIT;
		}
 	}

EXIT:

	// Checkpoint de SMS's
	xAPP_sms_checkpoint();

#ifdef BETA_TEST
	xprintf_PD( DF_COMMS, PSTR("COMMS: OUT st_espera_prendido.[%d,%d,%d](%d)\r\n"),xCOMMS_stateVars.gprs_prendido, xCOMMS_stateVars.gprs_inicializado,xCOMMS_stateVars.errores_comms, next_state);
#else
	xprintf_PD( DF_COMMS, PSTR("COMMS: OUT st_espera_prendido.\r\n"));
#endif

	return(next_state);
}
//------------------------------------------------------------------------------------




