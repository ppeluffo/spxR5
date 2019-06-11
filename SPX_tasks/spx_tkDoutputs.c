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

static void tk_doutputs_none(void);
static void tk_doutputs_consigna(void);
static void tk_doutputs_pilotos(void);
static void tk_doutputs_perforaciones(void);

static void tk_doutputs_init_none(void);
static void tk_doutputs_init_consigna(void);
static void tk_doutputs_init_perforaciones(void);
static void tk_doutputs_init_pilotos(void);

uint8_t o_control;
uint16_t o_timer_boya, o_timer_sistema;

#define WDG_DOUT_TIMEOUT	60

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
			tk_doutputs_consigna();
			break;
		case PERFORACIONES:
			tk_doutputs_perforaciones();
			break;
		case PILOTOS:
			tk_doutputs_pilotos();
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
		tk_doutputs_init_consigna();
		break;
	case PERFORACIONES:
		tk_doutputs_init_perforaciones();
		break;
	case PILOTOS:
		tk_doutputs_init_pilotos();
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
		doutput_set_douts( 0x00 );	// Setup dout.
	}

}
//------------------------------------------------------------------------------------
static void tk_doutputs_init_consigna(void)
{
	// Configuracion inicial cuando las salidas estan en CONSIGNA.
	// Solo es para SPX_IO5CH.

RtcTimeType_t rtcDateTime;
uint16_t now, horaConsNoc, horaConsDia ;
uint8_t consigna_a_aplicar = 99;

	if ( spx_io_board != SPX_IO5CH ) {
		xprintf_P(PSTR("DOUTPUTS Init ERROR: Consigna only in IO_5CH.\r\n\0"));
		systemVars.doutputs_conf.modo = NONE;
		return;
	}

	DRV8814_init();

	// Hora actual en minutos.
	if ( ! RTC_read_dtime(&rtcDateTime) )
		xprintf_P(PSTR("ERROR: I2C:RTC:pv_out_init_consignas\r\n\0"));

	// Caso 1: C.Diurna < C.Nocturna
	//           C.diurna                      C.nocturna
	// |----------|-------------------------------|---------------|
	// 0         hhmm1                          hhmm2            24
	//   nocturna             diurna                 nocturna

	now = rtcDateTime.hour * 60 + rtcDateTime.min;
	horaConsDia = systemVars.doutputs_conf.consigna.hhmm_c_diurna.hour * 60 + systemVars.doutputs_conf.consigna.hhmm_c_diurna.min;
	horaConsNoc = systemVars.doutputs_conf.consigna.hhmm_c_nocturna.hour * 60 + systemVars.doutputs_conf.consigna.hhmm_c_nocturna.min;

	if ( horaConsDia < horaConsNoc ) {
		// Caso A:
		if ( now <= horaConsDia ) {
			consigna_a_aplicar = CONSIGNA_NOCTURNA;
		}
		// Caso B:
		if ( ( horaConsDia <= now ) && ( now <= horaConsNoc )) {
			consigna_a_aplicar = CONSIGNA_DIURNA;
		}

		// Caso C:
		if ( now > horaConsNoc ) {
			consigna_a_aplicar = CONSIGNA_NOCTURNA;
		}
	}

	// Caso 2: C.Nocturna < Diurna
	//           C.Nocturna                      C.diurna
	// |----------|-------------------------------|---------------|
	// 0         hhmm2                          hhmm1            24
	//   diurna             nocturna                 diurna

	if (  horaConsNoc < horaConsDia ) {
		// Caso A:
		if ( now <= horaConsNoc ) {
			consigna_a_aplicar = CONSIGNA_DIURNA;
		}
		// Caso B:
		if ( ( horaConsNoc <= now ) && ( now <= horaConsDia )) {
			consigna_a_aplicar = CONSIGNA_NOCTURNA;
		}
		// Caso C:
		if ( now > horaConsDia ) {
			consigna_a_aplicar = CONSIGNA_DIURNA;
		}
	}

	// Aplico la consigna
	switch (consigna_a_aplicar) {
	case 99:
		// Incompatibilidad: seteo por default.
		xprintf_P( PSTR("OUTPUTS: INIT ERROR al setear consignas: horas incompatibles\r\n\0"));
		systemVars.doutputs_conf.consigna.hhmm_c_diurna.hour = 05;
		systemVars.doutputs_conf.consigna.hhmm_c_diurna.min = 30;
		systemVars.doutputs_conf.consigna.hhmm_c_nocturna.hour = 23;
		systemVars.doutputs_conf.consigna.hhmm_c_nocturna.min = 30;
		break;
	case CONSIGNA_DIURNA:
		DRV8814_set_consigna_diurna();
		systemVars.doutputs_conf.consigna.c_aplicada = CONSIGNA_DIURNA;
		xprintf_P(PSTR("Set consigna diurna\r\n\0"));
		break;
	case CONSIGNA_NOCTURNA:
		DRV8814_set_consigna_nocturna();
		systemVars.doutputs_conf.consigna.c_aplicada = CONSIGNA_NOCTURNA;
		xprintf_P(PSTR("Set consigna nocturna\r\n\0"));
		break;
	}
}
//------------------------------------------------------------------------------------
static void tk_doutputs_init_perforaciones(void)
{
	// Inicializa las salidas con el modo de trabajo PERFORACIONES.
	// Puede ser en cualquiera de las ioboards

	// AL iniciar el sistema debo leer el valor que tiene el MCP.OLATB
	// y activar las salidas con este valor. El control depende del valor del MCP.

int xBytes = 0;
uint8_t data;

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
			doutputs_RELOAD_TIMER_BOYA();
		} else {
			// Modo SISTEMA
			o_control = CTL_SISTEMA;
			systemVars.doutputs_conf.perforacion.control = CTL_SISTEMA;
			doutputs_RELOAD_TIMER_SISTEMA();
		}

		// Pongo las salidas que ya tenia.
		doutput_set_douts( data );
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
static void tk_doutputs_init_pilotos(void)
{
	// Los pilotos se inicializan igual que las consignas ya que uso las 2 valvulas.
	if ( spx_io_board != SPX_IO5CH ) {
		xprintf_P(PSTR("DOUTPUTS ERROR: Pilotos only in IO_5CH.\r\n\0"));
		systemVars.doutputs_conf.modo = NONE;
		return;
	}

	DRV8814_init();

}
//------------------------------------------------------------------------------------
static void tk_doutputs_none(void)
{
	// Es el caso en que no debo hacer nada con las salidas.
	// Duermo 25s para entrar en pwrdown.

	vTaskDelay( ( TickType_t)( 25000 / portTICK_RATE_MS ) );

}
//------------------------------------------------------------------------------------
static void tk_doutputs_consigna(void)
{
	// Las salidas estan configuradas para modo consigna.
	// c/25s reviso si debo aplicar una o la otra y aplico
	// Espero con lo que puedo entrar en tickless

RtcTimeType_t rtcDateTime;

	vTaskDelay( ( TickType_t)( 25000 / portTICK_RATE_MS ) );

	// Por las dudas
	if ( spx_io_board != SPX_IO5CH ) {
		xprintf_P(PSTR("DOUTPUTS loop ERROR: Consigna only in IO_5CH.\r\n\0"));
		systemVars.doutputs_conf.modo = NONE;
		return;
	}

	// Chequeo y aplico.
	// Las consignas se chequean y/o setean en cualquier modo de trabajo, continuo o discreto

	if ( ! RTC_read_dtime(&rtcDateTime) )
		xprintf_P(PSTR("ERROR: I2C:RTC:pv_dout_chequear_consignas\r\n\0"));

	if ( ( rtcDateTime.hour == systemVars.doutputs_conf.consigna.hhmm_c_diurna.hour ) &&
			( rtcDateTime.min == systemVars.doutputs_conf.consigna.hhmm_c_diurna.min )  ) {

		DRV8814_set_consigna_diurna();
		systemVars.doutputs_conf.consigna.c_aplicada = CONSIGNA_DIURNA;
		xprintf_P(PSTR("Set consigna diurna\r\n\0"));
		return;
	 }

	if ( ( rtcDateTime.hour == systemVars.doutputs_conf.consigna.hhmm_c_nocturna.hour ) &&
			( rtcDateTime.min == systemVars.doutputs_conf.consigna.hhmm_c_nocturna.min )  ) {

		DRV8814_set_consigna_nocturna();
		systemVars.doutputs_conf.consigna.c_aplicada = CONSIGNA_NOCTURNA;
		xprintf_P(PSTR("Set consigna nocturna\r\n\0"));
		return;
	}

}
//------------------------------------------------------------------------------------
static void tk_doutputs_pilotos(void)
{
	// Implementa el control por pilotos.
	// El tema es que debemos polear tambien para hacer el control
	// Queda para luego

	// Por las dudas
	if ( spx_io_board != SPX_IO5CH ) {
		systemVars.doutputs_conf.modo = NONE;
		return;
	}

	vTaskDelay( ( TickType_t)( 5000 / portTICK_RATE_MS ) );

}
//------------------------------------------------------------------------------------
static void tk_doutputs_perforaciones(void)
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
				doutputs_RELOAD_TIMER_BOYA();
				doutput_set_douts ( systemVars.doutputs_conf.perforacion.outs );
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
				doutput_set_douts( 0x00 );
				o_control = CTL_BOYA;			// Paso el control a las boyas.
				doutputs_RELOAD_TIMER_BOYA();	// Arranco el timer de las boyas
				systemVars.doutputs_conf.perforacion.control = CTL_BOYA;
				xprintf_P( PSTR("OUTPUT CTL to BOYAS !!!. (set outputs to 0x00)\r\n\0"));
			}
		}
		break;

	default:
		// Paso a control de boyas
		doutput_set_douts( 0x00 );
		o_control = CTL_BOYA;			// Paso el control a las boyas.
		doutputs_RELOAD_TIMER_BOYA();	// Arranco el timer de las boyas
		systemVars.doutputs_conf.perforacion.control = CTL_BOYA;
		xprintf_P( PSTR("ERROR Control outputs: Pasa a BOYA !!\r\n\0"));
		break;
	}

}
//------------------------------------------------------------------------------------
