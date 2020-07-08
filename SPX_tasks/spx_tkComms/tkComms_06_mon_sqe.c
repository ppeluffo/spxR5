/*
 * tkComms_06_mon_sqe.c
 *
 *  Created on: 6 mar. 2020
 *      Author: pablo
 */

#include <tkComms.h>

// Estoy en modo comando por lo que no importa tanto el wdg.
// Le doy 15 minutos
#define WDG_GPRS_TO_SQE	WDG_TO900

//------------------------------------------------------------------------------------
t_comms_states tkComms_st_mon_sqe(void)
{
	/*
	 * Mientras este la senal de MON_SQE activa, quedo en un loop
	 * leyendo c/10 segundos el SQE.
	 * Siempre debo leer al menos una vez en GPRS para mostrar el csq.
	 *
	 */

t_comms_states next_state = ST_SCAN;

	ctl_watchdog_kick(WDG_COMMS,WDG_GPRS_TO_SQE);
	xprintf_PD( DF_COMMS, PSTR("COMMS: IN st_mon_sqe.[%d,%d,%d]\r\n\0"),xCOMMS_stateVars.gprs_prendido, xCOMMS_stateVars.gprs_inicializado,xCOMMS_stateVars.errores_comms);
#ifdef MONITOR_STACK
	debug_print_stack_watermarks("6");
#endif

	/*
	 * Si esta activada la senal, monitoreo en modo continuo
	 * Si no, solo una vez para mostrar y registrar el CSQ
	 */
	if ( SPX_SIGNAL( SGN_MON_SQE ) ) {
		xCOMMS_mon_sqe(DF_COMMS, true, &xCOMMS_stateVars.csq );
	} else {
		xCOMMS_mon_sqe(DF_COMMS, false, &xCOMMS_stateVars.csq );
	}

	xprintf_PD( DF_COMMS, PSTR("COMMS: OUT st_mon_sqe.[%d,%d,%d](%d)\r\n\0"),xCOMMS_stateVars.gprs_prendido, xCOMMS_stateVars.gprs_inicializado,xCOMMS_stateVars.errores_comms, next_state);
	return(next_state);
}
//------------------------------------------------------------------------------------
