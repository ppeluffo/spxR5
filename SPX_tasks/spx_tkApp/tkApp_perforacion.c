/*
 * ul_perforaciones.c
 *
 *  Created on: 23 oct. 2019
 *      Author: pablo
 */

#include "tkApp.h"

uint8_t o_control = 0;
uint16_t o_timer_emergencia = 0;
uint16_t o_timer_sistema = 0;

#define TIMEOUT_O_TIMER_EMERGENCIA	 60
#define TIMEOUT_O_TIMER_SISTEMA	 600

#define RELOAD_TIMER_SISTEMA()			(  o_timer_sistema = TIMEOUT_O_TIMER_SISTEMA )
#define RELOAD_TIMER_EMERGENCIA()		( o_timer_emergencia = TIMEOUT_O_TIMER_EMERGENCIA )
#define STOP_TIMER_SISTEMA() 			( o_timer_sistema = 0 )
#define STOP_TIMER_EMERGENCIA()			( o_timer_emergencia = 0 )

static bool xAPP_perforacion_init(void);

//------------------------------------------------------------------------------------
void tkApp_perforacion(void)
{
	// Corre en los 2 ioboards. El control se hace a c/segundo por lo tanto en SPX_IO5
	// no puede entrar en pwrsave !!!

	// El control a EMERGENCIA se pasa aqui estando en modo SISTEMA, si expiro el timer de los datos.
	// El control a SISTEMA los pasa el recibir datos de GPRS.

	if ( !xAPP_perforacion_init() )
		return;


	for (;;) {

		ctl_watchdog_kick( WDG_APP,  WDG_APP_TIMEOUT );

		// Espera de 1s
		vTaskDelay( ( TickType_t)( 1000 / portTICK_RATE_MS ) );

		switch ( o_control ) {
		case PERF_CTL_EMERGENCIA:
			// Cuando el timer de boya expira, lo recargo y re-escribo las salidas
			// Solo paso a modo SISTEMA cuando recibo un dato desde el GPRS !!!
			if ( o_timer_emergencia > 0 ) {
				o_timer_emergencia--;
				if ( o_timer_emergencia == 0 ) {
					RELOAD_TIMER_EMERGENCIA();
					xAPP_set_douts ( sVarsApp.perforacion.outs );
					xprintf_P( PSTR("APP: PERFORACION MODO EMERGENCIA: reload timer emergencia. DOUTS=0x%0X\r\n\0"), sVarsApp.perforacion.outs );
				}
			}
			break;

		case PERF_CTL_SISTEMA:
			// disminuyo el timer
			if ( o_timer_sistema > 0 ) {
				o_timer_sistema--;
				if ( o_timer_sistema == 0 ) {
					// Expiro: Paso el control a modo EMERGENCIA(BOYA o TIMER) y las salidas a 0x00
					//xAPP_set_douts( 0x00 );
					xAPP_set_douts_emergencia();
					o_control = PERF_CTL_EMERGENCIA;			// Paso el control a emergencia( boya o timer).
					RELOAD_TIMER_EMERGENCIA();					// Arranco el timer de emergencia
					sVarsApp.perforacion.control = PERF_CTL_EMERGENCIA;
					xprintf_P( PSTR("APP: PERFORACION CTL to EMERGENCIA !!!. (set outputs to 0x00)\r\n\0"));
				}
			}
			break;

		default:
			// Paso a control de boyas
			//xAPP_set_douts( 0x00 );
			xAPP_set_douts_emergencia();
			o_control = PERF_CTL_EMERGENCIA;			// Paso el control a las boyas.
			RELOAD_TIMER_EMERGENCIA();					// Arranco el timer de las boyas
			sVarsApp.perforacion.control = PERF_CTL_EMERGENCIA;
			xprintf_P( PSTR("APP: PERFORACION ERROR: Control outputs: Pasa a EMERGENCIA !!\r\n\0"));
			break;
		}

	}
}
//------------------------------------------------------------------------------------
static bool xAPP_perforacion_init(void)
{
	// Inicializa las salidas con el modo de trabajo PERFORACIONES.
	// Puede ser en cualquiera de las ioboards

	// AL iniciar el sistema debo leer el valor que tiene el MCP.OLATB
	// y activar las salidas con este valor. El control depende del valor del MCP.

int xBytes = 0;
uint8_t data = 0;
bool retS = false;

	if ( spx_io_board != SPX_IO8CH ) {
		xprintf_P(PSTR("APP: PERFORACION Init ERROR: run only in IO_8CH.\r\n\0"));
		sVarsApp.aplicacion = APP_OFF;
		u_save_params_in_NVMEE();
		return(retS);
	}

	// Activacion de salidas en 8CH
	// Leo el OLATB y pongo la salida que tenia
	if ( spx_io_board == SPX_IO8CH ) {
		xBytes = MCP_read( MCP_OLATB, (char *)&data, 1 );
		if ( xBytes == -1 ) {
			xprintf_P(PSTR("APP: PERFORACION INIT ERROR: I2C:MCP:pv_cmd_rwMCP\r\n\0"));
			data = 0x00;
			retS = false;
		}

		if ( xBytes > 0 ) {
			xprintf_P( PSTR( "PERFORACION: Perforaciones INIT OK: VALUE=0x%x\r\n\0"),data);
			retS = true;
		}

		data = twiddle_bits(data);

		// Tengo el OLATB: Si el bit 0 y 2 son 0 estoy en modo boya
		if ( ( data & 0x5 ) == 0 ) {
			// Modo BOYA
			o_control = PERF_CTL_EMERGENCIA;
			sVarsApp.perforacion.control = PERF_CTL_EMERGENCIA;
			RELOAD_TIMER_EMERGENCIA();
		} else {
			// Modo SISTEMA
			o_control = PERF_CTL_SISTEMA;
			sVarsApp.perforacion.control = PERF_CTL_SISTEMA;
			RELOAD_TIMER_SISTEMA();
		}

		// Pongo las salidas que ya tenia.
		xAPP_set_douts( data );
	}

	return(retS);

}
//------------------------------------------------------------------------------------
uint8_t xAPP_perforacion_checksum(void)
{
uint8_t checksum = 0;
char dst[32];
char *p;

	memset(dst,'\0', sizeof(dst));
	snprintf_P(dst, sizeof(dst), PSTR("PERFORACION"));
	p = dst;
	while (*p != '\0') {
		checksum += *p++;
	}
	return(checksum);

}
//------------------------------------------------------------------------------------
void xAPP_perforacion_print_status( void )
{

uint8_t olatb = 0 ;

	xprintf_P( PSTR("  modo: Perforacion\r\n\0"));
	MCP_read( 0x15, (char *)&olatb, 1 );
	xprintf_P( PSTR("  outs=%d(0x%02x)[[%c%c%c%c%c%c%c%c](olatb=0x%02x)\r\n\0"), sVarsApp.perforacion.outs, sVarsApp.perforacion.outs, BYTE_TO_BINARY( sVarsApp.perforacion.outs ), olatb );

	switch( o_control ) {
	case PERF_CTL_EMERGENCIA:
		xprintf_P( PSTR("  control=EMERGENCIA, timer=%d\r\n\0"), o_timer_emergencia );
		break;
	case PERF_CTL_SISTEMA:
		xprintf_P( PSTR("  control=SISTEMA, timer=%d\r\n\0"), o_timer_sistema );
		break;
	}

}
//------------------------------------------------------------------------------------
void xAPP_perforacion_adjust_x_douts(uint8_t dout)
{

	if ( o_control == PERF_CTL_EMERGENCIA ) {
		xprintf_P( PSTR("APP: PERFORACION CTL to SISTEMA !!!. (set outputs to 0x%02x)\r\n\0"),dout);
	}

	o_control = PERF_CTL_SISTEMA;
	RELOAD_TIMER_SISTEMA();
	sVarsApp.perforacion.control = PERF_CTL_SISTEMA;
}
//------------------------------------------------------------------------------------
void xAPP_set_douts_emergencia(void)
{
	// Se utiliza en el modo EMERGENCIA ( BOYA o CTLFREQ).
	// Debo poner los bits 0 y 2 de dout en 0

uint8_t dout;

	// Leo el valor actual
	dout = sVarsApp.perforacion.outs;
	// Seteo los bits 0 y 2 en 0
	dout = dout & 0xF6;
	xAPP_set_douts( dout);

}
//------------------------------------------------------------------------------------

