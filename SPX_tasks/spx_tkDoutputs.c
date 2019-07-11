/*
 * spx_tkOutputs.c
 *
 *  Created on: 20 dic. 2018
 *      Author: pablo
 *
 *  En la tarea de salidas implementamos consignas para la placa IO5 y niveles para IO8
 *
 *  El SPX_IO8 solo implementa PERFORACIONES. El SPX_IO5 implementa ademas CONSIGNAS y PILOTOS.
 *
 *  TIMER_BOYA: Estando en modo BOYA, cuando expira lo recargo y re-escribo las salidas.
 *
 */


#include "spx.h"
#include "gprs.h"


static void tkDoutputs_init(void);
static void tk_doutputs_init_none(void);
static void tk_doutputs_none(void);

bool doutputs_reinit = false;

//------------------------------------------------------------------------------------
void tkDoutputs(void * pvParameters)
{

( void ) pvParameters;

	// Espero la notificacion para arrancar
	while ( !startTask )
		vTaskDelay( ( TickType_t)( 100 / portTICK_RATE_MS ) );

	xprintf_P( PSTR("starting tkOutputs..\r\n\0"));

	// El setear una consigna toma 10s de carga de condensadores por lo que debemos
	// evitar que se resetee por timeout del wdg.
	ctl_watchdog_kick( WDG_DOUT,  WDG_DOUT_TIMEOUT );

	//pv_tkDoutputs_init(); // Lo paso a tkCTL
	tkDoutputs_init();

	// loop
	for( ;; )
	{

		ctl_watchdog_kick( WDG_DOUT,  WDG_DOUT_TIMEOUT );

		// Si cambio la configuracion debo reiniciar las salidas
		if ( doutputs_reinit ) {
			doutputs_reinit = false;
			tkDoutputs_init();
		}

		switch( systemVars.doutputs_conf.modo ) {
		case NONE:
			tk_doutputs_none();
			break;
		case CONSIGNA:
			tk_consigna();
			break;
		case PERFORACIONES:
			tk_perforaciones();
			break;
		case PILOTOS:
			tk_pilotos();
			break;
		}

	}

}
//------------------------------------------------------------------------------------
static void tkDoutputs_init(void)
{

	if ( spx_io_board == SPX_IO5CH ) {
		DRV8814_init();
	}

	if ( spx_io_board == SPX_IO8CH ) {
		MCP_init();
	}

	switch( systemVars.doutputs_conf.modo ) {
	case NONE:
		tk_doutputs_init_none();
		break;
	case CONSIGNA:
		tk_init_consigna();
		break;
	case PERFORACIONES:
		tk_init_perforaciones();
		break;
	case PILOTOS:
		tk_init_pilotos();
		break;
	}
}
//------------------------------------------------------------------------------------
static void tk_doutputs_init_none(void)
{
	// Configuracion inicial cuando las salidas estan en NONE

	if ( spx_io_board == SPX_IO5CH ) {
		DRV8814_sleep_pin(0);
		DRV8814_power_off();
		return;
	}

	if ( spx_io_board == SPX_IO8CH ) {
		perforaciones_set_douts( 0x00 );	// Setup dout.
	}

}
//------------------------------------------------------------------------------------
static void tk_doutputs_none(void)
{
	// Es el caso en que no debo hacer nada con las salidas.
	// Duermo 25s para entrar en pwrdown.

	vTaskDelay( ( TickType_t)( 25000 / portTICK_RATE_MS ) );

}
//------------------------------------------------------------------------------------
