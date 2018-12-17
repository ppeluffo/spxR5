/*
 * spx_counters.c
 *
 *
 */

#include "spx.h"

static float counters[MAX_COUNTER_CHANNELS];
static void pv_tkCounter_init(void);

// La tarea puede estar hasta 10s en standby
#define WDG_COUNT_TIMEOUT	30

//------------------------------------------------------------------------------------
void tkCounter(void * pvParameters)
{

( void ) pvParameters;
uint32_t ulNotificationValue;
const TickType_t xMaxBlockTime = pdMS_TO_TICKS( 10000 );

	// Espero la notificacion para arrancar
	while ( !startTask )
		vTaskDelay( ( TickType_t)( 100 / portTICK_RATE_MS ) );

	pv_tkCounter_init();

	xprintf_P( PSTR("starting tkCounter..\r\n\0"));

	// loop
	for( ;; )
	{

		// Paso c/10s plt 30s es suficiente.
		ctl_watchdog_kick(WDG_COUNT, WDG_COUNT_TIMEOUT);

		// Cuando la interrupcion detecta un flanco, solo envia una notificacion
		// Espero que me avisen. Si no me avisaron en 10s salgo y repito el ciclo.
		// Esto es lo que me permite entrar en tickless.
		ulNotificationValue = ulTaskNotifyTake( pdTRUE, xMaxBlockTime );

		if( ulNotificationValue != 0 ) {
			// Fui notificado: llego algun flanco que determino.

			if ( cTask_wakeup_for_C0() ) {
				counters[0]++;
				reset_wakeup_for_C0();
			}

			if ( cTask_wakeup_for_C1() ) {
				counters[1]++;
				reset_wakeup_for_C1();
			}

			if ( systemVars.debug == DEBUG_COUNTER) {
				xprintf_P( PSTR("COUNTERS: C0=%d,C1=%d\r\n\0"),(uint16_t) counters[0], (uint16_t) counters[1]);
			}

			// Espero el tiempo de debounced
			vTaskDelay( ( TickType_t)( systemVars.counter_debounce_time / portTICK_RATE_MS ) );

			CNT_clr_CLRD();		// Borro el latch llevandolo a 0.
			CNT_set_CLRD();		// Lo dejo en reposo en 1

		} else   {
			// Expiro el timeout de la tarea. Por ahora no hago nada.
		}
	}
}
//------------------------------------------------------------------------------------
static void pv_tkCounter_init(void)
{
	// Configuracion inicial de la tarea

uint8_t i;

	COUNTERS_init( xHandle_tkCounter );

	for ( i = 0; i < NRO_COUNTERS; i++) {
		counters[ i ] = 0;
	}

}
//------------------------------------------------------------------------------------
// FUNCIONES PUBLICAS
//------------------------------------------------------------------------------------
void counters_read_frame( st_dataRecord_t *drcd, bool reset_counters )
{

	// Esta funcion la invoca tkData al completar un frame para agregar los datos
	// digitales.
	// Leo los niveles de las entradas digitales y copio a dframe.
	// Respecto de los contadores, no leemos pulsos sino magnitudes por eso antes lo
	// convertimos a la magnitud correspondiente.

uint8_t i;

	// Convierto los contadores a las magnitudes (todos, por ahora no importa cuales son contadores )
	// Siempre multiplico por magPP. Si quiero que sea en mt3/h, en el server debo hacerlo (  * 3600 / systemVars.timerPoll )

	for (i = 0; i < NRO_COUNTERS; i++) {
		switch (spx_io_board ) {
		case SPX_IO5CH:
			drcd->df.io5.counters[i] = counters[i] * systemVars.counters_magpp[i];
			if ( reset_counters )
				counters[i] = 0.0;
			break;
		case SPX_IO8CH:
			drcd->df.io8.counters[i] = counters[i] * systemVars.counters_magpp[i];
			if ( reset_counters )
				counters[i] = 0.0;
			break;
		}
	}
}
//------------------------------------------------------------------------------------


