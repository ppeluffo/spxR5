/*
 * tkApp_consignas.c
 *
 *  Created on: 23 oct. 2019
 *      Author: pablo
 */

#include "tkApp.h"

bool xAPP_consigna_init(void);

//------------------------------------------------------------------------------------
void tkApp_consigna(void)
{
	// Las salidas estan configuradas para modo consigna.
	// c/25s reviso si debo aplicar una o la otra y aplico
	// Espero con lo que puedo entrar en tickless

RtcTimeType_t rtcDateTime;

	xprintf_PD(DF_APP,"APP: CONSIGNA start\r\n\0");

	if ( !xAPP_consigna_init() )
		return;

	for (;;) {

		ctl_watchdog_kick( WDG_APP,  WDG_APP_TIMEOUT );

		vTaskDelay( ( TickType_t)( 25000 / portTICK_RATE_MS ) );

		// Chequeo y aplico.
		// Las consignas se chequean y/o setean en cualquier modo de trabajo, continuo o discreto
		memset( &rtcDateTime, '\0', sizeof(RtcTimeType_t));
		if ( ! RTC_read_dtime(&rtcDateTime) ) {
			xprintf_P(PSTR("APP: CONSIGNA ERROR: I2C:RTC:pv_dout_chequear_consignas\r\n\0"));
			continue;
		}

		// Consigna diurna ?
		if ( ( rtcDateTime.hour == systemVars.aplicacion_conf.consigna.hhmm_c_diurna.hour ) &&
				( rtcDateTime.min == systemVars.aplicacion_conf.consigna.hhmm_c_diurna.min )  ) {

			DRV8814_set_consigna_diurna();
			systemVars.aplicacion_conf.consigna.c_aplicada = CONSIGNA_DIURNA;
			xprintf_P(PSTR("APP: CONSIGNA diurna %02d:%02d\r\n\0"),rtcDateTime.hour,rtcDateTime.min);
			continue;
		}

		// Consigna nocturna ?
		if ( ( rtcDateTime.hour == systemVars.aplicacion_conf.consigna.hhmm_c_nocturna.hour ) &&
				( rtcDateTime.min == systemVars.aplicacion_conf.consigna.hhmm_c_nocturna.min )  ) {

			DRV8814_set_consigna_nocturna();
			systemVars.aplicacion_conf.consigna.c_aplicada = CONSIGNA_NOCTURNA;
			xprintf_P(PSTR("APP: CONSIGNA nocturna %02d:%02d\r\n\0"),rtcDateTime.hour,rtcDateTime.min);
			continue;
		}

	}
}
//------------------------------------------------------------------------------------
bool xAPP_consigna_init(void)
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
		xprintf_P(PSTR("APP: CONSIGNA ERROR: Modo Consigna only in IO_5CH.\r\n\0"));
		systemVars.aplicacion = APP_OFF;
		u_save_params_in_NVMEE();
		return(false);
	}

	DRV8814_init();

	// Hora actual en minutos.
	if ( ! RTC_read_dtime(&rtcDateTime) )
		xprintf_P(PSTR("APP: CONSIGNA ERROR: I2C:RTC:pv_out_init_consignas\r\n\0"));

	// Caso 1: C.Diurna < C.Nocturna
	//           C.diurna                      C.nocturna
	// |----------|-------------------------------|---------------|
	// 0         hhmm1                          hhmm2            24
	//   nocturna             diurna                 nocturna

	now = rtcDateTime.hour * 60 + rtcDateTime.min;
	horaConsDia = systemVars.aplicacion_conf.consigna.hhmm_c_diurna.hour * 60 + systemVars.aplicacion_conf.consigna.hhmm_c_diurna.min;
	horaConsNoc = systemVars.aplicacion_conf.consigna.hhmm_c_nocturna.hour * 60 + systemVars.aplicacion_conf.consigna.hhmm_c_nocturna.min;

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
		xprintf_P( PSTR("APP: CONSIGNA ERROR al setear consignas: horas incompatibles\r\n\0"));
		systemVars.aplicacion_conf.consigna.hhmm_c_diurna.hour = 05;
		systemVars.aplicacion_conf.consigna.hhmm_c_diurna.min = 30;
		systemVars.aplicacion_conf.consigna.hhmm_c_nocturna.hour = 23;
		systemVars.aplicacion_conf.consigna.hhmm_c_nocturna.min = 30;
		break;
	case CONSIGNA_DIURNA:
		DRV8814_set_consigna_diurna();
		systemVars.aplicacion_conf.consigna.c_aplicada = CONSIGNA_DIURNA;
		xprintf_P(PSTR("APP: CONSIGNA diurna init: %02d:%02d\r\n\0"),rtcDateTime.hour,rtcDateTime.min);
		break;
	case CONSIGNA_NOCTURNA:
		DRV8814_set_consigna_nocturna();
		systemVars.aplicacion_conf.consigna.c_aplicada = CONSIGNA_NOCTURNA;
		xprintf_P(PSTR("APP: CONSIGNA nocturna init: %02d:%02d\r\n\0"),rtcDateTime.hour,rtcDateTime.min);
		break;
	}

	return(true);

}
//------------------------------------------------------------------------------------
bool xAPP_consigna_config ( char *hhmm1, char *hhmm2 )
{

//	xprintf_P(PSTR("DEBUG CONSIGNA: %s, %s\r\n"), hhmm1, hhmm2);

	if ( hhmm1 != NULL ) {
//		xprintf_P(PSTR("DEBUG HHMM1: %02d,%02d"), systemVars.aplicacion_conf.consigna.hhmm_c_diurna.hour, systemVars.aplicacion_conf.consigna.hhmm_c_diurna.min );
		u_convert_int_to_time_t( atoi( hhmm1), &systemVars.aplicacion_conf.consigna.hhmm_c_diurna );
//		xprintf_P(PSTR("DEBUG HHMM1: %02d,%02d"), systemVars.aplicacion_conf.consigna.hhmm_c_diurna.hour, systemVars.aplicacion_conf.consigna.hhmm_c_diurna.min );

	}

	if ( hhmm2 != NULL ) {
//		xprintf_P(PSTR("DEBUG HHMM2: %02d,%02d"), systemVars.aplicacion_conf.consigna.hhmm_c_nocturna.hour, systemVars.aplicacion_conf.consigna.hhmm_c_nocturna.min );
		u_convert_int_to_time_t( atoi(hhmm2), &systemVars.aplicacion_conf.consigna.hhmm_c_nocturna );
//		xprintf_P(PSTR("DEBUG HHMM2: %02d,%02d"), systemVars.aplicacion_conf.consigna.hhmm_c_nocturna.hour, systemVars.aplicacion_conf.consigna.hhmm_c_nocturna.min );
	}

	return(true);

}
//------------------------------------------------------------------------------------
void xAPP_consigna_config_defaults(void)
{

	systemVars.aplicacion_conf.consigna.hhmm_c_diurna.hour = 05;
	systemVars.aplicacion_conf.consigna.hhmm_c_diurna.min = 30;
	systemVars.aplicacion_conf.consigna.hhmm_c_nocturna.hour = 23;
	systemVars.aplicacion_conf.consigna.hhmm_c_nocturna.min = 30;

}
//------------------------------------------------------------------------------------
bool xAPP_consigna_write( char *param0, char *param1, char *param2 )
{
	// write consigna
	// 		(diurna|nocturna)
	//		(enable|disable),(set|reset),(sleep|awake),(ph01|ph10) } {A/B}
	//		(open|close) (A|B)
	//		pulse (A|B) (secs)
	//		power {on|off}

	if ( spx_io_board != SPX_IO5CH ) {
		return(false);
	}

//	xprintf_P(PSTR("DEBUG: param0: %s\r\n"), param0);
//	xprintf_P(PSTR("DEBUG: param1: %s\r\n"), param1);
//	xprintf_P(PSTR("DEBUG: param2: %s\r\n"), param2);

	// (diurna|nocturna)
	if (!strcmp_P( strupr(param0) , PSTR("DIURNA\0")) ) {
		systemVars.aplicacion_conf.consigna.c_aplicada = CONSIGNA_DIURNA;
		DRV8814_set_consigna_diurna();
		return(true);
	}

	if (!strcmp_P( strupr(param0), PSTR("NOCTURNA\0")) ) {
		systemVars.aplicacion_conf.consigna.c_aplicada = CONSIGNA_NOCTURNA;
		DRV8814_set_consigna_nocturna();
		return(true);
	}

	// {(enable|disable),(set|reset),(sleep|awake),(ph01|ph10)} {A/B}
	if (!strcmp_P( strupr(param0), PSTR("ENABLE\0")) ) {
		DRV8814_enable_pin( toupper(param1[0]), 1);
		return(true);
	}

	// disable (A|B)
	if (!strcmp_P( strupr(param0), PSTR("DISABLE\0")) ) {
		DRV8814_enable_pin( toupper(param1[0]), 0);
		return(true);
	}

	// set
	if (!strcmp_P( strupr(param0), PSTR("SET\0")) ) {
		DRV8814_reset_pin(1);
		return(true);
	}

	// reset
	if (!strcmp_P( strupr(param0), PSTR("RESET\0")) ) {
		DRV8814_reset_pin(0);
		return(true);
	}

	// sleep
	if (!strcmp_P( strupr(param0), PSTR("SLEEP\0")) ) {
		DRV8814_sleep_pin(1);
		return(true);
	}

	// awake
	if (!strcmp_P( strupr(param0), PSTR("AWAKE\0")) ) {
		DRV8814_sleep_pin(0);
		return(true);
	}

	// ph01 (A|B)
	if (!strcmp_P( strupr(param0), PSTR("PH01\0")) ) {
		DRV8814_phase_pin( toupper(param1[0]), 1);
		return(true);
	}

	//  ph10 (A|B)
	if (!strcmp_P( strupr(param0), PSTR("PH10\0")) ) {
		DRV8814_phase_pin( toupper(param1[0]), 0);
		return(true);
	}

	// power on|off
	if ( strcmp_P( strupr(param0), PSTR("POWER\0")) == 0 ) {

		if (strcmp_P( strupr(param1), PSTR("ON\0")) == 0 ) {
			DRV8814_power_on();
			return(true);
		}
		if (strcmp_P( strupr(param1), PSTR("OFF\0")) == 0 ) {
			DRV8814_power_off();
			return(true);
		}
		return(false);
	}

	//  (open|close) (A|B) (ms)
	if (!strcmp_P( strupr(param0), PSTR("OPEN\0")) ) {

		// Proporciono corriente.
		DRV8814_power_on();
		// Espero 10s que se carguen los condensasores
		vTaskDelay( ( TickType_t)( 30000 / portTICK_RATE_MS ) );

		xprintf_P( PSTR("VALVE OPEN %c\r\n\0"), strupr(param1[0]) );
		DRV8814_vopen(toupper(param1[0]), 100);

		DRV8814_power_off();
		return(true);
	}

	if (!strcmp_P( strupr(param0), PSTR("CLOSE\0")) ) {
		// Proporciono corriente.
		DRV8814_power_on();
		// Espero 10s que se carguen los condensasores
		vTaskDelay( ( TickType_t)( 30000 / portTICK_RATE_MS ) );

		DRV8814_vclose( toupper(param1[0]), 100);
		xprintf_P( PSTR("VALVE CLOSE %c\r\n\0"), strupr(param1[0] ));

		DRV8814_power_off();
		return(true);
	}

	// pulse (A/B) ms
	if (!strcmp_P( strupr(param0), PSTR("PULSE\0")) ) {
		// Proporciono corriente.
		DRV8814_power_on();
		// Espero 10s que se carguen los condensasores
		vTaskDelay( ( TickType_t)( 30000 / portTICK_RATE_MS ) );
		// Abro la valvula
		xprintf_P( PSTR("VALVE OPEN...\0") );
		DRV8814_vopen( toupper(param1[0]), 100);

		// Espero en segundos
		vTaskDelay( ( TickType_t)( atoi(param2)*1000 / portTICK_RATE_MS ) );

		// Cierro
		xprintf_P( PSTR("CLOSE\r\n\0") );
		DRV8814_vclose( toupper(param1[0]), 100);

		DRV8814_power_off();
		return(true);
	}

	return(false);
}
//------------------------------------------------------------------------------------
uint8_t xAPP_consigna_checksum(void)
{

uint8_t checksum = 0;
char dst[32];
char *p;
uint8_t i = 0;

	// calculate own checksum
	// Vacio el buffer temoral
	memset(dst,'\0', sizeof(dst));

	i = snprintf_P( &dst[i], sizeof(dst), PSTR("CONSIGNA,"));
	i += snprintf_P(&dst[i], sizeof(dst), PSTR("%02d%02d,"), systemVars.aplicacion_conf.consigna.hhmm_c_diurna.hour, systemVars.aplicacion_conf.consigna.hhmm_c_diurna.min );
	i += snprintf_P(&dst[i], sizeof(dst), PSTR("%02d%02d"), systemVars.aplicacion_conf.consigna.hhmm_c_nocturna.hour, systemVars.aplicacion_conf.consigna.hhmm_c_nocturna.min );

	//xprintf_P( PSTR("DEBUG: CONS = [%s]\r\n\0"), dst );
	// Apunto al comienzo para recorrer el buffer
	p = dst;
	// Mientras no sea NULL calculo el checksum deol buffer
	while (*p != '\0') {
		checksum += *p++;
	}
	//xprintf_P( PSTR("DEBUG: cks = [0x%02x]\r\n\0"), checksum );

	return(checksum);

}
//------------------------------------------------------------------------------------
void xAPP_reconfigure_consigna(void)
{
	// CONSIGNAS:
	// TYPE=INIT&PLOAD=CLASS:APP_B;HHMM1:1230;HHMM2:0940

char *p = NULL;
char localStr[32] = { 0 };
char *stringp = NULL;
char *tk_cons_dia = NULL;
char *tk_cons_noche = NULL;
char *delim = ",=:;><";


	memset( &localStr, '\0', sizeof(localStr) );
	p = xCOMM_get_buffer_ptr("HHMM1");
	if ( p != NULL ) {
		memcpy(localStr,p,sizeof(localStr));
		stringp = localStr;
		tk_cons_dia = strsep(&stringp,delim);	// HHMM1
		tk_cons_dia = strsep(&stringp,delim);	// 1230
		tk_cons_noche = strsep(&stringp,delim); // HHMM2
		tk_cons_noche = strsep(&stringp,delim); // 0940
		xAPP_consigna_config(tk_cons_dia, tk_cons_noche );

		xprintf_PD( DF_COMMS, PSTR("COMMS: Reconfig CONSIGNA (%s,%s)\r\n\0"),tk_cons_dia, tk_cons_noche);

		u_save_params_in_NVMEE();
	}

}
//------------------------------------------------------------------------------------
