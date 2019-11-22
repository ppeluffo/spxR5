/*
 * l_psensor.c
 *
 *  Created on: 19 set. 2019
 *      Author: pablo
 *
 *  Rutinas para leer la presion del sensor integrado de presion.
 *
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
	systemVars.psensor_conf.span = 70;
	systemVars.psensor_conf.offset = 0;

}
//------------------------------------------------------------------------------------
bool psensor_config ( char *s_pname, char *s_offset, char *s_span )
{

	if ( s_pname != NULL ) {
		snprintf_P( systemVars.psensor_conf.name, PARAMNAME_LENGTH, PSTR("%s\0"), s_pname );
	}

	if ( s_offset != NULL ) {
		systemVars.psensor_conf.offset = atof(s_offset);
	}


	if ( s_span != NULL ) {
		systemVars.psensor_conf.span = atof(s_span);
	}

	return(true);

}
//------------------------------------------------------------------------------------
bool psensor_read( float *presion )
{

	// El sensor que usamos para leer la presion es el bps120.


float lpres;
int8_t xBytes = 0;

	xBytes = bps120_raw_read( &lpres );
	if ( xBytes == -1 ) {
		xprintf_P(PSTR("ERROR: psensor_read\r\n\0"));
		return(false);
	}

	if ( xBytes > 0 ) {
		// Corrijo por el offset
		lpres = lpres * systemVars.psensor_conf.span  + systemVars.psensor_conf.offset;
		*presion = lpres;
	}

	return(true);
}
//------------------------------------------------------------------------------------
void psensor_test_read (void)
{
	// Funcion de testing del sensor de presion I2C
	// La direccion es fija 0x50 y solo se leen 2 bytes.


float lpres;
float presion = 0;
int8_t xBytes = 0;

	xBytes = bps120_raw_read( &lpres );
	if ( xBytes == -1 ) {
		xprintf_P(PSTR("ERROR: psensor_read\r\n\0"));
		return;
	}

	if ( xBytes > 0 ) {
		// Corrijo por el offset
		presion = lpres * systemVars.psensor_conf.span  + systemVars.psensor_conf.offset;
	}

	xprintf_P( PSTR( "TEST PSENSOR: rawp=%.02f, pres=%0.2f\r\n\0"),lpres,presion);

}
//------------------------------------------------------------------------------------
void psensor_print(file_descriptor_t fd, float presion )
{

//	if ( ! strcmp ( systemVars.psensor_conf.name, "X" ) )
//		return;

	xCom_printf_P(fd, PSTR("%s%.02f;\0"), systemVars.psensor_conf.name, presion );

}
//------------------------------------------------------------------------------------
uint8_t psensor_checksum(void)
{

uint8_t checksum = 0;
char dst[32];
char *p;

	//	char name[PARAMNAME_LENGTH];
	//	float offset;
	//	float span;


	// calculate own checksum
	// Vacio el buffer temoral
	memset(dst,'\0', sizeof(dst));
	// Copio sobe el buffer una vista ascii ( imprimible ) de c/registro.
	snprintf_P( dst, sizeof(dst), PSTR("%s,%.01f,%.01f"), systemVars.psensor_conf.name, systemVars.psensor_conf.offset, systemVars.psensor_conf.span  );

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

