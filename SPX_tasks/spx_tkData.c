/*
 * spx_tkData.c
 *
 *  Created on: 11 dic. 2018
 *      Author: pablo
 *
 *  Esta tarea se encarga de leer los canales analogicos y luego armar un frame de datos
 *  con los datos de los contadores y entradas digitales.
 *  Luego los muestra en pantalla y los guarda en memoria.
 *  Todo esta pautado por el timerPoll.
 *  Usamos una estructura unica global para leer los datos.
 *  Cuando usamos el comando de 'read frame' desde el cmdMode, escribimos sobre la misma
 *  estructura de datos por lo cual si coincide con el momento de poleo se pueden
 *  llegar a sobreescribir.
 *  Esto en realidad no seria importante ya que en modo cmd esta el operador.
 *
 */

#include "spx.h"

// Estructura de datos de un dataframe ( considera todos los canales que se pueden medir )
typedef struct {
	float ainputs[MAX_ANALOG_CHANNELS];
	uint16_t dinputs[MAX_DINPUTS_CHANNELS];
	float counters[MAX_COUNTER_CHANNELS];
	int16_t range;
	float battery;
	RtcTimeType_t rtc;
} dataframe_s;

dataframe_s dataframe;

bool sensores_prendidos;

//------------------------------------------------------------------------------------
// PROTOTIPOS

