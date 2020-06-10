/*
 * tkComms_08_ip.c
 *
 *  Created on: 6 mar. 2020
 *      Author: pablo
 */

#include <tkComms.h>

// La tarea no puede demorar mas de 180s.
#define WDG_COMMS_TO_IP	WDG_TO180

//------------------------------------------------------------------------------------
t_comms_states tkComms_st_ip(void)
{
	/*
	 * El concepto de pedir una IP se aplica solo a GPRS.
	 */

uint8_t err_code;
t_comms_states next_state = ST_ENTRY;

	ctl_watchdog_kick( WDG_COMMS, WDG_COMMS_TO_IP );
	xprintf_PD( DF_COMMS, PSTR("COMMS: IN st_ip.[%d,%d]\r\n\0"),xCOMMS_stateVars.gprs_prendido, xCOMMS_stateVars.gprs_inicializado);
#ifdef MONITOR_STACK
	debug_print_stack_watermarks("8");
#endif
	//xprintf_P( PSTR("COMMS: ip.\r\n\0"));

	if ( xCOMMS_ip( DF_COMMS, sVarsComms.apn, xCOMMS_stateVars.ip_assigned, &err_code ) == true ) {
		next_state = ST_INITFRAME;
	} else {
		next_state = ST_ENTRY;
	}

	// Proceso las señales:
	if ( xCOMMS_procesar_senales( ST_IP , &next_state ) )
		goto EXIT;

EXIT:

	xprintf_PD( DF_COMMS, PSTR("COMMS: OUT st_ip.[%d,%d]\r\n\0"),xCOMMS_stateVars.gprs_prendido, xCOMMS_stateVars.gprs_inicializado);
	return(next_state);

}
//------------------------------------------------------------------------------------
