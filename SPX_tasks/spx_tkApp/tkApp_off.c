/*
 * tkApp_off.c
 *
 *  Created on: 12 mar. 2020
 *      Author: pablo
 */

#include "spx.h"
#include "tkApp.h"

//------------------------------------------------------------------------------------

void tkApp_off(void)
{
	// Cuando no hay una funcion especifica habilidada en el datalogger
	// ( solo monitoreo ), debemos dormir para que pueda entrar en
	// tickless

	xprintf_PD(DF_APP, PSTR("APP: Off\r\n\0"));

	// Borro los SMS de alarmas pendientes
	xSMS_init();

	for( ;; )
	{
		ctl_watchdog_kick( WDG_APP,  WDG_APP_TIMEOUT );
		vTaskDelay( ( TickType_t)( 25000 / portTICK_RATE_MS ) );
	}
}
//------------------------------------------------------------------------------------
uint8_t xAPP_off_hash(void)
{

uint8_t hash = 0;
char dst[32];
char *p;
uint8_t i = 0;

	// Vacio el buffer temoral
	memset(dst,'\0', sizeof(dst));

	i = snprintf_P( &dst[i], sizeof(dst), PSTR("OFF"));
	//xprintf_P( PSTR("DEBUG: CONS = [%s]\r\n\0"), dst );
	// Apunto al comienzo para recorrer el buffer
	p = dst;
	while (*p != '\0') {
		//checksum += *p++;
		hash = u_hash(hash, *p++);
	}
	return(hash);

}
//------------------------------------------------------------------------------------


