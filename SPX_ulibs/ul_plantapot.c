/*
 * ul_alarmas_ose.c
 *
 *  Created on: 5 nov. 2019
 *      Author: pablo
 */

#include "spx.h"


//------------------------------------------------------------------------------------
void appalarma_stk(void)
{

	if ( !appalarma_init() )
		return;

	for (;;) {
		ctl_watchdog_kick( WDG_APP,  WDG_APP_TIMEOUT );
		// Espera de 1s
		vTaskDelay( ( TickType_t)( 1000 / portTICK_RATE_MS ) );
	}
}
//------------------------------------------------------------------------------------
// FUNCIONES GENERALES
//------------------------------------------------------------------------------------
bool appalarma_init(void)
{
//uint8_t i;

	// Inicializa las salidas y las variables de trabajo.

	if ( spx_io_board != SPX_IO8CH ) {
		xprintf_P(PSTR("ALARMAS: Init ERROR: Control Alarmas only in IO_8CH.\r\n\0"));
		systemVars.aplicacion = APP_OFF;
		u_save_params_in_NVMEE();
		return(false);
	}

	appalarma_ac_luz_verde( act_OFF );
	appalarma_ac_luz_roja( act_OFF );
	appalarma_ac_luz_amarilla( act_OFF );
	appalarma_ac_luz_naranja( act_OFF );
	appalarma_ac_luz_azul( act_OFF );
	appalarma_ac_sirena( act_OFF );

	/*
	for (i=0; i<NRO_CANALES_MONITOREO; i++) {
		l_ppot_input_channels[i].enabled = false;					// Todo deshabilitado
		l_ppot_input_channels[i].value = 0.0;						// Valores en 0
		l_ppot_input_channels[i].timer = -1;						// Timers apagados
		l_ppot_input_channels[i].nivel_alarma = ALARMA_NIVEL_0;	// Nivel normal
	}
	*/
	alarmVars.llave_mantenimiento = false;
	alarmVars.sensor_puerta_open = false;

	return(true);

}
//------------------------------------------------------------------------------------
// ACCIONES
//------------------------------------------------------------------------------------
void appalarma_ac_luz_verde( t_dev_action action)
{

static uint8_t l_state = 0;

	switch(action) {
	case act_OFF:
		u_write_output_pins( OPIN_LUZ_VERDE, 0 );
		l_state = 0;
		break;
	case act_ON:
		u_write_output_pins( OPIN_LUZ_VERDE, 1 );
		l_state = 1;
		break;
	case act_FLASH:
		if ( l_state == 0 ) {
			l_state = 1;
			u_write_output_pins( OPIN_LUZ_VERDE, 1 );
		} else {
			l_state = 0;
			u_write_output_pins( OPIN_LUZ_VERDE, 0 );
		}
		break;
	default:
		break;
	}
}
//------------------------------------------------------------------------------------
void appalarma_ac_luz_roja( t_dev_action action)
{

static uint8_t l_state = 0;

	switch(action) {
	case act_OFF:
		u_write_output_pins( OPIN_LUZ_ROJA, 0 );
		l_state = 0;
		break;
	case act_ON:
		u_write_output_pins( OPIN_LUZ_ROJA, 1 );
		l_state = 1;
		break;
	case act_FLASH:
		if ( l_state == 0 ) {
			l_state = 1;
			u_write_output_pins( OPIN_LUZ_ROJA, 1 );
		} else {
			l_state = 0;
			u_write_output_pins( OPIN_LUZ_ROJA, 0 );
		}
		break;
	default:
		break;
	}
}
//------------------------------------------------------------------------------------
void appalarma_ac_luz_amarilla( t_dev_action action)
{

static uint8_t l_state = 0;

	switch(action) {
	case act_OFF:
		u_write_output_pins( OPIN_LUZ_AMARILLA, 0 );
		l_state = 0;
		break;
	case act_ON:
		u_write_output_pins( OPIN_LUZ_AMARILLA, 1 );
		l_state = 1;
		break;
	case act_FLASH:
		if ( l_state == 0 ) {
			l_state = 1;
			u_write_output_pins( OPIN_LUZ_AMARILLA, 1 );
		} else {
			l_state = 0;
			u_write_output_pins( OPIN_LUZ_AMARILLA, 0 );
		}
		break;
	default:
		break;
	}
}
//------------------------------------------------------------------------------------
void appalarma_ac_luz_naranja( t_dev_action action)
{

static uint8_t l_state = 0;

	switch(action) {
	case act_OFF:
		u_write_output_pins( OPIN_LUZ_NARANJA, 0 );
		l_state = 0;
		break;
	case act_ON:
		u_write_output_pins( OPIN_LUZ_NARANJA, 1 );
		l_state = 1;
		break;
	case act_FLASH:
		if ( l_state == 0 ) {
			l_state = 1;
			u_write_output_pins( OPIN_LUZ_NARANJA, 1 );
		} else {
			l_state = 0;
			u_write_output_pins( OPIN_LUZ_NARANJA, 0 );
		}
		break;
	default:
		break;
	}
}
//------------------------------------------------------------------------------------
void appalarma_ac_luz_azul( t_dev_action action)
{

static uint8_t l_state = 0;

	switch(action) {
	case act_OFF:
		u_write_output_pins( OPIN_LUZ_AZUL, 0 );
		l_state = 0;
		break;
	case act_ON:
		u_write_output_pins( OPIN_LUZ_AZUL, 1 );
		l_state = 1;
		break;
	case act_FLASH:
		if ( l_state == 0 ) {
			l_state = 1;
			u_write_output_pins( OPIN_LUZ_AZUL, 1 );
		} else {
			l_state = 0;
			u_write_output_pins( OPIN_LUZ_AZUL, 0 );
		}
		break;
	default:
		break;
	}
}
//------------------------------------------------------------------------------------
void appalarma_ac_sirena(t_dev_action action)
{

	switch(action) {
	case act_OFF:
		u_write_output_pins( OPIN_SIRENA, 0 );
		break;
	case act_ON:
		u_write_output_pins( OPIN_SIRENA, 1 );
		break;
	case act_FLASH:
		break;
	default:
		break;
	}
}
//------------------------------------------------------------------------------------
// GENERAL
//------------------------------------------------------------------------------------
uint8_t appalarma_checksum(void)
{

uint8_t checksum = 0;
char dst[32];
char *p;
uint8_t i = 0;

	// calculate own checksum
	// Vacio el buffer temoral
	memset(dst,'\0', sizeof(dst));

	i = snprintf_P( &dst[i], sizeof(dst), PSTR("ALARMAS,"));
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
bool appalarma_config( char *param0,char *param1, char *param2, char *param3, char *param4 )
{
	// config appalarm sms {id} {nro} {almlevel}\r\n\0"));
	// config appalarm nivel {chid} {alerta} {inf|sup} val\r\n\0"));

nivel_alarma_t nivel_alarma;
float valor;
uint8_t limite;
uint8_t pos;

	if ( param1 == NULL ) {
		return(false);
	}

	if ( param2 == NULL ) {
		return(false);
	}

	if ( param3 == NULL ) {
		return(false);
	}

	// config appalarm sms {id} {nro} {almlevel}\r\n\0"));
	if (!strcmp_P( strupr(param0), PSTR("SMS\0")) ) {

		pos = atoi(param1);
		memcpy( systemVars.aplicacion_conf.alarma_ppot.l_sms[pos].sms_nro, param2, SMS_NRO_LENGTH );
		systemVars.aplicacion_conf.alarma_ppot.l_sms[pos].alm_level = atoi(param3);
		return(true);

	}

	// config appalarm nivel {chid} {alerta} {inf|sup} val\r\n\0"));
	if (!strcmp_P( strupr(param0), PSTR("NIVEL\0")) ) {

		if ( param4 == NULL ) {
			return(false);
		}

		pos = atoi(param1);

		switch(atoi(param2)) {
		case 1:
			nivel_alarma = ALARMA_NIVEL_1;
			break;
		case 2:
			nivel_alarma = ALARMA_NIVEL_2;
			break;
		case 3:
			nivel_alarma = ALARMA_NIVEL_3;
			break;
		default:
			return(false);
		}

		if (!strcmp_P( strupr(param3), PSTR("INF\0")) ) {
			limite = 0;
		} else 	if (!strcmp_P( strupr(param3), PSTR("SUP\0")) ) {
			limite = 1;
		} else {
			return(false);
		}

		valor = atof(param4);

		if (!strcmp_P( strupr(param3), PSTR("INF\0")) ) {

		} else 	if (!strcmp_P( strupr(param3), PSTR("SUP\0")) ) {
			systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[pos].alarma0.lim_sup = valor;
		} else {
			return(false);
		}

		// Configuro
		switch ( nivel_alarma ) {
		case ALARMA_NIVEL_0:
			break;
		case ALARMA_NIVEL_1:
			if ( limite == 0 ) {
				systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[pos].alarma1.lim_inf = valor;
			} else 	if ( limite == 1 ) {
				systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[pos].alarma1.lim_sup = valor;
			}
			break;
		case ALARMA_NIVEL_2:
			if ( limite == 0 ) {
				systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[pos].alarma2.lim_inf = valor;
			} else 	if ( limite == 1 ) {
				systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[pos].alarma2.lim_sup = valor;
			}
			break;
		case ALARMA_NIVEL_3:
			if ( limite == 0 ) {
				systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[pos].alarma3.lim_inf = valor;
			} else 	if ( limite == 1 ) {
				systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[pos].alarma3.lim_sup = valor;
			}
			break;
		}

		return(true);
	}

	return(false);

}
//------------------------------------------------------------------------------------
void appalarma_config_defaults(void)
{

uint8_t i;

	for ( i = 0; i < NRO_CANALES_ALM; i++) {
		systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[i].alarma1.lim_inf = 6.7;
		systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[i].alarma1.lim_sup = 8.3;
		systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[i].alarma2.lim_inf = 6.0;
		systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[i].alarma2.lim_sup = 9.0;
		systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[i].alarma3.lim_inf = 5.5;
		systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[i].alarma3.lim_sup = 9.5;
	}

	for ( i = 0; i < MAX_NRO_SMS_ALARMAS; i++) {
		systemVars.aplicacion_conf.alarma_ppot.l_sms[i].alm_level = 1;
		memcpy( systemVars.aplicacion_conf.alarma_ppot.l_sms[i].sms_nro, "099123456", SMS_NRO_LENGTH );
	}

}
//------------------------------------------------------------------------------------
uint8_t appalarma_read_status_sensor_puerta(void)
{
	if ( alarmVars.sensor_puerta_open == true ) {
		return(1);
	} else {
		return(0);
	}
}
//------------------------------------------------------------------------------------
uint8_t appalarma_read_status_mantenimiento(void)
{
	if ( alarmVars.llave_mantenimiento == true ) {
		return(1);
	} else {
		return(0);
	}
}
//------------------------------------------------------------------------------------
uint8_t appalarma_read_status_boton_alarma(void)
{

	if ( alarmVars.boton_alarma == true ) {
		return(1);
	} else {
		return(0);
	}
}
//------------------------------------------------------------------------------------
void appalarma_servicio_tecnico( char *action, char *device )
{
	// action: (prender/apagar,rtimers )
	// device: (lroja,lverde,lamarilla,lnaranja, sirena)

//uint8_t i;

	/*
	if ( !strcmp_P( strupr(action), PSTR("RTIMERS\0"))) {
		for (i=0; i<NRO_CANALES_MONITOREO; i++) {
			xprintf_P(PSTR("DEBUG: timer[%d] status[%d] value[%d]\r\n"), i, l_ppot_input_channels[i].enabled, l_ppot_input_channels[i].timer );
		}
		return;
	}
	**/

	if ( !strcmp_P( strupr(device), PSTR("LROJA\0"))) {
		if ( !strcmp_P( strupr(action), PSTR("PRENDER\0"))) {
			appalarma_ac_luz_roja(act_ON);
		} else 	if ( !strcmp_P( strupr(action), PSTR("APAGAR\0"))) {
			appalarma_ac_luz_roja(act_OFF);
		}
		return;
	}

	if ( !strcmp_P( strupr(device), PSTR("LVERDE\0"))) {
		if ( !strcmp_P( strupr(action), PSTR("PRENDER\0"))) {
			appalarma_ac_luz_verde(act_ON);
		} else 	if ( !strcmp_P( strupr(action), PSTR("APAGAR\0"))) {
			appalarma_ac_luz_verde(act_OFF);
		}
		return;
	}

	if ( !strcmp_P( strupr(device), PSTR("LAMARILLA\0"))) {
		if ( !strcmp_P( strupr(action), PSTR("PRENDER\0"))) {
			appalarma_ac_luz_amarilla(act_ON);
		} else 	if ( !strcmp_P( strupr(action), PSTR("APAGAR\0"))) {
			appalarma_ac_luz_amarilla(act_OFF);
		}
		return;
	}

	if ( !strcmp_P( strupr(device), PSTR("LNARANJA\0"))) {
		if ( !strcmp_P( strupr(action), PSTR("PRENDER\0"))) {
			appalarma_ac_luz_naranja(act_ON);
		} else 	if ( !strcmp_P( strupr(action), PSTR("APAGAR\0"))) {
			appalarma_ac_luz_naranja(act_OFF);
		}
		return;
	}

	if ( !strcmp_P( strupr(device), PSTR("LAZUL\0"))) {
		if ( !strcmp_P( strupr(action), PSTR("PRENDER\0"))) {
			appalarma_ac_luz_azul(act_ON);
		} else 	if ( !strcmp_P( strupr(action), PSTR("APAGAR\0"))) {
			appalarma_ac_luz_azul(act_OFF);
		}
		return;
	}

	if ( !strcmp_P( strupr(device), PSTR("SIRENA\0"))) {
		if ( !strcmp_P( strupr(action), PSTR("PRENDER\0"))) {
			appalarma_ac_sirena(act_ON);
		} else 	if ( !strcmp_P( strupr(action), PSTR("APAGAR\0"))) {
			appalarma_ac_sirena(act_OFF);
		}
		return;
	}
}
//------------------------------------------------------------------------------------
void appalarma_print_status(void)
{

uint8_t pos;

	xprintf_P( PSTR("  modo: PLANTAPOT\r\n\0"));
	// Configuracion de los SMS
	xprintf_P( PSTR("  Nros.SMS configurados\r\n\0"));
	for (pos = 0; pos < MAX_NRO_SMS_ALARMAS; pos++) {
		xprintf_P( PSTR("  sms%02d: %s, L%d\r\n"),pos, systemVars.aplicacion_conf.alarma_ppot.l_sms[pos].sms_nro, systemVars.aplicacion_conf.alarma_ppot.l_sms[pos].alm_level );
	}

	// Configuracion de los canales y niveles de alarma configurados
	xprintf_P( PSTR("  Niveles de alarma:\r\n\0"));
	for ( pos=0; pos<NRO_CANALES_ALM; pos++) {
		if ( strcmp ( systemVars.ainputs_conf.name[pos], "X" ) != 0 ) {
			xprintf_P( PSTR("  ch%d: alm1:(%.02f,%.02f),"),pos, systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[pos].alarma1.lim_inf,  systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[pos].alarma1.lim_sup);
			xprintf_P( PSTR("  alm2:(%.02f,%.02f),"), systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[pos].alarma2.lim_inf,  systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[pos].alarma2.lim_sup);
			xprintf_P( PSTR("  alm3:(%.02f,%.02f) \r\n\0"),  systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[pos].alarma3.lim_inf,  systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[pos].alarma3.lim_sup);
		}
	}

	xprintf_P( PSTR("  mantenimiento: %d\r\n\0"), appalarma_read_status_mantenimiento());
	xprintf_P( PSTR("  sensor puerta: %d\r\n\0"), appalarma_read_status_sensor_puerta());
	xprintf_P( PSTR("  boton alarma: %d\r\n\0"), appalarma_read_status_boton_alarma());

}
//------------------------------------------------------------------------------------
