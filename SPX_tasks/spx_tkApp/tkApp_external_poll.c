/*
 * tkApp_external_poll.c
 *
 *  Created on: 26 oct. 2020
 *      Author: pablo
 */
#include "tkApp.h"

#define SLEEP_TIME_MS		1000
#define DEBOUNCE_TIME_MA	10000
//------------------------------------------------------------------------------------
void tkApp_external_poll(void)
{
	// Leo la entrada de control D1 c/1s.
	// Si está activa mando una senal a tkInputs y duermo 10s.

	xprintf_P(PSTR("APP: EXTERNAL POLL start\r\n\0"));

	for (;;) {

		ctl_watchdog_kick( WDG_APP,  WDG_APP_TIMEOUT );

		vTaskDelay( ( TickType_t)( SLEEP_TIME_MS / portTICK_RATE_MS ) );

		if ( IO_read_PA0() == 1 ) {
			xprintf_PD(DF_APP,"DEBUG EXTPOLL\r\n\0");
			SPX_SEND_SIGNAL( SGN_POLL_NOW );
			vTaskDelay( ( TickType_t)( DEBOUNCE_TIME_MA / portTICK_RATE_MS ) );
		}
	}
}
//------------------------------------------------------------------------------------
uint8_t xAPP_external_poll_hash(void)
{

uint8_t hash = 0;
//char dst[32];
char *p;
uint8_t i = 0;
int16_t free_size = sizeof(hash_buffer);

	// Vacio el buffer temoral
	memset(hash_buffer,'\0', sizeof(hash_buffer));

	i += snprintf_P( &hash_buffer[i], free_size, PSTR("EXTPOLL"));
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
	xprintf_P( PSTR("COMMS: app_hash ERROR !!!\r\n\0"));
	return(0x00);
}
//------------------------------------------------------------------------------------



