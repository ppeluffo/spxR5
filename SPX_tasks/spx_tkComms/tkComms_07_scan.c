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

uint8_t err_code;
t_comms_states exit_flag = ST_ENTRY;

	ctl_watchdog_kick( WDG_COMMS, WDG_COMMS_TO_SCAN );
	xprintf_PD( DF_COMMS, PSTR("COMMS: IN st_scan.\r\n\0"));
	xprintf_P( PSTR("COMMS: scan.\r\n\0"));


	if ( xCOMMS_scan(DF_COMMS, systemVars.comms_conf.apn ,systemVars.comms_conf.server_ip_address, systemVars.comms_conf.dlgId, &err_code ) == true ) {
		exit_flag = ST_IP;
		goto EXIT;
	}

	switch (err_code) {
	case ERR_APN_FAIL:
		/*
		 * No tengo el pin correcto. Reintento dentro de 1 hora
		 */
		xprintf_P( PSTR("COMMS: APN ERROR: Reconfiguro timerdial para 1H\r\n\0"));
		systemVars.comms_conf.timerDial = 3600;
		break;
	case ERR_IPSERVER_FAIL:
		xprintf_P( PSTR("COMMS: IPServer ERRROR: Reconfiguro timerdial para 1H.\r\n\0"));
		systemVars.comms_conf.timerDial = 3600;
		break;
	case ERR_DLGID_FAIL:
		xprintf_P( PSTR("COMMS: DLGID ERRROR: Reconfiguro timerdial para 1H.\r\n\0"));
		systemVars.comms_conf.timerDial = 3600;
		break;
	default:
		break;
	}

EXIT:

	xprintf_PD( DF_COMMS, PSTR("COMMS: OUT st_scan.\r\n\0"));
	return(exit_flag);

}
//------------------------------------------------------------------------------------




