/*
 * tkComms_02_entry.c
 *
 *  Created on: 5 mar. 2020
 *      Author: pablo
 */

#include "tkComms.h"

//------------------------------------------------------------------------------------
t_comms_states tkComms_st_entry(void)
{
	/*
	 * A este estado vuelvo siempre.
	 * Es donde determino si debo esperar prendido o apagado.
	 * Salidas:
	 * 	ST_ESPERA_APAGADO
	 * 	ST_ESPERA_PRENDIDO
	 *
	 */

	xprintf_PD( DF_COMMS, PSTR("COMMS: IN st_entry.\r\n\0"));


	xprintf_PD( DF_COMMS, PSTR("COMMS: OUT st_entry.\r\n\0"));
	return(ST_ESPERA_APAGADO);
	//return(ST_ESPERA_PRENDIDO);
}
//------------------------------------------------------------------------------------


