/*
 * tkComms_07_scan.c
 *
 *  Created on: 6 mar. 2020
 *      Author: pablo
 */

#include <tkComms.h>
#include "spx_tkApp/tkApp.h"

// La tarea no puede demorar mas de 180s.
#define WDG_COMMS_TO_SCAN	WDG_TO180

//------------------------------------------------------------------------------------
t_comms_states tkComms_st_scan(void)
{
	/*
	 * El SCAN significa que si alguno de los parametros ( APN / IPserver / DLGID ) esta
	 * en DEFAULT, se procede a tantear hasta configurarlos.
	 * En caso que no se pueda, no se puede seguir por lo que se da un mensaje de error y
	 * se pasa a espera 1H para reintentar.
	 * El concepto de APN e IPserver solo se aplica a GPRS.
	 */

t_comms_states next_state = ST_ENTRY;
t_scan_struct scan_boundle;

	ctl_watchdog_kick( WDG_COMMS, WDG_COMMS_TO_SCAN );
	xprintf_PD( DF_COMMS, PSTR("COMMS: IN st_scan.[%d,%d,%d]\r\n\0"),xCOMMS_stateVars.gprs_prendido, xCOMMS_stateVars.gprs_inicializado,xCOMMS_stateVars.errores_comms);
#ifdef MONITOR_STACK
	debug_print_stack_watermarks("7");
#endif

	// El scan_boundle ( punteros ) apunta a los sVarsComms homologos
	scan_boundle.apn = sVarsComms.apn;
	scan_boundle.dlgid = sVarsComms.dlgId;
	scan_boundle.server_ip = sVarsComms.server_ip_address;
	scan_boundle.script = sVarsComms.serverScript;
	scan_boundle.tcp_port = sVarsComms.server_tcp_port;
	scan_boundle.cpin = sVarsComms.simpwd;

	if ( xCOMMS_need_scan( &scan_boundle ) == true ) {
		// Necesito descubir los parametros.
		// Puedo demorar hasta 10 minutos  por lo que ajusto el watchdog !!!
		ctl_watchdog_kick( WDG_COMMS, WDG_TO600 );

		// Estoy en un scan: pongo a default el server/port/cpin.
		strncpy_P( sVarsComms.serverScript, PSTR("/cgi-bin/SPY/spy.py\0"),SCRIPT_LENGTH);
		strncpy_P( sVarsComms.server_tcp_port, PSTR("80\0"),PORT_LENGTH	);
		snprintf_P( sVarsComms.simpwd, sizeof( sVarsComms.simpwd), PSTR("%s\0"), SIMPIN_DEFAULT );

		if ( xCOMMS_scan( &scan_boundle ) == true ) {
			// Descubri los parametros. Ya estan en el sVarsComms.
			// Los salvo y salgo a reiniciarme con estos.
			u_save_params_in_NVMEE();

			xprintf_P( PSTR("COMMS: SCAN APN=[%s]\r\n\0"), sVarsComms.apn );
			xprintf_P( PSTR("COMMS: SCAN IP=[%s]\r\n\0"), sVarsComms.server_ip_address );
			xprintf_P( PSTR("COMMS: SCAN DLGID=[%s]\r\n\0"), sVarsComms.dlgId );
			xCOMMS_apagar_dispositivo();
			sVarsComms.timerDial = 10;	// Debo arrancar enseguida. Luego me reconfiguro
			next_state = ST_ENTRY;
		} else {
			// No pude descubrir los parametros. Espero 1H para reintentar.
			sVarsComms.timerDial = 3600;
			next_state = ST_ENTRY;
		}


	} else {
		// No need scan: go ahead to ST_NET
		next_state = ST_NET;
	}

	// Checkpoint de SMS's
	xAPP_sms_checkpoint();

	xprintf_PD( DF_COMMS, PSTR("COMMS: OUT st_scan.[%d,%d,%d]\r\n\0"),xCOMMS_stateVars.gprs_prendido, xCOMMS_stateVars.gprs_inicializado,xCOMMS_stateVars.errores_comms);
	return(next_state);

}
//------------------------------------------------------------------------------------




