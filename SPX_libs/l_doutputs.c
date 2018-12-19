/*
 * l_doutputs.c
 *
 *  Created on: 8 dic. 2018
 *      Author: pablo
 */

#include "l_doutputs.h"
#include "l_mcp23018.h"

//------------------------------------------------------------------------------------
void DOUTPUTS_init(void)
{
	// En los SPX_8CH la inicializacion la hace el MCP pero ya se hace en DINPUTS_init()
	// MCP_init();
}
//------------------------------------------------------------------------------------
// SALIDAS DIGITALES ( Solo en SPX_8CH )
//------------------------------------------------------------------------------------
void IO_set_DOUT(uint8_t pin)
{
	// Leo el MCP, aplico la mascara y lo escribo de nuevo

uint8_t data;
int8_t xBytes;

	// Control de entrada valida
	if ( pin > 7 ) {
		xprintf_P(PSTR("ERROR: IO_read_DOUT (pin<7)!!!\r\n\0"));
		return;
	}

	xBytes = MCP_read( MCP_GPIOB, (char *)&data, 1 );
	if ( xBytes == -1 ) {
		xprintf_P(PSTR("ERROR: IO_read_DOUT\r\n\0"));
		return;
	}

	// Aplico la mascara para setear el pin dado
	// En el MCP estan en orden inverso
	data |= ( 1 << ( 7 - pin )  );

	xBytes = MCP_write(MCP_OLATB, (char *)&data, 1 );

}
//------------------------------------------------------------------------------------
void IO_clr_DOUT(uint8_t pin)
{
	// Leo el MCP, aplico la mascara y lo escribo de nuevo

uint8_t data;
int8_t xBytes;

	// Control de entrada valida
	if ( pin > 7 ) {
		xprintf_P(PSTR("ERROR: IO_read_DOUT (pin<7)!!!\r\n\0"));
		return;
	}

	xBytes = MCP_read( MCP_GPIOB, (char *)&data, 1 );
	if ( xBytes == -1 ) {
		xprintf_P(PSTR("ERROR: IO_read_DOUT\r\n\0"));
		return;
	}

	// Aplico la mascara para setear el pin dado
	data &= ~( 1 << ( 7 - pin ) );

	xBytes = MCP_write(MCP_OLATB, (char *)&data, 1 );

}
//------------------------------------------------------------------------------------
void IO_reflect_DOUTPUTS(uint8_t output_value )
{
uint8_t data;

	// Escribe todas las salidas a la vez.
	// En el hardware las salidas son inversas a los bits ( posiciones )
	data = 0x00;
	if ( CHECK_BIT_IS_SET(output_value, 0) ) { data |= 0x80; }
	if ( CHECK_BIT_IS_SET(output_value, 1) ) { data |= 0x40; }
	if ( CHECK_BIT_IS_SET(output_value, 2) ) { data |= 0x20; }
	if ( CHECK_BIT_IS_SET(output_value, 3) ) { data |= 0x10; }
	if ( CHECK_BIT_IS_SET(output_value, 4) ) { data |= 0x08; }
	if ( CHECK_BIT_IS_SET(output_value, 5) ) { data |= 0x04; }
	if ( CHECK_BIT_IS_SET(output_value, 6) ) { data |= 0x02; }
	if ( CHECK_BIT_IS_SET(output_value, 7) ) { data |= 0x01; }

//	xprintf_P(PSTR("IO: %d 0x%0x, DAT=0x%0x\r\n\0"),output_value,output_value, data);

	MCP_write(MCP_OLATB, (char *)&data, 1 );

}
//------------------------------------------------------------------------------------
