/*
 * l_piloto.c
 *
 *  Created on: 8 jul. 2019
 *      Author: pablo
 */

#include "spx.h"
#include  "l_stacks.h"

#define P_STACK_SIZE	3
#define TIME_PWR_ON_VALVES 10
#define P_AVG_DIFF	0.1
#define MAX_WAIT_LOOPS	5
#define MAX_TIMES_FOR_STABILITY	5

typedef enum { WAIT_0 = 0, WAIT_NORMAL, WAIT_MAX } espera_t;
typedef enum { ALTA_PRESION = 0, BAJA_PRESION } t_init_limit_presion;

float pres_baja_data[ P_STACK_SIZE ];

typedef struct {
	// Presiones:
	float pA;
	float pB;
	float pB_avg;

	// Estado de las valvulas
	t_valve_status VA_status;
	t_valve_status VB_status;

	// Contadores de aperturas de valvulas
	uint8_t VA_cnt;
	uint8_t VB_cnt;

} st_dataPiloto_t;

st_dataPiloto_t ldata;

stack_t pres_baja_s;

void pv_pilotos_set_lineal_zone( t_init_limit_presion zona );
void pv_piloto_vopen ( char valve_id );
void pv_piloto_vclose ( char valve_id );
void pv_piloto_vpulse( char valve_id, float pulse_width_s );
void pv_piloto_espera_progresiva( espera_t espera );
float pv_piloto_calcular_pwidth( float delta_P, float presion_alta );
espera_t pv_piloto_regular_presion( void );
void pv_piloto_leer_pB_estable( t_valvula_reguladora tipo_valvula_reguladora  );

