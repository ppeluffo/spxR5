/*
 * l_ain.c
 *
 *  Created on: 8 dic. 2018
 *      Author: pablo
 */

#include "l_ainputs.h"

//------------------------------------------------------------------------------------
void AINPUTS_init( void )
{

}
//------------------------------------------------------------------------------------
uint16_t AINPUTS_read_channel( uint8_t dlg_type, uint8_t channel_id )
{
	// Como tenemos 2 arquitecturas de dataloggers, SPX_5CH y SPX_8CH,
	// los canales estan mapeados en INA con diferentes id, por eso
	// tomo como parametro el dlg_type que me indica cual arquitectura usar
	// 0 - SPX_5CH, 1 - SPX_8CH

	// ina_id es el parametro que se pasa a la funcion INA_id2busaddr para
	// que me devuelva la direccion en el bus I2C del dispositivo.


uint8_t ina_reg = 0;
uint8_t ina_id = 0;
uint16_t an_raw_val;
uint8_t MSB, LSB;
char res[3];
int8_t xBytes;
//float vshunt;

	switch(dlg_type) {

	case 0:	// Datalogger SPX_5CH
		switch ( channel_id ) {
		case 0:
			ina_id = 0; ina_reg = INA3221_CH1_SHV;
			break;
		case 1:
			ina_id = 0; ina_reg = INA3221_CH2_SHV;
			break;
		case 2:
			ina_id = 0; ina_reg = INA3221_CH3_SHV;
			break;
		case 3:
			ina_id = 1; ina_reg = INA3221_CH2_SHV;
			break;	// Battery
		case 4:
			ina_id = 1; ina_reg = INA3221_CH3_SHV;
			break;
		case 5:
			ina_id = 1; ina_reg = INA3221_CH1_BUSV;
			break;
		default:
			return(-1);
			break;
		}
		break;

	case 1:	// Datalogger SPX_8CH
		switch ( channel_id ) {
		case 0:
			ina_id = 1; ina_reg = INA3221_CH1_SHV;
			break;
		case 1:
			ina_id = 1; ina_reg = INA3221_CH2_SHV;
			break;
		case 2:
			ina_id = 1; ina_reg = INA3221_CH3_SHV;
			break;
		case 3:
			ina_id = 0; ina_reg = INA3221_CH1_SHV;
			break;
		case 4:
			ina_id = 0; ina_reg = INA3221_CH2_SHV;
			break;
		case 5:
			ina_id = 0; ina_reg = INA3221_CH3_SHV;
			break;
		case 6:
			ina_id = 2; ina_reg = INA3221_CH1_SHV;
			break;
		case 7:
			ina_id = 2; ina_reg = INA3221_CH2_SHV;
			break;
		case 8:
			ina_id = 2; ina_reg = INA3221_CH3_SHV;
			break;
		default:
			return(-1);
			break;
		}
		break;

	default:
		return(-1);
		break;
	}

	// Leo el valor del INA.

	xBytes = INA_read( ina_id, ina_reg, res ,2 );
	if ( xBytes == -1 )
		xprintf_P(PSTR("ERROR: I2C:INA:ACH_read_channel\r\n\0"));

	an_raw_val = 0;
	MSB = res[0];
	LSB = res[1];
	an_raw_val = ( MSB << 8 ) + LSB;
	an_raw_val = an_raw_val >> 3;

//	vshunt = (float) an_raw_val * 40 / 1000;
//	xprintf_P( PSTR("ACH: ch=%d, ina=%d, reg=%d, MSB=0x%x, LSB=0x%x, ANV=(0x%x)%d, VSHUNT = %.02f(mV)\r\n\0") ,channel_id, ina_id, ina_reg, MSB, LSB, an_raw_val, an_raw_val, vshunt );

	return( an_raw_val );
}
//------------------------------------------------------------------------------------


