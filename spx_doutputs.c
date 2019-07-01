/*
 * spx_doutputs.c
 *
 *  Created on: 5 jun. 2019
 *      Author: pablo
 */

#include "spx.h"

extern uint8_t o_control;
extern uint16_t o_timer_sistema, o_timer_boya;
extern bool doutputs_reinit;

#define TIMEOUT_O_TIMER_BOYA	 60
#define TIMEOUT_O_TIMER_SISTEMA	 600
//------------------------------------------------------------------------------------
void doutputs_RELOAD_TIMER_SISTEMA(void)
{
	o_timer_sistema = TIMEOUT_O_TIMER_SISTEMA;
}
//------------------------------------------------------------------------------------
void doutputs_RELOAD_TIMER_BOYA(void)
{
	o_timer_boya = TIMEOUT_O_TIMER_BOYA;

}
//------------------------------------------------------------------------------------
void doutputs_STOP_TIMER_BOYA(void)
{
	o_timer_boya = 0;

}
//------------------------------------------------------------------------------------
void doutputs_STOP_TIMER_SISTEMA(void)
{
	o_timer_sistema = 0;

}
//------------------------------------------------------------------------------------
void doutputs_config_defaults( char *opt )
{
	// En el caso de SPX_IO8, configura la salida a que inicialmente este todo en off.

	if ( spx_io_board == SPX_IO8CH ) {

		if (!strcmp_P( opt, PSTR("UTE\0"))) {
			systemVars.doutputs_conf.modo = NONE;
		} else {
			systemVars.doutputs_conf.modo = PERFORACIONES;
		}

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
	systemVars.doutputs_conf.perforacion.outs = 0x00;

	doutput_set_douts(0x00 );
	doutputs_reinit = true;

}
//------------------------------------------------------------------------------------
bool doutputs_config_mode( char *mode )
{

char l_data[10];

	memcpy(l_data, mode, sizeof(l_data));
	strupr(l_data);

	if (!strcmp_P( l_data, PSTR("NONE\0"))) {
		systemVars.doutputs_conf.modo = NONE;

	} else 	if (!strcmp_P( l_data, PSTR("CONS\0"))) {
		if ( spx_io_board != SPX_IO5CH ) {
			return(false);
		}
		systemVars.doutputs_conf.modo = CONSIGNA;

	} else if (!strcmp_P( l_data, PSTR("PERF\0"))) {
		systemVars.doutputs_conf.modo = PERFORACIONES;

	} else if (!strcmp_P( l_data, PSTR("PLT\0"))) {
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

	doutputs_reinit = true;
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

	doutputs_reinit = true;
	return(true);

}
//------------------------------------------------------------------------------------
bool doutputs_cmd_write_consigna( char *tipo_consigna_str)
{

char l_data[10];

	memcpy(l_data, tipo_consigna_str, sizeof(l_data));
	strupr(l_data);

	if ( spx_io_board != SPX_IO5CH ) {
		return(false);
	}

	if (!strcmp_P( l_data, PSTR("DIURNA\0")) ) {
		systemVars.doutputs_conf.consigna.c_aplicada = CONSIGNA_DIURNA;
		DRV8814_set_consigna_diurna();
		return(true);
	}

	if (!strcmp_P( l_data, PSTR("NOCTURNA\0")) ) {
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

char l_data[10];

	memcpy(l_data, param1, sizeof(l_data));
	strupr(l_data);

	if ( spx_io_board != SPX_IO5CH ) {
		return(false);
	}

	// write valve enable (A|B)
	if (!strcmp_P( l_data, PSTR("ENABLE\0")) ) {
		DRV8814_enable_pin( toupper(param2[0]), 1);
		return(true);
	}

	// write valve disable (A|B)
	if (!strcmp_P( l_data, PSTR("DISABLE\0")) ) {
		DRV8814_enable_pin( toupper(param2[0]), 0);
		return(true);
	}

	// write valve set
	if (!strcmp_P( l_data, PSTR("SET\0")) ) {
		DRV8814_reset_pin(1);
		return(true);
	}

	// write valve reset
	if (!strcmp_P( l_data, PSTR("RESET\0")) ) {
		DRV8814_reset_pin(0);
		return(true);
	}

	// write valve sleep
	if (!strcmp_P( l_data, PSTR("SLEEP\0")) ) {
		DRV8814_sleep_pin(1);
		return(true);
	}

	// write valve awake
	if (!strcmp_P( l_data, PSTR("AWAKE\0")) ) {
		DRV8814_sleep_pin(0);
		return(true);
	}

	// write valve ph01 (A|B)
	if (!strcmp_P( l_data, PSTR("PH01\0")) ) {
		DRV8814_phase_pin( toupper(param2[0]), 1);
		return(true);
	}

	// write valve ph10 (A|B)
	if (!strcmp_P( l_data, PSTR("PH10\0")) ) {
		DRV8814_phase_pin( toupper(param2[0]), 0);
		return(true);
	}

	// write valve power on|off
	if (!strcmp_P( l_data, PSTR("POWER\0")) ) {

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
	if (!strcmp_P( l_data, PSTR("OPEN\0")) ) {

		// Proporciono corriente.
		DRV8814_power_on();
		// Espero 10s que se carguen los condensasores
		vTaskDelay( ( TickType_t)( 1000 / portTICK_RATE_MS ) );

		xprintf_P( PSTR("VALVE OPEN %c\r\n\0"), toupper(param2[0] ));
		DRV8814_vopen( toupper(param2[0]), 100);

		DRV8814_power_off();
		return(true);
	}

	if (!strcmp_P( l_data, PSTR("CLOSE\0")) ) {
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
	if (!strcmp_P( l_data, PSTR("PULSE\0")) ) {
		// Proporciono corriente.
		DRV8814_power_on();
		// Espero 10s que se carguen los condensasores
		vTaskDelay( ( TickType_t)( 1000 / portTICK_RATE_MS ) );
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
char l_data[10];

	memcpy(l_data, param_state, sizeof(l_data));
	strupr(l_data);

	if ( spx_io_board == SPX_IO8CH ) {
		// Tenemos 8 salidas que las manejamos con el MCP
		pin = atoi(param_pin);
		if ( pin > 7 )
			return(false);

		if (!strcmp_P( l_data, PSTR("SET\0"))) {
			ret_code = IO_set_DOUT(pin);
			if ( ret_code == -1 ) {
				// Error de bus
				xprintf_P( PSTR("wDOUTPUT: I2C bus error(1)\n\0"));
				return(false);
			}
			return(true);
		}

		if (!strcmp_P( l_data, PSTR("CLEAR\0"))) {
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

		if (!strcmp_P( l_data, PSTR("SET\0"))) {
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

		if (!strcmp_P( l_data, PSTR("CLEAR\0"))) {
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
uint16_t doutputs_cmd_read_clt_timer(void)
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
void doutput_set_douts( uint8_t dout )
{
	// Funcion para setear el valor de las salidas desde el resto del programa.
	// La usamos desde tkGprs cuando en un frame nos indican cambiar las salidas.
	// Como el cambio depende de quien tiene el control y del timer, aqui vemos si
	// se cambia o se ignora.

uint8_t data;
int8_t xBytes;

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
		xprintf_P(PSTR("ERROR: doutput_set_douts MCP write\r\n\0"));
		return;
	}

	xprintf_P( PSTR("doutput_set_douts (set outputs to 0x%02x)\r\n\0"),dout);
}
//------------------------------------------------------------------------------------
void doutput_set_douts_from_gprs( uint8_t dout )
{
	// El GPRS recibio datos de setear la salida.
	// El control debe ser de SISTEMA y reiniciar el timer_SISTEMA

	if ( o_control == CTL_BOYA ) {
		xprintf_P( PSTR("OUTPUT CTL to SISTEMA !!!. (set outputs to 0x%02x)\r\n\0"),dout);
	}

	o_control = CTL_SISTEMA;
	doutputs_RELOAD_TIMER_SISTEMA();
	systemVars.doutputs_conf.perforacion.control = CTL_SISTEMA;

	doutput_set_douts( dout );
}
//------------------------------------------------------------------------------------
