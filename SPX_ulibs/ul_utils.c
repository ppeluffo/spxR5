/*
 * spx_utis.c
 *
 *  Created on: 10 dic. 2018
 *      Author: pablo
 */

#include "spx.h"
#include "tkApp.h"
#include "tkComms.h"

#define RTC32_ToscBusy()        !( VBAT.STATUS & VBAT_XOSCRDY_bm )

const uint8_t hash_table[] PROGMEM =  {
		 93,  153, 124,  98, 233, 146, 184, 207, 215,  54, 208, 223, 254, 216, 162, 141,
		 10,  148, 232, 115,   7, 202,  66,  31,   1,  33,  51, 145, 198, 181,  13,  95,
		 242, 110, 107, 231, 140, 170,  44, 176, 166,   8,   9, 163, 150, 105, 113, 149,
		 171, 152,  58, 133, 186,  27,  53, 111, 210,  96,  35, 240,  36, 168,  67, 213,
		 12,  123, 101, 227, 182, 156, 190, 205, 218, 139,  68, 217,  79,  16, 196, 246,
		 154, 116,  29, 131, 197, 117, 127,  76,  92,  14,  38,  99,   2, 219, 192, 102,
		 252,  74,  91, 179,  71, 155,  84, 250, 200, 121, 159,  78,  69,  11,  63,   5,
		 126, 157, 120, 136, 185,  88, 187, 114, 100, 214, 104, 226,  40, 191, 194,  50,
		 221, 224, 128, 172, 135, 238,  25, 212,   0, 220, 251, 142, 211, 244, 229, 230,
		 46,   89, 158, 253, 249,  81, 164, 234, 103,  59,  86, 134,  60, 193, 109,  77,
		 180, 161, 119, 118, 195,  82,  49,  20, 255,  90,  26, 222,  39,  75, 243, 237,
		 17,   72,  48, 239,  70,  19,   3,  65, 206,  32, 129,  57,  62,  21,  34, 112,
		 4,    56, 189,  83, 228, 106,  61,   6,  24, 165, 201, 167, 132,  45, 241, 247,
		 97,   30, 188, 177, 125,  42,  18, 178,  85, 137,  41, 173,  43, 174,  73, 130,
		 203, 236, 209, 235,  15,  52,  47,  37,  22, 199, 245,  23, 144, 147, 138,  28,
		 183,  87, 248, 160,  55,  64, 204,  94, 225, 143, 175, 169,  80, 151, 108, 122
};

