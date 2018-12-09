/*
 * l_ina3232.c
 *
 *  Created on: 8 dic. 2018
 *      Author: pablo
 */

#include "l_ina3221.h"

//------------------------------------------------------------------------------------
void INA_init(uint8_t spx_io_type )
{
	INA_config_avg128(0);
	INA_config_avg128(1);

	// La arquitectura SPX_8CH es tipo 1 o sea que tiene el ina_2.
	if ( spx_io_type == 1 ) {
		INA_config_avg128(2);
	}
}
//------------------------------------------------------------------------------------
i2c_bus_addr INA_id2busaddr( uint8_t id )
{
	switch(id) {
	case 0:
		// ina_U1 en SPX_5CH. Canales 0,1,2
		// ina_U2 en SPX_8CH. Canales 3,4,5
		return(BUSADDR_INA_A);
		break;
	case 1:
		// ina_U2 en SPX_5CH. Canales 3,4,5
		// ina_U1 en SPX_8CH. Canales 0,1,2
		return(BUSADDR_INA_B);
		break;
	case 2:
		// ina_U3 en SPX_8CH. Canales 6,7,8
		return(BUSADDR_INA_C);
		break;
	default:
		return(99);
		break;

	}

	return(99);

}
//------------------------------------------------------------------------------------
void INA_config( uint8_t ina_id, uint16_t conf_reg_value )
{
char res[3];
int8_t xBytes;

	res[0] = ( conf_reg_value & 0xFF00 ) >> 8;
	res[1] = ( conf_reg_value & 0x00FF );
	xBytes = INA_write( ina_id, INA3231_CONF, res, 2 );
	if ( xBytes == -1 )
		xprintf_P(PSTR("ERROR: I2C:INA_config\r\n\0"));

}
//------------------------------------------------------------------------------------



