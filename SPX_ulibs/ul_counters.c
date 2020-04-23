/*
 * spx_counters.c
 *
 *  Created on: 5 jun. 2019
 *      Author: pablo
 *
 *  Entradas contadores tipo B:
 *  Son las entradas que tienen optoacoplador.
 *  Normalmente con la entrada flotando ( 12V ), el transistor del opto esta cortado y la entrada al micro esta en 0V.
 *  Cuando se activa la entrada contra tierra ( 0V), el diodo conduce, el transistor satura y la entrada al micro pasa
 *  a ser 3.6V.
 *  Por esto, las entradas del micro ( interrupcion ) las configuramos para sensar RISING_EDGE.
 *  HIGH_SPEED: Solo cuenta c/flanco de subida que detecta.
 *  LOW_SPEED: Activa un timer de debounce.
 *  Cuando el timer expira vuelve a leer el pin y si esta en 1 incrementa el contador.
 *  Arranca otro timer de periodo que es el que rearma la interrupcion.
 *
 *
 */

#include "spx.h"

StaticTimer_t counter_xTimerBuffer0A,counter_xTimerBuffer0B, counter_xTimerBuffer1A,counter_xTimerBuffer1B;
TimerHandle_t counter_xTimer0A, counter_xTimer0B, counter_xTimer1A, counter_xTimer1B;

float pv_cnt0,pv_cnt1;

bool f_count0_running, f_count1_running;

BaseType_t xHigherPriorityTaskWoken0 = pdFALSE;
BaseType_t xHigherPriorityTaskWoken1 = pdFALSE;

static void pv_counters_TimerCallback0A( TimerHandle_t xTimer );
static void pv_counters_TimerCallback0B( TimerHandle_t xTimer );
static void pv_counters_TimerCallback1A( TimerHandle_t xTimer );
static void pv_counters_TimerCallback1B( TimerHandle_t xTimer );

#define DF_COUNTERS ( systemVars.debug == DEBUG_COUNTER )

