/*
 * ul_alarmas_ose.c
 *
 *  Created on: 5 nov. 2019
 *      Author: pablo
 */

#include "spx.h"

bool flash_luz_verde;
bool flash_luz_roja;
bool flash_luz_amarilla;
bool flash_luz_azul;
bool flash_luz_naranja;

static void pv_plantapot_TimerCallback( TimerHandle_t xTimer );

TimerHandle_t plantaplot_timerAlarmas;
StaticTimer_t plantaplot_xTimerAlarmas;

uint16_t din[IO8_DINPUTS_CHANNELS];

// Acciones
static void ac_luz_verde( t_dev_action action);
static void ac_luz_roja( t_dev_action action);
static void ac_luz_amarilla( t_dev_action action);
static void ac_luz_naranja( t_dev_action action);
static void ac_luz_azul( t_dev_action action);
static void ac_sirena(t_dev_action action);

static uint8_t ac_read_status_sensor_puerta(void);
static uint8_t ac_read_status_mantenimiento(void);
static uint8_t ac_read_status_boton_alarma(void);

//------------------------------------------------------------------------------------
void appalarma_stk(void)
{

	if ( !appalarma_init() )
		return;

	// Creo y arranco el timer de control del tiempo de alarmas
	plantaplot_timerAlarmas = xTimerCreateStatic ("PPOT",
			pdMS_TO_TICKS( 1000 ),
			pdTRUE,
			( void * ) 0,
			pv_plantapot_TimerCallback,
			&plantaplot_xTimerAlarmas
			);

	xTimerStart(plantaplot_timerAlarmas, 10);

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

	ac_luz_verde( act_OFF );
	ac_luz_roja( act_OFF );
	ac_luz_amarilla( act_OFF );
	ac_luz_naranja( act_OFF );
	ac_luz_azul( act_OFF );
	ac_sirena( act_OFF );

	alarmVars.boton_alarma_pressed = false;
	alarmVars.llave_mantenimiento_on= false;
	alarmVars.sensor_puerta_open = false;

	return(true);

}
//------------------------------------------------------------------------------------
static void pv_plantapot_TimerCallback( TimerHandle_t xTimer )
{
	// Se ejecuta cada 1s como callback del timer.

	// Flash de luces
	if ( flash_luz_verde ) ac_luz_verde(act_FLASH);
	if ( flash_luz_roja) ac_luz_roja(act_FLASH);
	if ( flash_luz_azul ) ac_luz_azul(act_FLASH);
	if ( flash_luz_amarilla ) ac_luz_amarilla(act_FLASH);
	if ( flash_luz_naranja ) ac_luz_naranja(act_FLASH);

	/*
	 * Leo las entradas digitales
	 * Las leo aqui porque en caso de activarse no debo esperar 1 minuto !!!
	 * La llave de mantenimiento t el boton de alarma corresponden a entradas
	 * digitales normales ( 6 y 7 ) por lo tanto deben activarse contra VCC
	 * La entrada del sensor de puerta se conecta a CNT0 que tiene un pull-up
	 * por lo tanto leo 1.
	 * La idea es que en reposo, el estado que midamos sea el 'activo' de modo
	 * que en caso de falla, se detecte como una condición de alarma.
	 * - Llave de mantenimiento / boton de alarma: normalmente conectados a 12V
	 *   Se activan 'abriendo' el circuito
	 *
	 */


	dinputs_read( din );

	if ( din[IPIN_LLAVE_MANTENIMIENTO] == 0 ) {
		alarmVars.llave_mantenimiento_on = true;
	} else {
		alarmVars.llave_mantenimiento_on = false;
	}

	if ( din[IPIN_BOTON_ALARMA] == 0 ) {
		alarmVars.boton_alarma_pressed = true;
	} else {
		alarmVars.boton_alarma_pressed = false;
	}

	// La entrada del sensor de puerta la leo directamente de la entrda del CNT0
	if ( CNT_read_CNT0() == 0 ) {
		alarmVars.sensor_puerta_open = true;
	} else {
		alarmVars.sensor_puerta_open = false;
	}

}
//------------------------------------------------------------------------------------
// ACCIONES
//------------------------------------------------------------------------------------
static void ac_luz_verde( t_dev_action action)
{
	/*
	 *  En reposo quiero que el led este apagado.
	 *  El led esta conectado al polo NO por lo tanto
	 *  el transistor debe estar cortado o sea que el
	 *  MCP debe poner un 0 y en la salida aparecera un 1
	 */

static uint8_t l_state = 0;

	switch(action) {
	case act_OFF:
		if ( flash_luz_verde ) {
			flash_luz_verde = false;
			vTaskDelay( ( TickType_t)( 1500 / portTICK_RATE_MS ) );
		}
		u_write_output_pins( OPIN_LUZ_VERDE, 1 );
		l_state = 1;
		break;
	case act_ON:
		u_write_output_pins( OPIN_LUZ_VERDE, 0 );
		l_state = 0;
		break;
	case act_FLASH:
		flash_luz_verde = true;
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
static void ac_luz_roja( t_dev_action action)
{

	static uint8_t l_state = 0;

		switch(action) {
		case act_OFF:
			if ( flash_luz_roja ) {
				flash_luz_roja = false;
				vTaskDelay( ( TickType_t)( 1500 / portTICK_RATE_MS ) );
			}
			u_write_output_pins( OPIN_LUZ_ROJA, 1 );
			l_state = 1;
			break;
		case act_ON:
			u_write_output_pins( OPIN_LUZ_ROJA, 0 );
			l_state = 0;
			break;
		case act_FLASH:
			flash_luz_roja = true;
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
static void ac_luz_amarilla( t_dev_action action)
{

	static uint8_t l_state = 0;

		switch(action) {
		case act_OFF:
			if ( flash_luz_amarilla ) {
				flash_luz_amarilla = false;
				vTaskDelay( ( TickType_t)( 1500 / portTICK_RATE_MS ) );
			}
			u_write_output_pins( OPIN_LUZ_AMARILLA, 1 );
			l_state = 1;
			break;
		case act_ON:
			u_write_output_pins( OPIN_LUZ_AMARILLA, 0 );
			l_state = 0;
			break;
		case act_FLASH:
			flash_luz_amarilla = true;
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
static void ac_luz_naranja( t_dev_action action)
{

	static uint8_t l_state = 0;

		switch(action) {
		case act_OFF:
			if ( flash_luz_naranja ) {
				flash_luz_naranja = false;
				vTaskDelay( ( TickType_t)( 1500 / portTICK_RATE_MS ) );
			}
			u_write_output_pins( OPIN_LUZ_NARANJA, 1 );
			l_state = 1;
			break;
		case act_ON:
			u_write_output_pins( OPIN_LUZ_NARANJA, 0 );
			l_state = 0;
			break;
		case act_FLASH:
			flash_luz_naranja = true;
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
static void ac_luz_azul( t_dev_action action)
{

	static uint8_t l_state = 0;

		switch(action) {
		case act_OFF:
			if ( flash_luz_azul ) {
				flash_luz_azul = false;
				vTaskDelay( ( TickType_t)( 1500 / portTICK_RATE_MS ) );
			}
			u_write_output_pins( OPIN_LUZ_AZUL, 1 );
			l_state = 1;
			break;
		case act_ON:
			u_write_output_pins( OPIN_LUZ_AZUL, 0 );
			l_state = 0;
			break;
		case act_FLASH:
			flash_luz_azul = true;
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
static void ac_sirena(t_dev_action action)
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
static uint8_t ac_read_status_sensor_puerta(void)
{
	if ( alarmVars.sensor_puerta_open == true ) {
		return(1);
	} else {
		return(0);
	}
}
//------------------------------------------------------------------------------------
static uint8_t ac_read_status_mantenimiento(void)
{
	if ( alarmVars.llave_mantenimiento_on ) {
		return(1);
	} else {
		return(0);
	}
}
//------------------------------------------------------------------------------------
static uint8_t ac_read_status_boton_alarma(void)
{

	if ( alarmVars.boton_alarma_pressed) {
		return(1);
	} else {
		return(0);
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

	xprintf_P(PSTR("p0=%s, p1=%s, p2=%s, p3=%s, p4=%s\r\n"), param0, param1, param2,param3, param4);

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
		if (pos >= MAX_NRO_SMS_ALARMAS ){
			return false;
		}

		nivel_alarma = atoi(param3);
		if ( ( nivel_alarma <= 0 ) || ( nivel_alarma > ALARMA_NIVEL_3) ) {
			return(false);
		}
		memcpy( systemVars.aplicacion_conf.alarma_ppot.l_sms[pos].sms_nro, param2, SMS_NRO_LENGTH );
		systemVars.aplicacion_conf.alarma_ppot.l_sms[pos].alm_level = nivel_alarma;
		return(true);

	}

	// config appalarm nivel {chid} {alerta} {inf|sup} val\r\n\0"));
	if (!strcmp_P( strupr(param0), PSTR("NIVEL\0")) ) {

		if ( param4 == NULL ) {
			return(false);
		}

		pos = atoi(param1);
		if ( pos > NRO_CANALES_ALM ) {
			return (false);
		}

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
		memcpy( systemVars.aplicacion_conf.alarma_ppot.l_sms[i].sms_nro, "099123456", SMS_NRO_LENGTH );
		if ( i < 3 ) {
			systemVars.aplicacion_conf.alarma_ppot.l_sms[i].alm_level = 1;
		} else if ( ( i >= 3) && ( i < 6 )) {
			systemVars.aplicacion_conf.alarma_ppot.l_sms[i].alm_level = 2;
		} else {
			systemVars.aplicacion_conf.alarma_ppot.l_sms[i].alm_level = 3;
		}
	}

}
//------------------------------------------------------------------------------------
void appalarma_servicio_tecnico( char *action, char *device )
{
	// action: (prender/apagar/flash,rtimers )
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
			ac_luz_roja(act_ON);
			return;
		} else 	if ( !strcmp_P( strupr(action), PSTR("APAGAR\0"))) {
			ac_luz_roja(act_OFF);
			return;
		}  else if ( !strcmp_P( strupr(action), PSTR("FLASH\0"))) {
			ac_luz_roja(act_FLASH);
			return;
		}
		xprintf_P(PSTR("ERROR\r\n\0"));
		return;
	}

	if ( !strcmp_P( strupr(device), PSTR("LVERDE\0"))) {
		if ( !strcmp_P( strupr(action), PSTR("PRENDER\0"))) {
			ac_luz_verde(act_ON);
			return;
		} else 	if ( !strcmp_P( strupr(action), PSTR("APAGAR\0"))) {
			ac_luz_verde(act_OFF);
			return;
		}  else if ( !strcmp_P( strupr(action), PSTR("FLASH\0"))) {
			ac_luz_verde(act_FLASH);
			return;
		}
		xprintf_P(PSTR("ERROR\r\n\0"));
		return;
	}

	if ( !strcmp_P( strupr(device), PSTR("LAMARILLA\0"))) {
		if ( !strcmp_P( strupr(action), PSTR("PRENDER\0"))) {
			ac_luz_amarilla(act_ON);
			return;
		} else 	if ( !strcmp_P( strupr(action), PSTR("APAGAR\0"))) {
			ac_luz_amarilla(act_OFF);
			return;
		}  else if ( !strcmp_P( strupr(action), PSTR("FLASH\0"))) {
			ac_luz_amarilla(act_FLASH);
			return;
		}
		xprintf_P(PSTR("ERROR\r\n\0"));
		return;
	}

	if ( !strcmp_P( strupr(device), PSTR("LNARANJA\0"))) {
		if ( !strcmp_P( strupr(action), PSTR("PRENDER\0"))) {
			ac_luz_naranja(act_ON);
			return;
		} else 	if ( !strcmp_P( strupr(action), PSTR("APAGAR\0"))) {
			ac_luz_naranja(act_OFF);
			return;
		} else if ( !strcmp_P( strupr(action), PSTR("FLASH\0"))) {
			ac_luz_naranja(act_FLASH);
			return;
		}
		xprintf_P(PSTR("ERROR\r\n\0"));
		return;
	}

	if ( !strcmp_P( strupr(device), PSTR("LAZUL\0"))) {
		if ( !strcmp_P( strupr(action), PSTR("PRENDER\0"))) {
			ac_luz_azul(act_ON);
			return;
		} else 	if ( !strcmp_P( strupr(action), PSTR("APAGAR\0"))) {
			ac_luz_azul(act_OFF);
			return;
		}  else if ( !strcmp_P( strupr(action), PSTR("FLASH\0"))) {
			ac_luz_azul(act_FLASH);
			return;
		}
		xprintf_P(PSTR("ERROR\r\n\0"));
		return;
	}

	if ( !strcmp_P( strupr(device), PSTR("SIRENA\0"))) {
		if ( !strcmp_P( strupr(action), PSTR("PRENDER\0"))) {
			ac_sirena(act_ON);
			return;
		} else 	if ( !strcmp_P( strupr(action), PSTR("APAGAR\0"))) {
			ac_sirena(act_OFF);
			return;
		}
		xprintf_P(PSTR("ERROR\r\n\0"));
		return;
	}

	xprintf_P(PSTR("ERROR\r\n\0"));
	return;
}
//------------------------------------------------------------------------------------
void appalarma_print_status(void)
{

uint8_t pos;

	xprintf_P( PSTR("  modo: PLANTAPOT\r\n\0"));
	// Configuracion de los SMS
	xprintf_P( PSTR("  Nros.SMS configurados\r\n\0"));
	xprintf_P( PSTR("    ALM_Nivel_1:"));
	for (pos = 0; pos < MAX_NRO_SMS_ALARMAS; pos++) {
		if ( systemVars.aplicacion_conf.alarma_ppot.l_sms[pos].alm_level == 1) {
			xprintf_P( PSTR("    sms%02d: %s "),pos,systemVars.aplicacion_conf.alarma_ppot.l_sms[pos].sms_nro );
		}
	}
	xprintf_P( PSTR("\r\n"));
	xprintf_P( PSTR("    ALM_Nivel_2:"));
	for (pos = 0; pos < MAX_NRO_SMS_ALARMAS; pos++) {
		if ( systemVars.aplicacion_conf.alarma_ppot.l_sms[pos].alm_level == 2) {
			xprintf_P( PSTR("    sms%02d: %s "),pos,systemVars.aplicacion_conf.alarma_ppot.l_sms[pos].sms_nro );
		}
	}
	xprintf_P( PSTR("\r\n"));
	xprintf_P( PSTR("    ALM_Nivel_3:"));
	for (pos = 0; pos < MAX_NRO_SMS_ALARMAS; pos++) {
		if ( systemVars.aplicacion_conf.alarma_ppot.l_sms[pos].alm_level == 3) {
			xprintf_P( PSTR("    sms%02d: %s "),pos,systemVars.aplicacion_conf.alarma_ppot.l_sms[pos].sms_nro );
		}
	}
	xprintf_P( PSTR("\r\n"));

	// Configuracion de los canales y niveles de alarma configurados
	xprintf_P( PSTR("  Niveles de alarma:\r\n\0"));
	for ( pos=0; pos<NRO_CANALES_ALM; pos++) {
		if ( strcmp ( systemVars.ainputs_conf.name[pos], "X" ) != 0 ){
			xprintf_P( PSTR("    ch%d: ALM_L1:(%.02f,%.02f),"),pos, systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[pos].alarma1.lim_inf,  systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[pos].alarma1.lim_sup);
			xprintf_P( PSTR(" ALM_L2:(%.02f,%.02f),"), systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[pos].alarma2.lim_inf,  systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[pos].alarma2.lim_sup);
			xprintf_P( PSTR(" ALM_L3:(%.02f,%.02f) \r\n\0"),  systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[pos].alarma3.lim_inf,  systemVars.aplicacion_conf.alarma_ppot.l_niveles_alarma[pos].alarma3.lim_sup);
		}
	}

	if ( ac_read_status_mantenimiento() ) {
		xprintf_P( PSTR("    mantenimiento: ON\r\n\0"));
	} else {
		xprintf_P( PSTR("    mantenimiento: OFF\r\n\0"));
	}

	if ( ac_read_status_sensor_puerta() ) {
		xprintf_P( PSTR("    Puerta: ABIERTA\r\n\0"));
	} else {
		xprintf_P( PSTR("    Puerta: CERRADA\r\n\0"));
	}

	if ( ac_read_status_boton_alarma() ) {
		xprintf_P( PSTR("    boton alarma: PRESIONADO\r\n\0"));
	} else {
		xprintf_P( PSTR("    boton alarma: NORMAL\r\n\0"));
	}

}
//------------------------------------------------------------------------------------
