/*
 * l_psensor.c
 *
 *  Created on: 19 set. 2019
 *      Author: pablo
 */

#include "spx.h"

//------------------------------------------------------------------------------------
void psensor_init(void)
{
}
//------------------------------------------------------------------------------------
void psensor_config_defaults(void)
{

	snprintf_P( systemVars.psensor_conf.name, PARAMNAME_LENGTH, PSTR("X\0"));
	systemVars.psensor_conf.pmax = 1.0;
	systemVars.psensor_conf.pmin = 0;
	systemVars.psensor_conf.poffset = 0;

}
//------------------------------------------------------------------------------------
bool psensor_config ( char *s_pname, char *s_pmin, char *s_pmax, char *s_poffset  )
{

	if ( s_pname != NULL ) {
		snprintf_P( systemVars.psensor_conf.name, PARAMNAME_LENGTH, PSTR("%s\0"), s_pname );
	}

	if ( s_pmin != NULL ) {
		systemVars.psensor_conf.pmin = atof(s_pmin);
	}

	if ( s_pmax != NULL ) {
		systemVars.psensor_conf.pmax = atof(s_pmax);
	}

	if ( s_poffset != NULL ) {
		systemVars.psensor_conf.poffset = atof(s_poffset);
	}

	//xprintf_P(PSTR("DEBUG PSENSOR [%s,%d],[%s,%d]\r\n\0"), s_pmin, systemVars.psensor_conf.pmin, s_pmax, systemVars.psensor_conf.pmax);
	return(true);

}
//------------------------------------------------------------------------------------
bool  psensor_read( int16_t *psens )
{

char buffer[2] = { 0 };
int8_t xBytes = 0;
int16_t pcounts;
bool retS = false;

	xBytes = PSENS_raw_read( buffer );
	if ( xBytes == -1 ) {
		xprintf_P(PSTR("ERROR: PSENSOR\r\n\0"));
		return(false);
	}

	if ( xBytes > 0 ) {
		pcounts = ( buffer[0]<<8 ) + buffer[1];
		*psens = (uint16_t) ( systemVars.psensor_conf.pmax * (pcounts - 1638)/13107 + systemVars.psensor_conf.poffset );
	}

	return(retS);
}
//------------------------------------------------------------------------------------
void psensor_test_read (void)
{
	// Funcion de testing del sensor de presion I2C
	// La direccion es fija 0x50 y solo se leen 2 bytes.

int8_t xBytes = 0;
char buffer[2] = { 0 };
int16_t pcounts = 0;
float hcl;


	xBytes = PSENS_raw_read( buffer );
	if ( xBytes == -1 )
		xprintf_P(PSTR("ERROR: I2C: PSENS_test_read\r\n\0"));

	if ( xBytes > 0 )
		pcounts = ( buffer[0]<<8 ) + buffer[1];
		hcl = systemVars.psensor_conf.pmax * (pcounts - 1638)/13107;

		xprintf_P( PSTR( "I2C_PSENSOR=0x%04x,pcount=%d,Hmt=%0.3f\r\n\0"),pcounts,pcounts,hcl);

}
//------------------------------------------------------------------------------------

void psensor_print(file_descriptor_t fd, uint16_t src )
{

//	if ( ! strcmp ( systemVars.psensor_conf.name, "X" ) )
//		return;

	xCom_printf_P(fd, PSTR("%s:%d;"), systemVars.psensor_conf.name, src );
}
//------------------------------------------------------------------------------------
uint8_t psensor_checksum(void)
{

uint8_t checksum = 0;
char dst[32];
char *p;
uint8_t j = 0;

	//	char name[PARAMNAME_LENGTH];
	//	uint16_t pmax;
	//	uint16_t pmin;


	// calculate own checksum
	// Vacio el buffer temoral
	memset(dst,'\0', sizeof(dst));
	// Copio sobe el buffer una vista ascii ( imprimible ) de c/registro.
	j += snprintf_P(&dst[j], sizeof(dst), PSTR("%s,%.03f,"), systemVars.psensor_conf.name,systemVars.psensor_conf.pmin );
	j += snprintf_P(&dst[j], sizeof(dst), PSTR("%.03f,%.03f"), systemVars.psensor_conf.pmax ,systemVars.psensor_conf.poffset);

	//xprintf_P( PSTR("DEBUG: PCKS = [%s]\r\n\0"), dst );
	// Apunto al comienzo para recorrer el buffer
	p = dst;
	// Mientras no sea NULL calculo el checksum deol buffer
	while (*p != '\0') {
		checksum += *p++;
	}
	//xprintf_P( PSTR("DEBUG: cks = [0x%02x]\r\n\0"), checksum );

	return(checksum);

}
//------------------------------------------------------------------------------------

