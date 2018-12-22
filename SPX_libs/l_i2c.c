/*
 * l_i2c.c
 *
 *  Created on: 26 de mar. de 2018
 *      Author: pablo
 */

#include "l_i2c.h"

const char str_i2c_dev_0[] PROGMEM = "ERR";
const char str_i2c_dev_1[] PROGMEM = "EE";
const char str_i2c_dev_2[] PROGMEM = "RTC";
const char str_i2c_dev_3[] PROGMEM = "MCP";
const char str_i2c_dev_4[] PROGMEM = "INA_A";
const char str_i2c_dev_5[] PROGMEM = "INA_B";
const char str_i2c_dev_6[] PROGMEM = "INA_C";

const char * const I2C_names[] PROGMEM = { str_i2c_dev_0, str_i2c_dev_1, str_i2c_dev_2, str_i2c_dev_3, str_i2c_dev_4, str_i2c_dev_5, str_i2c_dev_6 };

uint8_t pv_i2_addr_2_idx( uint8_t i2c_bus_address );
char buffer[10];

//------------------------------------------------------------------------------------
int8_t I2C_read( uint8_t i2c_bus_address, uint32_t rdAddress, char *data, uint8_t length )
{
	// Lee en la EE, desde la posicion 'address', 'length' bytes
	// y los deja en el buffer apuntado por 'data'
	// No utilizan el semaforo del bus I2C por lo que debe hacerlo
	// quien invoca !!!!!

size_t xReturn = 0U;
uint8_t bus_address;
uint8_t	dev_address_length = 1;
uint16_t device_address;
int8_t xBytes = 0;
uint8_t i2c_error_code;

	frtos_ioctl( fdI2C,ioctl_OBTAIN_BUS_SEMPH, NULL);

	// 1) Indicamos el periferico i2c en el cual queremos leer ( variable de 8 bits !!! )
	bus_address = i2c_bus_address;
	frtos_ioctl(fdI2C,ioctl_I2C_SET_DEVADDRESS, &bus_address);

	// 2) Luego indicamos la direccion desde donde leer:
	//    Largo: 1 byte indica el largo. El FRTOS espera 1 byte.
	if ( i2c_bus_address == BUSADDR_EEPROM_M2402 ) {
		dev_address_length = 2;	// Las direccione de la EEprom son de 16 bits
	} else {
		dev_address_length = 1;
	}
	frtos_ioctl(fdI2C,ioctl_I2C_SET_BYTEADDRESSLENGTH, &dev_address_length);
	// 	Direccion: El FRTOS espera siempre 2 bytes.
	//  Hago un cast para dejarla en 16 bits.
	device_address = (uint16_t)(rdAddress);
	frtos_ioctl(fdI2C,ioctl_I2C_SET_BYTEADDRESS,&device_address);

	//  3) Por ultimo leemos. No controlo fronteras.
	xBytes = length;
	xReturn = frtos_read(fdI2C, data, xBytes);
//	memset(buffer,'\0', 10);
//	strcpy_P(buffer, (PGM_P)pgm_read_word(&(I2C_names[pv_i2_addr_2_idx( i2c_bus_address )])));
//	xprintf_P(PSTR("I2C RD 0x0%X, %s.\r\n\0"), i2c_bus_address, buffer );

	i2c_error_code = frtos_ioctl(fdI2C, ioctl_I2C_GET_LAST_ERROR, NULL );
	if (i2c_error_code != I2C_OK ) {
		memset(buffer,'\0', 10);
		strcpy_P(buffer, (PGM_P)pgm_read_word(&(I2C_names[pv_i2_addr_2_idx( i2c_bus_address )])));
		xprintf_P(PSTR("ERROR: I2C RD err 0x0%X, %s.\r\n\0"), i2c_bus_address, buffer );
	}

	if (xReturn != xBytes ) {
		xReturn = -1;
	}

	frtos_ioctl( fdI2C,ioctl_RELEASE_BUS_SEMPH, NULL);
	return(xReturn);

}
//------------------------------------------------------------------------------------
int8_t I2C_write( uint8_t i2c_bus_address, uint32_t wrAddress, char *data, uint8_t length )
{
	// Escribe en la EE a partir de la posicion 'eeAddress', la cantidad
	// 'length' de bytes apuntados por 'data'
	// Puedo estar escribiendo un pageWrite entonces debo controlar no
	// salime de la pagina.

size_t xReturn = 0U;
uint8_t bus_address;
uint8_t	dev_address_length = 1;
uint16_t device_address;
int8_t xBytes = 0;
uint8_t i2c_error_code;

	frtos_ioctl( fdI2C,ioctl_OBTAIN_BUS_SEMPH, NULL);

	// 1) Indicamos el periferico i2c en el cual queremos leer ( variable de 8 bits !!! )
	bus_address = i2c_bus_address;
	frtos_ioctl(fdI2C,ioctl_I2C_SET_DEVADDRESS, & bus_address);

	// 2) Luego indicamos la direccion desde donde leer:
	//    Largo: 1 byte indica el largo. El FRTOS espera 1 byte.
	if ( i2c_bus_address == BUSADDR_EEPROM_M2402 ) {
		dev_address_length = 2;	// Las direccione de la EEprom son de 16 bits
	} else {
		dev_address_length = 1;
	}
	frtos_ioctl(fdI2C,ioctl_I2C_SET_BYTEADDRESSLENGTH, &dev_address_length);
	// 	Direccion: El FRTOS espera siempre 2 bytes.
	//  Hago un cast para dejarla en 16 bits.
	device_address = (uint16_t)(wrAddress);
	frtos_ioctl(fdI2C,ioctl_I2C_SET_BYTEADDRESS,&device_address);

	//  3) Por ultimo escribimos. No controlo fronteras.
	xBytes = length;
	xReturn = frtos_write(fdI2C, data, xBytes);
//	memset(buffer,'\0', 10);
//	strcpy_P(buffer, (PGM_P)pgm_read_word(&(I2C_names[pv_i2_addr_2_idx( i2c_bus_address )])));
//	xprintf_P(PSTR("I2C WR 0x0%X, %s.\r\n\0"), i2c_bus_address, buffer );

	i2c_error_code = frtos_ioctl(fdI2C, ioctl_I2C_GET_LAST_ERROR, NULL );
	if (i2c_error_code != I2C_OK ) {
		memset(buffer,'\0', 10);
		strcpy_P(buffer, (PGM_P)pgm_read_word(&(I2C_names[pv_i2_addr_2_idx( i2c_bus_address )])));
		xprintf_P(PSTR("ERROR: I2C WR err 0x0%X, %s.\r\n\0"), i2c_bus_address, buffer );
	}

	if (xReturn != xBytes ) {
		xReturn = -1 ;
	}

	frtos_ioctl(fdI2C,ioctl_RELEASE_BUS_SEMPH, NULL);
	return(xReturn);

}
//------------------------------------------------------------------------------------
bool I2C_test_device( uint8_t i2c_bus_address, uint32_t rdAddress, char *data, uint8_t length )
{
	// Es lo mismo que I2C_read pero como se usa para ver cual dispositivo esta en el bus
	// y asi determinar cual placa IO tenemos, no muestra errores.


size_t xReturn = 0U;
uint8_t bus_address;
uint8_t	dev_address_length = 1;
uint16_t device_address;
int8_t xBytes = 0;
uint8_t i2c_error_code;
bool retS = true;

	frtos_ioctl( fdI2C,ioctl_OBTAIN_BUS_SEMPH, NULL);

	// 1) Indicamos el periferico i2c en el cual queremos leer ( variable de 8 bits !!! )
	bus_address = i2c_bus_address;
	frtos_ioctl(fdI2C,ioctl_I2C_SET_DEVADDRESS, &bus_address);

	// 2) Luego indicamos la direccion desde donde leer:
	//    Largo: 1 byte indica el largo. El FRTOS espera 1 byte.
	if ( i2c_bus_address == BUSADDR_EEPROM_M2402 ) {
		dev_address_length = 2;	// Las direccione de la EEprom son de 16 bits
	} else {
		dev_address_length = 1;
	}
	frtos_ioctl(fdI2C,ioctl_I2C_SET_BYTEADDRESSLENGTH, &dev_address_length);
	// 	Direccion: El FRTOS espera siempre 2 bytes.
	//  Hago un cast para dejarla en 16 bits.
	device_address = (uint16_t)(rdAddress);
	frtos_ioctl(fdI2C,ioctl_I2C_SET_BYTEADDRESS,&device_address);

	//  3) Por ultimo leemos. No controlo fronteras.
	xBytes = length;
	xReturn = frtos_read(fdI2C, data, xBytes);
//	memset(buffer,'\0', 10);
//	strcpy_P(buffer, (PGM_P)pgm_read_word(&(I2C_names[pv_i2_addr_2_idx( i2c_bus_address )])));
//	xprintf_P(PSTR("I2C TST 0x0%X, %s.\r\n\0"), i2c_bus_address, buffer );

	i2c_error_code = frtos_ioctl(fdI2C, ioctl_I2C_GET_LAST_ERROR, NULL );
	if (i2c_error_code != I2C_OK ) {
//		memset(buffer,'\0', 10);
//		strcpy_P(buffer, (PGM_P)pgm_read_word(&(I2C_names[pv_i2_addr_2_idx( i2c_bus_address )])));
//		xprintf_P(PSTR("ERROR: I2C Test err 0x0%X, %s.\r\n\0"), i2c_bus_address, buffer );
	}

	if (xReturn != xBytes ) {
		xReturn = -1;
		retS = false;
	}

	frtos_ioctl( fdI2C,ioctl_RELEASE_BUS_SEMPH, NULL);
	return(retS);

}
//------------------------------------------------------------------------------------
uint8_t pv_i2_addr_2_idx( uint8_t i2c_bus_address )
{
	switch( i2c_bus_address ) {
	case BUSADDR_EEPROM_M2402:
		return(1);
		break;
	case BUSADDR_RTC_M79410:
		return(2);
		break;
	case BUSADDR_MCP23018:
		return(3);
		break;
	case BUSADDR_INA_A:
		return(4);
		break;
	case BUSADDR_INA_B:
		return(5);
		break;
	case BUSADDR_INA_C:
		return(6);
		break;
	default:
		return(0);
	}

	return(0);

}
//------------------------------------------------------------------------------------
