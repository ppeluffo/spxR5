/*
 * l_ina3232.c
 *
 *  Created on: 8 dic. 2018
 *      Author: pablo
 */

#include "l_ina3221.h"

//------------------------------------------------------------------------------------
void INA_init(uint8_t io_board )
{

	if ( io_board == 0 ) {
		// SPX_5CH
		INA_config_avg128( INA_A );
		INA_config_avg128( INA_B );

	} else if ( io_board == 1 ) {
		// SPX_8CH
		INA_config_avg128( INA_B );
		INA_config_avg128( INA_A );
		INA_config_avg128( INA_C );
	}

}
//------------------------------------------------------------------------------------
i2c_bus_addr INA_id2busaddr( uint8_t ina_id )
{
	switch(ina_id) {
	case INA_A:
		// ina_U1 en SPX_5CH. Canales 0,1,2
		// ina_U2 en SPX_8CH. Canales 3,4,5
		return(BUSADDR_INA_A);
		break;
	case INA_B:
		// ina_U2 en SPX_5CH. Canales 3,4,5
		// ina_U1 en SPX_8CH. Canales 0,1,2
		return(BUSADDR_INA_B);
		break;
	case INA_C:
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
int8_t INA_test_write ( char *ina_id_str, char *rconf_val_str )
{

	// Escribe en el registro de configuracion de un INA ( 0, 1, 2)

uint16_t val;
uint8_t ina_id;
char data[3];
int8_t xBytes;

	ina_id = atoi(ina_id_str);
	val = atoi( rconf_val_str);
	data[0] = ( val & 0xFF00 ) >> 8;
	data[1] = ( val & 0x00FF );
	xBytes = INA_write( ina_id, INA3231_CONF, data, 2 );
	if ( xBytes == -1 )
		xprintf_P(PSTR("ERROR: I2C:INA_test_write\r\n\0"));

	return (xBytes);
}
//------------------------------------------------------------------------------------
int8_t INA_test_read ( char *ina_id_str, char *regs )
{

uint16_t val;
uint8_t ina_id;
char data[3];
int8_t xBytes;

	// read ina id {conf|chxshv|chxbusv|mfid|dieid}
	ina_id = atoi(ina_id_str);

	if (!strcmp_P( strupr(regs), PSTR("CONF\0"))) {
		xBytes = INA_read(  ina_id, INA3231_CONF, data, 2 );
	} else if (!strcmp_P( strupr(regs), PSTR("CH1SHV\0"))) {
		xBytes = INA_read(  ina_id, INA3221_CH1_SHV, data, 2 );
	} else if (!strcmp_P( strupr(regs), PSTR("CH1BUSV\0"))) {
		xBytes = INA_read(  ina_id, INA3221_CH1_BUSV, data, 2 );
	} else if (!strcmp_P( strupr(regs), PSTR("CH2SHV\0"))) {
		xBytes = INA_read(  ina_id, INA3221_CH2_SHV, data, 2 );
	} else if (!strcmp_P( strupr(regs), PSTR("CH2BUSV\0"))) {
		xBytes = INA_read(  ina_id, INA3221_CH2_BUSV, data, 2 );
	} else if (!strcmp_P( strupr(regs), PSTR("CH3SHV\0"))) {
		xBytes = INA_read(  ina_id, INA3221_CH3_SHV, data, 2 );
	} else if (!strcmp_P( strupr(regs), PSTR("CH3BUSV\0"))) {
		xBytes = INA_read(  ina_id, INA3221_CH3_BUSV, data, 2 );
	} else if (!strcmp_P( strupr(regs), PSTR("MFID\0"))) {
		xBytes = INA_read(  ina_id, INA3221_MFID, data, 2 );
	} else if (!strcmp_P( strupr(regs), PSTR("DIEID\0"))) {
		xBytes = INA_read(  ina_id, INA3221_DIEID, data, 2 );
	} else {
		xBytes = -1;
	}

	if ( xBytes == -1 ) {
		xprintf_P(PSTR("ERROR: I2C:INA_test_read\r\n\0"));

	} else {

		val = ( data[0]<< 8 ) + data	[1];
		xprintf_P( PSTR("INAID=%d\r\n\0"), ina_id);
		xprintf_P( PSTR("VAL=0x%04x\r\n\0"), val);
	}

	return(xBytes);

}
//------------------------------------------------------------------------------------


