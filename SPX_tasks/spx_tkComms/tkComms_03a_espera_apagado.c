/*
 * tkComms_03_espera_apagado.c
 *
 *  Created on: 5 mar. 2020
 *      Author: pablo
 */

#include <tkComms.h>

static void configurar_tiempo_espera_apagado(void);
static void esperar_apagado(void);
static bool dentro_de_pwrSave(void);

// La tarea pasa por el mismo lugar c/1s.
#define WDG_COMMS_TO_ESPERA_OFF	30

static int32_t time_to_next_dial;

//------------------------------------------------------------------------------------
t_comms_states tkComms_st_espera_apagado(void)
{
	/*
	 * Estado que corresponde a esperar con el dispositivo apagado.
	 * Debe monitorear un timeout, las señales ( redial, send_sms ) para
	 * salir del sistema.
	 *
	 * Salidas:
	 * 	ST_PRENDER
	 *
	 */

// Entry:
	// Apago el dispositivo de comunicaciones.
	xprintf_PD( DF_COMMS, PSTR("COMMS: IN st_espera_apagado.\r\n\0"));

	xCOMMS_apagar_dispositivo();
	configurar_tiempo_espera_apagado();

// Loop:
	esperar_apagado();

// Exit:
	xprintf_PD( DF_COMMS, PSTR("COMMS: OUT st_espera_apagado.\r\n\0"));
	return(ST_PRENDER);

}
//------------------------------------------------------------------------------------
static void configurar_tiempo_espera_apagado(void)
{
	/*
	 * Dependiendo de la situacion es que calculo el tiempo que voy a estar esperando
	 * Es importante ya que en la espera es donde entro en tickless y
	 * ahorro energia.
	 * El tiempo es contado por un timer del sistema de TIMERS, para ahorrar energia.
	 *
	 */

static bool starting_flag = true;

	// Cuando arranco ( la primera vez) solo espero 10s y disco por primera vez
	if ( starting_flag ) {
		time_to_next_dial = 10;
		starting_flag = false;	// Ya no vuelvo a esperar 10s.
		//xprintf_PD( DF_COMMS, PSTR("COMMS: st_espera_apagado: starting_flag\r\n\0"));
		goto EXIT;
	}

	// En modo DISCRETO ( timerDial > 900 )
	if ( MODO_DISCRETO ) {
		time_to_next_dial = systemVars.comms_conf.timerDial;
		goto EXIT;
	} else {
		time_to_next_dial = 60;
	}

EXIT:

	/*
	 * Arranco el timer que va a generar la espera.
	 */
	ctl_set_timeToNextDial( time_to_next_dial );

	xprintf_PD( DF_COMMS, PSTR("COMMS: await %lu s\r\n\0"), time_to_next_dial );

}
//------------------------------------------------------------------------------------
static void esperar_apagado(void)
{
	/*
	 * Espero monitoreando las señales.
	 * Para entrar en tickless, el tiempo minimo util deben ser 5s.
	 * Por lo tanto c/10s me despierto y reviso las señales a ver si sigo
	 * durmiendo o debo salir.
	 * Salgo porque expiro el tiempo o porque recibi alguna señal.
	 */

	while ( time_to_next_dial > 0 )  {

		//xprintf_PD( DF_COMMS, PSTR("COMMS: st_espera_apagado.(1)TND=%d\r\n\0"), time_to_next_dial );

		// A) Reinicio el watchdog
		ctl_watchdog_kick(WDG_COMMS, WDG_COMMS_TO_ESPERA_OFF );

		// B) Espero de a 10s para poder entrar en tickless.
		vTaskDelay( (portTickType)( 10000 / portTICK_RATE_MS ) );

		// C) Ajusto el timer dial.
		if ( time_to_next_dial > 0) {
			// Expiro el tiempo ?.
			time_to_next_dial = time_to_next_dial - 10;
			//xprintf_PD( DF_COMMS, PSTR("COMMS: st_espera_apagado.(2)TND=%d\r\n\0"), time_to_next_dial );
			if (  time_to_next_dial <= 0 ) {
				time_to_next_dial = 0;

			}
		}

		// D) Proceso las señales:
		if ( SPX_SIGNAL( SGN_MON_SQE )) {
			/*
			 * La señal de monitorear sqe no la borro nunca ya que es un
			 * estado en el que entro en modo diagnostico y no debo salir mas.
			 */
			xprintf_PD( DF_COMMS, PSTR("COMMS: st_espera_apagado. SGN_MON_SQE rcvd.\r\n\0"));
			goto EXIT;
		}

		if ( SPX_SIGNAL( SGN_REDIAL )) {
			/*
			 * Debo apagar y prender el dispositivo. Como ya estoy apagado
			 * salgo para pasar al estado PRENDIENDO.
			 */
			xprintf_PD( DF_COMMS, PSTR("COMMS: st_espera_apagado. SGN_REDIAL rcvd.\r\n\0"));
			goto EXIT;

		}

		// E) Veo si expiro el tiempo de espera.
		if ( time_to_next_dial == 0 ) {
			/*
			 * Veo si estoy dentro del intervalo de
			 * pwrSave y entonces espero de a 10 minutos mas.
			 */
			if ( dentro_de_pwrSave() ) {
				time_to_next_dial = 600;
			} else {
				// Expiro el tiempo: salgo.
				goto EXIT;
			}
		}

	}

EXIT:
	//xprintf_PD( DF_COMMS, PSTR("COMMS: st_espera_apagado.(3)TND=%d\r\n\0"), time_to_next_dial );
	return;

}
//------------------------------------------------------------------------------------
static bool dentro_de_pwrSave(void)
{
	/*
	 * Calculo el waiting time en modo DISCRETO evaluando si estoy dentro
	 * del periodo de pwrSave.
	 * En caso afirmativo, seteo el tiempo en 10mins ( 600s )
	 * En caso negativo, lo seteo en systemVars.timerDial
	 */

static bool starting_flag_pws = true;

bool insidePwrSave_flag = false;
RtcTimeType_t rtc;
uint16_t now = 0;
uint16_t pwr_save_start = 0;
uint16_t pwr_save_end = 0;
int8_t xBytes = 0;

	memset( &rtc, '\0', sizeof(RtcTimeType_t));

	// Estoy en modo PWR_DISCRETO con PWR SAVE ACTIVADO
	if ( ( MODO_DISCRETO ) && ( spx_io_board == SPX_IO5CH ) && ( systemVars.comms_conf.pwrSave.pwrs_enabled == true )) {

		// Cuando arranco siempre me conecto sin importar si estoy o no en pwr save !!
		if ( starting_flag_pws ) {
			starting_flag_pws = false;
			insidePwrSave_flag = false;
			goto EXIT;
		}

		xBytes = RTC_read_dtime(&rtc);
		if ( xBytes == -1 )
			xprintf_P(PSTR("ERROR: I2C:RTC:pv_tkGprs_check_inside_pwrSave\r\n\0"));

		now = rtc.hour * 60 + rtc.min;
		pwr_save_start = systemVars.comms_conf.pwrSave.hora_start.hour * 60 + systemVars.comms_conf.pwrSave.hora_start.min;
		pwr_save_end = systemVars.comms_conf.pwrSave.hora_fin.hour * 60 + systemVars.comms_conf.pwrSave.hora_fin.min;

		if ( pwr_save_start < pwr_save_end ) {
			// Caso A:
			if ( now <= pwr_save_start ) { insidePwrSave_flag = false; goto EXIT; }
			// Caso B:
			if ( ( pwr_save_start <= now ) && ( now <= pwr_save_end )) { insidePwrSave_flag = true; goto EXIT; }
			// Caso C:
			if ( now > pwr_save_end ) { insidePwrSave_flag = false; goto EXIT; }
		}

		if (  pwr_save_end < pwr_save_start ) {
			// Caso A:
			if ( now <=  pwr_save_end ) { insidePwrSave_flag = true; goto EXIT; }
			// Caso B:
			if ( ( pwr_save_end <= now ) && ( now <= pwr_save_start )) { insidePwrSave_flag = false; goto EXIT; }
			// Caso C:
			if ( now > pwr_save_start ) { insidePwrSave_flag = true; goto EXIT; }
		}

	}

EXIT:

	if ( insidePwrSave_flag == true ) {
		xprintf_PD( DF_COMMS, PSTR("COMMS: inside pwrsave\r\n\0"));
	} else {
		xprintf_PD( DF_COMMS, PSTR("COMMS: out pwrsave\r\n\0"));
	}

	return(insidePwrSave_flag);

}
//------------------------------------------------------------------------------------
// FUNCIONES PUBLICAS
//------------------------------------------------------------------------------------
int32_t xcomms_time_to_next_dial(void)
{
	return(time_to_next_dial);
}
//------------------------------------------------------------------------------------

