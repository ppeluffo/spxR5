/*
 * spx_range.c
 *
 *  Created on: 8 mar. 2019
 *      Author: pablo
 */


#include "spx.h"

//------------------------------------------------------------------------------------
void range_config_defaults(void)
{
	systemVars.rangeMeter_enabled = false;
}
//------------------------------------------------------------------------------------
bool range_config ( char *s_mode )
{

	// Esta opcion es solo valida para IO5
	if ( spx_io_board != SPX_IO5CH )
		return(false);

	if ( !strcmp_P( strupr(s_mode), PSTR("ON\0"))) {
		systemVars.rangeMeter_enabled = true;
		return(true);
	} else if ( !strcmp_P( strupr(s_mode), PSTR("OFF\0"))) {
		systemVars.rangeMeter_enabled = false;
		return(true);
	}
	return(false);

}
//------------------------------------------------------------------------------------
int16_t range_read(void)
{

int16_t range = -1;

	// Solo si el equipo es IO5CH y esta el range habilitado !!!
	if ( ( spx_io_board == SPX_IO5CH )  && ( systemVars.rangeMeter_enabled ) ) {
		( systemVars.debug == DEBUG_DATA ) ?  RMETER_ping( &range, true ) : RMETER_ping( &range, false );
	}
	return(range);
}
//------------------------------------------------------------------------------------