//------------------------------------------------------------------------------------
void initMCU(void)
{
	// Inicializa los pines del micro

	// PWR_SLEEP
//	IO_config_PWR_SLEEP();
//	IO_set_PWR_SLEEP();

	// Configuro los pines del modulo Xbee
	IO_config_XBEE_PWR();
//	IO_config_XBEE_SLEEP();
//	IO_config_XBEE_RESET();

	// ANALOG: SENSOR VCC CONTROL
	IO_config_SENS_12V_CTL();

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
uint8_t u_control_string( char *s_name )
{
	// Controlo que el string terminado en \0 tenga solo letras o digitos.
	// Es porque si en un nombre de canal se cuela un caracter extranio, me
	// despelota los logs.
	// Si encuentro un caracter extraño, lo sustituyo por un \0 y salgo

uint8_t max_length = PARAMNAME_LENGTH;
char *p = NULL;
uint8_t cChar = 0;
uint8_t length = 0;

	p = (char *)s_name;
	while (*p && (max_length-- > 0) ) {

		cChar = (uint8_t)*p;
		if (  ! isalnum(cChar) )	{
			*p = '\0';
			return (length);
		}
		p++;
		length++;
	}

	return (length);
}
//------------------------------------------------------------------------------------
void u_convert_str_to_time_t ( char *time_str, st_time_t *time_struct )
{

	// Convierte un string hhmm en una estructura time_type que tiene
	// un campo hora y otro minuto

uint16_t time_num = 0;

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
void u_load_defaults( char *opt )
{
	// Carga la configuracion por defecto.

	systemVars.debug = DEBUG_NONE;

	if ( spx_io_board == SPX_IO8CH ) {
		systemVars.timerPoll = 60;
	} else {
		systemVars.timerPoll = 300;
	}

	// pwrsave se configura en gprs_utils
	// timepwrsensor se configura en ul_ainputs

	psensor_config_defaults();
	range_config_defaults();

	counters_config_defaults();
	dinputs_config_defaults();
	ainputs_config_defaults();
	xCOMMS_config_defaults(opt);

	// Modo de operacion
	sVarsApp.aplicacion = APP_OFF;

	xAPP_consigna_config_defaults();
	xAPP_plantapot_config_defaults();

}
//------------------------------------------------------------------------------------
uint8_t u_checksum( uint8_t *s, uint16_t size )
{
	/*
	 * Recibe un puntero a una estructura y un tamaño.
	 * Recorre la estructura en forma lineal y calcula el checksum
	 */

uint8_t *p = NULL;
uint8_t checksum = 0;
uint16_t i = 0;

	checksum = 0;
	p = s;
	for ( i = 0; i < size ; i++) {
		checksum += p[i];
	}
	checksum = ~checksum;
	return(checksum);
}
//------------------------------------------------------------------------------------
void u_save_params_in_NVMEE(void)
{
	/*
	 *  Tengo 3 estructuras de variables que debo guardar:
	 *  systemVars, sVarsApp, sVarsComms
	 *  Las guardo en este orden y luego de grabarlas, grabo un byte con el checksum de c/u
	 *
	 *  sVarsComms.checksum
	 *  sVarsComms
	 *  sVarsApp.checksum
	 *  sVarsApp
	 *  systemVars.checksum
	 *  systemVars	ADD 0x00
	 *
	 */


uint8_t checksum = 0;
uint16_t ee_wr_addr;

	// SystemVars.
	// Guardo systemVars en la EE
	ee_wr_addr = 0x00;
	nvm_eeprom_erase_and_write_buffer(ee_wr_addr, &systemVars, sizeof(systemVars));

	ee_wr_addr += sizeof(systemVars);
	checksum = u_checksum( (uint8_t *)&systemVars, sizeof(systemVars) );
	nvm_eeprom_erase_and_write_buffer(ee_wr_addr, &checksum, sizeof(checksum));

	// sVarsApp
	ee_wr_addr += 1;
	nvm_eeprom_erase_and_write_buffer(ee_wr_addr, &sVarsApp, sizeof(sVarsApp));

	ee_wr_addr += sizeof(sVarsApp);
	checksum = u_checksum( (uint8_t *)&sVarsApp, sizeof(sVarsApp) );
	nvm_eeprom_erase_and_write_buffer(ee_wr_addr, &checksum, sizeof(checksum));

	// sVarsComms
	ee_wr_addr += 1;
	nvm_eeprom_erase_and_write_buffer(ee_wr_addr, &sVarsComms, sizeof(sVarsComms));

	ee_wr_addr += sizeof(sVarsComms);
	checksum = u_checksum( (uint8_t *)&sVarsComms, sizeof(sVarsComms) );
	nvm_eeprom_erase_and_write_buffer(ee_wr_addr, &checksum, sizeof(checksum));


}
//------------------------------------------------------------------------------------
bool u_load_params_from_NVMEE(void)
{
	/*
	 * Leo el systemVars desde la EE.
	 * Calculo el checksum. Si no coincide es que hubo algun
	 * error por lo que cargo el default.
	 * Hago el proceso inverso de save
	 */

uint8_t stored_checksum;
uint8_t calculated_checksum;
uint16_t ee_rd_addr;

	// systemVars.
	ee_rd_addr = 0x00;
	nvm_eeprom_read_buffer(ee_rd_addr, (char *)&systemVars, sizeof(systemVars));
	calculated_checksum = u_checksum( (uint8_t *)&systemVars, sizeof(systemVars) );
	ee_rd_addr += sizeof(systemVars);
	nvm_eeprom_read_buffer(ee_rd_addr, (char *)&stored_checksum, sizeof(stored_checksum));
	if ( calculated_checksum != stored_checksum ) {
		xprintf_P( PSTR("ERROR: Checksum systemVars failed: calc[0x%0x], sto[0x%0x]\r\n"), calculated_checksum, stored_checksum );
		return(false);
	}

	// sVarsApp
	ee_rd_addr += 1;
	nvm_eeprom_read_buffer(ee_rd_addr, (char *)&sVarsApp, sizeof(sVarsApp));
	calculated_checksum = u_checksum( (uint8_t *)&sVarsApp, sizeof(sVarsApp) );
	ee_rd_addr += sizeof(sVarsApp);
	nvm_eeprom_read_buffer(ee_rd_addr, (char *)&stored_checksum, sizeof(stored_checksum));
	if ( calculated_checksum != stored_checksum ) {
		xprintf_P( PSTR("ERROR: Checksum sVarsApp failed: calc[0x%0x], sto[0x%0x]\r\n"), calculated_checksum, stored_checksum );
		return(false);
	}

	// sVarsComms
	ee_rd_addr += 1;
	nvm_eeprom_read_buffer(ee_rd_addr, (char *)&sVarsComms, sizeof(sVarsComms));
	calculated_checksum = u_checksum( (uint8_t *)&sVarsComms, sizeof(sVarsComms) );
	ee_rd_addr += sizeof(sVarsComms);
	nvm_eeprom_read_buffer(ee_rd_addr, (char *)&stored_checksum, sizeof(stored_checksum));
	if ( calculated_checksum != stored_checksum ) {
		xprintf_P( PSTR("ERROR: Checksum sVarsComms failed: calc[0x%0x], sto[0x%0x]\r\n"), calculated_checksum, stored_checksum );
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

	//xprintf_P( PSTR("DEBUG_A TPOLL CONFIG: [%s]\r\n\0"), s_timerpoll );

	systemVars.timerPoll = atoi(s_timerpoll);

	if ( systemVars.timerPoll < 15 )
		systemVars.timerPoll = 15;

	if ( systemVars.timerPoll > 3600 )
		systemVars.timerPoll = 300;

	//xprintf_P( PSTR("DEBUG_B TPOLL CONFIG: [%d]\r\n\0"), systemVars.timerPoll );
	//u_gprs_redial();

	return;
}
//------------------------------------------------------------------------------------
bool u_check_more_Rcds4Del ( FAT_t *fat )
{
	// Devuelve si aun quedan registros para borrar del FS

	memset ( fat, '\0', sizeof( FAT_t));

	FAT_read(fat);

	if ( fat->rcds4del > 0 ) {
		return(true);
	} else {
		return(false);
	}

}
//------------------------------------------------------------------------------------
bool u_check_more_Rcds4Tx(void)
{

	/* Veo si hay datos en memoria para trasmitir
	 * Memoria vacia: rcds4wr = MAX, rcds4del = 0;
	 * Memoria llena: rcds4wr = 0, rcds4del = MAX;
	 * Memoria toda leida: rcds4rd = 0;
	 */

bool retS = false;
FAT_t fat;

	memset( &fat, '\0', sizeof ( FAT_t));
	FAT_read( &fat );

	// Si hay registros para leer
	if ( fat.rcds4rd > 0) {
		retS = true;
	} else {
		retS = false;
		if ( systemVars.debug == DEBUG_COMMS ) {
			xprintf_P( PSTR("COMMS: bd EMPTY\r\n\0"));
		}
	}

	return(retS);
}
//------------------------------------------------------------------------------------
uint8_t u_base_hash(void)
{

	/*
	 *  La configuracin BASE incluye:
	 *  - timerdial
	 *  - timerpoll
	 *  - timepwrsensor
	 *  - pwrsave
	 *  - counters_hw
	 *
	 */
uint8_t hash = 0;
char dst[32];
char *p;
uint8_t i = 0;

	// calculate own checksum
	// Vacio el buffer temoral
	memset(dst,'\0', sizeof(dst));

	// Timerdial
	i = snprintf_P( &dst[i], sizeof(dst), PSTR("%d,"), sVarsComms.timerDial );
	// TimerPoll
	i += snprintf_P( &dst[i], sizeof(dst), PSTR("%d,"), systemVars.timerPoll );
	// TimepwrSensor
	i += snprintf_P( &dst[i], sizeof(dst), PSTR("%d,"), systemVars.ainputs_conf.pwr_settle_time );

	// PwrSave
	if ( sVarsComms.pwrSave.pwrs_enabled ) {
		i += snprintf_P( &dst[i], sizeof(dst), PSTR("ON,"));
	} else {
		i += snprintf_P( &dst[i], sizeof(dst), PSTR("OFF,"));
	}

	i += snprintf_P(&dst[i], sizeof(dst), PSTR("%02d%02d,"), sVarsComms.pwrSave.hora_start.hour, sVarsComms.pwrSave.hora_start.min );
	i += snprintf_P(&dst[i], sizeof(dst), PSTR("%02d%02d,"), sVarsComms.pwrSave.hora_fin.hour, sVarsComms.pwrSave.hora_fin.min );

	// Counters_hw ( 0: simple, 1 opto )
	if ( systemVars.counters_conf.hw_type == COUNTERS_HW_SIMPLE ) {
		i += snprintf_P(&dst[i], sizeof(dst), PSTR("SIMPLE,"));
	} else {
		i += snprintf_P(&dst[i], sizeof(dst), PSTR("OPTO,"));
	}

	//xprintf_P( PSTR("DEBUG: BASEHASH = [%s]\r\n\0"), dst );

	// Apunto al comienzo para recorrer el buffer
	p = dst;
	// Mientras no sea NULL calculo el checksum deol buffer
	while (*p != '\0') {
		//checksum += *p++;
		hash = u_hash(hash, *p++);
	}

	//xprintf_P( PSTR("DEBUG: cks = [0x%02x]\r\n\0"), checksum );

	return(hash);

}
//------------------------------------------------------------------------------------
uint8_t u_aplicacion_hash( void )
{
	// La aplicacion puede tener hasta 3 partes por lo tanto debo generar
	// siempre los 3 checksums

uint8_t hash = 0;

	switch(sVarsApp.aplicacion) {
	case APP_OFF:
		hash = xAPP_off_hash();
		break;

	case APP_CONSIGNA:
		hash = xAPP_consigna_hash();
		break;

	case APP_PERFORACION:
		hash = xAPP_perforacion_hash();
		break;

	case APP_PLANTAPOT:
		hash = xAPP_plantapot_hash();
		break;

	case APP_CAUDALIMETRO:
		hash = xAPP_caudalimetro_hash();
		break;
	}

	return(hash);

}
//------------------------------------------------------------------------------------
bool u_config_aplicacion( char *modo )
{

	if ( strcmp_P( strupr(modo), PSTR("OFF\0")) == 0) {
		sVarsApp.aplicacion = APP_OFF;
		return(true);
	}

	if ( ( strcmp_P( strupr(modo), PSTR("CONSIGNA\0"))  == 0) &&  ( spx_io_board == SPX_IO5CH ) ) {
		sVarsApp.aplicacion = APP_CONSIGNA;
		return(true);
	}

	if ( (strcmp_P( strupr(modo), PSTR("PERFORACION\0")) == 0) &&  ( spx_io_board == SPX_IO8CH ) ) {
		sVarsApp.aplicacion = APP_PERFORACION;
		return(true);
	}

	if ( (strcmp_P( strupr(modo), PSTR("CAUDALIMETRO\0")) == 0) &&  ( spx_io_board == SPX_IO8CH ) ) {
		sVarsApp.aplicacion = APP_CAUDALIMETRO;
		return(true);
	}

	return(false);

}
//------------------------------------------------------------------------------------
bool u_write_output_pins( uint8_t pin, int8_t val )
{

	/* Esta funcion pone los pines en un valor dado 0 o 1.
	 * El tema es visto desde donde ?.
	 * Porque como las salidas estan implementadas con un transistor
	 * a la salida del pin del MCP, si pongo un 0 en el MCP, en el
	 * circuito externo esto queda por un pull-up en 1.
	 * La norma que seguimos es que el valor que ponemos es el que vemos en
	 * el circuito !!!.
	 * Para poner un 0 (GND) en el circuito, entonces debo escribir un 1 en el MCP. !!!
	 */
int8_t ret_code = 0;
bool retS = false;


	if ( spx_io_board == SPX_IO8CH ) {

		// Tenemos 8 salidas que las manejamos con el MCP
		if ( pin > 7 ) {
			retS = false;
			goto exit;
		}


		if ( val == 0) {
			ret_code = IO_set_DOUT(pin);		// Pongo un 1 en el MCP
			if ( ret_code == -1 ) {
				// Error de bus
				xprintf_P( PSTR("wOUTPUT_PIN: I2C bus error(1)\r\n\0"));
				retS = false;
				goto exit;
			}
			retS = true;
			goto exit;

		}

		if ( val == 1) {
			ret_code = IO_clr_DOUT(pin);		// Pongo un 0 en el MCP
			if ( ret_code == -1 ) {
				// Error de bus
				xprintf_P( PSTR("wOUTPUT_PIN: I2C bus error(2)\n\0"));
				retS = false;
				goto exit;
			}
			retS = true;
			goto exit;
		}

	} else if ( spx_io_board == SPX_IO5CH ) {
		// Las salidas las manejamos con el DRV8814
		// Las manejo de modo que solo muevo el pinA y el pinB queda en GND para c/salida
		if ( pin > 2 )
			return(false);

		DRV8814_power_on();
		// Espero 10s que se carguen los condensasores
		vTaskDelay( ( TickType_t)( 1000 / portTICK_RATE_MS ) );

		if ( val == 1) {
			switch(pin) {
			case 0:
				DRV8814_vopen( 'A', 100);
				break;
			case 1:
				DRV8814_vopen( 'B', 100);
				break;
			}
			retS = true;
		}

		if ( val == 0) {
			switch(pin) {
			case 0:
				DRV8814_vclose( 'A', 100);
				break;
			case 1:
				DRV8814_vclose( 'B', 100);
				break;
			}
			retS = true;
		}

		DRV8814_power_off();

	}

exit:

	return(retS);
}
//------------------------------------------------------------------------------------
bool u_set_douts( uint8_t dout )
{
	// Funcion para setear el valor de las salidas desde el resto del programa.
	// La usamos desde tkGprs cuando en un frame nos indican cambiar las salidas.
	// Como el cambio depende de quien tiene el control y del timer, aqui vemos si
	// se cambia o se ignora.

uint8_t data = 0;
int8_t xBytes = 0;
bool retS = false;

	// Solo es para IO8CH
	if ( spx_io_board != SPX_IO8CH ) {
		return(false);
	}

	// Vemos que no se halla desconfigurado
	MCP_check();

	// Guardo el valor recibido
	data = dout;
	MCP_update_olatb( dout );

	// Invierto el byte antes de escribirlo !!!
	data = twiddle_bits(data);
	xBytes = MCP_write(MCP_OLATB, (char *)&data, 1 );
	if ( xBytes == -1 ) {
		xprintf_P(PSTR("SET DOUTS ERROR: MCP write\r\n\0"));
	} else {
		retS = true;
	}

	return(retS);
}
//------------------------------------------------------------------------------------
void u1_debug_printf_P( t_debug dmode,  PGM_P fmt, ...)
{

	/* Funcion de wrapper que imprime un mensaje de debug siempre que el modo configurado
	 * coincida con el dmode
	 * http://c-faq.com/varargs/handoff.html
	 */

va_list argp;

	if ( systemVars.debug == dmode ) {
		va_start(argp, fmt);

		xfprintf_V( fdTERM, fmt, argp );
		va_end(argp);
	}
}
//------------------------------------------------------------------------------------
bool SPX_SIGNAL( uint8_t signal )
{

bool retS = false;

	switch(signal) {
	case SGN_MON_SQE:
		retS = system_signals.sgn_mon_sqe;
		break;
	case SGN_REDIAL:
		retS = system_signals.sgn_redial;
		break;
	case SGN_FRAME_READY:
		retS = system_signals.sgn_frame_ready;
		break;
	case SGN_RESET_COMMS_DEV:
		retS = system_signals.sgn_reset_comms_device;
		break;
	case SGN_SMS:
		retS = system_signals.sgn_sms;
		break;
	default:
		break;
	}
	return(retS);
}
//------------------------------------------------------------------------------------
bool SPX_SEND_SIGNAL( uint8_t signal )
{
	/*
	 * El envio de señales a otras partes del sistema es prendiendo la
	 * flag correspondiente.
	 */

bool retS = false;

	switch(signal) {
	case SGN_MON_SQE:
		system_signals.sgn_mon_sqe = true;
		retS = true;
		break;
	case SGN_REDIAL:
		system_signals.sgn_redial = true;
		retS = true;
		break;
	case SGN_FRAME_READY:
		system_signals.sgn_frame_ready = true;
		retS = true;
		break;
	case SGN_RESET_COMMS_DEV:
		system_signals.sgn_reset_comms_device = true;
		retS = true;
		break;
	case SGN_SMS:
		system_signals.sgn_sms = true;
		retS = true;
		break;
	default:
		retS = false;
		break;
	}
	return(retS);
}
//------------------------------------------------------------------------------------
bool SPX_CLEAR_SIGNAL( uint8_t signal )
{

bool retS = false;

	switch(signal) {
	case SGN_MON_SQE:
		system_signals.sgn_mon_sqe = false;
		retS = true;
		break;
	case SGN_REDIAL:
		system_signals.sgn_redial = false;
		retS = true;
		break;
	case SGN_FRAME_READY:
		system_signals.sgn_frame_ready = false;
		retS = true;
		break;
	case SGN_RESET_COMMS_DEV:
		system_signals.sgn_reset_comms_device = false;
		retS = true;
		break;
	case SGN_SMS:
		system_signals.sgn_sms = false;
		retS = true;
		break;
	default:
		retS = false;
		break;
	}
	return(retS);
}
//------------------------------------------------------------------------------------
void u_config_timerdial ( char *s_timerdial )
{
	// El timer dial puede ser 0 si vamos a trabajar en modo continuo o mayor a
	// 15 minutos.
	// Es una variable de 32 bits para almacenar los segundos de 24hs.


	//xprintf_P( PSTR("DEBUG_A TDIAL CONFIG: [%s]\r\n\0"), s_timerdial );

	while ( xSemaphoreTake( sem_SYSVars, ( TickType_t ) 5 ) != pdTRUE )
		taskYIELD();

	sVarsComms.timerDial = atoi(s_timerdial);

	// Controlo que este en los rangos permitidos
	if ( (sVarsComms.timerDial > 0) && (sVarsComms.timerDial < TDIAL_MIN_DISCRETO ) ) {
		//systemVars.gprs_conf.timerDial = 0;
		//xprintf_P( PSTR("TDIAL warn !! Default to 0. ( continuo TDIAL=0, discreto TDIAL > 900)\r\n\0"));
		xprintf_P( PSTR("TDIAL warn: continuo TDIAL < 900, discreto TDIAL >= 900)\r\n\0"));
	}

	xSemaphoreGive( sem_SYSVars );
	//xprintf_P( PSTR("DEBUG_B TDIAL CONFIG: [%d]\r\n\0"), systemVars.gprs_conf.timerDial );
	return;
}
//------------------------------------------------------------------------------------
void u_configPwrSave( char *s_modo, char *s_startTime, char *s_endTime)
{
	// Recibe como parametros el modo ( 0,1) y punteros a string con las horas de inicio y fin del pwrsave
	// expresadas en minutos.

	while ( xSemaphoreTake( sem_SYSVars, ( TickType_t ) 5 ) != pdTRUE )
		taskYIELD();

	//xprintf_P( PSTR("DEBUG_A: PWRS modo=%s, startitme=%s, endtime=%s\r\n\0"), s_modo, s_startTime, s_endTime );

	if (strcmp_P( strupr(s_modo), PSTR( "OFF")) == 0 ) {
		sVarsComms.pwrSave.pwrs_enabled = false;
		//xprintf_P( PSTR("DEBUG: pwrsave off(%d)\r\n\0"), systemVars.gprs_conf.pwrSave.pwrs_enabled );
		if ( s_startTime != NULL ) { u_convert_str_to_time_t( s_startTime, &sVarsComms.pwrSave.hora_start); }
		if ( s_endTime != NULL ) { u_convert_str_to_time_t( s_endTime, &sVarsComms.pwrSave.hora_fin); }

		goto quit;
	}

	if (strcmp_P( strupr(s_modo), PSTR( "ON")) == 0) {
		sVarsComms.pwrSave.pwrs_enabled = true;
		//xprintf_P( PSTR("DEBUG: pwrsave ON(%d)\r\n\0"), systemVars.gprs_conf.pwrSave.pwrs_enabled );
		if ( s_startTime != NULL ) { u_convert_str_to_time_t( s_startTime, &sVarsComms.pwrSave.hora_start); }
		if ( s_endTime != NULL ) { u_convert_str_to_time_t( s_endTime, &sVarsComms.pwrSave.hora_fin); }
		//xprintf_P( PSTR("DEBUG: pwrsave start_time = %02d%02d\r\n\0"), systemVars.gprs_conf.pwrSave.hora_start.hour, systemVars.gprs_conf.pwrSave.hora_start.min);
		//xprintf_P( PSTR("DEBUG: pwrsave end_time = %02d%02d\r\n\0"), systemVars.gprs_conf.pwrSave.hora_fin.hour, systemVars.gprs_conf.pwrSave.hora_fin.min);
		goto quit;
	}

quit:

	xSemaphoreGive( sem_SYSVars );

	// En los SPXIO8 no se usa el pwrsave !!!!. Siempre debe venir OFF de la BD.
	//if ( spx_io_board == SPX_IO8CH ) {
	//	systemVars.gprs_conf.pwrSave.pwrs_enabled = false;
	//}
	//xprintf_P( PSTR("DEBUG_B: PWRS modo=%d, startitme=%02d%02d, endtime=%02d%02d\r\n\0"), systemVars.gprs_conf.pwrSave.pwrs_enabled, systemVars.gprs_conf.pwrSave.hora_start.hour, systemVars.gprs_conf.pwrSave.hora_start.min, systemVars.gprs_conf.pwrSave.hora_fin.hour, systemVars.gprs_conf.pwrSave.hora_fin.min );

}
//------------------------------------------------------------------------------------
uint8_t u_hash(uint8_t checksum, char ch )
{
	/*
	 * Funcion de hash de pearson modificada.
	 * https://es.qwe.wiki/wiki/Pearson_hashing
	 * La función original usa una tabla de nros.aleatorios de 256 elementos.
	 * Ya que son aleatorios, yo la modifico a algo mas simple.
	 */

uint8_t h_entry = 0;
uint8_t h_val = 0;
uint8_t ord;

	ord = (int)ch;
	h_entry = checksum ^ ord;
	h_val = (PGM_P)pgm_read_byte_far(&(hash_table[h_entry]));
	return(h_val);

}
//------------------------------------------------------------------------------------
void u_hash_test(void)
{

uint8_t checksum = 0;
char dst[64];
char *p;

	// C0:C0,1.000,100,10,0;C1:C1,1.000,100,10,0;

	strcpy(dst, "C0:C0,1.000,10,100,0;C1:C1,1.000,100,10,0;\0");

	p = dst;
	// Mientras no sea NULL calculo el checksum deol buffer
	while (*p != '\0') {
		checksum = u_hash(checksum, *p++);
	}
	xprintf_P( PSTR("DEBUG: CCKS = [%s]\r\n\0"), dst );
	xprintf_P( PSTR("DEBUG: cks = [0x%02x]\r\n\0"), checksum );

}

