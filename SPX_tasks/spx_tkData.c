/*
 * spx_tkData.c
 *
 *  Created on: 11 dic. 2018
 *      Author: pablo
 */

#include "spx.h"

//------------------------------------------------------------------------------------
// PROTOTIPOS

static void pv_read_analog_frame(void);
static void pv_data_init(void);

// VARIABLES LOCALES

RtcTimeType_t frame_rtc;
st_analog_inputs_t frame_ain;
st_counters_t frame_counters;

float battery;

// La tarea pasa por el mismo lugar c/timerPoll secs.
#define WDG_DAT_TIMEOUT	 ( systemVars.timerPoll + 60 )

//------------------------------------------------------------------------------------
void tkData(void * pvParameters)
{

( void ) pvParameters;

uint32_t waiting_ticks;
TickType_t xLastWakeTime;

	// Espero la notificacion para arrancar
	while ( !startTask )
		vTaskDelay( ( TickType_t)( 100 / portTICK_RATE_MS ) );

	xprintf_P( PSTR("starting tkData..\r\n\0"));

	pv_data_init();

    // Initialise the xLastWakeTime variable with the current time.
    xLastWakeTime = xTaskGetTickCount();

    // Al arrancar poleo a los 10s
    waiting_ticks = (uint32_t)(10) * 1000 / portTICK_RATE_MS;

	// loop
	for( ;; )
	{
		// Espero. Da el tiempo necesario para entrar en tickless.
		vTaskDelayUntil( &xLastWakeTime, waiting_ticks );

		// Leo analog,digital,rtc,salvo en BD e imprimo.
		data_read_frame();	// wdg_counter_data = 1

		// Muestro en pantalla.
		data_print_frame();	// wdg_counter_data = 2

		// Calculo el tiempo para una nueva espera
		while ( xSemaphoreTake( sem_SYSVars, ( TickType_t ) 5 ) != pdTRUE )
			taskYIELD();

			// timpo real que voy a dormir esta tarea
			waiting_ticks = (uint32_t)(systemVars.timerPoll) * 1000 / portTICK_RATE_MS;
			// tiempo similar que voy a decrementar y mostrar en consola.
			ctl_reload_timerPoll();

		xSemaphoreGive( sem_SYSVars );

	}

}
//------------------------------------------------------------------------------------
// FUNCIONES PUBLICAS
//------------------------------------------------------------------------------------
void data_read_frame(void)
{

	// Funcion usada para leer los datos de todos los modulos, guardarlos en memoria
	// e imprimirlos.
	// La usa por un lado tkData en forma periodica y desde el cmd line cuando se
	// da el comando read frame.

int8_t xBytes;

	// Leo los canales analogicos.
	pv_read_analog_frame();

	// Leo los contadores
	counters_read_frame( &frame_counters, true );

	// Agrego el timestamp
	xBytes = RTC_read_dtime( &frame_rtc);
	if ( xBytes == -1 )
		xprintf_P(PSTR("ERROR: I2C:RTC:pub_data_read_frame\r\n\0"));


}
//------------------------------------------------------------------------------------
void data_print_frame(void)
{
	// Imprime el frame actual en consola

uint8_t channel;

	// HEADER
	xprintf_P(PSTR("frame: " ) );

	// timeStamp.
	xprintf_P(PSTR("%04d%02d%02d,"),frame_rtc.year, frame_rtc.month, frame_rtc.day );
	xprintf_P(PSTR("%02d%02d%02d"), frame_rtc.hour, frame_rtc.min, frame_rtc.sec );

	// Canales analogicos: Solo muestro los que tengo configurados.
	for ( channel = 0; channel < NRO_ANINPUTS; channel++) {
		if ( ! strcmp ( systemVars.ain_name[channel], "X" ) )
			continue;

		xprintf_P(PSTR(",%s=%.02f"),systemVars.ain_name[channel],frame_ain.ainputs[channel] );
	}

	// Contadores
	for ( channel = 0; channel < NRO_COUNTERS; channel++) {
		// Si el canal no esta configurado no lo muestro.
		if ( ! strcmp ( systemVars.counters_name[channel], "X" ) )
			continue;

		xprintf_P(PSTR(",%s=%.02f"),systemVars.counters_name[channel], frame_counters.counters[channel] );
	}

	// bateria
	if ( spx_io_board == SPX_IO5CH ) {
		xprintf_P(PSTR(",BAT=%.02f"),battery );
	}

	// TAIL
	xprintf_P(PSTR("\r\n\0") );

}
//------------------------------------------------------------------------------------
// FUNCIONES PRIVADAS
//------------------------------------------------------------------------------------
static void pv_data_init(void)
{
	// Inicializo los INA con los que mido las entradas analogicas.
	xprintf_P( PSTR("INIT tkdata: IO_BOARD=%d\r\n\0"),spx_io_board );
	AINPUTS_init( spx_io_board );

    // Inicializo el sistema de medida de ancho de pulsos
//    if ( ( spx_io_board == SPX_IO5CH ) && systemVars.rangeMeter_enabled ) {
 //   	RMETER_init( SYSMAINCLK );
//    }


}
//------------------------------------------------------------------------------------
static void pv_read_analog_frame(void)
{
	// Leo todos los canales de a 1 que forman un frame de datos analogicos

uint16_t raw_val;

	// Prendo los sensores, espero un settle time de 1s, los leo y apago los sensores.
	// En las placas SPX_IO8 no prendo los sensores porque no los alimento.
	if ( spx_io_board == SPX_IO5CH ) {

		AINPUTS_prender_12V();
		INA_config(0, CONF_INAS_AVG128 );
		INA_config(1, CONF_INAS_AVG128 );

	}

	// Normalmente espero 1s de settle time que esta bien para los sensores
	// pero cuando hay un caudalimetro de corriente, necesita casi 5s
	// vTaskDelay( ( TickType_t)( 1000 / portTICK_RATE_MS ) );
	vTaskDelay( ( TickType_t)( ( 1000 * systemVars.pwr_settle_time ) / portTICK_RATE_MS ) );

	// Leo.
	if ( spx_io_board == SPX_IO5CH ) {

		// Los canales de IO no son los mismos que los canales del INA !! ya que la bateria
		// esta en el canal 1 del ina2
		u_read_analog_channel (spx_io_board, 0, &raw_val, &frame_ain.ainputs[0] );
		u_read_analog_channel (spx_io_board, 1, &raw_val, &frame_ain.ainputs[1] );
		u_read_analog_channel (spx_io_board, 2, &raw_val, &frame_ain.ainputs[2] );
		u_read_analog_channel (spx_io_board, 3, &raw_val, &frame_ain.ainputs[3] );
		u_read_analog_channel (spx_io_board, 4, &raw_val, &frame_ain.ainputs[4] );

		// Leo la bateria
		// Convierto el raw_value a la magnitud ( 8mV por count del A/D)
		battery = 0.008 * AINPUTS_read_battery();
	}

	// Pongo a los INA a dormir.
	if ( spx_io_board == SPX_IO5CH ) {

		INA_config(0, CONF_INAS_SLEEP );
		INA_config(1, CONF_INAS_SLEEP );
		AINPUTS_apagar_12V();

	}

}
//------------------------------------------------------------------------------------