//------------------------------------------------------------------------------------
void counters_setup(void)
{
	// Configura los timers que generan el delay de medida del ancho de pulso
	// y el periodo.
	// Se deben crear antes que las tarea y que arranque el scheduler

	// Counter de debounce de pulsos en linea A
	// Mide el tiempo minimo que el pulso está arriba
	// Mide el pulse_width
	counter_xTimer0A = xTimerCreateStatic ("CNT0A",
			pdMS_TO_TICKS( 10 ),
			pdFALSE,
			( void * ) 0,
			pv_counters_TimerCallback0A,
			&counter_xTimerBuffer0A
			);

	// Mide el periodo del pulso en la linea A.
	counter_xTimer1A = xTimerCreateStatic ("CNT1A",
			pdMS_TO_TICKS( 90 ),
			pdFALSE,
			( void * ) 0,
			pv_counters_TimerCallback1A,
			&counter_xTimerBuffer1A
			);


	// Counter de debounce de pulsos en linea B
	// Mide el tiempo minimo que el pulso está arriba
	counter_xTimer0B = xTimerCreateStatic ("CNT0B",
			pdMS_TO_TICKS( 10 ),
			pdFALSE,
			( void * ) 0,
			pv_counters_TimerCallback0B,
			&counter_xTimerBuffer0B
			);


	counter_xTimer1B = xTimerCreateStatic ("CNT1B",
			pdMS_TO_TICKS( 90 ),
			pdFALSE,
			( void * ) 0,
			pv_counters_TimerCallback1B,
			&counter_xTimerBuffer1B
			);

}
//------------------------------------------------------------------------------------
void counters_init(void)
{
	// Configuro los timers con el timeout dado por el tiempo de minimo pulse width.
	// Esto lo debo hacer aqui porque ya lei el systemVars y tengo los valores.
	// Esto arranca el timer por lo que hay que apagarlos

	if ( systemVars.counters_conf.period[0] <= systemVars.counters_conf.pwidth[0] ) {
		systemVars.counters_conf.period[0] = systemVars.counters_conf.pwidth[0] + 10;
		xprintf_P(PSTR("COUNTERS ERROR!! C0: periodo debe ser mayor que el ancho\r\n\0"));
	}

	if ( systemVars.counters_conf.period[1] <= systemVars.counters_conf.pwidth[1] ) {
		systemVars.counters_conf.period[1] = systemVars.counters_conf.pwidth[1] + 10;
		xprintf_P(PSTR("COUNTERS ERROR!! C1: periodo debe ser mayor que el ancho\r\n\0"));
	}

	// CNT0 (PA)
	// Pulse-width
	xTimerChangePeriod( counter_xTimer0A, systemVars.counters_conf.pwidth[0], 10 );
	xTimerStop(counter_xTimer0A, 10);

	// Period
	xTimerChangePeriod( counter_xTimer1A, ( systemVars.counters_conf.period[0] - systemVars.counters_conf.pwidth[0]) , 10 );
	xTimerStop(counter_xTimer1A, 10);

	// CNT1 (PB)
	// Pulse-width
	xTimerChangePeriod( counter_xTimer0B, systemVars.counters_conf.pwidth[1], 10 );
	xTimerStop(counter_xTimer0B, 10);

	// Period
	xTimerChangePeriod( counter_xTimer1B, ( systemVars.counters_conf.period[1] - systemVars.counters_conf.pwidth[1]) , 10 );
	xTimerStop(counter_xTimer1B, 10);

	COUNTERS_init(0, systemVars.counters_conf.hw_type );
	COUNTERS_init(1, systemVars.counters_conf.hw_type );

	f_count0_running = true;
	if ( strcmp ( systemVars.counters_conf.name[0], "X" ) == 0 ) {
		f_count0_running = false;
	}

	f_count1_running = true;
	if ( strcmp ( systemVars.counters_conf.name[1], "X" ) == 0 ) {
		f_count1_running = false;
	}

	pv_cnt0 = 0;
	pv_cnt1 = 0;

}
//------------------------------------------------------------------------------------
static void pv_counters_TimerCallback0A( TimerHandle_t xTimer )
{
	// Funcion de callback de la entrada de contador A.
	// Controla el pulse_width de la entrada A
	// Leo la entrada y si esta aun en 1, incremento el contador y
	// prendo el timer xTimer1X que termine el debounce.

uint8_t ref_value = 1;

	if ( systemVars.counters_conf.hw_type == COUNTERS_TYPE_A ) {
		ref_value = 0;
	}

	if ( CNT_read_CNT0() == ref_value ) {
		pv_cnt0++;
		xTimerStart( counter_xTimer1A, 1 );
		if ( systemVars.debug == DEBUG_COUNTER) {
			xprintf_P( PSTR("COUNTERS: DEBUG *C0=%0.3f,C1=%0.3f\r\n\0"),pv_cnt0, pv_cnt1);
		}
		return;
	}

	// No se cumplio el pulse_width minimo. No cuento el pulso y rearmo el sistema
	// para poder volver a interrumpir
	PORTA.INT0MASK = PIN2_bm;
	PORTA.INTCTRL = PORT_INT0LVL0_bm;
	PORTA.INTFLAGS = PORT_INT0IF_bm;

}
//------------------------------------------------------------------------------------
static void pv_counters_TimerCallback1A( TimerHandle_t xTimer )
{
	// Se cumplio es period de la linea A (CNT0)
	// Habilito a volver a interrumpir
	PORTA.INT0MASK = PIN2_bm;
	PORTA.INTCTRL = PORT_INT0LVL0_bm;
	PORTA.INTFLAGS = PORT_INT0IF_bm;

}
//------------------------------------------------------------------------------------
static void pv_counters_TimerCallback0B( TimerHandle_t xTimer )
{

	//IO_clr_LED_KA();

	// Mido el pulse_width de la linea B (CNT1)

uint8_t ref_value = 1;

	if ( systemVars.counters_conf.hw_type == COUNTERS_TYPE_A ) {
		ref_value = 0;
	}

	if ( CNT_read_CNT1() == ref_value ) {
		pv_cnt1++;
		xTimerStart( counter_xTimer1B, 1 );
		if ( systemVars.debug == DEBUG_COUNTER) {
			xprintf_P( PSTR("COUNTERS: DEBUG C0=%0.3f,*C1=%0.3f\r\n\0"),pv_cnt0, pv_cnt1);
		}
		return;
	}

	PORTB.INT0MASK = PIN2_bm;
	PORTB.INTCTRL = PORT_INT0LVL0_bm;
	PORTB.INTFLAGS = PORT_INT0IF_bm;


}
//------------------------------------------------------------------------------------
static void pv_counters_TimerCallback1B( TimerHandle_t xTimer )
{

	// Rearmo la interrupcion para el proximo
	//IO_clr_LED_KA();
	PORTB.INT0MASK = PIN2_bm;
	PORTB.INTCTRL = PORT_INT0LVL0_bm;
	PORTB.INTFLAGS = PORT_INT0IF_bm;


}
//------------------------------------------------------------------------------------
void counters_clear(void)
{
	pv_cnt0 = 0;
	pv_cnt1 = 0;
}
//------------------------------------------------------------------------------------
void counters_read(float cnt[])
{
	cnt[0] = pv_cnt0 * systemVars.counters_conf.magpp[0];
	cnt[1] = pv_cnt1 * systemVars.counters_conf.magpp[1];

}
//------------------------------------------------------------------------------------
void counters_print(file_descriptor_t fd, float cnt[] )
{
	// Imprime los canales configurados ( no X ) en un fd ( tty_gprs,tty_xbee,tty_term) en
	// forma formateada.
	// Los lee de una estructura array pasada como src

	if ( strcmp ( systemVars.counters_conf.name[0], "X" ) ) {
		xfprintf_P(fd, PSTR("%s:%.03f;"),systemVars.counters_conf.name[0], cnt[0] );
	}

	if ( strcmp ( systemVars.counters_conf.name[1], "X" ) ) {
		xfprintf_P(fd, PSTR("%s:%.03f;"),systemVars.counters_conf.name[1], cnt[1] );
	}

}
//------------------------------------------------------------------------------------
void counters_config_defaults(void)
{

	// Realiza la configuracion por defecto de los canales contadores.
	// Los valores son en ms.

uint8_t i = 0;

	for ( i = 0; i < MAX_COUNTER_CHANNELS; i++ ) {
		snprintf_P( systemVars.counters_conf.name[i], PARAMNAME_LENGTH, PSTR("C%d\0"), i );
		systemVars.counters_conf.magpp[i] = 1;
		systemVars.counters_conf.period[i] = 100;
		systemVars.counters_conf.pwidth[i] = 10;
		systemVars.counters_conf.speed[i] = CNT_LOW_SPEED;
	}

	// Por defecto quedan en modo B: Con optoacoplador.
	systemVars.counters_conf.hw_type = COUNTERS_TYPE_B;

}
//------------------------------------------------------------------------------------
bool counters_config_channel( uint8_t channel,char *s_name, char *s_magpp, char *s_pw, char *s_period, char *s_speed )
{
	// Configuro un canal contador.
	// channel: id del canal
	// s_param0: string del nombre del canal
	// s_param1: string con el valor del factor magpp.
	//
	// {0..1} dname magPP

bool retS = false;
char l_data[10] = { '\0','\0','\0','\0','\0','\0','\0','\0','\0','\0' };

	//xprintf_P( PSTR("DEBUG COUNTER CONFIG: C%d,name=%s, magpp=%s, pwidth=%s, period=%s, speed=%s\r\n\0"), channel, s_name, s_magpp, s_pw, s_period, s_speed );

	if ( s_name == NULL ) {
		return(retS);
	}

	if ( ( channel >=  0) && ( channel < MAX_COUNTER_CHANNELS ) ) {

		// NOMBRE
		if ( u_control_string(s_name) == 0 ) {
			return( false );
		}

		snprintf_P( systemVars.counters_conf.name[channel], PARAMNAME_LENGTH, PSTR("%s\0"), s_name );
		memcpy(l_data, systemVars.counters_conf.name[channel], sizeof(l_data));
		strupr(l_data);

		// MAGPP
		if ( s_magpp != NULL ) { systemVars.counters_conf.magpp[channel] = atof(s_magpp); }

		// PW
		if ( s_pw != NULL ) { systemVars.counters_conf.pwidth[channel] = atoi(s_pw); }

		// PERIOD
		if ( s_period != NULL ) { systemVars.counters_conf.period[channel] = atoi(s_period); }

		// SPEED
		if ( strcmp_P( s_speed, PSTR("LS\0")) == 0 ) {
			 systemVars.counters_conf.speed[channel] = CNT_LOW_SPEED;

		} else if ( strcmp_P( s_speed , PSTR("HS\0")) == 0 ) {
			systemVars.counters_conf.speed[channel] = CNT_HIGH_SPEED;
		}

		// Si el nombre es X deshabilito todo
/*		if ( strcmp ( systemVars.counters_conf.name[channel], "X" ) == 0 ) {
			systemVars.counters_conf.magpp[channel] = 0;
			systemVars.counters_conf.pwidth[channel] = 0;
			systemVars.counters_conf.period[channel] = 0;
			systemVars.counters_conf.speed[channel] = 0;
		}
*/
		retS = true;
	}

	return(retS);

}
//------------------------------------------------------------------------------------
bool counters_config_hw( char *s_type )
{
	/*
	 * Las entradas de contadores son el un hardware sin optoacoplador, por lo tanto detectan
	 * flancos de bajada y en otros casos son con opto y detectan flanco de subida.
	 */

bool retS = false;

	//xprintf_P(PSTR("DEBUG counters_conf: %s\r\n\0"), s_type);

	if ( strcmp_P( strupr(s_type), PSTR("SIMPLE")) == 0 ) {
		systemVars.counters_conf.hw_type = COUNTERS_TYPE_A;
		retS = true;
	} else if ( strcmp_P( strupr(s_type), PSTR("OPTO")) == 0 ) {
		systemVars.counters_conf.hw_type = COUNTERS_TYPE_B;
		retS = true;
	} else {
		retS = false;
	}

	return(retS);

}
//------------------------------------------------------------------------------------
ISR ( PORTA_INT0_vect )
{
	// Esta ISR se activa cuando el contador D2 (PA2) genera un flaco se subida.
	// Si el contador es de HS solo cuenta

	if ( !f_count0_running)
		return;

	if ( systemVars.counters_conf.speed[0] == CNT_HIGH_SPEED  ) {
		pv_cnt0++;
	} else {
		// Sino es de LS por lo que arranca un debounce.
		// Prende un timer de debounce para volver a polear el pin y ver si se cumple el pwidth.
		while ( xTimerStartFromISR( counter_xTimer0A, &xHigherPriorityTaskWoken0 ) != pdPASS )
			;
		// Deshabilita la interrupcion por ahora ( enmascara )
		PORTA.INT0MASK = 0x00;
		PORTA.INTCTRL = 0x00;
	}

}
//------------------------------------------------------------------------------------
ISR( PORTB_INT0_vect )
{
	// Esta ISR se activa cuando el contador D1 (PB2) genera un flaco se subida.

	if ( !f_count1_running)
		return;

	// Si el contador es de HS solo cuenta
	if ( systemVars.counters_conf.speed[1] == CNT_HIGH_SPEED  ) {
		pv_cnt1++;
	} else {
		// Aseguro arrancar el timer
		while ( xTimerStartFromISR( counter_xTimer0B, &xHigherPriorityTaskWoken1 ) != pdPASS )
			;
		PORTB.INT0MASK = 0x00;
		PORTB.INTCTRL = 0x00;
		//PORTF.OUTTGL = 0x80;	// Toggle A2
		//IO_set_LED_KA();
	}
}
//------------------------------------------------------------------------------------
uint8_t counters_hash(void)
{

uint16_t i;
uint8_t hash = 0;
char dst[32];
char *p;
uint8_t j = 0;

	// C0:C0,1.000,100,10,0;C1:C1,1.000,100,10,0;

	// calculate own checksum
	for(i=0;i< MAX_COUNTER_CHANNELS;i++) {
		// Vacio el buffer temoral
		memset(dst,'\0', sizeof(dst));
		j = 0;
		j += snprintf_P(&dst[j], sizeof(dst), PSTR("C%d:%s,"), i, systemVars.counters_conf.name[i]);
		j += snprintf_P(&dst[j], sizeof(dst), PSTR("%.03f,%d,"),systemVars.counters_conf.magpp[i], systemVars.counters_conf.period[i]);
		if ( systemVars.counters_conf.speed[i] == CNT_LOW_SPEED ) {
			j += snprintf_P(&dst[j], sizeof(dst), PSTR("%d,LS;"), systemVars.counters_conf.pwidth[i] );
		} else {
			j += snprintf_P(&dst[j], sizeof(dst), PSTR("%d,HS;"), systemVars.counters_conf.pwidth[i] );
		}
		// Apunto al comienzo para recorrer el buffer
		p = dst;
		// Mientras no sea NULL calculo el checksum deol buffer
		while (*p != '\0') {
			// checksum += *p++;
			hash = u_hash(hash, *p++);
		}
		//xprintf_P( PSTR("DEBUG: CCKS = [%s]\r\n\0"), dst );
		//xprintf_P( PSTR("DEBUG: cks = [0x%02x]\r\n\0"), checksum );

	}
	return(hash);

}
//------------------------------------------------------------------------------------

