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

	xprintf_PD( DF_COMMS, PSTR("COMMS: IN st_entry.[%d,%d]\r\n\0"),xCOMMS_stateVars.gprs_prendido, xCOMMS_stateVars.gprs_inicializado);

#ifdef MONITOR_STACK
	debug_print_stack_watermarks("1");
#endif

	ctl_watchdog_kick(WDG_COMMS, WDG_COMMS_TO_ENTRY);

	// Modo discreto: Espero apagado
	if ( MODO_DISCRETO ) {
		next_state = ST_ESPERA_APAGADO;

	} else {

		// Modo continuo:
		if ( xCOMMS_stateVars.gprs_prendido && xCOMMS_stateVars.gprs_inicializado ) {
			// Modo continuo: Dispositivo prendido e inicializado
			next_state = ST_ESPERA_PRENDIDO;
		} else {
			next_state = ST_ESPERA_APAGADO;
		}
	}

	xprintf_PD( DF_COMMS, PSTR("COMMS: OUT st_entry [%d,%d](%d)\r\n\0"), xCOMMS_stateVars.gprs_prendido, xCOMMS_stateVars.gprs_inicializado, next_state );
	return(next_state);

}
//------------------------------------------------------------------------------------


