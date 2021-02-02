/*
 * tkApp_piloto.c
 *
 *  Created on: 12 ene. 2021
 *      Author: pablo
 */

#include "tkApp.h"
#include "l_steppers.h"

// LAS PRESIONES SE MIDEN EN GRS. !!!

struct {
	bool start_presion_test;
	bool start_steppers_test;
	int16_t npulses;
	uint16_t dtime;
	uint16_t ptime;
	t_stepper_dir dir;
	int8_t slot;
	uint16_t pRef;
	uint16_t pB;
	uint16_t pError;
	int8_t pB_channel;
} spiloto;

void pvstk_piloto_presion_test( void );
void pvstk_piloto_stepper_test( void );
void pvstk_piloto(void);


#define MAX_INTENTOS	5
#define P_CONSIGNA_MIN	1000
#define P_CONSIGNA_MAX	3000
#define MAX_P_SAMPLES	10
#define P_SAMPLES		5
#define PULSOS_X_REV	3000		// 3000 pulsos para girar 1 rev
#define DPRES_X_REV		350			// 350 gr c/rev del piloto

void pv_calcular_pulsos(void);
void pv_aplicar_pulsos(void);
bool pv_determinar_canal_pB(void);
void pv_leer_presion_baja( int8_t samples, uint16_t intervalo_secs );
bool pv_select_slot_a_aplicar(void);
void pvstk_ajustar_presion(void);
void pv_print_parametros(void);

