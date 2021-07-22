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
#include "SPX_tasks/spx_tkApp/tkApp.h"

st_dataRecord_t dataRecd;
float battery;

//------------------------------------------------------------------------------------
// PROTOTIPOS
static void pv_data_guardar_en_BD(void);

// La tarea pasa por el mismo lugar c/timerPoll secs.
#define WDG_DIN_TIMEOUT	 ( systemVars.timerPoll + WDG_TO60 )

void tkInputs_normal(void);
void tkInputs_external_poll(void);
void tkInputs_modbus(void);

//------------------------------------------------------------------------------------
void tkInputs(void * pvParameters)
{

( void ) pvParameters;

	// Espero la notificacion para arrancar
	while ( !startTask )
		vTaskDelay( ( TickType_t)( 100 / portTICK_RATE_MS ) );

	xprintf_P( PSTR("starting tkInputs..\r\n\0"));

  	if ( sVarsApp.aplicacion == APP_EXTERNAL_POLL ) {
		tkInputs_external_poll();

	} else if ( sVarsApp.aplicacion == APP_MODBUS ) {
		tkInputs_modbus();

	} else {
		tkInputs_normal();
	}
}
//------------------------------------------------------------------------------------
void tkInputs_external_poll(void)
{
	// Espero la señal de poleo.

    // Creo y arranco el timer que va a contar las entradas digitales.
    ainputs_init();
    dinputs_init();
    counters_init();
    psensor_init();
    tempsensor_init();
    range_init();

	xprintf_P( PSTR("starting tkInputs External Poll..\r\n\0"));
	// loop
	for( ;; ) {

		ctl_watchdog_kick(WDG_DINPUTS , WDG_DIN_TIMEOUT);

		if ( SPX_SIGNAL( SGN_POLL_NOW )) {
			SPX_CLEAR_SIGNAL( SGN_POLL_NOW );
			xprintf_PD(DF_APP,"TKINPUTS: SSGN_POLL_NOW rcvd.\r\n\0");

			xprintf_P(PSTR("ExtPoll:"));

			// Poleo si no estoy en autocal
	 		if ( ! ainputs_autocal_running() ) {
	 			data_read_inputs(&dataRecd, false);
	 			data_print_inputs(fdTERM, &dataRecd, 0);
	 			pv_data_guardar_en_BD();

	 			// Aviso a tkGprs que hay un frame listo. En modo continuo lo va a trasmitir enseguida.
	 			if ( ! MODO_DISCRETO ) {
	 				SPX_SEND_SIGNAL( SGN_FRAME_READY );
	 			}

	 			// Dejo el range en una variable de intercambio con la aplicacion de caudalimetro
	 			sVarsApp.caudalimetro.range_actual = dataRecd.df.io5.range;
	 		}

		}

		vTaskDelay( ( TickType_t)( 1000 / portTICK_RATE_MS ) );
	}
}
//------------------------------------------------------------------------------------
void tkInputs_normal(void)
{

TickType_t xLastWakeTime = 0;
uint32_t waiting_ticks = 0;

	// Creo y arranco el timer que va a contar las entradas digitales.
	ainputs_init();
	dinputs_init();
	counters_init();
	psensor_init();
	tempsensor_init();
	range_init();

	xprintf_P( PSTR("starting tkInputs Normal..\r\n"));

	// Initialise the xLastWakeTime variable with the current time.
 	 xLastWakeTime = xTaskGetTickCount();

 	 // Al arrancar poleo a los 10s
 	 waiting_ticks = (uint32_t)(10) * 1000 / portTICK_RATE_MS;

 	// loop
 	for( ;; ) {

 		ctl_watchdog_kick(WDG_DINPUTS , WDG_DIN_TIMEOUT);

 		// Espero. Da el tiempo necesario para entrar en tickless.
 		vTaskDelayUntil( &xLastWakeTime, waiting_ticks );

 		// Poleo si no estoy en autocal
 		if ( ! ainputs_autocal_running() ) {
			data_read_inputs(&dataRecd, false);
 			data_print_inputs(fdTERM, &dataRecd, 0);
 			pv_data_guardar_en_BD();

 			// Aviso a tkGprs que hay un frame listo. En modo continuo lo va a trasmitir enseguida.
 			if ( ! MODO_DISCRETO ) {
				SPX_SEND_SIGNAL( SGN_FRAME_READY );
 			}

 			// Dejo el range en una variable de intercambio con la aplicacion de caudalimetro
 			sVarsApp.caudalimetro.range_actual = dataRecd.df.io5.range;
 		}

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
void tkInputs_modbus(void)
{

TickType_t xLastWakeTime = 0;
uint32_t waiting_ticks = 0;

	modbus_init();

	xprintf_P( PSTR("starting tkInputs Modbus..\r\n"));

	// Initialise the xLastWakeTime variable with the current time.
 	 xLastWakeTime = xTaskGetTickCount();

 	 // Al arrancar poleo a los 10s
 	 waiting_ticks = (uint32_t)(10) * 1000 / portTICK_RATE_MS;

 	// loop
 	for( ;; ) {

 		ctl_watchdog_kick(WDG_DINPUTS , WDG_DIN_TIMEOUT);

 		// Espero. Da el tiempo necesario para entrar en tickless.
 		vTaskDelayUntil( &xLastWakeTime, waiting_ticks );

		// Poleo
		data_read_inputs(&dataRecd, false);
		data_print_inputs(fdTERM, &dataRecd, 0);
		pv_data_guardar_en_BD();

		// Aviso a tkGprs que hay un frame listo. En modo continuo lo va a trasmitir enseguida.
		if ( ! MODO_DISCRETO ) {
			SPX_SEND_SIGNAL( SGN_FRAME_READY );
		}

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
void data_read_inputs(st_dataRecord_t *dst, bool f_copy )
{
	if ( sVarsApp.aplicacion == APP_MODBUS ) {
		data_read_inputs_modbus( dst, f_copy );
	} else {
		data_read_inputs_normal( dst, f_copy );
	}
}
//------------------------------------------------------------------------------------
void data_read_inputs_normal(st_dataRecord_t *dst, bool f_copy )
{
	// Leo las entradas digitales sobre la estructura local din.
	// En los IO5 el sensor de temperatura y presion actuan juntos !!!!.

int8_t xBytes = 0;

	// Solo copio el buffer. No poleo.
	if ( f_copy ) {
		memcpy(dst, &dataRecd, sizeof(dataRecd));
		return;
	}

	// Poleo.
	switch(spx_io_board) {
	case SPX_IO5CH:
		dinputs_read( dst->df.io5.dinputs );
		// dinputs y counters operan en background por lo que usan variables locales
		// que deben ser borradas apenas las leo.
		dinputs_clear();

		counters_read( dst->df.io5.counters );
		counters_clear();

		ainputs_read( dst->df.io5.ainputs, &dst->df.io5.battery );

		if ( strcmp( systemVars.psensor_conf.name, "X" ) != 0 ) {
			psensor_read( &dst->df.io5.psensor );
			tempsensor_read( &dst->df.io5.temp );
		}

		range_read( &dst->df.io5.range );
		break;

	case SPX_IO8CH:
		dinputs_read( dst->df.io8.dinputs );
		dinputs_clear();

		counters_read( dst->df.io8.counters );
		counters_clear();

		ainputs_read( dst->df.io8.ainputs, &battery );

		// Si mide bateria, el canal 7 es el que la lleva
		if ( systemVars.mide_bateria)
			 dst->df.io8.ainputs[7] = battery;

		// En el caso de la aplicacion PLANTAPOT debo ajustar los valores de las
		// entradas analogicas, digitales y contadores para que reflejen el estado de las alarmas

		if ( sVarsApp.aplicacion == APP_PLANTAPOT ) {
			xAPP_plantapot_adjust_vars(dst);
		}

		break;
	}

	// Agrego el timestamp
	xBytes = RTC_read_dtime( &dst->rtc );
	if ( xBytes == -1 )
		xprintf_P(PSTR("ERROR: I2C:RTC:data_read_inputs\r\n\0"));


}
//------------------------------------------------------------------------------------
void data_read_inputs_modbus(st_dataRecord_t *dst, bool f_copy )
{
	// Leo las entradas digitales sobre la estructura local din.
	// En los IO5 el sensor de temperatura y presion actuan juntos !!!!.

int8_t xBytes = 0;
uint8_t channel;

	// Solo copio el buffer. No poleo.
	if ( f_copy ) {
		memcpy(dst, &dataRecd, sizeof(dataRecd));
		return;
	}

	// Poleo.
	for (channel = 0; channel < MODBUS_CHANNELS; channel++) {
		// No poleo los canales no configurados
		if ( ! strcmp ( systemVars.modbus_conf.mbchannel[channel].name, "X" ) )
			continue;

		modbus_poll_channel( DF_MODBUS, channel, &dst->df.mbus.mbinputs[channel] );

	}

	// Agrego el timestamp
	xBytes = RTC_read_dtime( &dst->rtc );
	if ( xBytes == -1 )
		xprintf_P(PSTR("ERROR: I2C:RTC:data_read_inputs\r\n\0"));


}
//------------------------------------------------------------------------------------
void data_print_inputs(file_descriptor_t fd, st_dataRecord_t *dr, uint16_t ctl )
{

	if ( sVarsApp.aplicacion == APP_MODBUS ) {
		data_print_inputs_modbus( fd, dr, ctl );
	} else {
		data_print_inputs_normal( fd, dr, ctl );
	}
}
//------------------------------------------------------------------------------------
void data_print_inputs_normal(file_descriptor_t fd, st_dataRecord_t *dr, uint16_t ctl )
{

	// timeStamp.
	xfprintf_P(fd, PSTR("CTL:%d;DATE:%02d"),ctl, dr->rtc.year );
	xfprintf_P(fd, PSTR("%02d%02d;"),dr->rtc.month, dr->rtc.day );

	xfprintf_P(fd, PSTR("TIME:%02d"), dr->rtc.hour );
	xfprintf_P(fd, PSTR("%02d%02d;"), dr->rtc.min, dr->rtc.sec );

	switch(spx_io_board) {
	case SPX_IO5CH:
		ainputs_print( fd, dr->df.io5.ainputs );
		dinputs_print( fd, dr->df.io5.dinputs );
		counters_print( fd, dr->df.io5.counters );

		if ( strcmp( systemVars.psensor_conf.name, "X" ) != 0 ) {
			psensor_print( fd, dr->df.io5.psensor );
			tempsensor_print( fd, dr->df.io5.temp );
		}

		range_print( fd, dr->df.io5.range );

		ainputs_battery_print( fd, dr->df.io5.battery );
		break;
	case SPX_IO8CH:
		ainputs_print( fd, dr->df.io8.ainputs );
		dinputs_print( fd, dr->df.io8.dinputs );
		counters_print( fd, dr->df.io8.counters );
		break;
	}

	// TAIL
	// Esto es porque en gprs si mando un cr corto el socket !!!
	if ( fd == fdTERM ) {
		xfprintf_P(fd, PSTR("\r\n\0") );
	}
}
//------------------------------------------------------------------------------------
void data_print_inputs_modbus(file_descriptor_t fd, st_dataRecord_t *dr, uint16_t ctl )
{

uint8_t i = 0;

	// timeStamp.
	xfprintf_P(fd, PSTR("CTL:%d;DATE:%02d"),ctl, dr->rtc.year );
	xfprintf_P(fd, PSTR("%02d%02d;"),dr->rtc.month, dr->rtc.day );

	xfprintf_P(fd, PSTR("TIME:%02d"), dr->rtc.hour );
	xfprintf_P(fd, PSTR("%02d%02d;"), dr->rtc.min, dr->rtc.sec );

	for ( i = 0; i < MODBUS_CHANNELS; i++) {

		// Si no esta definido, salteo
		if ( strcmp ( systemVars.modbus_conf.mbchannel[i].name, "X" ) == 0 )
			continue;

		// Si hubo error de lectura, muestro NaN y salteo
		if ( strcmp ( dr->df.mbus.mbinputs[i].rep_str, "NaN") == 0 ) {
			xfprintf_P(fd, PSTR("%s:NaN;"), systemVars.modbus_conf.mbchannel[i].name );
			continue;
		}

		// Si es float, muestro y salteo
		if ( systemVars.modbus_conf.mbchannel[i].type == 'F') {
			xfprintf_P(fd, PSTR("%s:%.02f;"), systemVars.modbus_conf.mbchannel[i].name, dr->df.mbus.mbinputs[i].fvalue );
			continue;
		}

		// Si es entero con 2 bytes imprimo con %d y salteo
		if ( ( systemVars.modbus_conf.mbchannel[i].type == 'I') && ( systemVars.modbus_conf.mbchannel[i].length == 1 ) ) {
			xfprintf_P(fd, PSTR("%s:%d;"), systemVars.modbus_conf.mbchannel[i].name, (uint16_t) (dr->df.mbus.mbinputs[i].ivalue ) );
		}

		// Si es entero con 4 bytes imprimo con %lu y salteo
		if ( ( systemVars.modbus_conf.mbchannel[i].type == 'I') && ( systemVars.modbus_conf.mbchannel[i].length == 2 ) ) {
			xfprintf_P(fd, PSTR("%s:%lu;"), systemVars.modbus_conf.mbchannel[i].name, dr->df.mbus.mbinputs[i].ivalue );
		}

	}

	// TAIL
	// Esto es porque en gprs si mando un cr corto el socket !!!
	if ( fd == fdTERM ) {
		xfprintf_P(fd, PSTR("\r\n\0") );
	}
}
//------------------------------------------------------------------------------------
// FUNCIONES PRIVADAS
//------------------------------------------------------------------------------------
static void pv_data_guardar_en_BD(void)
{

int8_t bytes_written = 0;
static bool primer_frame = true;
FAT_t fat;

	memset( &fat, '\0', sizeof(FAT_t));

	// Para no incorporar el error de los contadores en el primer frame no lo guardo.
	if ( primer_frame ) {
		primer_frame = false;
		return;
	}

	// Guardo en BD
	bytes_written = FF_writeRcd( &dataRecd, sizeof(st_dataRecord_t) );

	if ( bytes_written == -1 ) {
		// Error de escritura o memoria llena ??
		xprintf_P(PSTR("DATA: WR ERROR (%d)\r\n\0"),FF_errno() );
		// Stats de memoria
		FAT_read(&fat);
		xprintf_P( PSTR("DATA: MEM [wr=%d,rd=%d,del=%d]\0"), fat.wrPTR,fat.rdPTR,fat.delPTR );
	}

}
//------------------------------------------------------------------------------------
