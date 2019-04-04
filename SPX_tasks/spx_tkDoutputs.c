/*
 * spx_tkOutputs.c
 *
 *  Created on: 20 dic. 2018
 *      Author: pablo
 *
 *  En la tarea de salidas implementamos consignas para la placa IO5 y niveles para IO8
 *
 */


#include "spx.h"

static uint8_t outputs;

static void pv_tkDoutputs_init(void);
static void pv_doutputs_chequear_consignas(void);
static void pv_doutputs_init_consignas(void);

#define WDG_DOUT_TIMEOUT	60

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
	pv_tkDoutputs_init();

	// loop
	for( ;; )
	{

		ctl_watchdog_kick( WDG_DOUT,  WDG_DOUT_TIMEOUT );

		if ( spx_io_board == SPX_IO8CH ) {

			// 1 vez por segundo chequeo las salidas
			vTaskDelay( ( TickType_t)( 1000 / portTICK_RATE_MS ) );

			// Siempre reviso las salidas.
			if ( outputs != systemVars.doutputs_conf.d_outputs ) {
				outputs = systemVars.doutputs_conf.d_outputs;		// Variable local que representa el estado de las salidas
				xprintf_P( PSTR("tkOutputs: SET.\r\n\0"));
				DOUTPUTS_reflect_byte(outputs);
			}

		} else if ( spx_io_board == SPX_IO5CH ) {

			// Espero con lo que puedo entrar en tickless
			// Con 25s aseguro chequear 2 veces por minuto.
			vTaskDelay( ( TickType_t)( 25000 / portTICK_RATE_MS ) );

			// Chequeo y aplico.
			pv_doutputs_chequear_consignas();

		} else {

			vTaskDelay( ( TickType_t)( 25000 / portTICK_RATE_MS ) );
		}

	}

}
//------------------------------------------------------------------------------------
static void pv_tkDoutputs_init(void)
{

	if ( spx_io_board == SPX_IO5CH ) {
		DRV8814_init();
		pv_doutputs_init_consignas();
		return;
	}

	if ( spx_io_board == SPX_IO8CH ) {
		// El MCP ya se configuro para el puerto de salidas en los dinputs
		outputs = systemVars.doutputs_conf.d_outputs;
		DOUTPUTS_reflect_byte(outputs);
		return;
	}

}
//------------------------------------------------------------------------------------
static void pv_doutputs_chequear_consignas(void)
{

	// Las consignas se chequean y/o setean en cualquier modo de trabajo, continuo o discreto

RtcTimeType_t rtcDateTime;

	if ( ! systemVars.doutputs_conf.consigna.c_enabled )
		return;

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
static void pv_doutputs_init_consignas(void)
{
	// Determino cual consigna corresponde aplicar y la aplico.

RtcTimeType_t rtcDateTime;
uint16_t now, horaConsNoc, horaConsDia ;
uint8_t consigna_a_aplicar = 99;


	if ( ! systemVars.doutputs_conf.consigna.c_enabled )
		return;

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
		systemVars.doutputs_conf.consigna.c_enabled = CONSIGNA_OFF;
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
// FUNCIONES PUBLICAS
//------------------------------------------------------------------------------------
void doutputs_config_defaults(void)
{
	// En el caso de SPX_IO8, configura la salida a que inicialmente este todo en off.
	systemVars.doutputs_conf.d_outputs = 0x00;

	systemVars.doutputs_conf.consigna.c_enabled = CONSIGNA_OFF;
	systemVars.doutputs_conf.consigna.hhmm_c_diurna.hour = 05;
	systemVars.doutputs_conf.consigna.hhmm_c_diurna.min = 30;
	systemVars.doutputs_conf.consigna.hhmm_c_nocturna.hour = 23;
	systemVars.doutputs_conf.consigna.hhmm_c_nocturna.min = 30;

}
//------------------------------------------------------------------------------------
bool doutputs_config_consignas( char *modo, char *hhmm_dia, char *hhmm_noche)
{

	if ( !strcmp_P( strupr(modo), PSTR("ON\0")) ) {
			systemVars.doutputs_conf.consigna.c_enabled = true;
		} else if (!strcmp_P( strupr(modo), PSTR("OFF\0")) ) {
			systemVars.doutputs_conf.consigna.c_enabled = false;
		} else {
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


