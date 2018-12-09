/*
 * l_dinputs.c
 *
 *  Created on: 8 dic. 2018
 *      Author: pablo
 */

#include "l_dinputs.h"

//------------------------------------------------------------------------------------
void DINPUTS_init( void )
{
	// En el caso del SPX_8CH se deberia inicializar el port de salidas del MCP
	// pero esto se hace en la funcion MCP_init(). Esta tambien inicializa el port
	// de entradas digitales.

	// SPX_5CH
	DIN_config_D0();
	DIN_config_D1();

	// SPX_8CH
	MCP_init();
}
//------------------------------------------------------------------------------------
uint8_t DIN_read_port( uint8_t pin)
{

	// Solo para el SPX_8CH
	// Leo todo el puerto ( 8 bits ) de entradas digitales
	// Los pines pueden ser 0 o 1. Cualquier otro valor es error

uint8_t data;
int8_t rdBytes;

//	xprintf_P(PSTR("DEBUG_1: IO_read_DIN\r\n\0"));
	rdBytes = MCP_read( MCP_GPIOA, (char *)&data, 1 );
//	xprintf_P(PSTR("DEBUG_2: IO_read_DIN\r\n\0"));

	if ( rdBytes == -1 ) {
		xprintf_P(PSTR("ERROR: IO_read_DIN\r\n\0"));
	}

	data = (data & ( 1 << pin )) >> pin;

	return(data);
}
//------------------------------------------------------------------------------------
