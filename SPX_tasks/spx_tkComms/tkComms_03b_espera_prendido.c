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
t_comms_states  tkComms_st_espera_prendido(void)
{
	/*
	 * Espero con el dispositivo prendido.
	 * Salgo solo al recibir una senal.
	 *
	 */

t_comms_states exit_flag = ST_ENTRY;

// Entry:
	ctl_watchdog_kick(WDG_COMMS,WDG_COMMS_TO_ESPERA_ON );
	xprintf_PD( DF_COMMS, PSTR("COMMS: IN st_espera_prendido.\r\n\0"));
	xprintf_P( PSTR("COMMS: espera_prendido.\r\n\0"));

// Loop:
	while (true) {
		ctl_watchdog_kick(WDG_COMMS,WDG_COMMS_TO_ESPERA_ON );

		// Espero de a 10s para poder entrar en tickless.
		vTaskDelay( (portTickType)( 10000 / portTICK_RATE_MS ) );

		// Proceso las señales:
		if ( SPX_SIGNAL( SGN_REDIAL )) {
			/*
			 * Debo apagar y prender el dispositivo. Como ya estoy apagado
			 * salgo para pasar al estado PRENDIENDO.
			 */
			xprintf_PD( DF_COMMS, PSTR("COMMS: st_espera_prendido. SGN_REDIAL rcvd.\r\n\0"));
			exit_flag = ST_ENTRY;
			goto EXIT;
		}

		if ( SPX_SIGNAL( SGN_FRAME_READY )) {
			/*
			 * Debo apagar y prender el dispositivo. Como ya estoy apagado
			 * salgo para pasar al estado PRENDIENDO.
			 */
			SPX_CLEAR_SIGNAL( SGN_FRAME_READY );
			xprintf_PD( DF_COMMS, PSTR("COMMS: st_espera_prendido. SGN_FRAME_READY rcvd.\r\n\0"));
			exit_flag = ST_DATAFRAME;
			goto EXIT;
		}

	}

EXIT:

	xprintf_PD( DF_COMMS, PSTR("COMMS: OUT st_espera_prendido.\r\n\0"));
	return(exit_flag);
}
//------------------------------------------------------------------------------------




