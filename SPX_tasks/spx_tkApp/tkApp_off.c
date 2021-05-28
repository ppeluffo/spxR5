/*
 * tkApp_off.c
 *
 *  Created on: 12 mar. 2020
 *      Author: pablo
 */

#include "spx.h"
#include "tkApp.h"
#include "l_steppers.h"

//------------------------------------------------------------------------------------

void tkApp_off(void)
{
	// Cuando no hay una funcion especifica habilidada en el datalogger
	// ( solo monitoreo ), debemos dormir para que pueda entrar en
	// tickless

	xprintf_P(PSTR("APP: Off\r\n\0"));

	stepper_init();

	// Borro los SMS de alarmas pendientes
	xSMS_init();


	for( ;; )
	{
		ctl_watchdog_kick( WDG_APP,  WDG_APP_TIMEOUT );
		vTaskDelay( ( TickType_t)( 25000 / portTICK_RATE_MS ) );

		// Corren todas las rutinas que quiero que lo hagan tambien en app=off !!!

		// Test de presion
		if ( spiloto.start_presion_test ) {
			run_piloto_presion_test();
			spiloto.start_presion_test = false;
		}

		// Test de movimiento de stepper
		if ( spiloto.start_steppers_test ) {
			run_piloto_stepper_test();
			spiloto.start_steppers_test = false;
		}

	}
}
//------------------------------------------------------------------------------------
uint8_t xAPP_off_hash(void)
{

uint8_t hash = 0;
//char dst[32];
char *p;
uint8_t i = 0;
int16_t free_size = sizeof(hash_buffer);

	// Vacio el buffer temoral
	memset(hash_buffer,'\0', sizeof(hash_buffer));

	i += snprintf_P( &hash_buffer[i], free_size, PSTR("OFF"));
	free_size = (  sizeof(hash_buffer) - i );
	if ( free_size < 0 ) goto exit_error;

	//xprintf_P( PSTR("DEBUG: CONS = [%s]\r\n\0"), hash_buffer );
	// Apunto al comienzo para recorrer el buffer
	p = hash_buffer;
	while (*p != '\0') {
		//checksum += *p++;
		hash = u_hash(hash, *p++);
	}
	//xprintf_P( PSTR("COMMS: app_hash OK[%d]\r\n\0"),free_size);
	return(hash);

exit_error:
	xprintf_P( PSTR("COMMS: off_hash ERROR !!!\r\n\0"));
	return(0x00);
}
//------------------------------------------------------------------------------------


