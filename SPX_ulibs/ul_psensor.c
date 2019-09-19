/*
 * l_psensor.c
 *
 *  Created on: 19 set. 2019
 *      Author: pablo
 */

#include "spx.h"

//------------------------------------------------------------------------------------
void psensor_config_defaults(void)
{

	snprintf_P( systemVars.psensor_conf.name, PARAMNAME_LENGTH, PSTR("PSEN\0"));
	systemVars.psensor_conf.pmax = 1000;
	systemVars.psensor_conf.pmin = 0;

}
//------------------------------------------------------------------------------------
bool psensor_config ( char *s_pname, char *s_pmin, char *s_pmax  )
{

	if ( s_pname != NULL ) {
		snprintf_P( systemVars.psensor_conf.name, PARAMNAME_LENGTH, PSTR("%s\0"), s_pname );
	}

	if ( s_pmin != NULL ) {
		systemVars.psensor_conf.pmin = atoi(s_pmin);
	}

	if ( s_pmax != NULL ) {
		systemVars.psensor_conf.pmax = atoi(s_pmax);
	}

	//xprintf_P(PSTR("DEBUG PSENSOR [%s,%d],[%s,%d]\r\n\0"), s_pmin, systemVars.psensor_conf.pmin, s_pmax, systemVars.psensor_conf.pmax);
	return(true);

}
//------------------------------------------------------------------------------------
int16_t psensor_read(void)
{

float psensor = -1.0;
char buffer[2] = { 0 };
int8_t xBytes = 0;
int16_t pcounts;

	xBytes = PSENS_read( buffer );
	if ( xBytes == -1 ) {
		xprintf_P(PSTR("ERROR: PSENSOR\r\n\0"));
		return(psensor);
	}

	if ( xBytes > 0 ) {
		pcounts = ( buffer[0]<<8 ) + buffer[1];
		psensor = pcounts;
		psensor *= systemVars.psensor_conf.pmax;
		psensor /= (0.9 * 16384);
		return((int16_t)psensor);
	}

	return(psensor);
}
//------------------------------------------------------------------------------------


