/*
 * spx_ainputs.c
 *
 *  Created on: 8 mar. 2019
 *      Author: pablo
 */

#include "spx.h"

//------------------------------------------------------------------------------------
void ainputs_read ( uint8_t io_channel, uint16_t *raw_val, float *mag_val )
{
	// Lee un canal analogico y devuelve el valor convertido a la magnitud configurada.
	// Es publico porque se utiliza tanto desde el modo comando como desde el modulo de poleo de las entradas.
	// Hay que corregir la correspondencia entre el canal leido del INA y el canal fisico del datalogger
	// io_channel. Esto lo hacemos en AINPUTS_read_ina.

uint16_t an_raw_val;
float an_mag_val;
float I,M,P;
uint16_t D;

	an_raw_val = AINPUTS_read_ina(spx_io_board, io_channel );
	*raw_val = an_raw_val;

	// Convierto el raw_value a la magnitud
	// Calculo la corriente medida en el canal
	I = (float)( an_raw_val) * 20 / ( systemVars.ainputs_conf.inaspan[io_channel] + 1);

	// Calculo la magnitud
	P = 0;
	D = systemVars.ainputs_conf.imax[io_channel] - systemVars.ainputs_conf.imin[io_channel];

	an_mag_val = 0.0;
	if ( D != 0 ) {
		// Pendiente
		P = (float) ( systemVars.ainputs_conf.mmax[io_channel]  -  systemVars.ainputs_conf.mmin[io_channel] ) / D;
		// Magnitud
		M = (float) (systemVars.ainputs_conf.mmin[io_channel] + ( I - systemVars.ainputs_conf.imin[io_channel] ) * P);

		// Al calcular la magnitud, al final le sumo el offset.
		an_mag_val = M + systemVars.ainputs_conf.offset[io_channel];
	} else {
		// Error: denominador = 0.
		an_mag_val = -999.0;
	}

	*mag_val = an_mag_val;

}
//------------------------------------------------------------------------------------
bool ainputs_config_channel( uint8_t channel,char *s_aname,char *s_imin,char *s_imax,char *s_mmin,char *s_mmax )
{

	// Configura los canales analogicos. Es usada tanto desde el modo comando como desde el modo online por gprs.

bool retS = false;

	u_control_string(s_aname);

	if ( ( channel >=  0) && ( channel < NRO_ANINPUTS ) ) {
		snprintf_P( systemVars.ainputs_conf.name[channel], PARAMNAME_LENGTH, PSTR("%s\0"), s_aname );
		if ( s_imin != NULL ) { systemVars.ainputs_conf.imin[channel] = atoi(s_imin); }
		if ( s_imax != NULL ) { systemVars.ainputs_conf.imax[channel] = atoi(s_imax); }
		if ( s_mmin != NULL ) { systemVars.ainputs_conf.mmin[channel] = atoi(s_mmin); }
		if ( s_mmax != NULL ) { systemVars.ainputs_conf.mmax[channel] = atof(s_mmax); }
		retS = true;
	}

	return(retS);
}
//------------------------------------------------------------------------------------
bool ainputs_config_offset( char *s_channel, char *s_offset )
{
	// Configuro el parametro offset de un canal analogico.

uint8_t channel;
float offset;

	channel = atoi(s_channel);
	if ( ( channel >=  0) && ( channel < NRO_ANINPUTS ) ) {
		offset = atof(s_offset);
		systemVars.ainputs_conf.offset[channel] = offset;
		return(true);
	}

	return(false);
}
//------------------------------------------------------------------------------------
void ainputs_config_sensortime ( char *s_sensortime )
{
	// Configura el tiempo de espera entre que prendo  la fuente de los sensores y comienzo el poleo.
	// Se utiliza solo desde el modo comando.
	// El tiempo de espera debe estar entre 1s y 15s

	systemVars.ainputs_conf.pwr_settle_time = atoi(s_sensortime);

	if ( systemVars.ainputs_conf.pwr_settle_time < 1 )
		systemVars.ainputs_conf.pwr_settle_time = 1;

	if ( systemVars.ainputs_conf.pwr_settle_time > 15 )
		systemVars.ainputs_conf.pwr_settle_time = 15;

	return;
}
//------------------------------------------------------------------------------------
void ainputs_config_span ( char *s_channel, char *s_span )
{
	// Configura el factor de correccion del span de canales delos INA.
	// Esto es debido a que las resistencias presentan una tolerancia entonces con
	// esto ajustamos que con 20mA den la excursión correcta.
	// Solo de configura desde modo comando.

uint8_t channel;
uint16_t span;

	channel = atoi(s_channel);
	if ( ( channel >=  0) && ( channel < NRO_ANINPUTS ) ) {
		span = atoi(s_span);
		systemVars.ainputs_conf.inaspan[channel] = span;
	}
	return;

}
//------------------------------------------------------------------------------------
bool ainputs_config_autocalibrar( char *s_channel, char *s_mag_val )
{
	// Para un canal, toma como entrada el valor de la magnitud y ajusta
	// mag_offset para que la medida tomada coincida con la dada.


uint16_t an_raw_val;
float an_mag_val;
float I,M,P;
uint16_t D;
uint8_t channel;

float an_mag_val_real;
float offset;

	channel = atoi(s_channel);

	if ( channel >= NRO_ANINPUTS ) {
		return(false);
	}

	// Leo el canal del ina.
	an_raw_val = AINPUTS_read_ina( spx_io_board, channel );
//	xprintf_P( PSTR("ANRAW=%d\r\n\0"), an_raw_val );

	// Convierto el raw_value a la magnitud
	I = (float)( an_raw_val) * 20 / ( systemVars.ainputs_conf.inaspan[channel] + 1);
	P = 0;
	D = systemVars.ainputs_conf.imax[channel] - systemVars.ainputs_conf.imin[channel];

	an_mag_val = 0.0;
	if ( D != 0 ) {
		// Pendiente
		P = (float) ( systemVars.ainputs_conf.mmax[channel]  -  systemVars.ainputs_conf.mmin[channel] ) / D;
		// Magnitud
		M = (float) ( systemVars.ainputs_conf.mmin[channel] + ( I - systemVars.ainputs_conf.imin[channel] ) * P);

		// En este caso el offset que uso es 0 !!!.
		an_mag_val = M;

	} else {
		return(false);
	}

//	xprintf_P( PSTR("ANMAG=%.02f\r\n\0"), an_mag_val );

	an_mag_val_real = atof(s_mag_val);
//	xprintf_P( PSTR("ANMAG_T=%.02f\r\n\0"), an_mag_val_real );

	offset = an_mag_val_real - an_mag_val;
//	xprintf_P( PSTR("AUTOCAL offset=%.02f\r\n\0"), offset );

	systemVars.ainputs_conf.offset[channel] = offset;

	xprintf_P( PSTR("OFFSET=%.02f\r\n\0"), systemVars.ainputs_conf.offset[channel] );

	return(true);

}
//------------------------------------------------------------------------------------
void ainputs_config_defaults(void)
{
	// Realiza la configuracion por defecto de los canales digitales.

uint8_t channel;

	systemVars.ainputs_conf.pwr_settle_time = 1;

	for ( channel = 0; channel < NRO_ANINPUTS; channel++) {
		systemVars.ainputs_conf.imin[channel] = 0;
		systemVars.ainputs_conf.imax[channel] = 20;
		systemVars.ainputs_conf.mmin[channel] = 0;
		systemVars.ainputs_conf.mmax[channel] = 6.0;
		systemVars.ainputs_conf.offset[channel] = 0.0;
		systemVars.ainputs_conf.inaspan[channel] = 3646;
		snprintf_P( systemVars.ainputs_conf.name[channel], PARAMNAME_LENGTH, PSTR("A%d\0"),channel );
	}

}
//------------------------------------------------------------------------------------

