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

static float counters[MAX_COUNTER_CHANNELS];	// Valores medidos de los contadores

// La tarea puede estar hasta 10s en standby
#define WDG_COUNT_TIMEOUT	60

//------------------------------------------------------------------------------------
void tkCounter0(void * pvParameters)
{

( void ) pvParameters;
uint32_t ulNotificationValue;
const TickType_t xMaxBlockTime = pdMS_TO_TICKS( 25000 );

	// Espero la notificacion para arrancar
	while ( !startTask )
		vTaskDelay( ( TickType_t)( 100 / portTICK_RATE_MS ) );

	tkCounter_init(0);

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
			if ( IO_read_PA2() == 0 ) {
				counters[0]++;
				if ( systemVars.debug == DEBUG_COUNTER) {
					xprintf_P( PSTR("COUNTERS: *C0=%d, C1=%d\r\n\0"),(uint16_t) counters[0],(uint16_t) counters[1] );
				}
			}

			// Espero perido
			vTaskDelay( ( TickType_t)( ( systemVars.counters_conf.period[0] - systemVars.counters_conf.pwidth[0] ) / portTICK_RATE_MS ) );
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

	tkCounter_init(1);

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
			if ( IO_read_PB2() == 0 ) {
				counters[1]++;
				if ( systemVars.debug == DEBUG_COUNTER) {
					xprintf_P( PSTR("COUNTERS: *C0=%d, C1=%d\r\n\0"),(uint16_t) counters[0],(uint16_t) counters[1] );
				}
			}

			// Espero perido
			vTaskDelay( ( TickType_t)( ( systemVars.counters_conf.period[1] - systemVars.counters_conf.pwidth[1] ) / portTICK_RATE_MS ) );
			COUNTERS_enable_interrupt(1);


		} else   {
			// Expiro el timeout de la tarea. Por ahora no hago nada.
		}
	}
}
//------------------------------------------------------------------------------------
// FUNCIONES PUBLICAS
//------------------------------------------------------------------------------------
void tkCounter_init(uint8_t cnt)
{
	// Configuracion inicial de la tarea

	switch(cnt) {
	case 0:
		COUNTERS_init( 0, xHandle_tkCounter0 );
		COUNTERS_enable_interrupt(0);
		counters[ 0 ] = 0;
		break;
	case 1:
		COUNTERS_init( 1, xHandle_tkCounter1 );
		COUNTERS_enable_interrupt(1);
		counters[ 1 ] = 0;
		break;
	}
}
//------------------------------------------------------------------------------------
float counters_read( uint8_t cnt, bool reset_counter )
{

	// Funcion para leer el valor de los contadores.
	// Respecto de los contadores, no leemos pulsos sino magnitudes por eso antes lo
	// convertimos a la magnitud correspondiente.
	// Siempre multiplico por magPP. Si quiero que sea en mt3/h, en el server debo hacerlo (  * 3600 / systemVars.timerPoll )

float val = 0;

	switch (cnt) {
	case 0:
		val = counters[0] * systemVars.counters_conf.magpp[0];
		if ( reset_counter )
			counters[0] = 0;
		break;

	case 1:
		val = counters[1] * systemVars.counters_conf.magpp[1];
		if ( reset_counter )
			counters[1] = 0;
		break;
	}

	return(val);
}
//------------------------------------------------------------------------------------
void counters_config_defaults(void)
{

	// Realiza la configuracion por defecto de los canales contadores.
	// Los valores son en ms.

uint8_t i;

	for ( i = 0; i < MAX_COUNTER_CHANNELS; i++ ) {
		snprintf_P( systemVars.counters_conf.name[i], PARAMNAME_LENGTH, PSTR("C%d\0"), i );
		systemVars.counters_conf.magpp[i] = 1;
		systemVars.counters_conf.period[i] = 100;
		systemVars.counters_conf.pwidth[i] = 10;
	}

//	systemVars.counters_conf.speed[0] = CNT_LOW_SPEED;
//	systemVars.counters_conf.speed[1] = CNT_HIGH_SPEED;

}
//------------------------------------------------------------------------------------
bool counters_config_channel( uint8_t channel,char *s_param0, char *s_param1, char *s_param2, char *s_param3, char *s_param4 )
{
	// Configuro un canal contador.
	// channel: id del canal
	// s_param0: string del nombre del canal
	// s_param1: string con el valor del factor magpp.
	//
	// {0..1} dname magPP

bool retS = false;

	if ( s_param0 == NULL ) {
		return(retS);
	}

	if ( ( channel >=  0) && ( channel < MAX_COUNTER_CHANNELS ) ) {

		// NOMBRE
		if ( u_control_string(s_param0) == 0 ) {
			xprintf_P( PSTR("DEBUG COUNTERS ERROR: C%d\r\n\0"), channel );
			return( false );
		}

		snprintf_P( systemVars.counters_conf.name[channel], PARAMNAME_LENGTH, PSTR("%s\0"), s_param0 );

		// MAGPP
		if ( s_param1 != NULL ) { systemVars.counters_conf.magpp[channel] = atof(s_param1); }

		// PW
		if ( s_param2 != NULL ) { systemVars.counters_conf.pwidth[channel] = atof(s_param2); }

		// MAGPP
		if ( s_param3 != NULL ) { systemVars.counters_conf.period[channel] = atof(s_param3); }

		// SPEED
//		if ( !strcmp_P( s_param4, PSTR("LS\0"))) {
//			 systemVars.counters_conf.speed[channel] = CNT_LOW_SPEED;
//		} else if ( !strcmp_P( s_param4 , PSTR("HS\0"))) {
//			 systemVars.counters_conf.speed[channel] = CNT_HIGH_SPEED;
//		}

		retS = true;
	}

	return(retS);

}
//------------------------------------------------------------------------------------