//------------------------------------------------------------------------------------
void tkApp_piloto(void)
{
	// Las salidas estan configuradas para modo consigna.
	// c/30s reviso si debo aplicar una o la otra y aplico
	// Espero con lo que puedo entrar en tickless

	xprintf_P( PSTR("APP: PILOTO start\r\n\0"));
	spiloto.start_presion_test = false;
	spiloto.start_steppers_test = false;

	for (;;) {

		ctl_watchdog_kick( WDG_APP,  WDG_APP_TIMEOUT );

		vTaskDelay( ( TickType_t)( 30000 / portTICK_RATE_MS ) );

		// Test de presion
		if ( spiloto.start_presion_test ) {
			pvstk_piloto_presion_test();
			spiloto.start_presion_test = false;
		}

		// Test de movimiento de stepper
		if ( spiloto.start_steppers_test ) {
			pvstk_piloto_stepper_test();
			spiloto.start_steppers_test = false;
		}

		// Modo normal.
		pvstk_piloto();

	}
}
//------------------------------------------------------------------------------------
// FUNCIONES PUBLICAS
//------------------------------------------------------------------------------------
void xAPP_piloto_config_defaults(void)
{
	/*
	 * Configura el default del piloto con todos los valores en 0
	 */

uint8_t slot;

	for (slot=0; slot<MAX_PILOTO_PSLOTS ;slot++) {
		sVarsApp.pSlots[slot].hhmm.hour = 0;
		sVarsApp.pSlots[slot].hhmm.min = 0;
		sVarsApp.pSlots[slot].presion = 0.0;
	}

}
//------------------------------------------------------------------------------------
void xAPP_piloto_print_status( void )
{

uint8_t slot;

	xprintf_P( PSTR("  modo: Piloto\r\n\0"));
	xprintf_P( PSTR("  Slots: "));
	for (slot=0; slot<MAX_PILOTO_PSLOTS ;slot++) {
		xprintf_P( PSTR("[%02d]%02d:%02d->%0.2f "), slot,sVarsApp.pSlots[slot].hhmm.hour,sVarsApp.pSlots[slot].hhmm.min,sVarsApp.pSlots[slot].presion  );
	}
	xprintf_P( PSTR("\r\n"));
}
//------------------------------------------------------------------------------------
bool xAPP_piloto_config( char *param1, char *param2, char *param3, char *param4 )
{
	// Configura un slot

uint8_t slot;

	if (!strcmp_P( strupr(param1), PSTR("SLOT\0"))) {

		// Intervalos tiempo:presion.
		slot = atoi(param2);
		if ( slot < MAX_PILOTO_PSLOTS ) {
			if ( param3 != NULL ) {
				u_convert_int_to_time_t( atoi( param3), &sVarsApp.pSlots[slot].hhmm );
			}
			if ( param4 != NULL ) {
				sVarsApp.pSlots[slot].presion = atof(param4);
			}
			return(true);
		}
	}

	return(false);
}
//------------------------------------------------------------------------------------
void xAPP_piloto_stepper_test( char *s_dir, char *s_npulses, char *s_dtime, char *s_ptime )
{
	// Genera la señal de arranque del test de movimiento del stepper del piloto.

	if ( strcmp_P( strupr(s_dir), PSTR("FW")) == 0 ) {
		spiloto.dir = STEPPER_FWD;
	} else if ( strcmp_P( strupr(s_dir), PSTR("REV")) == 0) {
		spiloto.dir = STEPPER_REV;
	} else {
		xprintf_P(PSTR("Error en direccion\r\n"));
		return;
	}

	spiloto.npulses = atoi(s_npulses);
	spiloto.dtime = atoi(s_dtime);
	spiloto.ptime = atoi(s_ptime);
	spiloto.start_steppers_test = true;
}
//------------------------------------------------------------------------------------
// Chequear
void xAPP_piloto_presion_test( char *s_out_pres, char *s_out_error )
{
	// Genera la señal de arranque del test de presion del piloto.
	// Fija una presion de referencia y hace que el piloto se mueva para regular en
	// este punto

	spiloto.pRef = atoi(s_out_pres);
	spiloto.pError = atoi(s_out_error);
	spiloto.start_presion_test = true;

}
//------------------------------------------------------------------------------------
// FUNCIONES PRIVADAS
//------------------------------------------------------------------------------------
void pv_aplicar_pulsos(void)
{
	// Giro el motor

uint16_t steps;
int8_t sequence = 2;

	// Activo el driver
	xprintf_P(PSTR("STEPPER driver pwr on\r\n"));
	stepper_pwr_on();
	// Espero 15s que se carguen los condensasores
	vTaskDelay( ( TickType_t)( spiloto.ptime*1000 / portTICK_RATE_MS ) );
	stepper_awake();
	vTaskDelay( ( TickType_t)( 100 / portTICK_RATE_MS ) );

	for (steps=0; steps < spiloto.npulses; steps++) {
		sequence = stepper_sequence(sequence, spiloto.dir );
		if ( (steps % 100) == 0 ) {
			xprintf_P(PSTR("pulse %03d, sec=%d\r\n"), steps, sequence);
		}
		stepper_pulse(sequence, spiloto.dtime);
		ctl_watchdog_kick( WDG_APP,  WDG_APP_TIMEOUT );
	}

	// Desactivo el driver
	xprintf_P(PSTR("STEPPER driver pwr off\r\n"));
	stepper_sleep();
	stepper_pwr_off();
}
//------------------------------------------------------------------------------------
void pvstk_piloto_stepper_test( void )
{
	/*
	 * Funcion privada, subtask que mueve el stepper en una direccion dada, una
	 * cantidad de pulsos dados, con un tiempo entre pulsos dados.
	 * Genera en el stepper una cantidad de pulsos npulses, separados
	 * un tiempo dtime entre c/u, de modo de girar el motor en la
	 * direccion dir.
	 *
	 */

	pv_print_parametros();
	pv_aplicar_pulsos();

}
//------------------------------------------------------------------------------------
void pv_print_parametros(void)
{
	xprintf_P(PSTR("	pB_channel=%d\r\n"),spiloto.pB_channel );
	xprintf_P(PSTR("	pB=%d\r\n"),spiloto.pB );
	xprintf_P(PSTR("	pRef=%d\r\n"),spiloto.pRef );
	xprintf_P(PSTR("	pulses=%d\r\n"),spiloto.npulses );
	xprintf_P(PSTR("	ptime=%d\r\n"),spiloto.ptime );
	xprintf_P(PSTR("	dtime=%d\r\n"),spiloto.dtime );
	if ( spiloto.dir == STEPPER_FWD ) {
		xprintf_P(PSTR("	dir=Forward\r\n"));
	} else {
		xprintf_P(PSTR("	dir=Reverse\r\n"));
	}
}
//------------------------------------------------------------------------------------
// Chequear
void pv_app_init(void)
{

}
//------------------------------------------------------------------------------------
void pv_calcular_pulsos(void)
{
	/*
	 * El motor es de 200 pasos /rev
	 * El servo reduce 15:1
	 * Esto hace que para girar el vastago 1 rev necesite 3000 pulsos
	 * El piloto es de 4rev->1500gr.
	 */

uint16_t dPres;

	// Calculo los pulsos aproximadamente
	spiloto.pRef = sVarsApp.pSlots[spiloto.slot].presion;
	dPres = abs(spiloto.pB - spiloto.pRef);
	spiloto.npulses = ( dPres * PULSOS_X_REV ) / DPRES_X_REV;

	// Calculo el sentido del giro
	if ( spiloto.pB < spiloto.pRef ) {
		spiloto.dir = STEPPER_FWD; // Giro forward, aprieto el tornillo, aumento la presion de salida
	} else {
		spiloto.dir = STEPPER_REV;
	}

	// Intervalo de tiempo entre pulsos en ms.
	spiloto.dtime = 10;
	spiloto.ptime = 10;

}
//------------------------------------------------------------------------------------
bool pv_select_slot_a_aplicar(void)
{
	// Dada la hora actual, determina que presion de slot corresponde aplicar
	// Determina en base a la hhmm actuales, en cual pslot estoy.
	// LOS PSLOTS DEBEN ESTAR ORDENADOS POR FECHA.
	// La presion=0.0  se usa como un tag para indicar vacio !!!!
	// Si no tengo ningun slot configurado salgo.

RtcTimeType_t rtcDateTime;
uint16_t time_now_s, time_slot_s;
uint8_t i;
int8_t slot;
int8_t last_slot = -1;

	// Paso 1.
	// Vemos si hay slots configuradas.
	// Solo chequeo el primero. DEBEN ESTAR ORDENADOS !!
	if ( sVarsApp.pSlots[0].presion < 0.1 ) {
		spiloto.slot = -1;
		return(false);
	}

	// Paso 2.
	// Hay slots configurados: Veo cual es el ultimo. Parto del 2 porque
	// el 1 se que existe del paso anterior.
	last_slot = MAX_PILOTO_PSLOTS - 1;
	for ( i = 0; i < MAX_PILOTO_PSLOTS; i++ ) {
		if ( sVarsApp.pSlots[0].presion < 0.1 ) {
			last_slot = (i-1);
			break;
		}
	}

	// Paso 3.
	// Determino en que hora estoy: NOW
	memset( &rtcDateTime, '\0', sizeof(RtcTimeType_t));
	if ( ! RTC_read_dtime(&rtcDateTime) ) {
		xprintf_P(PSTR("PILOTO ERROR: I2C:RTC:pv_dout_chequear_consignas\r\n\0"));
		spiloto.slot = -1;
		return(false);
	}
	time_now_s = rtcDateTime.hour * 60 + rtcDateTime.min;

	// Paso 4.
	// Vemos a que slot de tiempo corresponde NOW
	for ( i = 0; i <= last_slot; i++ ) {
		time_slot_s = sVarsApp.pSlots[i].hhmm.hour * 60 + sVarsApp.pSlots[i].hhmm.min;
		if ( time_now_s < time_slot_s ) {
			if ( i == 0 ) {
				slot = last_slot;
			} else {
				slot = i - 1;
			}
			spiloto.slot = slot;
			xprintf_P(PSTR("PILOTO: slot a aplicar %d\r\n"), slot);
			return(true);
		}
	}

	slot= last_slot;
	spiloto.slot = slot;
	xprintf_P(PSTR("PILOTO: slot a aplicar %d\r\n"), slot);
	return(true);
}
//------------------------------------------------------------------------------------
bool pv_determinar_canal_pB(void)
{
	// Busca en la configuracion de los canales aquel que se llama pB

uint8_t i;
bool sRet = false;

	spiloto.pB_channel = -1;
	for ( i = 0; i < NRO_ANINPUTS; i++) {
		if ( strcmp_P( strupr(systemVars.ainputs_conf.name[i]), "PB") ) {
			spiloto.pB_channel = i;
			xprintf_P(PSTR("PILOTO: pBchannel=%d\r\n"), spiloto.pB_channel);
			sRet = true;
			break;
		}
	};

	return(sRet);


}
//------------------------------------------------------------------------------------
void pv_leer_presion_baja( int8_t samples, uint16_t intervalo_secs )
{
	// Medir la presión de baja N veces a intervalos de 10 s y promediar

uint8_t i;
float pB;

	// Mido pB
	spiloto.pB_channel = 0;
	for ( i = 0; i < samples; i++) {
		pB = ainputs_read_channel(spiloto.pB_channel);
		xprintf_P(PSTR("PILOTO pB:[%d]->%0.3f"), i, pB );
		spiloto.pB_channel += (uint16_t)pB;
		vTaskDelay( ( TickType_t)( intervalo_secs * 1000 / portTICK_RATE_MS ) );
	}
	spiloto.pB_channel /= samples;

}
//------------------------------------------------------------------------------------
void pvstk_piloto_presion_test( void )
{
	// Funcion privada, subtask que dada una presion de referencia, mueve el piloto
	// hasta lograrla
	pvstk_ajustar_presion();
}
//------------------------------------------------------------------------------------
void pvstk_piloto(void)
{
	// Tarea propia de ajustar el piloto a la presion de la consigna del slot
}
//------------------------------------------------------------------------------------
void pvstk_ajustar_presion(void)
{

uint8_t loops;

	// Realiza la tarea de mover el piloto hasta ajustar la presion
	xprintf_P(PSTR("PILOTO: determino canal de pB...r\n"));
	if ( !pv_determinar_canal_pB() ) {
		xprintf_P(PSTR("PILOTO: No se puede determinar el canal de pB.!!\r\n"));
		return;
	}

	for ( loops = 0; loops < MAX_INTENTOS; loops++ ) {

		xprintf_P(PSTR("PILOTO: loop[%d]\r\n"), loops);
		xprintf_P(PSTR("	-leo pB...\r\n"));
		pv_leer_presion_baja(5,10);

		xprintf_P(PSTR("	-calculo npulsos...\r\n"));
		pv_calcular_pulsos();

		// Muestro el resumen de datos


		// Si la diferencia de presiones es superior al error tolerable muevo el piloto
		if ( abs(spiloto.pB - spiloto.pRef) > spiloto.pError ) {
			pv_aplicar_pulsos();
			//Espero que se estabilize la presión
			vTaskDelay( ( TickType_t)( 15000 / portTICK_RATE_MS ) );
		} else {
			xprintf_P(PSTR("Presion alcanzada\r\n"));
			break;
		}
	}
}
