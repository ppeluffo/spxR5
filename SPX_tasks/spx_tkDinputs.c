/*
 * spx_tkDinputs.c
 *
 *  Created on: 17 dic. 2018
 *      Author: pablo
 *
 *  Esta tarea atiende las entradas digitales.
 *  Dado que en las placas SPXIO8 las entradas superiores (4..7) pueden operar
 *  como entradas de nivel o timers, es necesario una tarea solo para este ultimo
 *  caso en particular ya que debe leer el nivel en forma sincrónica cada 100ms y contar
 *  los ticks que esta la linea alta.
 *  Si no fuese por esto, no necesitaríamos implementar una tarea.
 *
 */

#include "spx.h"

// Configuracion de canales digitales
typedef struct {
	char name[MAX_DINPUTS_CHANNELS][PARAMNAME_LENGTH];
	bool timers_enabled;
} dinputs_t;

dinputs_t din_conf;	// Estructura con la configuracion de las entradas digitales

uint16_t din_values[MAX_DINPUTS_CHANNELS];	// Valor de las medidas

static void pv_tkDinputs_init(void);

#define WDG_DIN_TIMEOUT	30

//------------------------------------------------------------------------------------
void tkDinputs(void * pvParameters)
{
	// C/100ms leo las entradas digitales.
	// Las que estan en 1 sumo 1 al contador.
	// En UTE dividimos las entradas digitales en 2: aquellas que solo miden nivel logico (0..3)
	// y las que miden el tiempo que estan en 1 ( 4..7)
	// En modo SPX solo mido el nivel logico

( void ) pvParameters;
uint32_t waiting_ticks;
TickType_t xLastWakeTime;
uint8_t din_port;
uint8_t i;

	// Espero la notificacion para arrancar
	while ( !startTask )
		vTaskDelay( ( TickType_t)( 100 / portTICK_RATE_MS ) );

	xprintf_P( PSTR("starting tkDigital..\r\n\0"));

	pv_tkDinputs_init();

	// Initialise the xLastWakeTime variable with the current time.
	xLastWakeTime = xTaskGetTickCount();

	// Cuento de a 100ms
	waiting_ticks = (uint32_t)(100) / portTICK_RATE_MS;

	// loop
	for( ;; )
	{

		ctl_watchdog_kick( WDG_DIN,  WDG_DIN_TIMEOUT );

		// Solo en la placa IO8 y con los timers habilitados
		if ( ( spx_io_board == SPX_IO8CH ) &&  ( din_conf.timers_enabled ) ) {

			// Cada 100 ms leo las entradas digitales. fmax=10Hz
			// vTaskDelay( ( TickType_t)( 100 / portTICK_RATE_MS ) );
			// Hago de cuenta que todos son timers pero al final solo paso el valor de
			// los DIN4..DIN7

			vTaskDelayUntil( &xLastWakeTime, waiting_ticks );

			din_port = DIN_read_port();
			for ( i = 0 ; i < NRO_DINPUTS; i++ ) {
				// Si esta HIGH incremento en 1 tick.
				if ( ( (din_port & ( 1 << i )) >> i ) == 1 ) {
					din_values[i]++;	// Si esta HIGH incremento en 1 tick.
				}
			}

		} else {

			// En modo SPX_3CH no tengo entradas digitales.
			// Espero para entrar en tickless
			vTaskDelay( ( TickType_t)( 25000 / portTICK_RATE_MS ) );
		}
	}
}
//------------------------------------------------------------------------------------
static void pv_tkDinputs_init(void)
{

uint8_t i;

	// Dado que los pines son diferentes para c/board, la inicializacion
	// depende de la board detectada.
	DINPUTS_init( spx_io_board );

	for ( i = 0; i < MAX_DINPUTS_CHANNELS; i++ ) {
		din_values[i] = 0;
	}
}
//------------------------------------------------------------------------------------
// FUNCIONES PUBLICAS
//------------------------------------------------------------------------------------
int16_t dinputs_read ( uint8_t din )
{

uint8_t port;
int16_t retVal = -1;

	// Esta funcion la invoca tkData al completar un frame para agregar los datos
	// digitales.
	// Leo los niveles de las entradas digitales y copio a dframe.

	switch (spx_io_board ) {
	case SPX_IO5CH:
		// Leo los canales digitales.
		if ( din < 2 ) {
			retVal = DIN_read_pin( din, SPX_IO5CH );
		}
		break;

	case SPX_IO8CH:

		port = DIN_read_port();	// Leo el puerto para tener los niveles logicos.
		//xprintf_P( PSTR("DEBUG DIN: 0x%02x [%c%c%c%c%c%c%c%c]\r\n\0"), port , BYTE_TO_BINARY( port ));

		if ( din < 4 ) {
			// Los 4 canales inferiores son solo entradas de nivel.
			retVal = ( port & ( 1 << din )) >> din;

		} else {

			// Canales de NIVEL
			if ( din_conf.timers_enabled == false ) {
				retVal = ( port & ( 1 << din )) >> din;

			} else {
				// Canales timers
				// D4..D7 son contadores de tiempo High.
				// Puede haber un drift en el timerpoll lo que generaria un falso error
				// Para evitarlo, controlo que los ticks no puedan ser mayor que el timerpoll
				if ( din_values[din] > systemVars.timerPoll * 10 ) {
					// Corrijo
					din_values[din] = systemVars.timerPoll * 10;
				}
				retVal = din_values[din];
				din_values[din] = 0;
			}
		}
		break;
	}

	return(retVal);

}
//------------------------------------------------------------------------------------
bool dinputs_config_channel( uint8_t channel,char *s_aname )
{

	// Configura los canales digitales. Es usada tanto desde el modo comando como desde el modo online por gprs.

bool retS = false;

	u_control_string(s_aname);

	if ( ( channel >=  0) && ( channel < NRO_DINPUTS ) ) {
		snprintf_P( din_conf.name[channel], PARAMNAME_LENGTH, PSTR("%s\0"), s_aname );
		retS = true;
	}

	return(retS);
}
//------------------------------------------------------------------------------------
bool dinputs_config_timermode ( char *s_mode )
{

	// Esta opcion es solo valida para IO8
	if ( spx_io_board != SPX_IO8CH )
		return(false);

	if ( !strcmp_P( strupr(s_mode), PSTR("ON\0"))) {
		din_conf.timers_enabled = true;
		return(true);
	} else if ( !strcmp_P( strupr(s_mode), PSTR("OFF\0"))) {
		din_conf.timers_enabled = false;
		return(true);
	}
	return(false);

}
//------------------------------------------------------------------------------------
bool dinputs_get_mode(void)
{
	return(din_conf.timers_enabled);
}
//------------------------------------------------------------------------------------
char * dinputs_get_name(uint8_t din )
{
	return(din_conf.name[din]);
}
//------------------------------------------------------------------------------------
void dinputs_config_defaults(void)
{
	// Realiza la configuracion por defecto de los canales digitales.

uint8_t i;

	din_conf.timers_enabled = false;

	for ( i = 0; i < MAX_DINPUTS_CHANNELS; i++ ) {
		snprintf_P( din_conf.name[i], PARAMNAME_LENGTH, PSTR("D%d\0"), i );
	}

}
//------------------------------------------------------------------------------------



