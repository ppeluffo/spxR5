/*
 * ul_perforaciones.c
 *
 *  Created on: 23 oct. 2019
 *      Author: pablo
 */


#include "ul_perforacion.h"

uint8_t o_control = 0;
uint16_t o_timer_boya = 0;
uint16_t o_timer_sistema = 0;

#define RELOAD_TIMER_SISTEMA()	(  o_timer_sistema = TIMEOUT_O_TIMER_SISTEMA )
#define RELOAD_TIMER_BOYA()		( o_timer_boya = TIMEOUT_O_TIMER_BOYA )
#define STOP_TIMER_SISTEMA() 	(o_timer_sistema = 0)
#define STOP_TIMER_BOYA()		( o_timer_boya = 0 )

//------------------------------------------------------------------------------------
void perforacion_stk(void)
{
	// Corre en los 2 ioboards. El control se hace a c/segundo por lo tanto en SPX_IO5
	// no puede entrar en pwrsave !!!

	// El control a las BOYAS se pasa aqui estando en modo SISTEMA, si expiro el timer de los datos.
	// El control al SISTEMA los pasa el recibir datos de GPRS.

	if ( !perforacion_init() )
		return;


	for (;;) {

		ctl_watchdog_kick( WDG_APP,  WDG_APP_TIMEOUT );

		// Espera de 1s
		vTaskDelay( ( TickType_t)( 1000 / portTICK_RATE_MS ) );

		switch ( o_control ) {
		case PERF_CTL_BOYA:
			// Cuando el timer de boya expira, lo recargo y re-escribo las salidas
			// Solo paso a modo SISTEMA cuando recibo un dato desde el GPRS !!!
			if ( o_timer_boya > 0 ) {
				o_timer_boya--;
				if ( o_timer_boya == 0 ) {
					RELOAD_TIMER_BOYA();
					perforacion_set_douts ( systemVars.aplicacion_conf.perforacion.outs );
					xprintf_P( PSTR("PERFORACION: MODO BOYA: reload timer boya. DOUTS=0x%0X\r\n\0"), systemVars.aplicacion_conf.perforacion.outs );
				}
			}
			break;

		case PERF_CTL_SISTEMA:
			// disminuyo el timer
			if ( o_timer_sistema > 0 ) {
				o_timer_sistema--;
				if ( o_timer_sistema == 0 ) {
					// Expiro: Paso el control a modo BOYA y las salidas a 0x00
					perforacion_set_douts( 0x00 );
					o_control = PERF_CTL_BOYA;			// Paso el control a las boyas.
					RELOAD_TIMER_BOYA();				// Arranco el timer de las boyas
					systemVars.aplicacion_conf.perforacion.control = PERF_CTL_BOYA;
					xprintf_P( PSTR("PERFORACION: CTL to BOYAS !!!. (set outputs to 0x00)\r\n\0"));
				}
			}
			break;

		default:
			// Paso a control de boyas
			perforacion_set_douts( 0x00 );
			o_control = PERF_CTL_BOYA;			// Paso el control a las boyas.
			RELOAD_TIMER_BOYA();	// Arranco el timer de las boyas
			systemVars.aplicacion_conf.perforacion.control = PERF_CTL_BOYA;
			xprintf_P( PSTR("PERFORACION ERROR: Control outputs: Pasa a BOYA !!\r\n\0"));
			break;
		}

	}
}
//------------------------------------------------------------------------------------
bool perforacion_init(void)
{
	// Inicializa las salidas con el modo de trabajo PERFORACIONES.
	// Puede ser en cualquiera de las ioboards

	// AL iniciar el sistema debo leer el valor que tiene el MCP.OLATB
	// y activar las salidas con este valor. El control depende del valor del MCP.

int xBytes = 0;
uint8_t data = 0;
bool retS = false;

	if ( spx_io_board != SPX_IO8CH ) {
		xprintf_P(PSTR("PERFORACION: Init ERROR: Perforaciones only in IO_8CH.\r\n\0"));
		systemVars.aplicacion = APP_OFF;
		u_save_params_in_NVMEE();
		return(retS);
	}

	// Activacion de salidas en 8CH
	// Leo el OLATB y pongo la salida que tenia
	if ( spx_io_board == SPX_IO8CH ) {
		xBytes = MCP_read( MCP_OLATB, (char *)&data, 1 );
		if ( xBytes == -1 ) {
			xprintf_P(PSTR("PERFORACION: Perforaciones INIT ERROR: I2C:MCP:pv_cmd_rwMCP\r\n\0"));
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
			o_control = PERF_CTL_BOYA;
			systemVars.aplicacion_conf.perforacion.control = PERF_CTL_BOYA;
			RELOAD_TIMER_BOYA();
		} else {
			// Modo SISTEMA
			o_control = PERF_CTL_SISTEMA;
			systemVars.aplicacion_conf.perforacion.control = PERF_CTL_SISTEMA;
			RELOAD_TIMER_SISTEMA();
		}

		// Pongo las salidas que ya tenia.
		perforacion_set_douts( data );
	}

	return(retS);

}
//------------------------------------------------------------------------------------
void perforacion_set_douts( uint8_t dout )
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
	systemVars.aplicacion_conf.perforacion.outs = dout;
	MCP_update_olatb( dout );

	// Invierto el byte antes de escribirlo !!!
	data = twiddle_bits(data);
	xBytes = MCP_write(MCP_OLATB, (char *)&data, 1 );
	if ( xBytes == -1 ) {
		xprintf_P(PSTR("PEFORACION ERROR: perforacion_set_douts MCP write\r\n\0"));
		return;
	}

	xprintf_P( PSTR("PERFORACION: set outputs to 0x%02x\r\n\0"),dout);
}
//------------------------------------------------------------------------------------
void perforacion_set_douts_from_gprs( uint8_t dout )
{
	// El GPRS recibio datos de setear la salida.
	// El control debe ser de SISTEMA y reiniciar el timer_SISTEMA

	if ( o_control == PERF_CTL_BOYA ) {
		xprintf_P( PSTR("PERFORACION: CTL to SISTEMA !!!. (set outputs to 0x%02x)\r\n\0"),dout);
	}

	o_control = PERF_CTL_SISTEMA;
	RELOAD_TIMER_SISTEMA();
	systemVars.aplicacion_conf.perforacion.control = PERF_CTL_SISTEMA;

	perforacion_set_douts( dout );
}
//------------------------------------------------------------------------------------
uint16_t perforacion_read_timer_activo(void)
{
	// Devuelve el valor del timer. Se usa en tkCMD.status
	switch (o_control ) {
	case PERF_CTL_BOYA:
		return(o_timer_boya);
		break;
	case PERF_CTL_SISTEMA:
		return(o_timer_sistema);
		break;
	}
	return(0);
}
//------------------------------------------------------------------------------------
uint8_t perforacion_read_control_mode(void)
{
	return(o_control);
}
//------------------------------------------------------------------------------------
uint8_t perforacion_checksum(void)
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
void perforacion_reconfigure_app(void)
{
	// TYPE=INIT&PLOAD=CLASS:APP;AP0:PERFORACION;

	systemVars.aplicacion = APP_PERFORACION;
	u_save_params_in_NVMEE();
	//f_reset = true;

	if ( systemVars.debug == DEBUG_COMMS ) {
		xprintf_P( PSTR("COMMS: Reconfig APLICACION:PERFORACION\r\n\0"));
	}

}
//------------------------------------------------------------------------------------
void perforacion_process_gprs_response( const char *gprsbuff )
{
	// Recibi algo del estilo PERF_OUTS:245
	// Es la respuesta del server para activar las salidas en perforaciones o modo remoto.

	// Extraigo el valor de las salidas y las seteo.

char localStr[32] = { 0 };
char *stringp = NULL;
char *tk_douts = NULL;
char *delim = ",=:><";
char *p = NULL;

	//p = strstr( (const char *)&commsRxBuffer.buffer, "PERF_OUTS");
	p = strstr( gprsbuff , "PERF_OUTS");
	if ( p == NULL ) {
		return;
	}

	// Copio el mensaje enviado a un buffer local porque la funcion strsep lo modifica.
	memset(localStr,'\0',32);
	memcpy(localStr,p,sizeof(localStr));

	stringp = localStr;
	tk_douts = strsep(&stringp,delim);	// PERF_OUTS
	tk_douts = strsep(&stringp,delim);	// Str. con el valor de las salidas. 0..128

	// Actualizo el status a travez de una funcion propia del modulo de outputs
	perforacion_set_douts_from_gprs( atoi( tk_douts ));

	if ( systemVars.debug == DEBUG_COMMS ) {
		xprintf_P( PSTR("COMMS: PERF_OUTS\r\n\0"));
	}

}
//------------------------------------------------------------------------------------
