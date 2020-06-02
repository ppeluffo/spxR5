/*
 * tkComms_02_entry.c
 *
 *  Created on: 5 mar. 2020
 *      Author: pablo
 */

#include "tkComms.h"

#define WDG_COMMS_TO_ENTRY	WDG_TO30

//------------------------------------------------------------------------------------
t_comms_states tkComms_st_entry(void)
{
	/*
	 * A este estado vuelvo siempre.
	 * Es donde determino si debo esperar prendido o apagado.
	 * En modo discreto, siempre voy a ESPERA_APAGADO
	 * En modo continuo, si el dispositivo esta prendido voy a ESPERA_PRENDIDO
	 * Pero si esta apagado, debo ir a PRENDER.
	 * Salidas:
	 * 	ST_ESPERA_APAGADO
	 * 	ST_ESPERA_PRENDIDO
	 *
	 */

t_comms_states next_state = ST_ESPERA_APAGADO;

	xprintf_PD( DF_COMMS, PSTR("COMMS: IN st_entry.\r\n\0"));

#ifdef MONITOR_STACK
	debug_print_stack_watermarks("1");
#endif

	ctl_watchdog_kick(WDG_COMMS, WDG_COMMS_TO_ENTRY);

	// en XBEE siempre estoy en modo CONTINUO
	if ( sVarsComms.comms_channel == COMMS_CHANNEL_XBEE ) {
		sVarsComms.timerDial = 0;
	}

	// Modo discreto: Espero apagado
	if ( MODO_DISCRETO ) {
		next_state = ST_ESPERA_APAGADO;
		goto EXIT;

	} else {

		// Modo continuo:
		// Si el dispositivo esta prendido e inicializado voy a ESPERA_PRENDIDO.
		// En otro caso voy a PRENDER

		if ( xCOMMS_stateVars.gprs_prendido && xCOMMS_stateVars.gprs_inicializado ) {
			// Modo continuo pero dispositivo apagado: salgo a prenderlo
			next_state = ST_ESPERA_PRENDIDO;
			goto EXIT;

		}

		next_state = ST_ESPERA_APAGADO;
		goto EXIT;

	}

EXIT:

	xprintf_PD( DF_COMMS, PSTR("COMMS: OUT st_entry.\r\n\0"));
	return(next_state);

}
//------------------------------------------------------------------------------------


