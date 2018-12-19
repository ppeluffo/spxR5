/*
 * spx_tkDinputs.c
 *
 *  Created on: 17 dic. 2018
 *      Author: pablo
 */

#include "spx.h"

uint16_t digital_inputs[MAX_DINPUTS_CHANNELS];

static void pv_tkDinputs_init(void);

#define WDG_DIN_TIMEOUT	30

//------------------------------------------------------------------------------------
void tkDinputs(void * pvParameters)
{
	// C/100ms leo las entradas digitales.
	// Las que estan en 1 sumo 1 al contador.
	// En UTE dividimos las entradas digitales en 2: aquellas que solo miden nivel logico (0..3)
	// y las que miden el tiempo que estan en 1 ( 4..7)
	// En modo SPX solo mido el nivel logico

( void ) pvParameters;
uint32_t waiting_ticks;
TickType_t xLastWakeTime;
uint8_t din_port;
uint8_t i;

	// Espero la notificacion para arrancar
	while ( !startTask )
		vTaskDelay( ( TickType_t)( 100 / portTICK_RATE_MS ) );

	xprintf_P( PSTR("starting tkDigital..\r\n\0"));

	pv_tkDinputs_init();

	// Initialise the xLastWakeTime variable with the current time.
	xLastWakeTime = xTaskGetTickCount();

	// Cuento de a 100ms
	waiting_ticks = (uint32_t)(100) / portTICK_RATE_MS;

	// loop
	for( ;; )
	{

		ctl_watchdog_kick( WDG_DIN,  WDG_DIN_TIMEOUT );

		if ( ( spx_io_board == SPX_IO8CH ) && systemVars.dinputs_timers ) {

			// Cada 100 ms leo las entradas digitales. fmax=10Hz
			// vTaskDelay( ( TickType_t)( 100 / portTICK_RATE_MS ) );
			// Hago de cuenta que todos son timers pero al final solo paso el valor de
			// los DIN4..DIN7

			vTaskDelayUntil( &xLastWakeTime, waiting_ticks );

			din_port = DIN_read_port();
			for ( i = 0 ; i < NRO_DINPUTS; i++ ) {
				// Si esta HIGH incremento en 1 tick.
				if ( ( (din_port & ( 1 << i )) >> i ) == 1 ) {
					digital_inputs[i]++;	// Si esta HIGH incremento en 1 tick.
				}
			}

		} else {

			// Espero para entrar en tickless
			vTaskDelay( ( TickType_t)( 5000 / portTICK_RATE_MS ) );
		}
	}
}
//------------------------------------------------------------------------------------
static void pv_tkDinputs_init(void)
{

uint8_t i;

	for ( i = 0; i < MAX_DINPUTS_CHANNELS; i++ ) {
		digital_inputs[i] = 0;
	}
}
//------------------------------------------------------------------------------------
// FUNCIONES PUBLICAS
//------------------------------------------------------------------------------------
void dinputs_read_frame( st_dataRecord_t *drcd )
{

uint8_t port;
uint8_t i = 0;

	// Esta funcion la invoca tkData al completar un frame para agregar los datos
	// digitales.
	// Leo los niveles de las entradas digitales y copio a dframe.

	switch (spx_io_board ) {
	case SPX_IO5CH:
		// Leo los canales digitales.
		drcd->df.io5.dinputs[0] = DIN_read_pin(0, spx_io_board );
		drcd->df.io5.dinputs[1] = DIN_read_pin(1, spx_io_board );
		break;
	case SPX_IO8CH:
		port = DIN_read_port();
		drcd->df.io8.dinputs[0] = ( port & ( 1 << 0 )) >> 0;
		drcd->df.io8.dinputs[1] = ( port & ( 1 << 1 )) >> 1;
		drcd->df.io8.dinputs[2] = ( port & ( 1 << 2 )) >> 2;
		drcd->df.io8.dinputs[3] = ( port & ( 1 << 3 )) >> 3;
		drcd->df.io8.dinputs[4] = ( port & ( 1 << 4 )) >> 4;
		drcd->df.io8.dinputs[5] = ( port & ( 1 << 5 )) >> 5;
		drcd->df.io8.dinputs[6] = ( port & ( 1 << 6 )) >> 6;
		drcd->df.io8.dinputs[7] = ( port & ( 1 << 7 )) >> 7;

		// Si tengo los timers digitales habilitados, reemplazo los ultimos 4 valores.
		if ( systemVars.dinputs_timers ) {

			// D4..D7 son contadores de tiempo High.
			// Puede haber un drift en el timerpoll lo que generaria un falso error
			// Para evitarlo, controlo que los ticks no puedan ser mayor que el timerpoll
			if ( digital_inputs[i] > systemVars.timerPoll * 10 ) {
				// Corrijo
				digital_inputs[i] = systemVars.timerPoll * 10;
			}

			drcd->df.io8.dinputs[4] = digital_inputs[4];
			drcd->df.io8.dinputs[5] = digital_inputs[5];
			drcd->df.io8.dinputs[6] = digital_inputs[6];
			drcd->df.io8.dinputs[7] = digital_inputs[7];

			pv_tkDinputs_init();
		}
		break;
	}

}
//------------------------------------------------------------------------------------
