/*
 * l_perforaciones.c
 *
 *  Created on: 8 jul. 2019
 *      Author: pablo
 */

#include "spx.h"

#define TIMEOUT_O_TIMER_BOYA	 60
#define TIMEOUT_O_TIMER_SISTEMA	 600

uint8_t o_control = 0;
uint16_t o_timer_boya = 0;
uint16_t o_timer_sistema = 0;

void perforaciones_RELOAD_TIMER_SISTEMA(void);
void perforaciones_RELOAD_TIMER_BOYA(void);
void perforaciones_STOP_TIMER_BOYA(void);
void perforaciones_STOP_TIMER_SISTEMA(void);

//------------------------------------------------------------------------------------
void tk_init_perforaciones(void)
{
	// Inicializa las salidas con el modo de trabajo PERFORACIONES.
	// Puede ser en cualquiera de las ioboards

	// AL iniciar el sistema debo leer el valor que tiene el MCP.OLATB
	// y activar las salidas con este valor. El control depende del valor del MCP.

int xBytes = 0;
uint8_t data = 0;

	if ( spx_io_board != SPX_IO8CH ) {
		xprintf_P(PSTR("DOUTPUTS Init ERROR: Perforaciones only in IO_8CH.\r\n\0"));
		systemVars.doutputs_conf.modo = NONE;
		return;
	}

	// Activacion de salidas en 8CH
	// Leo el OLATB y pongo la salida que tenia
	if ( spx_io_board == SPX_IO8CH ) {
		xBytes = MCP_read( MCP_OLATB, (char *)&data, 1 );
		if ( xBytes == -1 ) {
			xprintf_P(PSTR("OUTPUTS INIT ERROR: I2C:MCP:pv_cmd_rwMCP\r\n\0"));
			data = 0x00;
		}

		if ( xBytes > 0 ) {
			xprintf_P( PSTR( "OUTPUTS INIT OK: VALUE=0x%x\r\n\0"),data);
		}

		data = twiddle_bits(data);

		// Tengo el OLATB: Si el bit 0 y 2 son 0 estoy en modo boya
		if ( ( data & 0x5 ) == 0 ) {
			// Modo BOYA
			o_control = CTL_BOYA;
			systemVars.doutputs_conf.perforacion.control = CTL_BOYA;
			perforaciones_RELOAD_TIMER_BOYA();
		} else {
			// Modo SISTEMA
			o_control = CTL_SISTEMA;
			systemVars.doutputs_conf.perforacion.control = CTL_SISTEMA;
			perforaciones_RELOAD_TIMER_SISTEMA();
		}

		// Pongo las salidas que ya tenia.
		perforaciones_set_douts( data );
		return;
	}

	// Activacion de salidas en 5CH
	// No tengo MCP de modo que las salidas las mantiene el micro y se inicializan en 0.
//	if ( spx_io_board == SPX_IO5CH ) {
//		DRV8814_init();
//		DRV8814_sleep_pin(0);
//		DRV8814_power_off();
//		return;
//	}

}
//------------------------------------------------------------------------------------
void tk_perforaciones(void)
{
	// Corre en los 2 ioboards. El control se hace a c/segundo por lo tanto en SPX_IO5
	// no puede entrar en pwrsave !!!

	// El control a las BOYAS se pasa aqui estando en modo SISTEMA, si expiro el timer de los datos.
	// El control al SISTEMA los pasa el recibir datos de GPRS.

	vTaskDelay( ( TickType_t)( 1000 / portTICK_RATE_MS ) );

	switch ( o_control ) {
	case CTL_BOYA:
		// Cuando el timer de boya expira, lo recargo y re-escribo las salidas
		// Solo paso a modo SISTEMA cuando recibo un dato desde el GPRS !!!
		if ( o_timer_boya > 0 ) {
			o_timer_boya--;
			if ( o_timer_boya == 0 ) {
				perforaciones_RELOAD_TIMER_BOYA();
				perforaciones_set_douts ( systemVars.doutputs_conf.perforacion.outs );
				xprintf_P( PSTR("MODO BOYA: reload timer boya. DOUTS=0x%0X\r\n\0"), systemVars.doutputs_conf.perforacion.outs );
			}
		}
		break;

	case CTL_SISTEMA:
		// disminuyo el timer
		if ( o_timer_sistema > 0 ) {
			o_timer_sistema--;
			if ( o_timer_sistema == 0 ) {
				// Expiro: Paso el control a modo BOYA y las salidas a 0x00
				perforaciones_set_douts( 0x00 );
				o_control = CTL_BOYA;			// Paso el control a las boyas.
				perforaciones_RELOAD_TIMER_BOYA();	// Arranco el timer de las boyas
				systemVars.doutputs_conf.perforacion.control = CTL_BOYA;
				xprintf_P( PSTR("OUTPUT CTL to BOYAS !!!. (set outputs to 0x00)\r\n\0"));
			}
		}
		break;

	default:
		// Paso a control de boyas
		perforaciones_set_douts( 0x00 );
		o_control = CTL_BOYA;			// Paso el control a las boyas.
		perforaciones_RELOAD_TIMER_BOYA();	// Arranco el timer de las boyas
		systemVars.doutputs_conf.perforacion.control = CTL_BOYA;
		xprintf_P( PSTR("ERROR Control outputs: Pasa a BOYA !!\r\n\0"));
		break;
	}

}
//------------------------------------------------------------------------------------
void perforaciones_set_douts( uint8_t dout )
{
	// Funcion para setear el valor de las salidas desde el resto del programa.
	// La usamos desde tkGprs cuando en un frame nos indican cambiar las salidas.
	// Como el cambio depende de quien tiene el control y del timer, aqui vemos si
	// se cambia o se ignora.

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
	systemVars.doutputs_conf.perforacion.outs = dout;
	MCP_update_olatb( systemVars.doutputs_conf.perforacion.outs );

	// Invierto el byte antes de escribirlo !!!
	data = twiddle_bits(data);
	xBytes = MCP_write(MCP_OLATB, (char *)&data, 1 );
	if ( xBytes == -1 ) {
		xprintf_P(PSTR("ERROR: perforaciones_set_douts MCP write\r\n\0"));
		return;
	}

	xprintf_P( PSTR("perforaciones_set_douts (set outputs to 0x%02x)\r\n\0"),dout);
}
//------------------------------------------------------------------------------------
void perforaciones_set_douts_from_gprs( uint8_t dout )
{
	// El GPRS recibio datos de setear la salida.
	// El control debe ser de SISTEMA y reiniciar el timer_SISTEMA

	if ( o_control == CTL_BOYA ) {
		xprintf_P( PSTR("OUTPUT CTL to SISTEMA !!!. (set outputs to 0x%02x)\r\n\0"),dout);
	}

	o_control = CTL_SISTEMA;
	perforaciones_RELOAD_TIMER_SISTEMA();
	systemVars.doutputs_conf.perforacion.control = CTL_SISTEMA;

	perforaciones_set_douts( dout );
}
//------------------------------------------------------------------------------------
void perforaciones_RELOAD_TIMER_SISTEMA(void)
{
	o_timer_sistema = TIMEOUT_O_TIMER_SISTEMA;
}
//------------------------------------------------------------------------------------
void perforaciones_RELOAD_TIMER_BOYA(void)
{
	o_timer_boya = TIMEOUT_O_TIMER_BOYA;

}
//------------------------------------------------------------------------------------
void perforaciones_STOP_TIMER_BOYA(void)
{
	o_timer_boya = 0;

}
//------------------------------------------------------------------------------------
void perforaciones_STOP_TIMER_SISTEMA(void)
{
	o_timer_sistema = 0;

}
//------------------------------------------------------------------------------------
uint16_t perforaciones_read_clt_timer(void)
{
	// Devuelve el valor del timer. Se usa en tkCMD.status
	switch (o_control ) {
	case CTL_BOYA:
		return(o_timer_boya);
		break;
	case CTL_SISTEMA:
		return(o_timer_sistema);
		break;
	}
	return(0);
}
//------------------------------------------------------------------------------------

