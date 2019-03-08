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

// Configuracion de Canales analogicos
typedef struct {
	uint8_t imin[MAX_ANALOG_CHANNELS];	// Coeficientes de conversion de I->magnitud (presion)
	uint8_t imax[MAX_ANALOG_CHANNELS];
	float mmin[MAX_ANALOG_CHANNELS];
	float mmax[MAX_ANALOG_CHANNELS];
	char name[MAX_ANALOG_CHANNELS][PARAMNAME_LENGTH];
	float offset[MAX_ANALOG_CHANNELS];
	float inaspan[MAX_ANALOG_CHANNELS];
	uint8_t pwr_settle_time;
	bool rangeMeter_enabled;
} analogin_t;

analogin_t ain_conf;	// Estructura con la configuracion de las entradas analogicas


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
bool ainputs_config_channel( uint8_t channel,char *s_aname,char *s_imin,char *s_imax,char *s_mmin,char *s_mmax )
{

	// Configura los canales analogicos. Es usada tanto desde el modo comando como desde el modo online por gprs.

bool retS = false;

	u_control_string(s_aname);

	if ( ( channel >=  0) && ( channel < NRO_ANINPUTS ) ) {
		snprintf_P( ain_conf.name[channel], PARAMNAME_LENGTH, PSTR("%s\0"), s_aname );
		if ( s_imin != NULL ) { ain_conf.imin[channel] = atoi(s_imin); }
		if ( s_imax != NULL ) { ain_conf.imax[channel] = atoi(s_imax); }
		if ( s_mmin != NULL ) { ain_conf.mmin[channel] = atoi(s_mmin); }
		if ( s_mmax != NULL ) { ain_conf.mmax[channel] = atof(s_mmax); }
		retS = true;
	}

	return(retS);
}
//------------------------------------------------------------------------------------
void ainputs_read ( uint8_t io_channel, uint16_t *raw_val, float *mag_val )
{
	// Lee un canal analogico y devuelve el valor convertido a la magnitud configurada.
	// Es publico porque se utiliza tanto desde el modo comando como desde el modulo de poleo de las entradas.
	// Hay que corregir la correspondencia entre el canal leido del INA y el canal fisico del datalogger
	// io_channel. Esto lo hacemos en AINPUTS_read_ina.

uint16_t an_raw_val;
float an_mag_val;
float I,M,P;
uint16_t D;

	an_raw_val = AINPUTS_read_ina(spx_io_board, io_channel );
	*raw_val = an_raw_val;

	// Convierto el raw_value a la magnitud
	// Calculo la corriente medida en el canal
	I = (float)( an_raw_val) * 20 / ( ain_conf.inaspan[io_channel] + 1);

	// Calculo la magnitud
	P = 0;
	D = ain_conf.imax[io_channel] - ain_conf.imin[io_channel];

	an_mag_val = 0.0;
	if ( D != 0 ) {
		// Pendiente
		P = (float) ( ain_conf.mmax[io_channel]  -  ain_conf.mmin[io_channel] ) / D;
		// Magnitud
		M = (float) (ain_conf.mmin[io_channel] + ( I - ain_conf.imin[io_channel] ) * P);

		// Al calcular la magnitud, al final le sumo el offset.
		an_mag_val = M + ain_conf.offset[io_channel];
	} else {
		// Error: denominador = 0.
		an_mag_val = -999.0;
	}

	*mag_val = an_mag_val;

}
//------------------------------------------------------------------------------------
bool data_get_rmeter_enabled(void)
{
	return ( ain_conf.rangeMeter_enabled);
}
//------------------------------------------------------------------------------------
uint8_t data_get_pwr_settle_time(void)
{
	return(ain_conf.pwr_settle_time);
}
//------------------------------------------------------------------------------------
uint8_t data_get_imin( uint8_t ain )
{
	return( ain_conf.imin[ain]);
}
//------------------------------------------------------------------------------------
uint8_t data_get_imax( uint8_t ain )
{
	return( ain_conf.imax[ain]);
}
//------------------------------------------------------------------------------------
float data_get_mmin( uint8_t ain )
{
	return( ain_conf.mmin[ain]);
}
//------------------------------------------------------------------------------------
float data_get_mmax( uint8_t ain )
{
	return( ain_conf.mmax[ain]);
}
//------------------------------------------------------------------------------------
float data_get_offset( uint8_t ain )
{
	return( ain_conf.offset[ain]);
}
//------------------------------------------------------------------------------------
float data_get_span( uint8_t ain )
{
	return( ain_conf.inaspan[ain]);
}
//------------------------------------------------------------------------------------
char * data_get_name(uint8_t ain )
{
	return(ain_conf.name[ain]);
}
//------------------------------------------------------------------------------------
bool data_config_rangemeter ( char *s_mode )
{

	// Esta opcion es solo valida para IO5
	if ( spx_io_board != SPX_IO5CH )
		return(false);

	if ( !strcmp_P( strupr(s_mode), PSTR("ON\0"))) {
		ain_conf.rangeMeter_enabled = true;
		return(true);
	} else if ( !strcmp_P( strupr(s_mode), PSTR("OFF\0"))) {
		ain_conf.rangeMeter_enabled = false;
		return(true);
	}
	return(false);

}
//------------------------------------------------------------------------------------
bool data_config_offset( char *s_channel, char *s_offset )
{
	// Configuro el parametro offset de un canal analogico.

uint8_t channel;
float offset;

	channel = atoi(s_channel);
	if ( ( channel >=  0) && ( channel < NRO_ANINPUTS ) ) {
		offset = atof(s_offset);
		ain_conf.offset[channel] = offset;
		return(true);
	}

	return(false);
}
//------------------------------------------------------------------------------------
void data_config_sensortime ( char *s_sensortime )
{
	// Configura el tiempo de espera entre que prendo  la fuente de los sensores y comienzo el poleo.
	// Se utiliza solo desde el modo comando.
	// El tiempo de espera debe estar entre 1s y 15s

	ain_conf.pwr_settle_time = atoi(s_sensortime);

	if ( ain_conf.pwr_settle_time < 1 )
		ain_conf.pwr_settle_time = 1;

	if ( ain_conf.pwr_settle_time > 15 )
		ain_conf.pwr_settle_time = 15;

	return;
}
//------------------------------------------------------------------------------------
void data_config_span ( char *s_channel, char *s_span )
{
	// Configura el factor de correccion del span de canales delos INA.
	// Esto es debido a que las resistencias presentan una tolerancia entonces con
	// esto ajustamos que con 20mA den la excursión correcta.
	// Solo de configura desde modo comando.

uint8_t channel;
uint16_t span;

	channel = atoi(s_channel);
	if ( ( channel >=  0) && ( channel < NRO_ANINPUTS ) ) {
		span = atoi(s_span);
		ain_conf.inaspan[channel] = span;
	}
	return;

}
//------------------------------------------------------------------------------------
bool data_config_autocalibrar( char *s_channel, char *s_mag_val )
{
	// Para un canal, toma como entrada el valor de la magnitud y ajusta
	// mag_offset para que la medida tomada coincida con la dada.


uint16_t an_raw_val;
float an_mag_val;
float I,M,P;
uint16_t D;
uint8_t channel;

float an_mag_val_real;
float offset;

	channel = atoi(s_channel);

	if ( channel >= NRO_ANINPUTS ) {
		return(false);
	}

	// Leo el canal del ina.
	an_raw_val = AINPUTS_read_ina( spx_io_board, channel );
//	xprintf_P( PSTR("ANRAW=%d\r\n\0"), an_raw_val );

	// Convierto el raw_value a la magnitud
	I = (float)( an_raw_val) * 20 / ( ain_conf.inaspan[channel] + 1);
	P = 0;
	D = ain_conf.imax[channel] - ain_conf.imin[channel];

	an_mag_val = 0.0;
	if ( D != 0 ) {
		// Pendiente
		P = (float) ( ain_conf.mmax[channel]  -  ain_conf.mmin[channel] ) / D;
		// Magnitud
		M = (float) (ain_conf.mmin[channel] + ( I - ain_conf.imin[channel] ) * P);

		// En este caso el offset que uso es 0 !!!.
		an_mag_val = M;

	} else {
		return(false);
	}

//	xprintf_P( PSTR("ANMAG=%.02f\r\n\0"), an_mag_val );

	an_mag_val_real = atof(s_mag_val);
//	xprintf_P( PSTR("ANMAG_T=%.02f\r\n\0"), an_mag_val_real );

	offset = an_mag_val_real - an_mag_val;
//	xprintf_P( PSTR("AUTOCAL offset=%.02f\r\n\0"), offset );

	ain_conf.offset[channel] = offset;

	xprintf_P( PSTR("OFFSET=%.02f\r\n\0"), ain_conf.offset[channel] );

	return(true);

}
//------------------------------------------------------------------------------------
void ainputs_config_defaults(void)
{
	// Realiza la configuracion por defecto de los canales digitales.

uint8_t channel;

	ain_conf.pwr_settle_time = 1;

	for ( channel = 0; channel < NRO_ANINPUTS; channel++) {
		ain_conf.imin[channel] = 0;
		ain_conf.imax[channel] = 20;
		ain_conf.mmin[channel] = 0;
		ain_conf.mmax[channel] = 6.0;
		ain_conf.offset[channel] = 0.0;
		ain_conf.inaspan[channel] = 3646;
		snprintf_P( ain_conf.name[channel], PARAMNAME_LENGTH, PSTR("A%d\0"),channel );
	}

}
//------------------------------------------------------------------------------------
void range_config_defaults(void)
{
	ain_conf.rangeMeter_enabled = false;
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
		vTaskDelay( ( TickType_t)( ( 1000 * ain_conf.pwr_settle_time ) / portTICK_RATE_MS ) );

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

	// Leo el ancho de pulso ( rangeMeter ). Demora 5s.
	// Solo si el equipo es IO5CH y esta el range habilitado !!!
	if ( ( spx_io_board == SPX_IO5CH )  && (ain_conf.rangeMeter_enabled ) ) {
		( systemVars.debug == DEBUG_DATA ) ?  RMETER_ping( &dataframe.range, true ) : RMETER_ping( &dataframe.range, false );
	}

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
		if ( ! strcmp ( ain_conf.name[channel], "X" ) )
			continue;

		xprintf_P(PSTR(",%s=%.02f"),ain_conf.name[channel],dataframe.ainputs[channel] );
	}

	// Calanes digitales.
	for ( channel = 0; channel < NRO_DINPUTS; channel++) {
		if ( ! strcmp ( dinputs_get_name(channel), "X" ) )
			continue;

		xprintf_P(PSTR(",%s=%d"), dinputs_get_name(channel), dataframe.dinputs[channel] );
	}

	// Contadores
	for ( channel = 0; channel < NRO_COUNTERS; channel++) {
		// Si el canal no esta configurado no lo muestro.
		if ( ! strcmp ( counters_get_name(channel), "X" ) )
			continue;

		xprintf_P(PSTR(",%s=%.02f"),counters_get_name(channel), dataframe.counters[channel] );
	}

	// Range
	if ( ( spx_io_board == SPX_IO5CH ) && ( ain_conf.rangeMeter_enabled ) ) {
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

	// Para no incorporar el error de los contadores en el primer frame no lo guardo.
	if ( primer_frame ) {
		primer_frame = false;
		return;
	}
/*
	// Guardo en BD
	bytes_written = FF_writeRcd( &dataRecord, sizeof(st_dataRecord_t) );

	if ( bytes_written == -1 ) {
		// Error de escritura o memoria llena ??
		xprintf_P(PSTR("DATA: WR ERROR (%d)\r\n\0"),FF_errno() );
		// Stats de memoria
		FAT_read(&l_fat);
		xprintf_P( PSTR("DATA: MEM [wr=%d,rd=%d,del=%d]\0"), l_fat.wrPTR,l_fat.rdPTR, l_fat.delPTR );
	}
*/
}
//------------------------------------------------------------------------------------


