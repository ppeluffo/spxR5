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
 */


#include "spx.h"
#include "gprs.h"


static void tk_doutputs_none(void);
static void tk_doutputs_consigna(void);
static void tk_doutputs_pilotos(void);
static void tk_doutputs_perforaciones(void);

static void tk_doutputs_init_none(void);
static void tk_doutputs_init_consigna(void);
static void tk_doutputs_init_perforaciones(void);
static void tk_doutputs_init_pilotos(void);

void pv_set_hwoutputs(void);

uint8_t o_control;
static uint16_t o_timer, o_timer_boya, o_timer_sistema;

#define TIMEOUT_O_TIMER			600
#define TIMEOUT_O_TIMER_BOYA	 60

#define RELOAD_TIMER()	( o_timer = TIMEOUT_O_TIMER )
#define RELOAD_TIMER_BOYA()	( o_timer_boya = TIMEOUT_O_TIMER_BOYA )
#define STOP_TIMER_BOYA()	( o_timer_boya = 0 )

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
void tkDoutputs_init(void)
{

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
		DRV8814_init();
		DRV8814_sleep_pin(0);
		DRV8814_power_off();
		return;
	}

	if ( spx_io_board == SPX_IO8CH ) {
		//systemVars.doutputs_conf.perforacion.outs = 0x00;
		doutput_write_perforaciones_outs(0x00);
		pv_set_hwoutputs();
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
	// y activar las salidas con este valor

int xBytes = 0;
uint8_t data;

	// Control de las salidas
	o_control = CTL_BOYA;
	systemVars.doutputs_conf.perforacion.control = CTL_BOYA;
	o_timer = 0;
	o_timer_boya = 0;
	o_timer_sistema = 15;

	// Activacion de salidas en 8CH
	// Leo el OLATB y pongo la salida que tenia
	if ( spx_io_board == SPX_IO8CH ) {
		xBytes = MCP_read( 21, (char *)&data, 1 );
		if ( xBytes == -1 ) {
			xprintf_P(PSTR("OUTPUTS INIT ERROR: I2C:MCP:pv_cmd_rwMCP\r\n\0"));
			data = 0x00;
		}

		if ( xBytes > 0 ) {
			xprintf_P( PSTR( "OUTPUTS INIT OK: VALUE=0x%x\r\n\0"),data);
		}

		// Como el dato esta espejado lo debo girar
		data = twiddle_bits(data);
		//systemVars.doutputs_conf.perforacion.outs = data;
		doutput_write_perforaciones_outs(data);
		// Fijo las salidas en el mismo valor que tenia el OLATB
		pv_set_hwoutputs();
		return;
	}

	// Activacion de salidas en 5CH
	// No tengo MCP de modo que las salidas las mantiene el micro y se inicializan en 0.
	if ( spx_io_board == SPX_IO5CH ) {
		DRV8814_init();
		DRV8814_sleep_pin(0);
		DRV8814_power_off();
		return;
	}

}
//------------------------------------------------------------------------------------
static void tk_doutputs_init_pilotos(void)
{
	// Los pilotos se inicializan igual que las consignas ya que uso las 2 valvulas.
	if ( spx_io_board != SPX_IO5CH ) {
		xprintf_P(PSTR("DOUTPUTS ERROR: Pilotos only in IO_5CH.\r\n\0"));
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
		return;
	}

	vTaskDelay( ( TickType_t)( 5000 / portTICK_RATE_MS ) );

}
//------------------------------------------------------------------------------------
static void tk_doutputs_perforaciones(void)
{
	// Corre en los 2 ioboards. El control se hace a c/segundo por lo tanto en SPX_IO5
	// no puede entrar en pwrsave !!!

	// Monitoreo quien controla las salidas: BOYA o SISTEMA
	// 1) Si las salidas estan en BOYA y el valor pasa a ser != 0x00 el control pasa
	//    al SISTEMA y arranca el timer.
	// 2) Si el control es del SISTEMA y el timer expiro, paso el control a las BOYAS

	vTaskDelay( ( TickType_t)( 1000 / portTICK_RATE_MS ) );

	switch ( o_control ) {
	case CTL_BOYA:
		// 1) Si las salidas estan en BOYA y el valor de las salidas pasa a ser != 0x00 el control pasa
		//    al SISTEMA y arranca el timer.
		//	  Solo salgo del control xBoya cuando el GPRS recibio datos de las salidas.
		if ( systemVars.doutputs_conf.perforacion.outs != 0x00 ) {
			o_control = CTL_SISTEMA;	// Paso el control al sistema
			systemVars.doutputs_conf.perforacion.control = CTL_SISTEMA;
			pv_set_hwoutputs();			// Seteo las salidas HW.
			o_timer = TIMEOUT_O_TIMER;	// Arranco el timer
			xprintf_P( PSTR("OUTPUT CTL to SISTEMA !!!. (Reinit outputs timer)\r\n\0"));
			STOP_TIMER_BOYA();			// Paro el timer de la boya
		}
		break;

	case CTL_SISTEMA:

		if ( o_timer == 0 ) {
			// El control es del SISTEMA y el timer expiro: paso el control a las BOYAS
			//systemVars.doutputs_conf.perforacion.outs = 0x00;	// Apago las salidas
			doutput_write_perforaciones_outs(0x00);
			o_control = CTL_BOYA;			// Paso el control a las boyas.
			systemVars.doutputs_conf.perforacion.control = CTL_BOYA;
			pv_set_hwoutputs();				// Seteo las salidas HW.( 0x00 )
			xprintf_P( PSTR("OUTPUT CTL to BOYAS !!!. (set outputs to 0x00)\r\n\0"));

			// Arranco el timer de las boyas
			RELOAD_TIMER_BOYA();
		}

		// Cuando el modem esta prendido no hago nada ya que las salidas las maneja el server
		if ( u_gprs_modem_link_up() ) {
			o_timer_sistema = 15;		// reseteo el timer
		} else {
			// Con el modem apagado.
			if ( o_timer_sistema-- == 0 ) {		// disminuyo el timer
				o_timer_sistema = 15;
				pv_set_hwoutputs();
				xprintf_P( PSTR("OUTPUT reload: Sistema con modem link down.)\r\n\0"));
			}
		}
		break;

	default:
		xprintf_P( PSTR("ERROR Control outputs: Pasa a BOYA !!\r\n\0"));
		o_control = CTL_BOYA;
		systemVars.doutputs_conf.perforacion.control = CTL_BOYA;
		break;
	}

	// Corro el timer
	if ( o_timer > 0) {
		// Estoy en control por SISTEMA: voy contando la antiguedad del dato.
		o_timer--;
	}

	if ( o_timer_boya > 0 ) {
		o_timer_boya--;
		if ( o_timer_boya == 0 ) {
			RELOAD_TIMER_BOYA();
			//systemVars.doutputs_conf.perforacion.outs = 0x00;
			doutput_write_perforaciones_outs(0x00);
			pv_set_hwoutputs();
			xprintf_P( PSTR("MODO BOYA: reload timer boya\r\n\0"));
		}
	}
}
//------------------------------------------------------------------------------------
// FUNCIONES PUBLICAS
//------------------------------------------------------------------------------------
void doutputs_config_defaults(void)
{
	// En el caso de SPX_IO8, configura la salida a que inicialmente este todo en off.

	if ( spx_io_board == SPX_IO8CH ) {
		systemVars.doutputs_conf.modo = PERFORACIONES;
	} else if ( spx_io_board == SPX_IO5CH ) {
		systemVars.doutputs_conf.modo = NONE;
	}

	systemVars.doutputs_conf.consigna.hhmm_c_diurna.hour = 05;
	systemVars.doutputs_conf.consigna.hhmm_c_diurna.min = 30;
	systemVars.doutputs_conf.consigna.hhmm_c_nocturna.hour = 23;
	systemVars.doutputs_conf.consigna.hhmm_c_nocturna.min = 30;

	systemVars.doutputs_conf.piloto.band = 0.2;
	systemVars.doutputs_conf.piloto.max_steps = 6;
	systemVars.doutputs_conf.piloto.pout = 1.5;

	systemVars.doutputs_conf.perforacion.control = CTL_BOYA;
	//systemVars.doutputs_conf.perforacion.outs = 0x00;
	doutput_write_perforaciones_outs(0x00);

}
//------------------------------------------------------------------------------------
bool doutputs_config_mode( char *mode )
{

	if (!strcmp_P( strupr(mode), PSTR("NONE\0"))) {
		systemVars.doutputs_conf.modo = NONE;

	} else 	if (!strcmp_P( strupr(mode), PSTR("CONS\0"))) {
		if ( spx_io_board != SPX_IO5CH ) {
			return(false);
		}
		systemVars.doutputs_conf.modo = CONSIGNA;

	} else if (!strcmp_P( strupr(mode), PSTR("PERF\0"))) {
		systemVars.doutputs_conf.modo = PERFORACIONES;

	} else if (!strcmp_P( strupr(mode), PSTR("PLT\0"))) {
		if ( spx_io_board != SPX_IO5CH ) {
			return(false);
		}
		systemVars.doutputs_conf.modo = PILOTOS;

	} else {
		return(false);
	}

	// Debo re-inicializar las salidas
	doutputs_reinit = true;
	return ( true );
}
//------------------------------------------------------------------------------------
bool doutputs_config_consignas( char *hhmm_dia, char *hhmm_noche)
{
	// Configura las horas de consigna diurna y noctura

	if ( spx_io_board != SPX_IO5CH ) {
		return(false);
	}

	if ( hhmm_dia != NULL ) {
		u_convert_int_to_time_t( atoi(hhmm_dia), &systemVars.doutputs_conf.consigna.hhmm_c_diurna );
	}

	if ( hhmm_noche != NULL ) {
		u_convert_int_to_time_t( atoi(hhmm_noche), &systemVars.doutputs_conf.consigna.hhmm_c_nocturna );
	}

	return(true);

}
//------------------------------------------------------------------------------------
bool doutputs_config_piloto( char *pref, char *pband, char *psteps )
{

	if ( spx_io_board != SPX_IO5CH ) {
		return(false);
	}

	// Configura la presion de referencia, la banda y la cantidad de pasos
	systemVars.doutputs_conf.piloto.pout = atof( pref);

	if ( pband != NULL ) {
		systemVars.doutputs_conf.piloto.band = atof( pband);
	}

	if ( psteps != NULL ) {
		systemVars.doutputs_conf.piloto.max_steps = atoi( psteps );
	}

	return(true);

}
//------------------------------------------------------------------------------------
bool doutputs_cmd_write_consigna( char *tipo_consigna_str)
{

	if ( spx_io_board != SPX_IO5CH ) {
		return(false);
	}

	if (!strcmp_P( strupr(tipo_consigna_str), PSTR("DIURNA\0")) ) {
		systemVars.doutputs_conf.consigna.c_aplicada = CONSIGNA_DIURNA;
		DRV8814_set_consigna_diurna();
		return(true);
	}

	if (!strcmp_P( strupr(tipo_consigna_str), PSTR("NOCTURNA\0")) ) {
		systemVars.doutputs_conf.consigna.c_aplicada = CONSIGNA_NOCTURNA;
		DRV8814_set_consigna_nocturna();
		return(true);
	}

	return(false);
}
//------------------------------------------------------------------------------------
bool doutputs_cmd_write_valve( char *param1, char *param2 )
{
	// write valve (enable|disable),(set|reset),(sleep|awake),(ph01|ph10) } {A/B}
	//             (open|close) (A|B) (ms)
	//              power {on|off}

	if ( spx_io_board != SPX_IO5CH ) {
		return(false);
	}

	// write valve enable (A|B)
	if (!strcmp_P( strupr(param1), PSTR("ENABLE\0")) ) {
		DRV8814_enable_pin( toupper(param2[0]), 1);
		return(true);
	}

	// write valve disable (A|B)
	if (!strcmp_P( strupr(param1), PSTR("DISABLE\0")) ) {
		DRV8814_enable_pin( toupper(param2[0]), 0);
		return(true);
	}

	// write valve set
	if (!strcmp_P( strupr(param1), PSTR("SET\0")) ) {
		DRV8814_reset_pin(1);
		return(true);
	}

	// write valve reset
	if (!strcmp_P( strupr(param1), PSTR("RESET\0")) ) {
		DRV8814_reset_pin(0);
		return(true);
	}

	// write valve sleep
	if (!strcmp_P( strupr(param1), PSTR("SLEEP\0")) ) {
		DRV8814_sleep_pin(1);
		return(true);
	}

	// write valve awake
	if (!strcmp_P( strupr(param1), PSTR("AWAKE\0")) ) {
		DRV8814_sleep_pin(0);
		return(true);
	}

	// write valve ph01 (A|B)
	if (!strcmp_P( strupr(param1), PSTR("PH01\0")) ) {
		DRV8814_phase_pin( toupper(param2[0]), 1);
		return(true);
	}

	// write valve ph10 (A|B)
	if (!strcmp_P( strupr(param1), PSTR("PH10\0")) ) {
		DRV8814_phase_pin( toupper(param2[0]), 0);
		return(true);
	}

	// write valve power on|off
	if (!strcmp_P( strupr(param1), PSTR("POWER\0")) ) {

		if (!strcmp_P( strupr(param2), PSTR("ON\0")) ) {
			DRV8814_power_on();
			return(true);
		}
		if (!strcmp_P( strupr(param2), PSTR("OFF\0")) ) {
			DRV8814_power_off();
			return(true);
		}
		return(false);
	}

	//  write valve (open|close) (A|B) (ms)
	if (!strcmp_P( strupr(param1), PSTR("OPEN\0")) ) {

		// Proporciono corriente.
		DRV8814_power_on();
		// Espero 10s que se carguen los condensasores
		vTaskDelay( ( TickType_t)( 1000 / portTICK_RATE_MS ) );

		xprintf_P( PSTR("VALVE OPEN %c\r\n\0"), toupper(param2[0] ));
		DRV8814_vopen( toupper(param2[0]), 100);

		DRV8814_power_off();
		return(true);
	}

	if (!strcmp_P( strupr(param1), PSTR("CLOSE\0")) ) {
		// Proporciono corriente.
		DRV8814_power_on();
		// Espero 10s que se carguen los condensasores
		vTaskDelay( ( TickType_t)( 1000 / portTICK_RATE_MS ) );

		DRV8814_vclose( toupper(param2[0]), 100);
		xprintf_P( PSTR("VALVE CLOSE %c\r\n\0"), toupper(param2[0] ));

		DRV8814_power_off();
		return(true);
	}

	// write valve pulse (A/B) ms
	if (!strcmp_P( strupr(param1), PSTR("PULSE\0")) ) {
		// Proporciono corriente.
		DRV8814_power_on();
		// Espero 10s que se carguen los condensasores
		vTaskDelay( ( TickType_t)( 10000 / portTICK_RATE_MS ) );
		// Abro la valvula
		xprintf_P( PSTR("VALVE OPEN...\0") );
		DRV8814_vopen( toupper(param2[0]), 100);

		// Espero en segundos
		vTaskDelay( ( TickType_t)( atoi(argv[4])*1000 / portTICK_RATE_MS ) );

		// Cierro
		xprintf_P( PSTR("CLOSE\r\n\0") );
		DRV8814_vclose( toupper(param2[0]), 100);

		DRV8814_power_off();
		return(true);
	}

	return(false);

}
//------------------------------------------------------------------------------------
bool doutputs_cmd_write_outputs( char *param_pin, char *param_state )
{
	// Escribe un valor en las salidas.
	//

uint8_t pin;
int8_t ret_code;

	if ( spx_io_board == SPX_IO8CH ) {
		// Tenemos 8 salidas que las manejamos con el MCP
		pin = atoi(param_pin);
		if ( pin > 7 )
			return(false);

		if (!strcmp_P( strupr(param_state), PSTR("SET\0"))) {
			ret_code = IO_set_DOUT(pin);
			if ( ret_code == -1 ) {
				// Error de bus
				xprintf_P( PSTR("wDOUTPUT: I2C bus error(1)\n\0"));
				return(false);
			}
			return(true);
		}

		if (!strcmp_P( strupr(param_state), PSTR("CLEAR\0"))) {
			ret_code = IO_clr_DOUT(pin);
			if ( ret_code == -1 ) {
				// Error de bus
				xprintf_P( PSTR("wDOUTPUT: I2C bus error(2)\n\0"));
				return(false);
			}
			return(true);
		}

	} else if ( spx_io_board == SPX_IO5CH ) {
		// Las salidas las manejamos con el DRV8814
		// Las manejo de modo que solo muevo el pinA y el pinB queda en GND para c/salida
		pin = atoi(param_pin);
		if ( pin > 2 )
			return(false);

		DRV8814_power_on();
		// Espero 10s que se carguen los condensasores
		vTaskDelay( ( TickType_t)( 1000 / portTICK_RATE_MS ) );

		if (!strcmp_P( strupr(param_state), PSTR("SET\0"))) {
			switch(pin) {
			case 0:
				DRV8814_vopen( 'A', 100);
				break;
			case 1:
				DRV8814_vopen( 'B', 100);
				break;
			}
			return(true);
		}

		if (!strcmp_P( strupr(param_state), PSTR("CLEAR\0"))) {
			switch(pin) {
			case 0:
				DRV8814_vclose( 'A', 100);
				break;
			case 1:
				DRV8814_vclose( 'B', 100);
				break;
			}
			return(true);
		}

		DRV8814_power_off();

	}
	return(false);



}
//------------------------------------------------------------------------------------
void pv_set_hwoutputs(void)
{
	// Pone el valor de systemVars.d_outputs.perforaciones.outs en los pines de la salida
	// Es diferente en c/ioboard.

int8_t ret_code;

	if ( spx_io_board == SPX_IO8CH ) {
		ret_code = IO_reflect_DOUTPUTS(systemVars.doutputs_conf.perforacion.outs);
		if ( ret_code == -1 ) {
			xprintf_P( PSTR("I2C ERROR pv_set_hwoutputs\r\n\0"));
		}
	} else if ( spx_io_board == SPX_IO8CH ) {

	}

	xprintf_P( PSTR("tkOutputs: SET [0x%02x][%c%c%c%c%c%c%c%c]\r\n\0"), systemVars.doutputs_conf.perforacion.outs,  BYTE_TO_BINARY( systemVars.doutputs_conf.perforacion.outs ) );

}
//------------------------------------------------------------------------------------
void doutput_set( uint8_t dout, bool force )
{
	// Funcion para setear el valor de las salidas desde el resto del programa.
	// La usamos desde tkGprs cuando en un frame nos indican cambiar las salidas.
	// Como el cambio depende de quien tiene el control y del timer, aqui vemos si
	// se cambia o se ignora.

	// Solo es para IO8CH
	if ( spx_io_board != SPX_IO8CH ) {
		return;
	}

	// Guardo el valor recibido
	if ( systemVars.doutputs_conf.modo == PERFORACIONES ) {
		//systemVars.doutputs_conf.perforacion.outs = dout;
		doutput_write_perforaciones_outs(dout);
	};

	// Vemos que no se halla desconfigurado
	MCP_check();

	// En caso de que venga del modo comando, forzamos el setear el HW.
	if ( force == true ) {
		pv_set_hwoutputs();
		return;
	}

	// Si viene del gprs
	// Solo lo reflejo en el HW si el control lo tiene el sistema
	if ( force == false ) {
		if ( o_control == CTL_SISTEMA ) {
			pv_set_hwoutputs();
			RELOAD_TIMER();
		}
	}

}
//------------------------------------------------------------------------------------
uint16_t doutput_read_datatimer(void)
{
	// Devuelve el valor del timer. Se usa en tkCMD.status

	return(o_timer);
}
//------------------------------------------------------------------------------------
doutputs_control_t doutput_read_control(void)
{
	// Devuelve quien tiene el control de las salidas: BOYA o SISTEMA
	// Se usa en tkCMD.statius

	return(o_control);
}
//------------------------------------------------------------------------------------
void doutput_mcp_raise_error(void)
{
	// Funcion invocada al leer un PIN ( c/100ms) cuando hubo un error en el
	// bus I2C.
	// Esto cambia la configuracion del MCP por lo que deberia reconfigurarlo.

	xprintf_P( PSTR("I2C BUS ERROR: Reconfiguro MCP !!!\r\n\0"));

	vTaskDelay( ( TickType_t)( 2000 / portTICK_RATE_MS ) );	// Espero 2s que se vaya el ruido
	MCP_init();				// Reconfiguro el MCP
	pv_set_hwoutputs();		// Reconfiguro las salidas del OLATB

}
//------------------------------------------------------------------------------------
void doutput_write_perforaciones_outs( uint8_t val)
{
	// Escribe val en systemVars.doutputs_conf.perforacion.outs y actualiza el
	// valor en la variable del driver de MCP.

	systemVars.doutputs_conf.perforacion.outs = val;
	MCP_update_olatb( twiddle_bits( systemVars.doutputs_conf.perforacion.outs ));
}
//------------------------------------------------------------------------------------

