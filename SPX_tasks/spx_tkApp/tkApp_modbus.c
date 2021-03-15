/*
 * tkApp_modbus.c
 *
 *  Created on: 5 mar. 2021
 *      Author: pablo
 */
#include "spx.h"
#include "tkApp.h"
#include "l_steppers.h"

//------------------------------------------------------------------------------------
void tkApp_modbus(void)
{
	// Cuando no hay una funcion especifica habilidada en el datalogger
	// ( solo monitoreo ), debemos dormir para que pueda entrar en
	// tickless

	xprintf_P(PSTR("APP: Modbus\r\n\0"));

	for( ;; )
	{
		ctl_watchdog_kick( WDG_APP,  WDG_APP_TIMEOUT );
		vTaskDelay( ( TickType_t)( 25000 / portTICK_RATE_MS ) );
	}
}
//------------------------------------------------------------------------------------
uint8_t xAPP_modbus_hash(void)
{

uint8_t hash = 0;
char *p;
uint8_t i,j;
int16_t free_size = sizeof(hash_buffer);

	// Vacio el buffer temoral
	memset(hash_buffer,'\0', sizeof(hash_buffer));

	j = 0;
	j += snprintf_P( &hash_buffer[j], free_size, PSTR("MODBUS;SLA:%04d;"), systemVars.modbus_conf.modbus_slave_address );
	//xprintf_P( PSTR("DEBUG_MBHASH = [%s]\r\n\0"), hash_buffer );
	free_size = (  sizeof(hash_buffer) - j );
	if ( free_size < 0 ) goto exit_error;

	p = hash_buffer;
	while (*p != '\0') {
		//checksum += *p++;
		hash = u_hash(hash, *p++);
	}

	for (i=0; i < MODBUS_CHANNELS; i++ ) {
		// Vacio el buffer temoral
		memset(hash_buffer,'\0', sizeof(hash_buffer));
		free_size = sizeof(hash_buffer);
		// Copio sobe el buffer una vista ascii ( imprimible ) de c/registro.
		j = snprintf_P( hash_buffer, free_size, PSTR("M%d:%s,%04d,%02d,%02d,%c;"), i,
				systemVars.modbus_conf.mbchannel[i].name,
				systemVars.modbus_conf.mbchannel[i].address,
				systemVars.modbus_conf.mbchannel[i].length,
				systemVars.modbus_conf.mbchannel[i].function_code,
				systemVars.modbus_conf.mbchannel[i].type
				);
		free_size = (  sizeof(hash_buffer) - j );
		if ( free_size < 0 ) goto exit_error;
		//xprintf_P( PSTR("DEBUG_MBHASH = [%s]\r\n\0"), hash_buffer );
		// Apunto al comienzo para recorrer el buffer
		p = hash_buffer;
		while (*p != '\0') {
			hash = u_hash(hash, *p++);
		}

	}
	return(hash);

exit_error:

	xprintf_P( PSTR("COMMS: modbus_hash ERROR !!!\r\n\0"));
	return(0x00);
}
//------------------------------------------------------------------------------------




