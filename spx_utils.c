/*
 * spx_utis.c
 *
 *  Created on: 10 dic. 2018
 *      Author: pablo
 */

#include "spx.h"
#include "gprs.h"

static void pv_load_defaults_ainputs(void);
static void pv_load_defaults_counters(void);
static void pv_load_defaults_dinputs(void);
static void pv_load_defaults_doutputs(void);
static void pv_load_defaults_consignas(void);
static void pv_load_default_rangeMeter(void);

#define RTC32_ToscBusy()        !( VBAT.STATUS & VBAT_XOSCRDY_bm )

//------------------------------------------------------------------------------------
void initMCU(void)
{
	// Inicializa los pines del micro

	// ANALOG: SENSOR VCC CONTROL
//	IO_config_SENS_12V_CTL();


	// TICK:
	//IO_config_TICK();

	// PWR_SLEEP
//	IO_config_PWR_SLEEP();
//	IO_set_PWR_SLEEP();

}
//------------------------------------------------------------------------------------
void RTC32_ToscEnable( bool use1khz );
//------------------------------------------------------------------------------------
void u_configure_systemMainClock(void)
{
/*	Configura el clock principal del sistema
	Inicialmente se arranca en 2Mhz.
	La configuracion del reloj tiene 2 componentes: el clock y el oscilador.
	OSCILADOR:
	Primero debo elejir cual oscilador voy a usar para alimentar los prescalers que me den
	el clock del sistema y esperar a que este este estable.
	CLOCK:
	Elijo cual oscilador ( puedo tener mas de uno prendido ) va a ser la fuente principal
	del closck del sistema.
	Luego configuro el prescaler para derivar los clocks de los perifericos.
	Puedo por ultimo 'lockear' esta configuracion para que no se cambie por error.
	Los registros para configurar el clock son 'protegidos' por lo que los cambio
	utilizando la funcion CCPwrite.

	Para nuestra aplicacion vamos a usar un clock de 32Mhz.
	Como vamos a usar el ADC debemos prestar atencion al clock de perifericos clk_per ya que luego
	el ADC clock derivado del clk_per debe estar entre 100khz y 1.4Mhz ( AVR1300 ).

	Opcionalmente podriamos deshabilitar el oscilador de 2Mhz para ahorrar energia.
*/

#if SYSMAINCLK == 32
	// Habilito el oscilador de 32Mhz
	OSC.CTRL |= OSC_RC32MEN_bm;

	// Espero que este estable
	do {} while ( (OSC.STATUS & OSC_RC32MRDY_bm) == 0 );

	// Seteo el clock para que use el oscilador de 32Mhz.
	// Uso la funcion CCPWrite porque hay que se cuidadoso al tocar estos
	// registros que son protegidos.
	CCPWrite(&CLK.CTRL, CLK_SCLKSEL_RC32M_gc);
	//
	// El prescaler A ( CLK.PSCCTRL ), B y C ( PSBCDIV ) los dejo en 0 de modo que no
	// hago division y con esto tengo un clk_per = clk_sys. ( 32 Mhz ).
	//
#endif

#if SYSMAINCLK == 8
	// Habilito el oscilador de 32Mhz y lo divido por 4
	OSC.CTRL |= OSC_RC32MEN_bm;

	// Espero que este estable
	do {} while ( (OSC.STATUS & OSC_RC32MRDY_bm) == 0 );

	// Seteo el clock para que use el oscilador de 32Mhz.
	// Uso la funcion CCPWrite porque hay que se cuidadoso al tocar estos
	// registros que son protegidos.
	CCPWrite(&CLK.CTRL, CLK_SCLKSEL_RC32M_gc);
	//
	// Pongo el prescaler A por 4 y el B y C en 0.
	CLKSYS_Prescalers_Config( CLK_PSADIV_4_gc, CLK_PSBCDIV_1_1_gc );

	//
#endif

#if SYSMAINCLK == 2
	// Este es el oscilador por defecto por lo cual no tendria porque configurarlo.
	// Habilito el oscilador de 2Mhz
	OSC.CTRL |= OSC_RC2MEN_bm;
	// Espero que este estable
	do {} while ( (OSC.STATUS & OSC_RC2MRDY_bm) == 0 );

	// Seteo el clock para que use el oscilador de 2Mhz.
	// Uso la funcion CCPWrite porque hay que se cuidadoso al tocar estos
	// registros que son protegidos.
	CCPWrite(&CLK.CTRL, CLK_SCLKSEL_RC2M_gc);
	//
	// El prescaler A ( CLK.PSCCTRL ), B y C ( PSBCDIV ) los dejo en 0 de modo que no
	// hago division y con esto tengo un clk_per = clk_sys. ( 2 Mhz ).
	//
#endif

//#ifdef configUSE_TICKLESS_IDLE
	// Para el modo TICKLESS
	// Configuro el RTC con el osc externo de 32Khz
	// Pongo como fuente el xtal externo de 32768 contando a 32Khz.
	//CLK.RTCCTRL = CLK_RTCSRC_TOSC32_gc | CLK_RTCEN_bm;
	//do {} while ( ( RTC.STATUS & RTC_SYNCBUSY_bm ) );

	// Disable RTC interrupt.
	// RTC.INTCTRL = 0x00;
	//
	// Si uso el RTC32, habilito el oscilador para 1ms.

	RTC32_ToscEnable(true);
//#endif

	// Lockeo la configuracion.
	CCPWrite( &CLK.LOCK, CLK_LOCK_bm );

}
//------------------------------------------------------------------------------------
void u_configure_RTC32(void)
{
	// El RTC32 lo utilizo para desperarme en el modo tickless.
	// V-bat needs to be reset, and activated
	VBAT.CTRL |= VBAT_ACCEN_bm;
	// Este registro esta protegido de escritura con CCP.
	CCPWrite(&VBAT.CTRL, VBAT_RESET_bm);

	// Pongo el reloj en 1.024Khz.
	VBAT.CTRL |=  VBAT_XOSCSEL_bm | VBAT_XOSCFDEN_bm ;

	// wait for 200us see AVR1321 Application note page 8
	_delay_us(200);

	// Turn on 32.768kHz crystal oscillator
	VBAT.CTRL |= VBAT_XOSCEN_bm;

	// Wait for stable oscillator
	while(!(VBAT.STATUS & VBAT_XOSCRDY_bm));

	// Disable RTC32 module before setting counter values
	RTC32.CTRL = 0;

	// Wait for sync
	do { } while ( RTC32.SYNCCTRL & RTC32_SYNCBUSY_bm );

	// EL RTC corre a 1024 hz y quiero generar un tick de 10ms,
	RTC32.PER = 1024;
	RTC32.CNT = 0;

	// Interrupt: on Overflow
	RTC32.INTCTRL = RTC32_OVFINTLVL_LO_gc;

	// Enable RTC32 module
	RTC32.CTRL = RTC32_ENABLE_bm;

	/* Wait for sync */
	do { } while ( RTC32.SYNCCTRL & RTC32_SYNCBUSY_bm );
}
//------------------------------------------------------------------------------------
void RTC32_ToscEnable( bool use1khz )
{
	/* Enable 32 kHz XTAL oscillator, with 1 kHz or 1 Hz output. */
	if (use1khz)
		VBAT.CTRL |= ( VBAT_XOSCEN_bm | VBAT_XOSCSEL_bm );
	else
		VBAT.CTRL |= ( VBAT_XOSCEN_bm );

	RTC32.PER = 10;
	RTC32.CNT = 0;

	/* Wait for oscillator to stabilize before returning. */
//	do { } while ( RTC32_ToscBusy() );
}
//------------------------------------------------------------------------------------
void u_control_string( char *s_name )
{
	// Controlo que el string terminado en \0 tenga solo letras o digitos.
	// Es porque si en un nombre de canal se cuela un caracter extranio, me
	// despelota los logs.
	// Si encuentro un caracter extraño, lo sustituyo por un \0 y salgo

uint8_t max_length = PARAMNAME_LENGTH;
char *p;
uint8_t cChar;

	p = (char *)s_name;
	while (*p && (max_length-- > 0) ) {

		cChar = (uint8_t)*p;
		if (  ! isalnum(cChar) )	{
			*p = '\0';
			return;
		}
		p++;
	}

}
//------------------------------------------------------------------------------------
void u_convert_str_to_time_t ( char *time_str, st_time_t *time_struct )
{

	// Convierte un string hhmm en una estructura time_type que tiene
	// un campo hora y otro minuto

uint16_t time_num;

	time_num = atol(time_str);
	time_struct->hour = (uint8_t) (time_num / 100);
	time_struct->min = (uint8_t)(time_num % 100);

//	xprintf_P( PSTR("DEBUG: u_convert_str_to_time_t (hh=%d,mm=%d)\r\n\0"), time_struct->hour,time_struct->min );

}
//------------------------------------------------------------------------------------
void u_convert_int_to_time_t ( int int16time, st_time_t *time_struct )
{

	// Convierte un int16  hhmm en una estructura time_type que tiene
	// un campo hora y otro minuto

	time_struct->hour = (uint8_t) (int16time / 100);
	time_struct->min = (uint8_t)(int16time % 100);

//	xprintf_P( PSTR("DEBUG: u_convert_str_to_time_t (hh=%d,mm=%d)\r\n\0"), time_struct->hour,time_struct->min );

}
//------------------------------------------------------------------------------------
void u_load_defaults( void )
{
	// Carga la configuracion por defecto.

	systemVars.debug = DEBUG_NONE;
	systemVars.timerPoll = 300;
	systemVars.pwr_settle_time = 1;

	pv_load_defaults_counters();
	pv_load_defaults_ainputs();
	pv_load_defaults_dinputs();
	pv_load_defaults_doutputs();
	pv_load_default_rangeMeter();
	pv_load_defaults_consignas();

	u_gprs_load_defaults();

}
//------------------------------------------------------------------------------------
uint8_t u_save_params_in_NVMEE(void)
{
	// Calculo el checksum del systemVars.
	// Considero el systemVars como un array de chars.

uint8_t *p;
uint8_t checksum;
uint16_t data_length;
uint16_t i;


	// Calculo el checksum del systemVars.
	systemVars.checksum = 0;
	data_length = sizeof(systemVars);
	p = (uint8_t *)&systemVars;
	checksum = 0;
	// Recorro todo el systemVars considerando c/byte como un char, hasta
	// llegar al ultimo ( checksum ) que no lo incluyo !!!.
	for ( i = 0; i < (data_length - 1) ; i++) {
		checksum += p[i];
	}
	checksum = ~checksum;
	systemVars.checksum = checksum;

	// Guardo systemVars en la EE
	NVMEE_write (0x00, &systemVars, sizeof(systemVars));

	return(checksum);

}
//------------------------------------------------------------------------------------
bool u_load_params_from_NVMEE(void)
{
	// Leo el systemVars desde la EE.
	// Calculo el checksum. Si no coincide es que hubo algun
	// error por lo que cargo el default.


uint8_t *p;
uint8_t stored_checksum, checksum;
uint16_t data_length;
uint16_t i;

	// Leo de la EE es systemVars.
	NVMEE_read_buffer(0x00, (char *)&systemVars, sizeof(systemVars));

	// Guardo el checksum que lei.
	stored_checksum = systemVars.checksum;

	// Calculo el checksum del systemVars leido
	systemVars.checksum = 0;
	data_length = sizeof(systemVars);
	p = ( uint8_t *)&systemVars;	// Recorro el systemVars como si fuese un array de int8.
	checksum = 0;
	for ( i = 0; i < ( data_length - 1 ); i++) {
		checksum += p[i];
	}
	checksum = ~checksum;

	if ( stored_checksum != checksum ) {
		return(false);
	}

	return(true);
}
//------------------------------------------------------------------------------------
void u_config_timerpoll ( char *s_timerpoll )
{
	// Configura el tiempo de poleo.
	// Se utiliza desde el modo comando como desde el modo online
	// El tiempo de poleo debe estar entre 15s y 3600s

	while ( xSemaphoreTake( sem_SYSVars, ( TickType_t ) 5 ) != pdTRUE )
		taskYIELD();

	systemVars.timerPoll = atoi(s_timerpoll);

	if ( systemVars.timerPoll < 15 )
		systemVars.timerPoll = 15;

	if ( systemVars.timerPoll > 3600 )
		systemVars.timerPoll = 300;

	xSemaphoreGive( sem_SYSVars );
	return;
}
//------------------------------------------------------------------------------------
bool u_config_counter_channel( uint8_t channel,char *s_param0, char *s_param1 )
{
	// Configuro un canal contador.
	// channel: id del canal
	// s_param0: string del nombre del canal
	// s_param1: string con el valor del factor magpp.
	//
	// {0..1} dname magPP

bool retS = false;

	while ( xSemaphoreTake( sem_SYSVars, ( TickType_t ) 5 ) != pdTRUE )
		taskYIELD();

	if ( ( channel >=  0) && ( channel < NRO_COUNTERS ) ) {

		// NOMBRE
		u_control_string(s_param0);
		snprintf_P( systemVars.counters_name[channel], PARAMNAME_LENGTH, PSTR("%s\0"), s_param0 );

		// MAGPP
		if ( s_param1 != NULL ) { systemVars.counters_magpp[channel] = atof(s_param1); }

		retS = true;
	}

	xSemaphoreGive( sem_SYSVars );
	return(retS);

}
//------------------------------------------------------------------------------------
bool u_config_analog_channel( uint8_t channel,char *s_aname,char *s_imin,char *s_imax,char *s_mmin,char *s_mmax )
{

	// Configura los canales analogicos. Es usada tanto desde el modo comando como desde el modo online por gprs.

bool retS = false;

	while ( xSemaphoreTake( sem_SYSVars, ( TickType_t ) 5 ) != pdTRUE )
		taskYIELD();

	u_control_string(s_aname);

	if ( ( channel >=  0) && ( channel < NRO_ANINPUTS ) ) {
		snprintf_P( systemVars.ain_name[channel], PARAMNAME_LENGTH, PSTR("%s\0"), s_aname );
		if ( s_imin != NULL ) { systemVars.ain_imin[channel] = atoi(s_imin); }
		if ( s_imax != NULL ) { systemVars.ain_imax[channel] = atoi(s_imax); }
		if ( s_mmin != NULL ) { systemVars.ain_mmin[channel] = atoi(s_mmin); }
		if ( s_mmax != NULL ) { systemVars.ain_mmax[channel] = atof(s_mmax); }
		retS = true;
	}

	xSemaphoreGive( sem_SYSVars );
	return(retS);
}
//------------------------------------------------------------------------------------
void u_read_analog_channel ( uint8_t io_board, uint8_t io_channel, uint16_t *raw_val, float *mag_val )
{
	// Lee un canal analogico y devuelve el valor convertido a la magnitud configurada.
	// Es publico porque se utiliza tanto desde el modo comando como desde el modulo de poleo de las entradas.
	// Hay que corregir la correspondencia entre el canal leido del INA y el canal fisico del datalogger
	// io_channel. Esto lo hacemos en AINPUTS_read_ina.

uint16_t an_raw_val;
float an_mag_val;
float I,M,P;
uint16_t D;


	an_raw_val = AINPUTS_read_ina(io_board, io_channel );
	*raw_val = an_raw_val;

	// Convierto el raw_value a la magnitud
	// Calculo la corriente medida en el canal
	I = (float)( an_raw_val) * 20 / ( systemVars.ain_inaspan[io_channel] + 1);

	// Calculo la magnitud
	P = 0;
	D = systemVars.ain_imax[io_channel] - systemVars.ain_imin[io_channel];

	an_mag_val = 0.0;
	if ( D != 0 ) {
		// Pendiente
		P = (float) ( systemVars.ain_mmax[io_channel]  -  systemVars.ain_mmin[io_channel] ) / D;
		// Magnitud
		M = (float) (systemVars.ain_mmin[io_channel] + ( I - systemVars.ain_imin[io_channel] ) * P);

		// Al calcular la magnitud, al final le sumo el offset.
		an_mag_val = M + systemVars.ain_offset[io_channel];
	} else {
		// Error: denominador = 0.
		an_mag_val = -999.0;
	}

	*mag_val = an_mag_val;

}
//------------------------------------------------------------------------------------
bool u_config_digital_channel( uint8_t channel,char *s_aname )
{

	// Configura los canales analogicos. Es usada tanto desde el modo comando como desde el modo online por gprs.

bool retS = false;

	while ( xSemaphoreTake( sem_SYSVars, ( TickType_t ) 5 ) != pdTRUE )
		taskYIELD();

	u_control_string(s_aname);

	if ( ( channel >=  0) && ( channel < NRO_DINPUTS ) ) {
		snprintf_P( systemVars.din_name[channel], PARAMNAME_LENGTH, PSTR("%s\0"), s_aname );
		retS = true;
	}

	xSemaphoreGive( sem_SYSVars );
	return(retS);
}
//------------------------------------------------------------------------------------
bool u_config_consignas( char *modo, char *hhmm_dia, char *hhmm_noche)
{

	if ( !strcmp_P( strupr(modo), PSTR("ON\0")) ) {
			systemVars.consigna.c_enabled = true;
		} else if (!strcmp_P( strupr(modo), PSTR("OFF\0")) ) {
			systemVars.consigna.c_enabled = false;
		} else {
			return(false);
		}

	if ( hhmm_dia != NULL ) {
		u_convert_int_to_time_t( atoi(hhmm_dia), &systemVars.consigna.hhmm_c_diurna );
	}

	if ( hhmm_noche != NULL ) {
		u_convert_int_to_time_t( atoi(hhmm_noche), &systemVars.consigna.hhmm_c_nocturna );
	}

	return(true);

}
//------------------------------------------------------------------------------------
// FUNCIONES PRIVADAS
//------------------------------------------------------------------------------------
static void pv_load_defaults_ainputs(void)
{

uint8_t channel;

	for ( channel = 0; channel < NRO_ANINPUTS; channel++) {
		systemVars.ain_imin[channel] = 0;
		systemVars.ain_imax[channel] = 20;
		systemVars.ain_mmin[channel] = 0;
		systemVars.ain_mmax[channel] = 6.0;
		systemVars.ain_offset[channel] = 0.0;
		systemVars.ain_inaspan[channel] = 3646;
		snprintf_P( systemVars.ain_name[channel], PARAMNAME_LENGTH, PSTR("A%d\0"),channel );
	}
}
//------------------------------------------------------------------------------------
static void pv_load_defaults_counters(void)
{

	// Realiza la configuracion por defecto de los canales contadores.
uint8_t i;

	for ( i = 0; i < NRO_COUNTERS; i++ ) {
		snprintf_P( systemVars.counters_name[i], PARAMNAME_LENGTH, PSTR("C%d\0"), i );
		systemVars.counters_magpp[i] = 0.1;
	}

	// Debounce Time
	systemVars.counter_debounce_time = 50;

}
//------------------------------------------------------------------------------------
static void pv_load_defaults_dinputs(void)
{

	// Realiza la configuracion por defecto de los canales digitales.
uint8_t i;

	systemVars.dinputs_timers = false;

	for ( i = 0; i < NRO_DINPUTS; i++ ) {
		snprintf_P( systemVars.din_name[i], PARAMNAME_LENGTH, PSTR("D%d\0"), i );
	}

}
//------------------------------------------------------------------------------------
static void pv_load_defaults_doutputs(void)
{
	// En el caso de SPX_IO8, configura la salida a que inicialmente este todo en off.
	systemVars.d_outputs = 0x00;
}
//------------------------------------------------------------------------------------
static void pv_load_defaults_consignas(void)
{

	systemVars.consigna.c_enabled = CONSIGNA_OFF;
	systemVars.consigna.hhmm_c_diurna.hour = 05;
	systemVars.consigna.hhmm_c_diurna.min = 30;
	systemVars.consigna.hhmm_c_nocturna.hour = 23;
	systemVars.consigna.hhmm_c_nocturna.min = 30;

}
//------------------------------------------------------------------------------------
static void pv_load_default_rangeMeter(void)
{
	systemVars.rangeMeter_enabled = false;
}
//------------------------------------------------------------------------------------

