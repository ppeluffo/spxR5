/*
 * tkComms_05_configurar.c
 *
 *  Created on: 5 mar. 2020
 *      Author: pablo
 */

#include <tkComms.h>

// La tarea no puede demorar mas de 180s.
#define WDG_COMMS_TO_CONFIG	WDG_TO180

//------------------------------------------------------------------------------------
t_comms_states tkComms_st_configurar(void)
{
	/*
	 * Configuro el dispositivo para que pueda usarse.
	 * Salidas:
	 * 	ST_MON_SQE
	 * 	ST_ENTRY
	 *
	 */

uint8_t err_code;
t_comms_states next_state = ST_ENTRY;

	ctl_watchdog_kick(WDG_COMMS, WDG_COMMS_TO_CONFIG);
	xprintf_PD( DF_COMMS, PSTR("COMMS: IN st_configurar.[%d,%d,%d]\r\n\0"),xCOMMS_stateVars.gprs_prendido, xCOMMS_stateVars.gprs_inicializado,xCOMMS_stateVars.errores_comms);
#ifdef MONITOR_STACK
	debug_print_stack_watermarks("5");
#endif
	//xprintf_PD( DF_COMMS, PSTR("COMMS: configurar.\r\n\0"));

	if ( xCOMMS_configurar_dispositivo(DF_COMMS, sVarsComms.simpwd, &err_code ) == true ) {
		next_state = ST_MON_SQE;
		goto EXIT;
	}

//	xprintf_P( PSTR("DEBUG: gprs_prendido=%d\r\n\0"), xCOMMS_stateVars.gprs_prendido);
//	xprintf_P(PSTR("DEBUG: gprs_inicializado=%d\r\n\0"), xCOMMS_stateVars.gprs_inicializado);

	// Error de configuracion.
	xCOMMS_stateVars.errores_comms++;
	xCOMMS_stateVars.reset_dlg = false;
	switch (err_code) {
	case ERR_CPIN_FAIL:
		/*
		 * No tengo el pin correcto. Reintento dentro de 1 hora
		 */
		xprintf_P( PSTR("COMMS: pin ERROR: Reconfiguro timerdial para 1H\r\n\0"));
		sVarsComms.timerDial = 3600;
		xCOMMS_stateVars.reset_dlg = true;
		next_state = ST_ENTRY;
		break;

	case ERR_NETATTACH_FAIL:
		xprintf_P( PSTR("COMMS: net ERRROR: Reconfiguro timerdial para 30min.\r\n\0"));
		sVarsComms.timerDial = 1800;
		xCOMMS_stateVars.reset_dlg = true;
		next_state = ST_ENTRY;
		break;

	default:
		xprintf_P( PSTR("COMMS: config ERRROR: Reconfiguro timerdial para 30min.\r\n\0"));
		sVarsComms.timerDial = 1800;
		xCOMMS_stateVars.reset_dlg = true;
		next_state = ST_ENTRY;
		break;
	}

	// No proceso las señales:

EXIT:

	xprintf_PD( DF_COMMS, PSTR("COMMS: OUT st_configurar.[%d,%d,%d](%d)\r\n\0"),xCOMMS_stateVars.gprs_prendido, xCOMMS_stateVars.gprs_inicializado,xCOMMS_stateVars.errores_comms, next_state);
	return(next_state);
}
//------------------------------------------------------------------------------------



