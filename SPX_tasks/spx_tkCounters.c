/*
 * spx_counters.c
 *
 *
 */

#include "spx.h"

static st_counters_t cframe;
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
		pub_ctl_watchdog_kick(WDG_COUNT, WDG_COUNT_TIMEOUT);

		// Cuando la interrupcion detecta un flanco, solo envia una notificacion
		// Espero que me avisen. Si no me avisaron en 10s salgo y repito el ciclo.
		// Esto es lo que me permite entrar en tickless.
		ulNotificationValue = ulTaskNotifyTake( pdTRUE, xMaxBlockTime );

		if( ulNotificationValue != 0 ) {
			// Fui notificado: llego algun flanco que determino.

			if ( cTask_wakeup_for_C0() ) {
				cframe.counters[0]++;
				reset_wakeup_for_C0();
			}

			if ( cTask_wakeup_for_C1() ) {
				cframe.counters[1]++;
				reset_wakeup_for_C1();
			}

			if ( systemVars.debug == DEBUG_COUNTER) {
				xprintf_P( PSTR("COUNTERS: C0=%d,C1=%d\r\n\0"),(uint16_t) cframe.counters[0], (uint16_t) cframe.counters[1]);
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

uint8_t counter;

	COUNTERS_init( xHandle_tkCounter );

	for ( counter = 0; counter < NRO_COUNTER_CHANNELS; counter++) {
		cframe.counters[ counter ] = 0;
	}

}
//------------------------------------------------------------------------------------
// FUNCIONES PUBLICAS
//------------------------------------------------------------------------------------
void pub_counters_read_frame( st_counters_t dst_frame[], bool reset_counters )
{

	// Esta funcion la invoca tkData al completar un frame para agregar los datos
	// digitales.
	// Leo los niveles de las entradas digitales y copio a dframe.
	// Respecto de los contadores, no leemos pulsos sino magnitudes por eso antes lo
	// convertimos a la magnitud correspondiente.

uint8_t i;

	// Convierto los contadores a las magnitudes (todos, por ahora no importa cuales son contadores )
	// Siempre multiplico por magPP. Si quiero que sea en mt3/h, en el server debo hacerlo (  * 3600 / systemVars.timerPoll )
	for (i = 0; i < NRO_COUNTER_CHANNELS; i++) {
		cframe.counters[i] = cframe.counters[i] * systemVars.counters_magpp[i];
	}

	// Copio el resultado
	memcpy( dst_frame , &cframe, sizeof(st_counters_t) );

	// Borro los contadores para iniciar un nuevo ciclo.
	if ( reset_counters ) {
		for (i = 0; i < NRO_COUNTER_CHANNELS; i++) {
			cframe.counters[i] = 0.0;
		}
	}

}
//------------------------------------------------------------------------------------
void pub_counters_load_defaults(void)
{

	// Realiza la configuracion por defecto de los canales digitales.
uint8_t i;

	for ( i = 0; i < NRO_COUNTER_CHANNELS; i++ ) {
		snprintf_P( systemVars.counters_name[i], PARAMNAME_LENGTH, PSTR("C%d\0"), i );
		systemVars.counters_magpp[i] = 0.1;
	}

	// Debounce Time
	systemVars.counter_debounce_time = 50;

}
//------------------------------------------------------------------------------------
bool pub_counters_config_channel( uint8_t channel,char *s_param0, char *s_param1 )
{
	// Configuro un canal contador.
	// channel: id del canal
	// s_param0: string del nombre del canal
	// s_param1: string con el valor del factor magpp.
	//
	// {0..1} dname magPP

bool retS = false;

	while ( xSemaphoreTake( sem_SYSVars, ( TickType_t ) 5 ) != pdTRUE )
		taskYIELD();

	if ( ( channel >=  0) && ( channel < NRO_COUNTER_CHANNELS ) ) {

		// NOMBRE
		pub_control_string(s_param0);
		snprintf_P( systemVars.counters_name[channel], PARAMNAME_LENGTH, PSTR("%s\0"), s_param0 );

		// MAGPP
		if ( s_param1 != NULL ) { systemVars.counters_magpp[channel] = atof(s_param1); }

		retS = true;
	}

	xSemaphoreGive( sem_SYSVars );
	return(retS);

}
//------------------------------------------------------------------------------------
void pub_counter_config_debounce_time( char *s_counter_debounce_time )
{
	// Configura el tiempo de debounce del conteo de pulsos

	while ( xSemaphoreTake( sem_SYSVars, ( TickType_t ) 5 ) != pdTRUE )
		taskYIELD();

	systemVars.counter_debounce_time = atoi(s_counter_debounce_time);

	if ( systemVars.counter_debounce_time < 1 )
		systemVars.counter_debounce_time = 50;

	xSemaphoreGive( sem_SYSVars );
	return;
}
//------------------------------------------------------------------------------------


