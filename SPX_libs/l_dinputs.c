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
uint8_t DIN_read_port( void )
{
	// Solo para el SPX_8CH
	// Leo todo el puerto ( 8 bits ) de entradas digitales
	// Los pines pueden ser 0 o 1. Cualquier otro valor es error

uint8_t data;
int8_t rdBytes;
uint8_t retVal = 0x00;
int8_t i;

	rdBytes = MCP_read( MCP_GPIOA, (char *)&data, 1 );
	if ( rdBytes == -1 ) {
		xprintf_P(PSTR("ERROR: IO_DIN_read_port\r\n\0"));
		return(0x00);
	}

	// Debo rotar data para que coincidan los bits con los pines.
	for ( i = 7; i >= 0; i-- ) {
		retVal |= (data & ( 1 << i )) >> i;
	}

	return(retVal);
}
//------------------------------------------------------------------------------------
int8_t DIN_read_pin( uint8_t pin , uint8_t io_board )
{

int8_t val = -1 ;
uint8_t data;
int8_t rdBytes;

	switch (io_board ) {

	case 0:	// SPX_IO5
		switch ( pin ) {
		case 0:
			val = IO_read_PA0();
			break;
		case 1:
			val = IO_read_PB7();
			break;
		default:
			val = -1;
			break;
		}
		break;

	case 1:	// SPX_IO8CH
		rdBytes = MCP_read( MCP_GPIOA, (char *)&data, 1 );
		if ( rdBytes == -1 ) {
			xprintf_P(PSTR("ERROR: IO_DIN_read_pin\r\n\0"));
			return(-1);
		}
		val = (data & ( 1 << pin )) >> pin;
		break;
	}

	return(val);

}
//------------------------------------------------------------------------------------
