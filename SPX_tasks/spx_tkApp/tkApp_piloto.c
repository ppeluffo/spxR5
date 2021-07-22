/*
 * tkApp_piloto.c
 *
 *  Created on: 12 ene. 2021
 *      Author: pablo
 *
 *  Aproximarme siempre por abajo. Apuntar al 75% ( modificarlo x loop nbr)
 *  Los pilotos demoran en ajustar la presion mucho tiempo !!!
 *  Resultado al finalizar el ajuste.
 *  Numca mas de 1 vuelta
 *  Al bajar la presion el pilot demora mas en ajustar. !!!
 *  Controlar c/15 mins que este en el intervalo de p+-error
 *
 *
 *
 */

#include "tkApp.h"
#include "l_steppers.h"

// LAS PRESIONES SE MIDEN EN GRS. !!!

typedef enum { AJUSTE70x100 = 0, AJUSTE_BASICO = 1 } t_ajuste_npulses;
typedef enum { TICK_NORMAL = 0, TICK_INICIO, TICK_CONTROL } t_slot_time_ticks;

#define MAX_INTENTOS			5
#define P_CONSIGNA_MIN			1000
#define P_CONSIGNA_MAX			3000
#define MAX_P_SAMPLES			10
#define P_SAMPLES				5
#define PULSOS_X_REV			3000		// 3000 pulsos para girar 1 rev
#define DPRES_X_REV				0.500		// 500 gr c/rev del piloto
#define PERROR					0.065
#define INTERVALO_PB_SECS		5
#define INTERVALO_TRYES_SECS	15

#define MODO_STANDARD	false
#define MODO_TEST		true

void pv_calcular_parametros_ajuste(void);
void pv_aplicar_pulsos(void);
bool pv_determinar_canales_presion(void);
void pv_leer_presiones( int8_t samples, uint16_t intervalo_secs );
int8_t pv_get_slot_actual(uint16_t hhmm );
t_slot_time_ticks pv_check_tipo_tick( uint16_t hhmm, int8_t slot );
bool pv_get_hhhmm_now( uint16_t *hhmm);

void pv_ajustar_presion(bool modo_test);
void pv_print_parametros(void);
void pv_calcular_npulses( t_ajuste_npulses metodo_ajuste );
void pv_xapp_init(void);

void pvstk_piloto(void);