static void pv_data_init(void);
static void pv_data_read_frame( void );
static void pv_data_print_frame( void );
static void pv_data_guardar_en_BD(void);

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
		pv_data_read_frame();

		// Muestro en pantalla.
		pv_data_print_frame();

		// Guardo en la BD
		pv_data_guardar_en_BD();

		// Calculo el tiempo para una nueva espera
		while ( xSemaphoreTake( sem_SYSVars, ( TickType_t ) 5 ) != pdTRUE )
			taskYIELD();

			// timpo real que voy a dormir esta tarea
			waiting_ticks = (uint32_t)(systemVars.timerPoll) * 1000 / portTICK_RATE_MS;
			// tiempo similar que voy a decrementar y mostrar en consola.
			ctl_reload_timerPoll(systemVars.timerPoll);

		xSemaphoreGive( sem_SYSVars );

	}

}
//------------------------------------------------------------------------------------
// FUNCIONES PUBLICAS
//------------------------------------------------------------------------------------
void data_read_frame( bool polling  )
{
	// Esta funcion puede polear y mostrar el resultado.
	// Si la flag polling es false, no poleo. ( requerido en cmd::status )
	// Si el puntero a la estructura el NULL, uso la estructura estatica local
	// Si no es null, imprimo esta ( cmd::read_memory )

	if ( polling ) {
		// Leo analog,digital,rtc e imprimo.
		pv_data_read_frame();
	}

	pv_data_print_frame();

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

    sensores_prendidos = false;

}
//------------------------------------------------------------------------------------
static void pv_data_read_frame( void )
{
	// Leo todos los canales de a 1 que forman un frame de datos analogicos

uint16_t raw_val;
uint8_t i;
int8_t xBytes;

	// Prendo los sensores, espero un settle time de 1s, los leo y apago los sensores.
	// En las placas SPX_IO8 no prendo los sensores porque no los alimento.
	// Los prendo si estoy con una placa IO5 y los sensores estan apagados.
	if ( ( spx_io_board == SPX_IO5CH ) && ( ! sensores_prendidos ) ) {

		AINPUTS_prender_12V();
		INA_config(0, CONF_INAS_AVG128 );
		INA_config(1, CONF_INAS_AVG128 );
		sensores_prendidos = true;

		// Normalmente espero 1s de settle time que esta bien para los sensores
		// pero cuando hay un caudalimetro de corriente, necesita casi 5s
		// vTaskDelay( ( TickType_t)( 1000 / portTICK_RATE_MS ) );
		vTaskDelay( ( TickType_t)( ( 1000 * systemVars.ainputs_conf.pwr_settle_time ) / portTICK_RATE_MS ) );

	}

	// Leo.
	// Los canales de IO no son los mismos que los canales del INA !! ya que la bateria
	// esta en el canal 1 del ina2
	// Lectura general.
	ainputs_read ( 0, &raw_val, &dataframe.ainputs[0] );
	ainputs_read ( 1, &raw_val, &dataframe.ainputs[1] );
	ainputs_read ( 2, &raw_val, &dataframe.ainputs[2] );
	ainputs_read ( 3, &raw_val, &dataframe.ainputs[3] );
	ainputs_read ( 4, &raw_val, &dataframe.ainputs[4] );

	if ( spx_io_board == SPX_IO8CH ) {
		ainputs_read ( 5, &raw_val, &dataframe.ainputs[5] );
		ainputs_read ( 6, &raw_val, &dataframe.ainputs[6] );
		ainputs_read ( 7, &raw_val, &dataframe.ainputs[7] );
	}

	// Bateria
	if ( spx_io_board == SPX_IO5CH ) {
		// Leo la bateria
		// Convierto el raw_value a la magnitud ( 8mV por count del A/D)
		dataframe.battery = 0.008 * AINPUTS_read_battery();
	}

	// Apago los sensores y pongo a los INA a dormir si estoy con la board IO5 y
	// el timerPoll > 180s.
	// Sino dejo todo prendido porque estoy en modo continuo
	if ( (spx_io_board == SPX_IO5CH) && ( systemVars.timerPoll > 180 ) ) {

		INA_config(0, CONF_INAS_SLEEP );
		INA_config(1, CONF_INAS_SLEEP );
		AINPUTS_apagar_12V();
		sensores_prendidos = false;
	}

	// Leo los contadores
	for ( i = 0; i < NRO_COUNTERS; i++ ) {
		dataframe.counters[i] = counters_read(i, true);
	}

	// Leo las entradas digitales
	for ( i = 0; i < NRO_DINPUTS; i++ ) {
		dataframe.dinputs[i] = dinputs_read(i);
	}

	// Leo el ancho de pulso ( rangeMeter ). Demora 5s si esta habilitado
	dataframe.range = range_read();

	// Agrego el timestamp
	xBytes = RTC_read_dtime( &dataframe.rtc );
	if ( xBytes == -1 )
		xprintf_P(PSTR("ERROR: I2C:RTC:pub_data_read_frame\r\n\0"));

}
//------------------------------------------------------------------------------------
static void pv_data_print_frame( void )
{
	// Imprime el frame actual en consola

uint8_t channel;

	// HEADER
	xprintf_P(PSTR("frame: " ) );

	// timeStamp.
	xprintf_P(PSTR("%04d%02d%02d,"),dataframe.rtc.year, dataframe.rtc.month, dataframe.rtc.day );
	xprintf_P(PSTR("%02d%02d%02d"), dataframe.rtc.hour, dataframe.rtc.min, dataframe.rtc.sec );

	// Canales analogicos: Solo muestro los que tengo configurados.
	for ( channel = 0; channel < NRO_ANINPUTS; channel++) {
		if ( ! strcmp ( systemVars.ainputs_conf.name[channel], "X" ) )
			continue;

		xprintf_P(PSTR(",%s=%.02f"),systemVars.ainputs_conf.name[channel], dataframe.ainputs[channel] );
	}

	// Calanes digitales.
	for ( channel = 0; channel < NRO_DINPUTS; channel++) {
		if ( ! strcmp ( systemVars.dinputs_conf.name[channel], "X" ) )
			continue;

		xprintf_P(PSTR(",%s=%d"), systemVars.dinputs_conf.name[channel], dataframe.dinputs[channel] );
	}

	// Contadores
	for ( channel = 0; channel < NRO_COUNTERS; channel++) {
		// Si el canal no esta configurado no lo muestro.
		if ( ! strcmp ( systemVars.counters_conf.name[channel], "X" ) )
			continue;

		xprintf_P(PSTR(",%s=%.02f"),systemVars.counters_conf.name[channel], dataframe.counters[channel] );
	}

	// Range
	if ( ( spx_io_board == SPX_IO5CH ) && ( systemVars.rangeMeter_enabled ) ) {
		xprintf_P(PSTR(",RANGE=%d"), dataframe.range );
	}

	// bateria
	if ( spx_io_board == SPX_IO5CH ) {
		xprintf_P(PSTR(",BAT=%.02f"), dataframe.battery );
	}

	// TAIL
	xprintf_P(PSTR("\r\n\0") );

}
//------------------------------------------------------------------------------------
static void pv_data_guardar_en_BD(void)
{

	// Solo los salvo en la BD si estoy en modo normal.
	// En otros casos ( service, monitor_frame, etc, no.

FAT_t l_fat;
int8_t bytes_written;
static bool primer_frame = true;
st_dataRecord_t dr;

	// Para no incorporar el error de los contadores en el primer frame no lo guardo.
	if ( primer_frame ) {
		primer_frame = false;
		return;
	}

	// Copio al dr solo los campos que correspondan
	switch ( spx_io_board ) {
	case SPX_IO5CH:
		memcpy( &dr.df.io5.ainputs, &dataframe.ainputs, ( NRO_ANINPUTS * sizeof(float)));
		memcpy( &dr.df.io5.dinputs, &dataframe.dinputs, ( NRO_DINPUTS * sizeof(uint16_t)));
		memcpy( &dr.df.io5.counters, &dataframe.counters, ( NRO_COUNTERS * sizeof(float)));
		dr.df.io5.range = dataframe.range;
		dr.df.io5.battery = dataframe.battery;
		memcpy( &dr.rtc, &dataframe.rtc, sizeof(RtcTimeType_t) );
		break;
	case SPX_IO8CH:
		memcpy( &dr.df.io8.ainputs, &dataframe.ainputs, ( NRO_ANINPUTS * sizeof(float)));
		memcpy( &dr.df.io8.dinputs, &dataframe.dinputs, ( NRO_DINPUTS * sizeof(uint16_t)));
		memcpy( &dr.df.io8.counters, &dataframe.counters, ( NRO_COUNTERS * sizeof(float)));
		memcpy( &dr.rtc, &dataframe.rtc, sizeof(RtcTimeType_t) );
		break;
	default:
		return;
	}

	// Guardo en BD
	bytes_written = FF_writeRcd( &dr, sizeof(st_dataRecord_t) );

	if ( bytes_written == -1 ) {
		// Error de escritura o memoria llena ??
		xprintf_P(PSTR("DATA: WR ERROR (%d)\r\n\0"),FF_errno() );
		// Stats de memoria
		FAT_read(&l_fat);
		xprintf_P( PSTR("DATA: MEM [wr=%d,rd=%d,del=%d]\0"), l_fat.wrPTR,l_fat.rdPTR, l_fat.delPTR );
	}

}
//------------------------------------------------------------------------------------


