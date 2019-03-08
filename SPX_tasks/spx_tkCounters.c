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

// Estructura que define todos las variables necesarias para el manejo de los contadores.

typedef struct {
	// Configuracion de canales contadores
	char name[MAX_COUNTER_CHANNELS][PARAMNAME_LENGTH];
	float magpp[MAX_COUNTER_CHANNELS];
	uint8_t debounce_time;
} counters_conf_t;

counters_conf_t counters_conf;	// Estructura con la configuracion de los contadores

float cvalues[MAX_COUNTER_CHANNELS];	// Valores medidos de los contadores

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
				cvalues[0]++;
				reset_wakeup_for_C0();
			}

			if ( cTask_wakeup_for_C1() ) {
				cvalues[1]++;
				reset_wakeup_for_C1();
			}

			if ( systemVars.debug == DEBUG_COUNTER) {
				xprintf_P( PSTR("COUNTERS: C0=%d,C1=%d\r\n\0"),(uint16_t) cvalues[0], (uint16_t) cvalues[1]);
			}

			// Espero el tiempo de debounced
			vTaskDelay( ( TickType_t)( counters_conf.debounce_time / portTICK_RATE_MS ) );

			CNT_clr_CLRD();		// Borro el latch llevandolo a 0.
			CNT_set_CLRD();		// Lo dejo en reposo en 1

		}

	}
}
//------------------------------------------------------------------------------------
static void pv_tkCounter_init(void)
{
	// Configuracion inicial de la tarea

uint8_t i;

	COUNTERS_init( xHandle_tkCounter );

	for ( i = 0; i < MAX_COUNTER_CHANNELS; i++) {
		cvalues[ i ] = 0;
	}

}
//------------------------------------------------------------------------------------
// FUNCIONES PUBLICAS
//------------------------------------------------------------------------------------
float counters_read( uint8_t cnt, bool reset_counter )
{

	// Funcion para leer el valor de los contadores.
	// Respecto de los contadores, no leemos pulsos sino magnitudes por eso antes lo
	// convertimos a la magnitud correspondiente.
	// Siempre multiplico por magPP. Si quiero que sea en mt3/h, en el server debo hacerlo (  * 3600 / systemVars.timerPoll )

float val;

	val = cvalues[cnt] * counters_conf.magpp[cnt];
	if ( reset_counter )
		cvalues[cnt] = 0.0;

	return(val);
}
//------------------------------------------------------------------------------------
void counters_config_defaults(void)
{

	// Realiza la configuracion por defecto de los canales contadores.

uint8_t i;

	for ( i = 0; i < MAX_COUNTER_CHANNELS; i++ ) {
		snprintf_P( counters_conf.name[i], PARAMNAME_LENGTH, PSTR("C%d\0"), i );
		counters_conf.magpp[i] = 0.1;
	}

	// Debounce Time
	counters_conf.debounce_time = 50;

}
//------------------------------------------------------------------------------------
void counters_config_debounce_time( char *s_counter_debounce_time )
{
	// Configura el tiempo de debounce del conteo de pulsos.
	// Se utiliza desde cmd_mode.

	counters_conf.debounce_time = atoi(s_counter_debounce_time);

	if ( counters_conf.debounce_time < 1 )
		counters_conf.debounce_time = 50;

	return;
}
//------------------------------------------------------------------------------------
bool counters_config_channel( uint8_t channel,char *s_param0, char *s_param1 )
{
	// Configuro un canal contador.
	// channel: id del canal
	// s_param0: string del nombre del canal
	// s_param1: string con el valor del factor magpp.
	//
	// {0..1} dname magPP

bool retS = false;

	if ( ( channel >=  0) && ( channel < MAX_COUNTER_CHANNELS ) ) {

		// NOMBRE
		u_control_string(s_param0);
		snprintf_P( counters_conf.name[channel], PARAMNAME_LENGTH, PSTR("%s\0"), s_param0 );

		// MAGPP
		if ( s_param1 != NULL ) { counters_conf.magpp[channel] = atof(s_param1); }

		retS = true;
	}

	return(retS);

}
//------------------------------------------------------------------------------------
uint8_t counters_get_debounce_time(void)
{
	return(counters_conf.debounce_time);
}
//------------------------------------------------------------------------------------
char * counters_get_name(uint8_t cnt )
{
	return(counters_conf.name[cnt]);
}
//------------------------------------------------------------------------------------
float counters_get_magpp( uint8_t cnt )
{
	return(counters_conf.magpp[cnt]);
}
//------------------------------------------------------------------------------------