//------------------------------------------------------------------------------------
void tkApp_piloto(void)
{
	// Las salidas estan configuradas para modo consigna.
	// c/30s reviso si debo aplicar una o la otra y aplico
	// Espero con lo que puedo entrar en tickless

	pv_xapp_init();

	for (;;) {

		ctl_watchdog_kick( WDG_APP,  WDG_APP_TIMEOUT );

		vTaskDelay( ( TickType_t)( 25000 / portTICK_RATE_MS ) );

		// Test de presion
		if ( spiloto.start_presion_test ) {
			run_piloto_presion_test();
			spiloto.start_presion_test = false;
		}

		// Test de movimiento de stepper
		if ( spiloto.start_steppers_test ) {
			run_piloto_stepper_test();
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

		// Intervalos tiempo:presion:

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
void xAPP_piloto_stepper_test( char *s_dir, char *s_npulses, char *s_pwidth, char *s_startup_time )
{
	/* Genera la señal de arranque del test de movimiento del stepper del piloto.
	 * Parametros:
	 * - Direccion
	 * - Cantidad de pulsos
	 * - dtime: duracion del pulso
	 * - ptime: Tiempo de espera de carga de condensadores
	 */


	if ( strcmp_P( strupr(s_dir), PSTR("FW")) == 0 ) {
		spiloto.dir = STEPPER_FWD;
	} else if ( strcmp_P( strupr(s_dir), PSTR("REV")) == 0) {
		spiloto.dir = STEPPER_REV;
	} else {
		xprintf_P(PSTR("Error en direccion\r\n"));
		return;
	}

	spiloto.npulses = atoi(s_npulses);
	spiloto.pwidth = atoi(s_pwidth);
	spiloto.startup_time = atoi(s_startup_time);
	spiloto.start_steppers_test = true;
}
//------------------------------------------------------------------------------------
void xAPP_piloto_presion_test( char *s_out_pres, char *s_out_error )
{
	// Genera la señal de arranque del test de presion del piloto.
	// Fija una presion de referencia y hace que el piloto se mueva para regular en
	// este punto

	spiloto.pRef = atof(s_out_pres);
	spiloto.pError = atof(s_out_error);
	spiloto.start_presion_test = true;
}
//------------------------------------------------------------------------------------
uint8_t xAPP_piloto_hash( void )
{

	// En app_A_cks ponemos el checksum de los SLOTS

uint8_t hash = 0;
//char dst[48];
char *p;
uint8_t i;
uint8_t j;
int16_t free_size = sizeof(hash_buffer);

	memset(hash_buffer,'\0', sizeof(hash_buffer));

	j = 0;
	j += snprintf_P( hash_buffer, free_size, PSTR("PLT;"));
	free_size = (  sizeof(hash_buffer) - j );
	if ( free_size < 0 ) goto exit_error;

	// Apunto al comienzo para recorrer el buffer
	p = hash_buffer;
	while (*p != '\0') {
		hash = u_hash(hash, *p++);
	}

	// SLOTS
	for (i=0; i < MAX_PILOTO_PSLOTS;i++) {
		// Vacio el buffer temoral
		memset(hash_buffer,'\0', sizeof(hash_buffer));
		free_size = sizeof(hash_buffer);
		// Copio sobe el buffer una vista ascii ( imprimible ) de c/registro.
		j = snprintf_P( hash_buffer, free_size, PSTR("SLOT%d:%02d%02d,%0.2f;"), i, sVarsApp.pSlots[i].hhmm.hour, sVarsApp.pSlots[i].hhmm.min, sVarsApp.pSlots[i].presion );
		free_size = (  sizeof(hash_buffer) - j );
		if ( free_size < 0 ) goto exit_error;

		// Apunto al comienzo para recorrer el buffer
		p = hash_buffer;
		while (*p != '\0') {
			hash = u_hash(hash, *p++);
		}
	}

	return(hash);

exit_error:
	xprintf_P( PSTR("COMMS: piloto_hash ERROR !!!\r\n\0"));
	return(0x00);

}
//------------------------------------------------------------------------------------
// FUNCIONES PRIVADAS
//------------------------------------------------------------------------------------
void pv_xapp_init(void)
{

uint16_t hhmm_now;
int8_t slot;

	xprintf_P( PSTR("APP: PILOTO start\r\n\0"));
	spiloto.start_presion_test = false;
	spiloto.start_steppers_test = false;

	vTaskDelay( ( TickType_t)( 30000 / portTICK_RATE_MS ) );

	if ( pv_get_hhhmm_now( &hhmm_now)) {
		slot = pv_get_slot_actual(hhmm_now);
		spiloto.pRef = sVarsApp.pSlots[slot].presion;
		spiloto.pError = 0.05;
		pv_ajustar_presion(MODO_STANDARD);
	}
}
//------------------------------------------------------------------------------------
void pv_aplicar_pulsos(void)
{
	// Giro el motor

uint16_t steps;
int8_t sequence = 2;
//char cChar[] = {'|', '/','-','\\', '|','/','-', '\\' };
//uint8_t cChar_pos = 0;

	// Activo el driver
	xprintf_P(PSTR("STEPPER driver pwr on\r\n"));
	stepper_pwr_on();
	// Espero que se carguen los condensasores
	xprintf_P(PSTR("STEPPER driver charging start\r\n"));
	vTaskDelay( ( TickType_t)( spiloto.startup_time*1000 / portTICK_RATE_MS ) );
	stepper_awake();
	vTaskDelay( ( TickType_t)( 100 / portTICK_RATE_MS ) );

	xprintf_P(PSTR("STEPPER driver pulses start\r\n"));
	for (steps=0; steps < spiloto.npulses; steps++) {
		sequence = stepper_sequence(sequence, spiloto.dir );
		//xprintf_P(PSTR("pulse %03d, sec=%d\r\n"), steps, sequence);
		if ( (steps % 100) == 0 ) {
			//xprintf_P(PSTR("%c\r"),cChar[cChar_pos]);
			//if ( cChar_pos++ == 8)
			//	cChar_pos=0;
			xprintf_P(PSTR("."));
		}
		stepper_pulse(sequence, spiloto.pwidth);
		ctl_watchdog_kick( WDG_APP,  WDG_APP_TIMEOUT );
	}
	xprintf_P(PSTR("\r\n"));

	// Desactivo el driver
	xprintf_P(PSTR("STEPPER driver pwr off\r\n"));
	stepper_sleep();
	stepper_pwr_off();
}
//------------------------------------------------------------------------------------
void pv_print_parametros(void)
{
	xprintf_P(PSTR("    pA_channel=%d\r\n"),spiloto.pA_channel );
	xprintf_P(PSTR("    pA=%.02f\r\n"),spiloto.pA );
	xprintf_P(PSTR("    pB_channel=%d\r\n"),spiloto.pB_channel );
	xprintf_P(PSTR("    pB=%.02f\r\n"),spiloto.pB );
	xprintf_P(PSTR("    pRef=%.02f\r\n"),spiloto.pRef );
	xprintf_P(PSTR("    dP=%.03f\r\n"), (spiloto.pB - spiloto.pRef));
	xprintf_P(PSTR("    pulses=%d\r\n"),spiloto.npulses );
	xprintf_P(PSTR("    startup_time=%d\r\n"),spiloto.startup_time );
	xprintf_P(PSTR("    pwidth=%d\r\n"),spiloto.pwidth );
	if ( spiloto.dir == STEPPER_FWD ) {
		xprintf_P(PSTR("    dir=Forward\r\n"));
	} else {
		xprintf_P(PSTR("    dir=Reverse\r\n"));
	}
}
//------------------------------------------------------------------------------------
void pv_calcular_parametros_ajuste(void)
{

	// Paso 1: Calculo el sentido del giro
	if ( spiloto.pB < spiloto.pRef ) {
		// Debo aumentar pB o sxprintf_P(PSTR("PILOTO: npulses=%d\r\n"), spiloto.npulses);ea apretar el tornillo (FWD)
		spiloto.dir = STEPPER_FWD; // Giro forward, aprieto el tornillo, aumento la presion de salida
	} else {
		spiloto.dir = STEPPER_REV;
	}

	// Paso 2: Intervalo de tiempo entre pulsos en ms.
	spiloto.pwidth = 10;
	spiloto.startup_time = 30;

	// Paso 3: Calculo los pulsos a aplicar.
	pv_calcular_npulses(AJUSTE70x100);

}
//------------------------------------------------------------------------------------
void pv_calcular_npulses(t_ajuste_npulses metodo_ajuste )
{
	/*
	 * El motor es de 200 pasos /rev
	 * El servo reduce 15:1
	 * Esto hace que para girar el vastago 1 rev necesite 3000 pulsos
	 * El piloto es de 4->1500gr.
	 */

float delta_pres = 0.0;

	// Calculo los pulsos aproximadamente
	delta_pres = fabs(spiloto.pB - spiloto.pRef);
	spiloto.npulses = (uint16_t) ( delta_pres * PULSOS_X_REV  / DPRES_X_REV );
	if ( spiloto.npulses < 0) {
		xprintf_P(PSTR("PILOTO: ERROR npulses < 0\r\n"));
		spiloto.npulses = 0;
	}
	xprintf_P(PSTR("PILOTO: npulses_calc=%d\r\n"), spiloto.npulses);

	switch (metodo_ajuste) {
	case  AJUSTE70x100:
		/*
		 * METODO 1: Cuando estoy en reverse( bajando la presion ) aplico los pulsos calculados
		 *           Si estoy en forward (subiendo la presion), aplico solo el 70% de los pulsos
		 *           calculados si estos son altos.
		 *           Si son menos de 500 no lo corrijo.
		 */
		if ( spiloto.dir == STEPPER_FWD) {
			if (spiloto.npulses > 500 )
				spiloto.npulses = (uint16_t) (0.7 * spiloto.npulses);

			xprintf_P(PSTR("PILOTO: npulses_M1=%d\r\n"), spiloto.npulses);
		}
		break;
	case AJUSTE_BASICO:
		// METODO 2: Los pulsos son los que me da el calculo.
		xprintf_P(PSTR("PILOTO: npulses_M2=%d\r\n"), spiloto.npulses);
		break;
	}

	// Controlo no avanzar mas de 500gr aprox x loop !!!
	if ( spiloto.npulses > 3000 ) {
		spiloto.npulses = 3000;
		xprintf_P(PSTR("PILOTO: npulses_corr=%d\r\n"), spiloto.npulses);
	}

}
//------------------------------------------------------------------------------------
bool pv_get_hhhmm_now( uint16_t *hhmm)
{

RtcTimeType_t rtcDateTime;

	memset( &rtcDateTime, '\0', sizeof(RtcTimeType_t));
	if ( ! RTC_read_dtime(&rtcDateTime) ) {
		xprintf_P(PSTR("PILOTO ERROR: I2C:RTC:pv_get_hhhmm_now\r\n\0"));
		return(false);
	}

	*hhmm = rtcDateTime.hour * 100 + rtcDateTime.min;
	xprintf_PD( DF_APP, PSTR("PILOTO Now=%d\r\n"), *hhmm );
	return(true);

}
//------------------------------------------------------------------------------------
int8_t pv_get_slot_actual(uint16_t hhmm )
{
	// Devuelve en que slot estoy parado. El calculo de hhmm es hh * 100 + mm !!!

uint16_t time_slot_s;
int8_t last_slot = -1;
int8_t slot = -1;
int8_t i;

	// Paso 1.
	// Vemos si hay slots configuradas.
	// Solo chequeo el primero. DEBEN ESTAR ORDENADOS !!
	if ( sVarsApp.pSlots[0].presion < 0.1 ) {
		return(-1);
	}

	// Paso 3.
	// Hay slots configurados: Veo cual es el ultimo. Parto del 2 porque
	// el 1 se que existe del paso anterior.
	last_slot = MAX_PILOTO_PSLOTS - 1;
	for ( i = 0; i < MAX_PILOTO_PSLOTS; i++ ) {
		if ( sVarsApp.pSlots[0].presion < 0.1 ) {
			last_slot = (i-1);
			break;
		}
	}

	for ( i = 0; i <= last_slot; i++ ) {
		time_slot_s = sVarsApp.pSlots[i].hhmm.hour * 100 + sVarsApp.pSlots[i].hhmm.min;
    	// Chequeo inside
		if ( hhmm < time_slot_s ) {
			if ( i == 0 ) {
				slot = last_slot;
			} else {
				slot = i - 1;
			}

			xprintf_PD( DF_APP, PSTR("PILOTO: Slot=%d\r\n"), slot);
			return(slot);
		}
	}

	slot = last_slot;
	return(slot);

}
//------------------------------------------------------------------------------------
t_slot_time_ticks pv_check_tipo_tick( uint16_t hhmm, int8_t slot )
{

uint16_t time_slot_s;

	time_slot_s = sVarsApp.pSlots[slot].hhmm.hour * 100 + sVarsApp.pSlots[slot].hhmm.min;

	if ( hhmm == time_slot_s ) {
		xprintf_P(PSTR("PILOTO: init_tick\r\n"));
		return(TICK_INICIO);
	}

	if  ( (hhmm % 15) == 0 ) {
		xprintf_P(PSTR("PILOTO: control_tick\r\n"));
		return(TICK_CONTROL);
	}

	xprintf_PD( DF_APP, PSTR("PILOTO: normal_tick\r\n"));
	return(TICK_NORMAL);

}
//------------------------------------------------------------------------------------
bool pv_determinar_canales_presion(void)
{
	// Busca en la configuracion de los canales aquel que se llama pB

uint8_t i;
bool sRet = false;
char l_data[10] = { '\0','\0','\0','\0','\0','\0','\0','\0','\0','\0' };

	spiloto.pA_channel = -1;
	spiloto.pB_channel = -1;
	for ( i = 0; i < NRO_ANINPUTS; i++) {
		// xprintf_P(PSTR("DEBUG: ch=%d, name=%s\r\n"), i, strupr(systemVars.ainputs_conf.name[i]) );
		memcpy(l_data, systemVars.ainputs_conf.name[i], sizeof(l_data));
		strupr(l_data);
		if ( ! strcmp_P( l_data, PSTR("PA") ) ) {
			spiloto.pA_channel = i;
			xprintf_P(PSTR("PILOTO: pAchannel=%d\r\n"), spiloto.pA_channel);
		}
		if ( ! strcmp_P( l_data, PSTR("PB") ) ) {
			spiloto.pB_channel = i;
			xprintf_P(PSTR("PILOTO: pBchannel=%d\r\n"), spiloto.pB_channel);
		}
	};

	if ( (spiloto.pA_channel != -1) && (spiloto.pB_channel != -1) )
		sRet = true;

	return(sRet);

}
//------------------------------------------------------------------------------------
void pv_leer_presiones( int8_t samples, uint16_t intervalo_secs )
{
	// Medir la presión de baja N veces a intervalos de 10 s y promediar
	// Deja el valor EN GRAMOS en spiloto.pB

uint8_t i;
float pA, pB;

	// Mido pA/pB
	spiloto.pA = 0;
	spiloto.pB = 0;
	for ( i = 0; i < samples; i++) {
		ctl_watchdog_kick( WDG_APP,  WDG_APP_TIMEOUT );
		pA = ainputs_read_channel(spiloto.pA_channel);
		pB = ainputs_read_channel(spiloto.pB_channel);
		xprintf_P(PSTR("PILOTO pA:[%d]->%0.3f, pB:[%d]->%0.3f\r\n"), i, pA, i, pB );
		// La presion la expreso en gramos !!!
		spiloto.pA += pA;
		spiloto.pB += pB;
		vTaskDelay( ( TickType_t)( intervalo_secs * 1000 / portTICK_RATE_MS ) );
	}

	spiloto.pA /= samples;
	spiloto.pB /= samples;
	xprintf_P(PSTR("PILOTO pA=%.02f, pB=%.02f\r\n"), spiloto.pA, spiloto.pB );
}
//------------------------------------------------------------------------------------
void pv_ajustar_presion(bool modo_test)
{

uint8_t loops;
uint16_t hhmm_now;
int8_t slot;

	// Realiza la tarea de mover el piloto hasta ajustar la presion

	xprintf_P(PSTR("PILOTO: determino canal de pB...\r\n"));
	if ( ! pv_determinar_canales_presion() ) {
		xprintf_P(PSTR("PILOTO: Ajuste ERROR: No se puede determinar el canal de pA/pB.!!\r\n"));
		return;
	}

	for ( loops = 0; loops < MAX_INTENTOS; loops++ ) {

		if (modo_test == MODO_STANDARD ) {
			if ( ! pv_get_hhhmm_now( &hhmm_now)) {
				xprintf_P(PSTR("PILOTO: ERROR pv_get_hhmm.!!\r\n"));
				return;
			}

			slot = pv_get_slot_actual(hhmm_now);
			spiloto.pRef = sVarsApp.pSlots[slot].presion;
			spiloto.pError = PERROR;

		}


		xprintf_P(PSTR("PILOTO: Ajuste: pREf=%.02f\r\n"),spiloto.pRef);

		// El ajuste se realiza cuando la presion a setear esta entre 1K y 3K
		if ( (spiloto.pRef < 1.0) || (spiloto.pRef > 3.0)) {
			xprintf_P(PSTR("PILOTO: Ajuste ERROR: pRef fuera de rango.\r\n"));
			return;
		}

		xprintf_P(PSTR("PILOTO: loop[%d]\r\n"), loops);
		xprintf_P(PSTR("PILOTO: leo pA/pB...\r\n"));
		pv_leer_presiones(5, INTERVALO_PB_SECS );

		// Controles de presion:

		if ( spiloto.pA < 0) {
			xprintf_P(PSTR("PILOTO: Ajuste ERROR: pA < 0.!!\r\n"));
			return;
		}

		if ( spiloto.pB < 0) {
			xprintf_P(PSTR("PILOTO: Ajuste ERROR: pB < 0.!!\r\n"));
			return;
		}

		// Debe haber una diferencia de 500 gr minima para ajustar
		if ( ( spiloto.pA - spiloto.pB ) < 0.5) {
			xprintf_P(PSTR("PILOTO: Ajuste ERROR: (pA-pB) < 500gr.!!\r\n"));
			return;
		}

		// La presion de referencia debe ser menor a pA
		if ( ( spiloto.pA - spiloto.pRef ) < 0.5) {
			xprintf_P(PSTR("PILOTO: Ajuste ERROR: (pA-pRef) < 500gr.!!\r\n"));
			return;
		}

		xprintf_P(PSTR("PILOTO: calculo npulsos...\r\n"));
		pv_calcular_parametros_ajuste();

		// Muestro el resumen de datos
		pv_print_parametros();

		// Si la diferencia de presiones es superior al error tolerable muevo el piloto
		if ( fabs(spiloto.pB - spiloto.pRef) > spiloto.pError ) {
			pv_aplicar_pulsos();
		} else {
			xprintf_P(PSTR("Presion alcanzada\r\n"));
			break;
		}

		//Espero que se estabilize la presión 30 segundos antes de repetir.
		vTaskDelay( ( TickType_t)( INTERVALO_TRYES_SECS * 1000 / portTICK_RATE_MS ) );

	}
	// Muestro la presión que quedo.
	pv_leer_presiones(2, INTERVALO_PB_SECS );
	xprintf_P(PSTR("PILOTO: Fin de ajuste\r\n"));

}
//------------------------------------------------------------------------------------
void run_piloto_presion_test( void )
{
	// Funcion privada, subtask que dada una presion de referencia, mueve el piloto
	// hasta lograrla
	pv_ajustar_presion(MODO_TEST);
}
//------------------------------------------------------------------------------------
void run_piloto_stepper_test( void )
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
void pvstk_piloto(void)
{
	/*
	 * Tarea propia de ajustar el piloto a la presion de la consigna del slot
	 * Si la hhmm coincide con la de inicio de slot, invoca a pv_ajustar_presion
	 * para fijar la nueva presion del slot.
	 * Cada 15 minutos chequea la presion para confirmar si hay que hacer algun ajuste
	 * fino.
	 */

uint16_t hhmm_now;
int8_t slot;
t_slot_time_ticks tipo_slot_tick;

	if ( pv_get_hhhmm_now( &hhmm_now)) {
		slot = pv_get_slot_actual(hhmm_now);
		tipo_slot_tick = pv_check_tipo_tick( hhmm_now, slot);

		if ( tipo_slot_tick == TICK_INICIO ) {
			xprintf_P(PSTR("PILOTO: Ajuste slot;inicio #%d\r\n"), slot);
			pv_ajustar_presion(MODO_STANDARD);
			return;
		}
		/*
		else if ( tipo_slot_tick == TICK_CONTROL ) {
			xprintf_P(PSTR("PILOTO: Ajuste slot;inside #%d\r\n"), slot);
			pv_ajustar_presion(MODO_STANDARD);
			return;
		}
		*/
	}
}
//------------------------------------------------------------------------------------

