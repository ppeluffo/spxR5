/*
 * spx_tkDinputs.c
 *
 *  Created on: 17 dic. 2018
 *      Author: pablo
 *
 *  Esta tarea atiende las entradas digitales.
 *  Dado que en las placas SPXIO8 las entradas superiores (4..7) pueden operar
 *  como entradas de nivel o timers, es necesario una tarea solo para este ultimo
 *  caso en particular ya que debe leer el nivel en forma sincrónica cada 100ms y contar
 *  los ticks que esta la linea alta.
 *  Si no fuese por esto, no necesitaríamos implementar una tarea.
 *
 */

#include "spx.h"

static uint16_t dtimers[MAX_DTIMERS_CHANNELS];	// Valor de las medidas

static void pv_tkDtimers_init(void);

#define WDG_DTIM_TIMEOUT	30

//------------------------------------------------------------------------------------
void tkDtimers(void * pvParameters)
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

	xprintf_P( PSTR("starting tkDtimers..\r\n\0"));

	pv_tkDtimers_init();

	// Initialise the xLastWakeTime variable with the current time.
	xLastWakeTime = xTaskGetTickCount();

	// Cuento de a 100ms
	waiting_ticks = (uint32_t)(100) / portTICK_RATE_MS;

	// loop
	for( ;; )
	{

		ctl_watchdog_kick( WDG_DTIM,  WDG_DTIM_TIMEOUT );

		// Solo en la placa IO8 y con los timers habilitados
		if ( ( spx_io_board == SPX_IO8CH ) &&  ( systemVars.dtimers_enabled ) ) {

			// Cada 100 ms leo las entradas digitales. fmax=10Hz
			// vTaskDelay( ( TickType_t)( 100 / portTICK_RATE_MS ) );
			// Hago de cuenta que todos son timers pero al final solo paso el valor de
			// los DIN4..DIN7

			vTaskDelayUntil( &xLastWakeTime, waiting_ticks );

			din_port = DIN_read_port();
			din_port = din_port << 4;
			for ( i = 0 ; i < IO8_DTIMERS_CHANNELS; i++ ) {
				// Si esta HIGH incremento en 1 tick.
				if ( ( (din_port & ( 1 << i )) >> i ) == 1 ) {
					dtimers[i]++;	// Si esta HIGH incremento en 1 tick.
				}
			}

		} else {

			// En modo SPX_5CH no monitoreo las entradas digitales sino que las leo al fin del timerpoll ( niveles )
			// Espero para entrar en tickless
			vTaskDelay( ( TickType_t)( 25000 / portTICK_RATE_MS ) );
		}
	}
}
//------------------------------------------------------------------------------------
static void pv_tkDtimers_init(void)
{

uint8_t i;

	// Dado que los pines son diferentes para c/board, la inicializacion
	// depende de la board detectada.
	DINPUTS_init( spx_io_board );

	for ( i = 0; i < MAX_DTIMERS_CHANNELS; i++ ) {
		dtimers[i] = 0;
	}
}
//------------------------------------------------------------------------------------
// FUNCIONES PUBLICAS
//------------------------------------------------------------------------------------
int16_t dtimers_read ( uint8_t din )
{

int16_t retVal = -1;

	// Esta funcion la invoca tkData al completar un frame para agregar los datos
	// digitales.
	// Leo los niveles de las entradas digitales y copio a dframe.

	if ( spx_io_board == SPX_IO8CH ) {
		if ( dtimers[din] > systemVars.timerPoll * 10 ) {
			// Corrijo
			dtimers[din] = systemVars.timerPoll * 10;
		}
		retVal = dtimers[din];
		dtimers[din] = 0;
	}

	return(retVal);

}
//------------------------------------------------------------------------------------
bool dtimers_config_timermode ( char *s_mode )
{

	// Esta opcion es solo valida para IO8
	if ( spx_io_board != SPX_IO8CH )
		return(false);

	if ( !strcmp_P( strupr(s_mode), PSTR("ON\0"))) {
		systemVars.dtimers_enabled = true;
		return(true);
	} else if ( !strcmp_P( strupr(s_mode), PSTR("OFF\0"))) {
		systemVars.dtimers_enabled = false;
		return(true);
	}
	return(false);

}
//------------------------------------------------------------------------------------
void dtimers_config_defaults(void)
{

	systemVars.dtimers_enabled = false;

}
//------------------------------------------------------------------------------------
