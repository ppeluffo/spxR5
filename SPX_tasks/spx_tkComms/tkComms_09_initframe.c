/*
 * tkComms_09_initframes.c
 *
 *  Created on: 6 mar. 2020
 *      Author: pablo
 */

#include <tkComms.h>

// La tarea no puede demorar mas de 180s.
#define WDG_COMMS_TO_INITFRAME	180

//------------------------------------------------------------------------------------
t_comms_states tkComms_st_initframe(void)
{
	/*
	 *
	 */

uint8_t err_code;
t_comms_states exit_flag = ST_ENTRY;

	ctl_watchdog_kick(WDG_COMMS, WDG_COMMS_TO_INITFRAME);
	xprintf_PD( DF_COMMS, PSTR("COMMS: IN st_initframe.\r\n\0"));
	xprintf_P( PSTR("COMMS: initframe.\r\n\0"));


EXIT:

	xprintf_PD( DF_COMMS, PSTR("COMMS: OUT st_initframe.\r\n\0"));
	return(exit_flag);
}
//------------------------------------------------------------------------------------




