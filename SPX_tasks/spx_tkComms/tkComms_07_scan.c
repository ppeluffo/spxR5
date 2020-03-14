/*
 * tkComms_07_scan.c
 *
 *  Created on: 6 mar. 2020
 *      Author: pablo
 */

#include <tkComms.h>

// La tarea no puede demorar mas de 180s.
#define WDG_COMMS_TO_SCAN	180

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
	xprintf_PD( DF_COMMS, PSTR("COMMS: IN st_scan.\r\n\0"));
	//xprintf_P( PSTR("COMMS: scan.\r\n\0"));

	scan_boundle.apn = systemVars.comms_conf.apn;
	scan_boundle.dlgid = systemVars.comms_conf.dlgId;
	scan_boundle.server_ip = systemVars.comms_conf.server_ip_address;
	scan_boundle.tcp_port = systemVars.comms_conf.server_tcp_port;
	scan_boundle.f_debug = DF_COMMS;
	scan_boundle.cpin = systemVars.comms_conf.simpwd;
	scan_boundle.script = systemVars.comms_conf.serverScript;

	if ( xCOMMS_need_scan( scan_boundle ) == true ) {
		// Necesito descubir los parametros.

		if ( xCOMMS_scan( scan_boundle ) == true ) {
			// Descubri los parametros. Salgo a reiniciarme con estos.
			u_save_params_in_NVMEE();
			xprintf_P( PSTR("COMMS: SCAN APN=[%s]\r\n\0"), systemVars.comms_conf.apn );
			xprintf_P( PSTR("COMMS: SCAN IP=[%s]\r\n\0"), systemVars.comms_conf.server_ip_address );
			xprintf_P( PSTR("COMMS: SCAN DLGID=[%s]\r\n\0"), systemVars.comms_conf.dlgId );
			xCOMMS_apagar_dispositivo();
			systemVars.comms_conf.timerDial = 10;	// Debo arrancar enseguida. Luego me reconfiguro
			next_state = ST_ENTRY;
		} else {
			// No pude descubrir los parametros. Espero 1H para reintentar.
			systemVars.comms_conf.timerDial = 3600;
			next_state = ST_ENTRY;
		}


	} else {
		// No need scan: go ahead to ST_IP
		next_state = ST_IP;
	}


	xprintf_PD( DF_COMMS, PSTR("COMMS: OUT st_scan.\r\n\0"));
	return(next_state);

}
//------------------------------------------------------------------------------------




