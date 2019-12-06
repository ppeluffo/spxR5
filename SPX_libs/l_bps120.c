/*
 * l_psensor.c
 *
 *  Created on: 23 ago. 2019
 *      Author: pablo
 */



#include "l_i2c.h"
#include "l_bps120.h"


//------------------------------------------------------------------------------------
int8_t __bps120_raw_read( float *presion )
{

		// EL sensor BOURNS bps120 mide presion diferencial.
		// Por lo tanto retorna PRESION de acuerdo a la funcion de transferencia
		// con la pMax la de la hoja de datos.
		// De acuerdo a la hoja de datos, la funcion de transferencia es:
		// p(psi) = (pmax - pmin ) * ( counts - 0.1Max) / ( 0.8Max) + pmin
		// pmin = 0
		// Max = 16384 ( 14 bits )
		// 0.1xMax = 1638
		// 0.8xMax = 13107
		// counts es el valor leido del sensor.
		// PMAX = 1.0 psi
		// PMIN = 0 psi

int8_t rcode = 0;
uint8_t times = 3;
char buffer[2] = { 0 };
int16_t pcounts;

	while ( times-- > 0 ) {

		// Leo 2 bytes del sensor de presion.
		// El resultado es de 14 bits.
		rcode =  I2C_read( BUSADDR_BPS120, 0, &buffer[0], 0x02 );

		if ( rcode == -1 ) {
			// Hubo error: trato de reparar el bus y reintentar la operacion
			// Espero 1s que se termine la fuente de ruido.
			vTaskDelay( ( TickType_t)( 1000 / portTICK_RATE_MS ) );
			// Reconfiguro los dispositivos I2C del bus que pueden haberse afectado
			xprintf_P(PSTR("ERROR: BPS120_raw_read recovering i2c bus (%d)\r\n\0"), times );
			I2C_reinit_devices();
		} else {
			// No hubo error: salgo normalmente
			pcounts = ( buffer[0]<<8 ) + buffer[1];
			// Aplico la funcion de transferencia.
			*presion = ( 1 * (pcounts - 1638)/13107 );
			//xprintf_P(PSTR("BPS120: [0x%X][0x%X]\r\n\0"), buffer[0], buffer[1] );
			//xprintf_P(PSTR("BPS120: pcounts=%d\r\n\0"), pcounts );
			//xprintf_P(PSTR("BPS120: presion=%.02f\r\n\0"), *presion );
			break;
		}
	}

	return( rcode );
}
//------------------------------------------------------------------------------------
int8_t bps120_raw_read( char *data )
{
	// EL sensor BOURNS bps120 mide presion diferencial.
	// Por lo tanto retorna PRESION de acuerdo a la funcion de transferencia
	// con la pMax la de la hoja de datos.
	// De acuerdo a la hoja de datos, la funcion de transferencia es:
	// p(psi) = (pmax - pmin ) * ( counts - 0.1Max) / ( 0.8Max) + pmin
	// pmin = 0
	// Max = 16384 ( 14 bits )
	// 0.1xMax = 1638
	// 0.8xMax = 13107
	// counts es el valor leido del sensor.
	// PMAX = 1.0 psi
	// PMIN = 0 psi

int8_t rcode = 0;
uint8_t times = 3;


	while ( times-- > 0 ) {

		// Leo 2 bytes del sensor de presion.
		rcode =  I2C_read( BUSADDR_BPS120, 0, data, 0x02 );

		if ( rcode == -1 ) {
			// Hubo error: trato de reparar el bus y reintentar la operacion
			// Espero 1s que se termine la fuente de ruido.
			vTaskDelay( ( TickType_t)( 1000 / portTICK_RATE_MS ) );
			// Reconfiguro los dispositivos I2C del bus que pueden haberse afectado
			xprintf_P(PSTR("ERROR: BPS120_raw_read recovering i2c bus (%d)\r\n\0"), times );
			I2C_reinit_devices();
		} else {
			// No hubo error: salgo normalmente
			break;
		}
	}
	return( rcode );
}
//------------------------------------------------------------------------------------

