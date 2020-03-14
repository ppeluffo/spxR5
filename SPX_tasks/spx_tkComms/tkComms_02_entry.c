/*
 * tkComms_02_entry.c
 *
 *  Created on: 5 mar. 2020
 *      Author: pablo
 */

#include "tkComms.h"

#define WDG_COMMS_TO_ENTRY	10

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

	ctl_watchdog_kick(WDG_COMMS, WDG_COMMS_TO_ENTRY);

	if ( MODO_DISCRETO ) {
		// Modo discreto: Espero apagado
		next_state = ST_ESPERA_APAGADO;

	} else {
		// Modo continuo: Espero prendido ( si el modem esta prendido )
		if ( ! xCOMMS_stateVars.dispositivo_prendido ) {
			// Modo continuo pero dispositivo apagado: salgo a prenderlo
			next_state = ST_PRENDER;
		} else {
			// Dispositivo prendido. Salgo a esperar.
			next_state = ST_ESPERA_PRENDIDO;
		}

	}

	xprintf_PD( DF_COMMS, PSTR("COMMS: OUT st_entry.\r\n\0"));
	return(next_state);

}
//------------------------------------------------------------------------------------


