/*
 * spx_tkData.c
 *
 *  Created on: 11 dic. 2018
 *      Author: pablo
 */

#include "spx.h"

//------------------------------------------------------------------------------------
// PROTOTIPOS

static void pv_read_analog_frame( st_dataRecord_t *drcd );
static void pv_data_init(void);
static void pv_data_guardar_en_BD(void);
static void pv_data_read_frame( st_dataRecord_t *drcd );
static void pv_data_print_frame( st_dataRecord_t *drcd );

st_dataRecord_t dataRecord;

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

		ctl_watchdog_kick(WDG_DAT , WDG_DAT_TIMEOUT);

		// Espero. Da el tiempo necesario para entrar en tickless.
		vTaskDelayUntil( &xLastWakeTime, waiting_ticks );

		// Leo analog,digital,rtc,salvo en BD e imprimo.
		pv_data_read_frame(  &dataRecord );

		// Muestro en pantalla.
		pv_data_print_frame( &dataRecord );

		// Guardo en la BD
		pv_data_guardar_en_BD();

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
void data_show_frame( st_dataRecord_t *drcd, bool polling  )
{
	// Esta funcion puede polear y mostrar el resultado.
	// Si la flag polling es false, no poleo. ( requerido en cmd::status )
	// Si el puntero a la estructura el NULL, uso la estructura estatica local
	// Si no es null, imprimo esta ( cmd::read_memory )

	if ( polling ) {
		// Leo analog,digital,rtc,salvo en BD e imprimo.
		pv_data_read_frame(  &dataRecord );
	}

	// Muestro en pantalla.
	if ( drcd == NULL ) {
		pv_data_print_frame( &dataRecord );
	} else {
		pv_data_print_frame( drcd );
	}
}
//------------------------------------------------------------------------------------
// FUNCIONES PRIVADAS
//------------------------------------------------------------------------------------
static void pv_data_init(void)
{
	// Inicializo los INA con los que mido las entradas analogicas.
	AINPUTS_init( spx_io_board );

    // Inicializo el sistema de medida de ancho de pulsos
    if ( spx_io_board == SPX_IO5CH ) {
    	RMETER_init( SYSMAINCLK );
   }

}
//------------------------------------------------------------------------------------
static void pv_read_analog_frame( st_dataRecord_t *drcd )
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
		u_read_analog_channel (spx_io_board, 0, &raw_val, &drcd->df.io5.ainputs[0] );
		u_read_analog_channel (spx_io_board, 1, &raw_val, &drcd->df.io5.ainputs[1] );
		u_read_analog_channel (spx_io_board, 2, &raw_val, &drcd->df.io5.ainputs[2] );
		u_read_analog_channel (spx_io_board, 3, &raw_val, &drcd->df.io5.ainputs[3] );
		u_read_analog_channel (spx_io_board, 4, &raw_val, &drcd->df.io5.ainputs[4] );

		// Leo la bateria
		// Convierto el raw_value a la magnitud ( 8mV por count del A/D)
		drcd->df.io5.battery = 0.008 * AINPUTS_read_battery();

	} if ( spx_io_board == SPX_IO8CH ) {
		u_read_analog_channel (spx_io_board, 0, &raw_val, &drcd->df.io8.ainputs[0] );
		u_read_analog_channel (spx_io_board, 1, &raw_val, &drcd->df.io8.ainputs[1] );
		u_read_analog_channel (spx_io_board, 2, &raw_val, &drcd->df.io8.ainputs[2] );
		u_read_analog_channel (spx_io_board, 3, &raw_val, &drcd->df.io8.ainputs[3] );
		u_read_analog_channel (spx_io_board, 4, &raw_val, &drcd->df.io8.ainputs[4] );
		u_read_analog_channel (spx_io_board, 5, &raw_val, &drcd->df.io8.ainputs[5] );
		u_read_analog_channel (spx_io_board, 6, &raw_val, &drcd->df.io8.ainputs[6] );
		u_read_analog_channel (spx_io_board, 7, &raw_val, &drcd->df.io8.ainputs[7] );

	}

	// Pongo a los INA a dormir.
	if ( spx_io_board == SPX_IO5CH ) {

		INA_config(0, CONF_INAS_SLEEP );
		INA_config(1, CONF_INAS_SLEEP );
		AINPUTS_apagar_12V();

	}

}
//------------------------------------------------------------------------------------
static void pv_data_guardar_en_BD(void)
{

	// Solo los salvo en la BD si estoy en modo normal.
	// En otros casos ( service, monitor_frame, etc, no.

FAT_t l_fat;
int8_t bytes_written;
static bool primer_frame = true;

	// Para no incorporar el error de los contadores en el primer frame no lo guardo.
	if ( primer_frame ) {
		primer_frame = false;
		return;
	}

	// Guardo en BD
	bytes_written = FF_writeRcd( &dataRecord, sizeof(st_dataRecord_t) );

	if ( bytes_written == -1 ) {
		// Error de escritura o memoria llena ??
		xprintf_P(PSTR("DATA: WR ERROR (%d)\r\n\0"),FF_errno() );
		// Stats de memoria
		FAT_read(&l_fat);
		xprintf_P( PSTR("DATA: MEM [wr=%d,rd=%d,del=%d]\0"), l_fat.wrPTR,l_fat.rdPTR, l_fat.delPTR );
	}
}
//------------------------------------------------------------------------------------
static void pv_data_read_frame( st_dataRecord_t *drcd )
{

	// Funcion usada para leer los datos de todos los modulos, guardarlos en memoria
	// e imprimirlos.
	// La usa por un lado tkData en forma periodica y desde el cmd line cuando se
	// da el comando read frame.

int8_t xBytes;

	// Leo los canales analogicos.
	pv_read_analog_frame( drcd );

	// Leo los contadores
	counters_read_frame( drcd, true );

	// Leo las entradas digitales
	dinputs_read_frame( drcd );

	// Leo el ancho de pulso ( rangeMeter ). Demora 5s.
	// Solo si el equipo es IO5CH y esta el range habilitado !!!
	if ( spx_io_board == SPX_IO5CH ) {
		if (systemVars.rangeMeter_enabled ) {
			( systemVars.debug == DEBUG_DATA ) ?  RMETER_ping( &drcd->df.io5.range, true ) : RMETER_ping( &drcd->df.io5.range, false );
		}
	}

	// Agrego el timestamp
	xBytes = RTC_read_dtime( &drcd->rtc );
	if ( xBytes == -1 )
		xprintf_P(PSTR("ERROR: I2C:RTC:pub_data_read_frame\r\n\0"));



}
//------------------------------------------------------------------------------------
static void pv_data_print_frame( st_dataRecord_t *drcd )
{
	// Imprime el frame actual en consola

uint8_t channel;

	// HEADER
	xprintf_P(PSTR("frame: " ) );

	// timeStamp.
	xprintf_P(PSTR("%04d%02d%02d,"),drcd->rtc.year, drcd->rtc.month, drcd->rtc.day );
	xprintf_P(PSTR("%02d%02d%02d"), drcd->rtc.hour, drcd->rtc.min, drcd->rtc.sec );

	// Canales analogicos: Solo muestro los que tengo configurados.
	for ( channel = 0; channel < NRO_ANINPUTS; channel++) {
		if ( ! strcmp ( systemVars.ain_name[channel], "X" ) )
			continue;

		if ( spx_io_board == SPX_IO5CH ) {
			xprintf_P(PSTR(",%s=%.02f"),systemVars.ain_name[channel],drcd->df.io5.ainputs[channel] );

		} else if ( spx_io_board == SPX_IO8CH ) {
			xprintf_P(PSTR(",%s=%.02f"),systemVars.ain_name[channel],drcd->df.io8.ainputs[channel] );
		}
	}

	// Calanes digitales.
	for ( channel = 0; channel < NRO_DINPUTS; channel++) {
		if ( ! strcmp ( systemVars.din_name[channel], "X" ) )
			continue;

		if ( spx_io_board == SPX_IO5CH ) {
			xprintf_P(PSTR(",%s=%d"),systemVars.din_name[channel], drcd->df.io5.dinputs[channel] );

		} else if ( spx_io_board == SPX_IO8CH ) {
			xprintf_P(PSTR(",%s=%d"),systemVars.din_name[channel], drcd->df.io8.dinputs[channel] );
		}
	}

	// Contadores
	for ( channel = 0; channel < NRO_COUNTERS; channel++) {
		// Si el canal no esta configurado no lo muestro.
		if ( ! strcmp ( systemVars.counters_name[channel], "X" ) )
			continue;

		if ( spx_io_board == SPX_IO5CH ) {
			xprintf_P(PSTR(",%s=%.02f"),systemVars.counters_name[channel], drcd->df.io5.counters[channel] );

		} else if ( spx_io_board == SPX_IO8CH ) {
			xprintf_P(PSTR(",%s=%.02f"),systemVars.counters_name[channel], drcd->df.io8.counters[channel] );
		}
	}

	if ( spx_io_board == SPX_IO5CH ) {
		// Range
		if ( systemVars.rangeMeter_enabled ) {
			xprintf_P(PSTR(",RANGE=%d"), drcd->df.io5.range );
		}

		// bateria
		xprintf_P(PSTR(",BAT=%.02f"), drcd->df.io5.battery );
	}

	// TAIL
	xprintf_P(PSTR("\r\n\0") );

}
//------------------------------------------------------------------------------------


