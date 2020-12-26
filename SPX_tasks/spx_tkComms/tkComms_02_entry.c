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

	xprintf_PD( DF_COMMS, PSTR("COMMS: IN st_entry.\r\n"));
	ctl_watchdog_kick(WDG_COMMS, WDG_COMMS_TO_ENTRY);

	if ( ! MODO_DISCRETO )									// Solo en modo continuo
		if ( xCOMMS_stateVars.gprs_prendido )				// con el modem prendido
			if ( xCOMMS_stateVars.gprs_inicializado )		// e inicializado
				next_state = ST_ESPERA_PRENDIDO;


	// CONTROL DE ERRORES DE COMUNICACIONES
	// Si la cantidad de errores de comunicacion llego al maximo, reseteo el micro
	if ( xCOMMS_stateVars.errores_comms >= MAX_ERRORES_COMMS ) {
		xprintf_PD( DF_COMMS, PSTR("COMMS: RESET x MAX_ERRORES_COMMS\r\n\0"));
		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
		CCPWrite( &RST.CTRL, RST_SWRST_bm );   /* Issue a Software Reset to initilize the CPU */
	}

	xprintf_PD( DF_COMMS, PSTR("COMMS: OUT st_entry.\r\n"));
	return(next_state);

}
//------------------------------------------------------------------------------------