//------------------------------------------------------------------------------------
void tk_init_pilotos(void)
{
	// Los pilotos se inicializan igual que las consignas ya que uso las 2 valvulas.
	if ( spx_io_board != SPX_IO5CH ) {
		xprintf_P(PSTR("DOUTPUTS ERROR: Pilotos only in IO_5CH.\r\n\0"));
		systemVars.doutputs_conf.modo = NONE;
		return;
	}

	DRV8814_init();

	// LLevo el piloto al minimo ( baja ) y de ahi a la zona lineal.
	pv_pilotos_set_lineal_zone( BAJA_PRESION );

	ldata.VA_cnt = 0;
	ldata.VB_cnt = 0;

	pv_init_stack( &pres_baja_s, &pres_baja_data[0] , P_STACK_SIZE );
	pv_flush_stack(&pres_baja_s );

}
//------------------------------------------------------------------------------------
void tk_pilotos(void)
{

espera_t espera = WAIT_NORMAL;

	// Implementa el control por pilotos.
	// El tema es que debemos polear tambien para hacer el control
	// Queda para luego

	// Por las dudas
	if ( spx_io_board != SPX_IO5CH ) {
		systemVars.doutputs_conf.modo = NONE;
		return;
	}

	pv_piloto_leer_pB_estable( systemVars.doutputs_conf.piloto.tipo_valvula );
	espera = pv_piloto_regular_presion();
	pv_piloto_espera_progresiva(espera);

}
//------------------------------------------------------------------------------------
void pilotos_readCounters( uint8_t *VA_cnt, uint8_t *VB_cnt, uint8_t *VA_status, uint8_t *VB_status )
{
	*VA_cnt = ldata.VA_cnt;
	*VB_cnt = ldata.VB_cnt;
	*VA_status = ldata.VA_status;
	*VB_status = ldata.VB_status;
}
//------------------------------------------------------------------------------------
void pilotos_df_print( dataframe_s *df )
{

	if ( systemVars.doutputs_conf.modo == PILOTOS ) {
		xprintf_P(PSTR(",VAp=%d,VBp=%d"), df->plt_Vcounters[0], df->plt_Vcounters[1] );
	}
}
//------------------------------------------------------------------------------------
// FUNCIONES PRIVADAS
//------------------------------------------------------------------------------------
void pv_pilotos_set_lineal_zone( t_init_limit_presion zona )
{
	// Lleva el piloto a una zona de trabajo lineal.
	// Lo llena para llevar la presion al maximo y luego abre la valvula de escape por
	// 1.5s para llevarlo a unos 1.6K.

	if ( systemVars.debug == DEBUG_PILOTO ) {
		xprintf_P( PSTR("%s: Set lineal_zone..\r\n\0"), RTC_logprint() );
	}

	if ( zona == ALTA_PRESION ) {
		// Al iniciar lleno la cabeza del piloto y llevo la presion al maximo
		pv_piloto_vclose('B');
		pv_piloto_vopen('A');
		vTaskDelay( ( TickType_t)( 10000 / portTICK_RATE_MS ) );
		pv_piloto_vclose('A');
	} else {
		// Al iniciar vacio la cabeza del piloto y llevo la presion al minimo
		pv_piloto_vclose('A');
		pv_piloto_vopen('B');
		vTaskDelay( ( TickType_t)( 10000 / portTICK_RATE_MS ) );
		pv_piloto_vclose('B');
	}

	// Espero
	vTaskDelay( ( TickType_t)( 3000 / portTICK_RATE_MS ) );

	if ( zona == ALTA_PRESION ) {
		// Llevo la presion a la zona lineal
		pv_piloto_vopen('B');
		vTaskDelay( ( TickType_t)( 1500 / portTICK_RATE_MS ) );
		pv_piloto_vclose('B');
	} else {
		pv_piloto_vopen('A');
		vTaskDelay( ( TickType_t)( 1500 / portTICK_RATE_MS ) );
		pv_piloto_vclose('A');
	}

	vTaskDelay( ( TickType_t)( 10000 / portTICK_RATE_MS ) );

}
//------------------------------------------------------------------------------------
void pv_piloto_vopen ( char valve_id )
{
	// Abre una de las valvulas (A o B)

	// Proporciono corriente.
	DRV8814_power_on();
	// Espero 10s que se carguen los condensasores
	vTaskDelay( ( TickType_t)( TIME_PWR_ON_VALVES / portTICK_RATE_MS ) );

	if ( systemVars.debug == DEBUG_PILOTO ) {
		xprintf_P( PSTR("%s: VALVE OPEN %c\r\n\0"), RTC_logprint(), valve_id );
	}

	DRV8814_vopen( valve_id, 100);

	switch (valve_id) {
	case 'A':
		ldata.VA_status = OPEN;
		break;
	case 'B':
		ldata.VB_status = OPEN;
	}

	DRV8814_power_off();


}
//------------------------------------------------------------------------------------
void pv_piloto_vclose ( char valve_id )
{
	// Cierra una de las valvulas A o B

	// Proporciono corriente.
	DRV8814_power_on();
	// Espero 10s que se carguen los condensasores
	vTaskDelay( ( TickType_t)( TIME_PWR_ON_VALVES / portTICK_RATE_MS ) );

	if ( systemVars.debug == DEBUG_PILOTO ) {
		xprintf_P( PSTR("%s: VALVE CLOSE %c\r\n\0"), RTC_logprint(), valve_id );
	}

	DRV8814_vclose( valve_id, 100);

	switch (valve_id) {
	case 'A':
		ldata.VA_status = CLOSE;
		break;
	case 'B':
		ldata.VB_status = CLOSE;
	}

	DRV8814_power_off();

}
//------------------------------------------------------------------------------------
void pv_piloto_vpulse( char valve_id, float pulse_width_s )
{
	// Genera un pulso de apertura / cierre en la valvula valve_id
	// de duracion 	pulse_width_ms

	// Proporciono corriente.
	DRV8814_power_on();
	// Espero 10s que se carguen los condensasores
	vTaskDelay( ( TickType_t)( TIME_PWR_ON_VALVES / portTICK_RATE_MS ) );

	DRV8814_vopen( toupper(valve_id), 100);
	if ( systemVars.debug == DEBUG_PILOTO ) {
		xprintf_P( PSTR("%s: Vopen %c\r\n\0"), RTC_logprint(), valve_id );
	}

	switch (valve_id) {
	case 'A':
		ldata.VA_status = OPEN;
		break;
	case 'B':
		ldata.VB_status = OPEN;
	}

	vTaskDelay( ( TickType_t)( pulse_width_s * 1000 / portTICK_RATE_MS ) );

	DRV8814_vclose( toupper(valve_id), 100);

	if ( systemVars.debug == DEBUG_PILOTO ) {
		xprintf_P( PSTR("%s: Vclose %c\r\n\0"), RTC_logprint(), valve_id );
	}

	switch (valve_id) {
	case 'A':
		ldata.VA_status = CLOSE;
		break;
	case 'B':
		ldata.VB_status = CLOSE;
	}

	DRV8814_power_off();

}
//------------------------------------------------------------------------------------
void pv_piloto_leer_pB_estable( t_valvula_reguladora tipo_valvula_reguladora  )
{
	// Leo las presiones hasta que sean estable.
	// Criterio:
	// Mido 3 presiones y tomo como presion estable el promedio.

uint32_t waiting_ticks;
int8_t counts;

	counts = P_STACK_SIZE;

	pv_flush_stack( &pres_baja_s );

	while( counts-- > 0 ) {

		// Espero
		switch ( tipo_valvula_reguladora ) {
		case VR_CHICA:
			waiting_ticks = ( 15000 / portTICK_RATE_MS );
			break;
		case VR_MEDIA:
			waiting_ticks = ( 30000 / portTICK_RATE_MS );
			break;
		case VR_GRANDE:
			waiting_ticks = ( 45000 / portTICK_RATE_MS );
			break;
		default:
			waiting_ticks = ( 30000 / portTICK_RATE_MS );
			break;
		}

		vTaskDelay( waiting_ticks );

		// Leo presiones
		data_read_pAB( &ldata.pA, &ldata.pB );

		pv_push_stack( &pres_baja_s, ldata.pB );

		if ( systemVars.debug == DEBUG_PILOTO ) {
			xprintf_P(PSTR("%s: Mon: P.alta=%.02f, P.baja=%.02f\r\n\0"), RTC_logprint(), ldata.pA, ldata.pB );
		}
	}

	if ( systemVars.debug == DEBUG_PILOTO ) {
		xprintf_P( PSTR("%s: Mon: \0"), RTC_logprint());
		pv_print_stack( &pres_baja_s );
	}

	ldata.pB_avg = pv_get_stack_avg( &pres_baja_s );

	if ( systemVars.debug == DEBUG_PILOTO ) {
		xprintf_P( PSTR("%s: Mon: P.baja avg = %0.02f\r\n\0"), RTC_logprint(), ldata.pB_avg );
	}
}
//------------------------------------------------------------------------------------
float pv_piloto_calcular_pwidth( float delta_P, float presion_alta )
{


	// De las graficas vemos que en la zona lineal, un pulso de 0.1 corresponde aprox. a 90gr.
	// Cada pulso de 0.1s genera un cambio del orden de 90-110 gr.

float pw = 0.1;
float dP;

	dP = fabs(delta_P);

	if ( dP > 0.2 ) {
		pw = 0.2;
	} else if (dP > 0.15 ) {
		pw = 0.1;
	} else if ( dP > 0.1 ) {
		pw = 0.05;
	} else {
		pw = 0.01;
	}

	return( pw);

}
//------------------------------------------------------------------------------------
espera_t pv_piloto_regular_presion( void )
{

float delta_P;
float pulseW;
espera_t espera;

	// Calculos
	// Diferencia de presion. Puede ser positiva o negativa
	delta_P = ( systemVars.doutputs_conf.piloto.pout - ldata.pB_avg );

	if ( fabs(delta_P) <= systemVars.doutputs_conf.piloto.band  ) {
		// No debo regular
		if ( systemVars.debug == DEBUG_PILOTO ) {
			xprintf_P(PSTR("%s: Reg(in-band): deltaP(%.02f) = %.02f (%.02f - %.02f)\r\n\0"), RTC_logprint(), systemVars.doutputs_conf.piloto.band, delta_P, systemVars.doutputs_conf.piloto.pout, ldata.pB_avg );
			xprintf_P(PSTR("%s: Reg STEPS: total=%d, VA=%d, VB=%d\r\n\0"), RTC_logprint(), (ldata.VA_cnt + ldata.VB_cnt), ldata.VA_cnt,ldata.VB_cnt );
			xprintf_P(PSTR("\r\n\0"));
		}

		ldata.VA_cnt = 0;
		ldata.VB_cnt = 0;
		// Estoy in-band. Espero progresivamente
		espera = WAIT_NORMAL;
		//xprintf_P(PSTR("DEBUG espera wait_normal\r\n\0"));

	} else {
		// Si debo regular
		pulseW = pv_piloto_calcular_pwidth( delta_P, ldata.pA );
		if ( systemVars.debug == DEBUG_PILOTO ) {
			xprintf_P(PSTR("%s: Reg(out-band): deltaP(%.02f) = %.02f (%.02f - %.02f), pulse_width = %.02f\r\n\0"), RTC_logprint(), systemVars.doutputs_conf.piloto.band, delta_P, systemVars.doutputs_conf.piloto.pout, ldata.pB_avg, pulseW );
		}
		// Aplico correcciones.
		if ( systemVars.doutputs_conf.piloto.pout > ldata.pB_avg ) {
			// Delta > 0
			// Debo subir la presion ( llenar el piston )
			ldata.VA_cnt++;
			pv_piloto_vpulse( 'A', pulseW );
		} else if ( systemVars.doutputs_conf.piloto.pout < ldata.pB_avg ) {
			// Delta  < 0
			// Debo bajar la presion ( vaciar el piston )
			pv_piloto_vpulse( 'B', pulseW );
			ldata.VB_cnt++;
		}

		if ( systemVars.debug == DEBUG_PILOTO ) {
			xprintf_P(PSTR("%s: Reg Pulsos: total=%d, VA=%d, VB=%d\r\n\0"), RTC_logprint(), (ldata.VA_cnt + ldata.VB_cnt), ldata.VA_cnt, ldata.VB_cnt );
		}

		if ( (ldata.VA_cnt + ldata.VB_cnt) > systemVars.doutputs_conf.piloto.max_steps ) {
			//xprintf_P(PSTR("DEBUG espera wait_max\r\n\0"));

			espera = WAIT_MAX;
			ldata.VA_cnt = 0;
			ldata.VB_cnt = 0;

		} else {
			// Estoy off-band: No espero
			//xprintf_P(PSTR("DEBUG espera wait_0\r\n\0"));
			espera = WAIT_0;
		}

	}

	return(espera);

}
//------------------------------------------------------------------------------------
void pv_piloto_espera_progresiva(espera_t espera )
{

	// Genero una espera progresiva de hasta 5 minutos.
static uint8_t wait_loops = 1;
uint16_t i;

	switch ( espera ) {
	case WAIT_0:
		// Espero 60s
		wait_loops = 1;
		break;
	case WAIT_NORMAL:
		// Espra progresiva
		if ( wait_loops++ > MAX_WAIT_LOOPS ) {
			wait_loops = MAX_WAIT_LOOPS;
		}
		break;
	case WAIT_MAX:
		// Espera maxima
		wait_loops = MAX_WAIT_LOOPS;
		break;
	}

	// Espera
	if ( systemVars.debug == DEBUG_PILOTO ) {
		xprintf_P( PSTR("%s: Espera progresiva (%d)secs.\r\n\0"), RTC_logprint(), ( wait_loops * 60 ));
	}

	// Espero de a 1 minuto
	for (i = 1; i<= wait_loops; i++) {
		ctl_watchdog_kick( WDG_DOUT,  WDG_DOUT_TIMEOUT );
		vTaskDelay( 60000 / portTICK_RATE_MS );
	}

}
//------------------------------------------------------------------------------------



