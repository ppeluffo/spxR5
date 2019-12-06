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

	// Esta opcion es solo valida para IO5
	if ( spx_io_board != SPX_IO5CH ) {
		psensor_config_defaults();
		return(false);
	}

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

int8_t xBytes = 0;
char buffer[2] = { 0 };
uint8_t msbPres = 0;
uint8_t lsbPres = 0;
float pres = 0;
int16_t pcounts;

	xBytes = bps120_raw_read( buffer );
	if ( xBytes == -1 ) {
		xprintf_P(PSTR("ERROR: I2C: psensor_read\r\n\0"));
		*presion = -100;
		return(false);
	}

	if ( xBytes > 0 ) {
		msbPres = buffer[0]  & 0x3F;
		lsbPres = buffer[1];
		pcounts = (msbPres << 8) + lsbPres;
		// pcounts = ( buffer[0]<<8 ) + buffer[1];

		// Aplico la funcion de transferencia.
		//pres = ( 1 * (pcounts - 1638)/13107 );
		pres = (( 0.85 * (pcounts - 1638)/13107 ) + 0.15) * 70.31;

		*presion = pres;
		return(true);
	}

	return(true);

}
//------------------------------------------------------------------------------------
bool psensor_test_read( void )
{

int8_t xBytes = 0;
char buffer[2] = { 0 };
uint8_t msbPres = 0;
uint8_t lsbPres = 0;
float pres = 0;
int16_t pcounts;

	xBytes = bps120_raw_read( buffer );
	if ( xBytes == -1 ) {
		xprintf_P(PSTR("ERROR: I2C: psensor_test_read\r\n\0"));
		return(false);
	}

	if ( xBytes > 0 ) {
		msbPres = buffer[0] & 0x3F;
		lsbPres = buffer[1];
		pcounts = (msbPres << 8) + lsbPres;
		// pcounts = ( buffer[0]<<8 ) + buffer[1];

		// Aplico la funcion de transferencia.
		pres = (( 0.85 * (pcounts - 1638)/13107 ) + 0.15) * 70.31;

		xprintf_P( PSTR( "PRES TEST: raw=b0[0x%02x],b1[0x%02x]\r\n\0"),buffer[0],buffer[1]);
		xprintf_P( PSTR( "PRES TEST: pcounts=%d, presion=%.03f\r\n\0"), pcounts, pres );

		return(true);
	}

	return(true);

}
//------------------------------------------------------------------------------------
void psensor_print(file_descriptor_t fd, float presion )
{

//	if ( ! strcmp ( systemVars.psensor_conf.name, "X" ) )
//		return;

	xCom_printf_P(fd, PSTR("%s:%.03f;\0"), systemVars.psensor_conf.name, presion );

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

