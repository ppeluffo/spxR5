/*
 * l_consignas.c
 *
 *  Created on: 8 jul. 2019
 *      Author: pablo
 */

#include "spx.h"

extern bool doutputs_reinit;
//------------------------------------------------------------------------------------
bool consigna_write( char *tipo_consigna_str)
{

char l_data[10] = { '\0','\0','\0','\0','\0','\0','\0','\0','\0','\0' } ;

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
void tk_init_consigna(void)
{
	// Configuracion inicial cuando las salidas estan en CONSIGNA.
	// Solo es para SPX_IO5CH.

RtcTimeType_t rtcDateTime;
uint16_t now = 0;
uint16_t horaConsNoc = 0;
uint16_t horaConsDia = 0;
uint8_t consigna_a_aplicar = 99;

	memset( &rtcDateTime, '\0', sizeof(RtcTimeType_t));

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
		xprintf_P(PSTR("Set consigna diurna init: %02d:%02d\r\n\0"),rtcDateTime.hour,rtcDateTime.min);
		break;
	case CONSIGNA_NOCTURNA:
		DRV8814_set_consigna_nocturna();
		systemVars.doutputs_conf.consigna.c_aplicada = CONSIGNA_NOCTURNA;
		xprintf_P(PSTR("Set consigna nocturna init: %02d:%02d\r\n\0"),rtcDateTime.hour,rtcDateTime.min);
		break;
	}
}
//------------------------------------------------------------------------------------
void tk_consigna(void)
{
	// Las salidas estan configuradas para modo consigna.
	// c/25s reviso si debo aplicar una o la otra y aplico
	// Espero con lo que puedo entrar en tickless

RtcTimeType_t rtcDateTime;

	memset( &rtcDateTime, '\0', sizeof(RtcTimeType_t));

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
		xprintf_P(PSTR("Set consigna diurna %02d:%02d\r\n\0"),rtcDateTime.hour,rtcDateTime.min);
		return;
	 }

	if ( ( rtcDateTime.hour == systemVars.doutputs_conf.consigna.hhmm_c_nocturna.hour ) &&
			( rtcDateTime.min == systemVars.doutputs_conf.consigna.hhmm_c_nocturna.min )  ) {

		DRV8814_set_consigna_nocturna();
		systemVars.doutputs_conf.consigna.c_aplicada = CONSIGNA_NOCTURNA;
		xprintf_P(PSTR("Set consigna nocturna %02d:%02d\r\n\0"),rtcDateTime.hour,rtcDateTime.min);
		return;
	}

}
//------------------------------------------------------------------------------------
bool consigna_config ( char *hhmm1, char *hhmm2 )
{
	if ( hhmm1 != NULL ) {
		u_convert_int_to_time_t( atoi( hhmm1), &systemVars.doutputs_conf.consigna.hhmm_c_diurna );
	}

	if ( hhmm2 != NULL ) {
		u_convert_int_to_time_t( atoi(hhmm2), &systemVars.doutputs_conf.consigna.hhmm_c_nocturna );
	}

	doutputs_reinit = true;
	return(true);

}
//------------------------------------------------------------------------------------
