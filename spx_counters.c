/*
 * spx_counters.c
 *
 *  Created on: 5 jun. 2019
 *      Author: pablo
 */

#include "spx.h"

extern bool counters_enabled[MAX_COUNTER_CHANNELS];
extern uint16_t counters[MAX_COUNTER_CHANNELS];

//------------------------------------------------------------------------------------
float counters_read_channel( uint8_t cnt, bool reset_counter )
{

	// Funcion para leer el valor de los contadores.
	// Respecto de los contadores, no leemos pulsos sino magnitudes por eso antes lo
	// convertimos a la magnitud correspondiente.
	// Siempre multiplico por magPP. Si quiero que sea en mt3/h, en el server debo hacerlo (  * 3600 / systemVars.timerPoll )

float val = 0;

	val = counters[cnt] * systemVars.counters_conf.magpp[cnt];
	if ( reset_counter )
		counters[cnt] = 0;

	return(val);
}
//------------------------------------------------------------------------------------
void counters_config_defaults(void)
{

	// Realiza la configuracion por defecto de los canales contadores.
	// Los valores son en ms.

uint8_t i;

	for ( i = 0; i < MAX_COUNTER_CHANNELS; i++ ) {
		snprintf_P( systemVars.counters_conf.name[i], PARAMNAME_LENGTH, PSTR("C%d\0"), i );
		systemVars.counters_conf.magpp[i] = 1;
		systemVars.counters_conf.period[i] = 100;
		systemVars.counters_conf.pwidth[i] = 10;
		counters_enabled[i] = true;
	}

	systemVars.counters_conf.speed[0] = CNT_LOW_SPEED;
	systemVars.counters_conf.speed[1] = CNT_HIGH_SPEED;

}
//------------------------------------------------------------------------------------
bool counters_config_channel( uint8_t channel,char *s_param0, char *s_param1, char *s_param2, char *s_param3, char *s_param4 )
{
	// Configuro un canal contador.
	// channel: id del canal
	// s_param0: string del nombre del canal
	// s_param1: string con el valor del factor magpp.
	//
	// {0..1} dname magPP

bool retS = false;

	if ( s_param0 == NULL ) {
		return(retS);
	}

	if ( ( channel >=  0) && ( channel < MAX_COUNTER_CHANNELS ) ) {

		// NOMBRE
		if ( u_control_string(s_param0) == 0 ) {
			xprintf_P( PSTR("DEBUG COUNTERS ERROR: C%d\r\n\0"), channel );
			return( false );
		}

		snprintf_P( systemVars.counters_conf.name[channel], PARAMNAME_LENGTH, PSTR("%s\0"), s_param0 );
		if (!strcmp_P( strupr(systemVars.counters_conf.name[channel]), PSTR("X")) ) {
			counters_enabled[channel] = false;
			COUNTERS_disable_interrupt(channel);
		} else {
			counters_enabled[channel] = true;
			COUNTERS_enable_interrupt(channel);
		}

		// MAGPP
		if ( s_param1 != NULL ) { systemVars.counters_conf.magpp[channel] = atof(s_param1); }

		// PW
		if ( s_param2 != NULL ) { systemVars.counters_conf.pwidth[channel] = atof(s_param2); }

		// PERIOD
		if ( s_param3 != NULL ) { systemVars.counters_conf.period[channel] = atof(s_param3); }

		// SPEED
		if ( !strcmp_P( s_param4, PSTR("LS\0"))) {
			 systemVars.counters_conf.speed[channel] = CNT_LOW_SPEED;
		} else if ( !strcmp_P( s_param4 , PSTR("HS\0"))) {
			 systemVars.counters_conf.speed[channel] = CNT_HIGH_SPEED;
		}

		retS = true;
	}

	return(retS);

}
//------------------------------------------------------------------------------------
void counters_df_print( dataframe_s *df )
{
uint8_t channel;

	// Contadores
	for ( channel = 0; channel < NRO_COUNTERS; channel++) {
		// Si el canal no esta configurado no lo muestro.
		if ( ! strcmp ( systemVars.counters_conf.name[channel], "X" ) )
			continue;

		xprintf_P(PSTR(",%s=%.03f"),systemVars.counters_conf.name[channel], df->counters[channel] );
	}
}
//------------------------------------------------------------------------------------
