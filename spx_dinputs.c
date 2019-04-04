/*
 * spx_dinputs.c
 *
 *  Created on: 4 abr. 2019
 *      Author: pablo
 */

#include "spx.h"

//------------------------------------------------------------------------------------
bool dinputs_config_channel( uint8_t channel,char *s_aname )
{

	// Configura los canales digitales. Es usada tanto desde el modo comando como desde el modo online por gprs.

bool retS = false;

	u_control_string(s_aname);

	if ( ( channel >=  0) && ( channel < NRO_DINPUTS ) ) {
		snprintf_P( systemVars.dinputs_conf.name[channel], PARAMNAME_LENGTH, PSTR("%s\0"), s_aname );
		retS = true;
	}

	return(retS);
}
//------------------------------------------------------------------------------------
void dinputs_config_defaults(void)
{
	// Realiza la configuracion por defecto de los canales digitales.

uint8_t i;

	for ( i = 0; i < MAX_DINPUTS_CHANNELS; i++ ) {
		snprintf_P( systemVars.dinputs_conf.name[i], PARAMNAME_LENGTH, PSTR("D%d\0"), i );
	}

}
//------------------------------------------------------------------------------------
int8_t dinputs_read ( uint8_t din )
{
	// Solo devuleve el nivel logico de la entrada. ( OJO: No el dtimer !!! )

uint8_t port;
int16_t retVal = -1;

	// Esta funcion la invoca tkData al completar un frame para agregar los datos
	// digitales.
	// Leo los niveles de las entradas digitales y copio a dframe.

	switch (spx_io_board ) {
	case SPX_IO5CH:
		if ( din < IO5_DINPUTS_CHANNELS ) {
			retVal = DIN_read_pin( din, SPX_IO5CH );
		}
		break;

	case SPX_IO8CH:
		if ( din <  8 ) {
			port = DIN_read_port();	// Leo el puerto para tener los niveles logicos.
			//xprintf_P( PSTR("DEBUG DIN: 0x%02x [%c%c%c%c%c%c%c%c]\r\n\0"), port , BYTE_TO_BINARY( port ));
			retVal = ( port & ( 1 << din )) >> din;
		}
		break;
	}

	return(retVal);

}
//------------------------------------------------------------------------------------
