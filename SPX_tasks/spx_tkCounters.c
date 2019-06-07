/*
 * spx_counters.c
 *
 * Esta tarea se encarga de contar los pulsos en cualquiera de las entradas
 * y cuando se lo solicitan, los devuelve y/o pone a 0 los contadores.
 * Tanto la placa IO5 como la IO8 tienen el mismo sistema de contadores por lo
 * que la tarea no hace diferencia de la placa detectada.
 * Inicialmente configura las interrupciones y los pines utilizando los servicios
 * de la librería l_counters.
 * Los pines los configura con pull_down para reducir los consumos al minimo
 * La tarea esta durmiendo y se despierta por una interrupcion de uno de los pines
 * atacheados a la interrupcion.
 * En este caso cuenta un pulso y vuelve a dormir luego de un tiempo de debounced que
 * es configurable.
 *
 *
 */

#include "spx.h"


bool counters_enabled[MAX_COUNTER_CHANNELS];
uint16_t counters[MAX_COUNTER_CHANNELS];	// Valores medidos de los contadores

// La tarea puede estar hasta 10s en standby
#define WDG_COUNT_TIMEOUT	60

static void pv_tkCounter_init(uint8_t cnt);

//------------------------------------------------------------------------------------
void tkCounter0(void * pvParameters)
{

( void ) pvParameters;
uint32_t ulNotificationValue;
const TickType_t xMaxBlockTime = pdMS_TO_TICKS( 25000 );

	// Espero la notificacion para arrancar
	while ( !startTask )
		vTaskDelay( ( TickType_t)( 100 / portTICK_RATE_MS ) );

	pv_tkCounter_init(0);

	xprintf_P( PSTR("starting tkCounter0..\r\n\0"));

	// loop
	for( ;; )
	{

		// Paso c/10s plt 30s es suficiente.
		ctl_watchdog_kick(WDG_COUNT0, WDG_COUNT_TIMEOUT);

		// Cuando la interrupcion detecta un flanco, solo envia una notificacion
		// Espero que me avisen. Si no me avisaron en 25s salgo y repito el ciclo.
		// Esto es lo que me permite entrar en tickless.
		ulNotificationValue = ulTaskNotifyTake( pdTRUE, xMaxBlockTime );

		if( ulNotificationValue != 0 ) {
			// Fui notificado:
			COUNTERS_disable_interrupt(0);

			// Espero el ancho de pulso para ver si es valido
			vTaskDelay( ( TickType_t)( systemVars.counters_conf.pwidth[0] / portTICK_RATE_MS ) );
			// Leo el pin. Si esta en 1 el ancho es valido y entonces cuento el pulso
			if ( CNT_read_CNT0() == 0 ) {
				counters[0]++;
				if ( systemVars.debug == DEBUG_COUNTER) {
					xprintf_P( PSTR("COUNTERS: *C0=%d, C1=%d\r\n\0"),(uint16_t) counters[0],(uint16_t) counters[1] );
				}
			}

			// Espero perido
			vTaskDelay( ( TickType_t)( ( systemVars.counters_conf.period[0] - systemVars.counters_conf.pwidth[0] ) / portTICK_RATE_MS ) );
			if ( counters_enabled[0] )
				COUNTERS_enable_interrupt(0);

		} else   {
			// Expiro el timeout de la tarea. Por ahora no hago nada.
		}
	}
}
//------------------------------------------------------------------------------------
void tkCounter1(void * pvParameters)
{

( void ) pvParameters;
uint32_t ulNotificationValue;
const TickType_t xMaxBlockTime = pdMS_TO_TICKS( 25000 );

	// Espero la notificacion para arrancar
	while ( !startTask )
		vTaskDelay( ( TickType_t)( 100 / portTICK_RATE_MS ) );

	pv_tkCounter_init(1);

	xprintf_P( PSTR("starting tkCounter1..\r\n\0"));

	// loop
	for( ;; )
	{

		// Paso c/10s plt 30s es suficiente.
		ctl_watchdog_kick(WDG_COUNT1, WDG_COUNT_TIMEOUT);

		// Cuando la interrupcion detecta un flanco, solo envia una notificacion
		// Espero que me avisen. Si no me avisaron en 25s salgo y repito el ciclo.
		// Esto es lo que me permite entrar en tickless.
		ulNotificationValue = ulTaskNotifyTake( pdTRUE, xMaxBlockTime );

		if( ulNotificationValue != 0 ) {
			// Fui notificado:
			COUNTERS_disable_interrupt(1);

			// Espero el ancho de pulso para ver si es valido
			vTaskDelay( ( TickType_t)( systemVars.counters_conf.pwidth[1] / portTICK_RATE_MS ) );
			// Leo el pin. Si esta en 1 el ancho es valido y entonces cuento el pulso
			if ( CNT_read_CNT1() == 0 ) {
				counters[1]++;
				if ( systemVars.debug == DEBUG_COUNTER) {
					xprintf_P( PSTR("COUNTERS: *C0=%d, C1=%d\r\n\0"),(uint16_t) counters[0],(uint16_t) counters[1] );
				}
			}

			// Espero perido
			vTaskDelay( ( TickType_t)( ( systemVars.counters_conf.period[1] - systemVars.counters_conf.pwidth[1] ) / portTICK_RATE_MS ) );
			if ( counters_enabled[1] )
				COUNTERS_enable_interrupt(1);


		} else   {
			// Expiro el timeout de la tarea. Por ahora no hago nada.
		}
	}
}
//------------------------------------------------------------------------------------
static void pv_tkCounter_init(uint8_t cnt)
{
	// Configuracion inicial de la tarea

	switch(cnt) {
	case 0:
		COUNTERS_init( 0, xHandle_tkCounter0 );
		break;
	case 1:
		COUNTERS_init( 1, xHandle_tkCounter1 );
		break;
	}

	counters[ cnt ] = 0;
	if (!strcmp_P( strupr(systemVars.counters_conf.name[cnt]), PSTR("X")) ) {
		counters_enabled[cnt] = false;
		COUNTERS_disable_interrupt(cnt);
	} else {
		counters_enabled[cnt] = true;
		COUNTERS_enable_interrupt(cnt);
	}
}
//------------------------------------------------------------------------------------
