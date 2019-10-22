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

static void tkSistema_init(void);

//------------------------------------------------------------------------------------
void tkSistema(void * pvParameters)
{

( void ) pvParameters;

	// Espero la notificacion para arrancar
	while ( !startTask )
		vTaskDelay( ( TickType_t)( 100 / portTICK_RATE_MS ) );

	xprintf_P( PSTR("starting tkSistema..\r\n\0"));

	// El setear una consigna toma 10s de carga de condensadores por lo que debemos
	// evitar que se resetee por timeout del wdg.
	ctl_watchdog_kick( WDG_SYSTEM,  WDG_SYSTEM_TIMEOUT );

	//pv_tkDoutputs_init(); // Lo paso a tkCTL
	tkSistema_init();

	// loop
	for( ;; )
	{

		ctl_watchdog_kick( WDG_SYSTEM,  WDG_SYSTEM_TIMEOUT );

		switch( systemVars.doutputs_conf.modo ) {
		case OFF:
			// Es el caso en que no debo hacer nada con las salidas.
			// Duermo 25s para entrar en pwrdown.
			vTaskDelay( ( TickType_t)( 25000 / portTICK_RATE_MS ) );
			break;
		case CONSIGNA:
			consigna_stk();
			break;
		case PERFORACIONES:
			perforaciones_stk();
			break;
		case PILOTOS:
			piloto_stk();
			break;
		}

	}

}
//------------------------------------------------------------------------------------
static void tkSistema_init(void)
{

	// Inicializo los dispositivos
	if ( spx_io_board == SPX_IO5CH ) {
		DRV8814_init();
	}

	if ( spx_io_board == SPX_IO8CH ) {
		MCP_init();
	}

	// Inicializo las funciones
	switch( systemVars.doutputs_conf.modo ) {
	case OFF:
		switch (spx_io_board) {
		case SPX_IO5CH:
			DRV8814_sleep_pin(0);
			DRV8814_power_off();
			break;
		case SPX_IO8CH:
			perforaciones_set_douts( 0x00 );	// Setup dout.
			break;
		}
		break;
	case CONSIGNA:
		consigna_init();
		break;
	case PERFORACIONES:
		perforaciones_init();
		break;
	case PILOTOS:
		piloto_init();
		break;
	}
}
//------------------------------------------------------------------------------------
