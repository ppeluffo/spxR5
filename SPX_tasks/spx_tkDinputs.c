/*
 * spx_tkDigital.c
 *
 *  Created on: 12 ago. 2019
 *      Author: pablo
 *
 *  Se utiliza para controlar las entradas digitales.
 *  C/entrada tiene un parametro tpoll que es el tiempo cada cuanto hay que
 *  polearla.
 *  Si ambos son 0, entonces la tarea duerme 25s.
 *  C/vez que se despierta ve el valor de las entradas digitales e incrementa
 *  en 1 su valor.
 *
 */

#include "spx.h"

// La tarea pasa por el mismo lugar en el caso mas largo cada
#define WDG_DIN_TIMEOUT	60

uint16_t din0, din1;

uint16_t pv_time_to_poll(void);

//------------------------------------------------------------------------------------
void tkDinputs(void * pvParameters)
{

( void ) pvParameters;
uint16_t waiting_time;

	// Espero la notificacion para arrancar
	while ( !startTask )
		vTaskDelay( ( TickType_t)( 100 / portTICK_RATE_MS ) );

	dinputs_init();
	din0 = 0;
	din1 = 0;

	xprintf_P( PSTR("starting tkDigital..\r\n\0"));

	// loop
	for( ;; )
	{

		ctl_watchdog_kick(WDG_DINPUTS , WDG_DIN_TIMEOUT);
		// En IO8 no poleo
		if ( spx_io_board == SPX_IO8CH ) {
			vTaskDelay( ( TickType_t)( 25000 / portTICK_RATE_MS ) );
			continue;
		}

		// Espero. Da el tiempo necesario para entrar en tickless.
		waiting_time = pv_time_to_poll();
		if ( waiting_time > 1000 )
			waiting_time = 1000;

		if (  waiting_time == 0 ) {
			// No usamos la funcionalidad de poleo
			vTaskDelay( ( TickType_t)( 25000 / portTICK_RATE_MS ) );
		} else {
			// Espero
			vTaskDelay( ( TickType_t)( waiting_time / portTICK_RATE_MS ) );
			// Y poleo ( tienen pull-ups !!)
			if (dinputs_read_channel(0) == 0 )
				din0 += 1;
			if (dinputs_read_channel(1) == 0 )
				din1 += 1;

		}
	}

}
//------------------------------------------------------------------------------------
// FUNCIONES PRIVADAS
//------------------------------------------------------------------------------------
uint16_t pv_time_to_poll(void)
{
uint16_t pt;

	if ( systemVars.dinputs_conf.tpoll[0] == 0 ) {
		pt = systemVars.dinputs_conf.tpoll[1];
		//xprintf_P( PSTR("DEBUG TIMETOPOLL = %d(1)\r\n\0"), pt);
		return(pt);
	}

	if ( systemVars.dinputs_conf.tpoll[1] == 0 ) {
		pt = systemVars.dinputs_conf.tpoll[0];
		//xprintf_P( PSTR("DEBUG TIMETOPOLL = %d(0)\r\n\0"), pt);
		return(pt);
	}

	pt = min( systemVars.dinputs_conf.tpoll[0], systemVars.dinputs_conf.tpoll[1]);
	//xprintf_P( PSTR("DEBUG TIMETOPOLL = %d(min)\r\n\0"), pt);
	return(pt);
}
//------------------------------------------------------------------------------------
// FUNCIONES PUBLICAS
//------------------------------------------------------------------------------------
void dinputs_read_din( uint16_t *d0, uint16_t *d1 )
{

	if ( pv_time_to_poll() == 0 ) {
		// No estoy en modo poleo. Retorno el valor del nivel
		*d0 = dinputs_read_channel(0);
		*d1 = dinputs_read_channel(1);
		//xprintf_P( PSTR("DEBUG READ DIN tpll=0\r\n\0"));
		return;

	}

	*d0 = din0;
	din0 = 0;

	*d1 = din1;
	din1 = 0;
}
//------------------------------------------------------------------------------------

