/*
 * spx_tkSistema.c
 *
 *  Created on: 23 oct. 2019
 *      Author: pablo
 */


#include "spx.h"
#include "tkApp.h"

//------------------------------------------------------------------------------------
void tkAplicacion(void * pvParameters)
{

( void ) pvParameters;

	// Espero la notificacion para arrancar
	while ( !startTask )
		vTaskDelay( ( TickType_t)( 100 / portTICK_RATE_MS ) );

	vTaskDelay( ( TickType_t)( 5000 / portTICK_RATE_MS ) );
	xprintf_P( PSTR("starting tkAplicacion..\r\n\0"));

	ctl_watchdog_kick( WDG_APP,  WDG_APP_TIMEOUT );

	// Inicializo los dispositivos
	switch (spx_io_board ) {
	case SPX_IO5CH:
		DRV8814_init();
		break;
	case SPX_IO8CH:
		MCP_init();
		break;
	}

	// De acuedo al modo de operacion disparo la tarea que realiza la
	// funcion especializada del datalogger.
	spiloto.start_presion_test = false;
	spiloto.start_steppers_test = false;


	switch( sVarsApp.aplicacion ) {
	case APP_OFF:
		// Es el caso en que no debo hacer nada con las salidas.
		// Duermo 25s para entrar en pwrdown.
		tkApp_off();
		break;
	case APP_CONSIGNA:
		tkApp_consigna();
		break;
	case APP_PERFORACION:
		tkApp_perforacion();
		break;
	case APP_PLANTAPOT:
		tkApp_plantapot();
		break;
	case APP_CAUDALIMETRO:
		tkApp_caudalimetro();
		break;
	case APP_EXTERNAL_POLL:
		tkApp_external_poll();
		break;
	case APP_PILOTO:
		tkApp_piloto();
		break;
	case APP_MODBUS:
		tkApp_modbus();
		break;
	default:
		break;
	}

	tkApp_off();
}
//------------------------------------------------------------------------------------
void xAPP_sms_checkpoint(void)
{
	/*
	 * Los SMS los chequeamos solo en las aplicaciones que los usan
	 */

	if ( sVarsApp.aplicacion == APP_PLANTAPOT ) {
		// Checkpoint de SMS's
		xSMS_txcheckpoint();
		xSMS_rxcheckpoint();
	}
}
//------------------------------------------------------------------------------------
void xAPP_set_douts( uint8_t dout, uint8_t mask )
{
	// Funcion para setear el valor de las salidas desde el resto del programa.
	// La usamos desde tkGprs cuando en un frame nos indican cambiar las salidas.
	// Como el cambio depende de quien tiene el control y del timer, aqui vemos si
	// se cambia o se ignora.
	// La mascara se aplica luego del twidle.

uint8_t data = 0;
int8_t xBytes = 0;

	// Solo es para IO8CH
	if ( spx_io_board != SPX_IO8CH ) {
		return;
	}

	// Vemos que no se halla desconfigurado
	MCP_check();

	// Guardo el valor recibido
	data = dout;

	// Si el valor no cambia, no lo seteo para que no suenen los relé.
	if ( sVarsApp.perforacion.outs == dout ) {
		xprintf_P( PSTR("APP: SET OUTPUTS(0x%02x): No cambio valor. Exit \r\n\0"),dout);
		return;
	}

	sVarsApp.perforacion.outs = dout;
	MCP_update_olatb( dout );

	// Invierto el byte antes de escribirlo !!!
	data = twiddle_bits(data);

	data = data & mask;

	xBytes = MCP_write(MCP_OLATB, (char *)&data, 1 );
	if ( xBytes == -1 ) {
		xprintf_P(PSTR("APP: ERROR: set_douts MCP write\r\n\0"));
		return;
	}

	xprintf_P( PSTR("APP: SET OUTPUTS to 0x%02x\r\n\0"),dout);
}
//------------------------------------------------------------------------------------
void xAPP_set_douts_lazy( uint8_t dout, uint8_t mask )
{
	// Funcion para setear el valor de las salidas desde el resto del programa.
	// La usamos desde tkGprs cuando en un frame nos indican cambiar las salidas.
	// Como el cambio depende de quien tiene el control y del timer, aqui vemos si
	// se cambia o se ignora.
	// La mascara se aplica luego del twidle.

uint8_t data = 0;
int8_t xBytes = 0;
//uint8_t i;
//uint8_t odata, msk;

	// Solo es para IO8CH
	if ( spx_io_board != SPX_IO8CH ) {
		return;
	}

	// Vemos que no se halla desconfigurado
	MCP_check();

	// Guardo el valor recibido
	data = dout;

	/*
	 * R3.0.6g @ 2021-02-10.
	// Si el valor no cambia, no lo seteo para que no suenen los relé.
	if ( sVarsApp.perforacion.outs == dout ) {
		xprintf_P( PSTR("APP: SET OUTPUTS(0x%02x): No cambio valor. Exit \r\n\0"),dout);
		return;
	}
	*/

	sVarsApp.perforacion.outs = dout;
	MCP_update_olatb( dout );

	// Invierto el byte antes de escribirlo !!!
	data = twiddle_bits(data);

	data = data & mask;

	/*
	 * R3.0.6g @ 2021-02-10
	// Escribo los bytes de a 1 bit
	msk = 0x01;
	for (i=0; i<8; i++) {
		msk = msk | (0x1 << i );
		if ( (i > 0) && ( odata == (data & msk) ) )
			continue;
		odata = data & msk;
		xBytes = MCP_write(MCP_OLATB, (char *)&odata, 1 );
		xprintf_PD( DF_COMMS, PSTR("APP: Set douts lazy: data=0x%02x, i=%02d, msk=0x%02x, odata=0x%02x\r\n"), data, i, msk, odata);
		if ( xBytes == -1 ) {
			xprintf_P(PSTR("APP: ERROR: set_douts MCP write(%d)[%d]\r\n\0"), i, odata);
			return;
		}
		vTaskDelay( ( TickType_t)( 2000 / portTICK_RATE_MS ) );
	}

	*/

	xBytes = MCP_write(MCP_OLATB, (char *)&data, 1 );
	if ( xBytes == -1 ) {
		xprintf_P(PSTR("APP: ERROR: set_douts MCP write\r\n\0"));
		return;
	}

	xprintf_P( PSTR("APP: SET OUTPUTS to 0x%02x\r\n\0"),dout);
}
//------------------------------------------------------------------------------------
void xAPP_set_douts_remote( uint8_t dout )
{
	// El GPRS recibio datos de setear la salida.
	// El control debe ser de SISTEMA y reiniciar el timer_SISTEMA

	if ( sVarsApp.aplicacion == APP_PERFORACION ) {
		xAPP_perforacion_adjust_x_douts(dout);
	}

	xAPP_set_douts( dout, MASK_NORMAL );
}
//------------------------------------------------------------------------------------
void xAPP_set_douts_remote_lazy( uint8_t dout )
{
	// El GPRS recibio datos de setear la salida.
	// El control debe ser de SISTEMA y reiniciar el timer_SISTEMA

	if ( sVarsApp.aplicacion == APP_PERFORACION ) {
		xAPP_perforacion_adjust_x_douts(dout);
	}

	xAPP_set_douts_lazy( dout, MASK_NORMAL );
}
//------------------------------------------------------------------------------------
