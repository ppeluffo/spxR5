/*
 * spx_ainputs.c
 *
 *  Created on: 8 mar. 2019
 *      Author: pablo
 */

#include "spx.h"

static bool sensores_prendidos = false;
static bool autocal_running = false;
float battery;

static uint16_t pv_ainputs_read_battery_raw(void);
static uint16_t pv_ainputs_read_channel_raw(uint8_t channel_id );
static void pv_ainputs_apagar_12Vsensors(void);
static void pv_ainputs_prender_12V_sensors(void);
static float pv_ainputs_read_channel ( uint8_t io_channel );
static void pv_ainputs_read_battery(float *battery);
static float pv_analog_convert_ieqv( float i_real, uint8_t io_channel );

//------------------------------------------------------------------------------------
void ainputs_init(void)
{
	// Inicializo los INA con los que mido las entradas analogicas.
	ainputs_awake();
	ainputs_sleep();

}
//------------------------------------------------------------------------------------
void ainputs_awake(void)
{
	switch (spx_io_board) {
	case SPX_IO5CH:
		INA_config_avg128(INA_A );
		INA_config_avg128(INA_B );
		break;
	case SPX_IO8CH:
		INA_config_avg128(INA_A );
		INA_config_avg128(INA_B );
		INA_config_avg128(INA_C );
		break;
	}
}
//------------------------------------------------------------------------------------
void ainputs_sleep(void)
{
	switch (spx_io_board) {
	case SPX_IO5CH:
		INA_config_sleep(INA_A );
		INA_config_sleep(INA_B );
		break;
	case SPX_IO8CH:
		INA_config_sleep(INA_A );
		INA_config_sleep(INA_B );
		INA_config_sleep(INA_C );
		break;
	}
}
//------------------------------------------------------------------------------------
bool ainputs_config_channel( uint8_t channel,char *s_aname,char *s_imin,char *s_imax,char *s_mmin,char *s_mmax )
{

	// Configura los canales analogicos. Es usada tanto desde el modo comando como desde el modo online por gprs.
	// Detecto si hubo un cambio en el rango de valores de corriente para entonces restaurar los valores de
	// las corrientes equivalente.


bool retS = false;
bool change_ieqv = false;

	//xprintf_P( PSTR("DEBUG ANALOG CONFIG: A%d,name=%s,imin=%s,imax=%s,mmin=%s, mmax=%s\r\n\0"), channel, s_aname, s_imin, s_imax, s_mmin, s_mmax );

	if ( u_control_string(s_aname) == 0 ) {
		xprintf_P( PSTR("DEBUG ANALOG ERROR: A%d[%s]\r\n\0"), channel, s_aname );
		return( false );
	}

	if ( s_aname == NULL ) {
		return(retS);
	}

	if ( ( channel >=  0) && ( channel < NRO_ANINPUTS ) ) {
		snprintf_P( systemVars.ainputs_conf.name[channel], PARAMNAME_LENGTH, PSTR("%s\0"), s_aname );

		if ( s_imin != NULL ) {
			if ( systemVars.ainputs_conf.imin[channel] != atoi(s_imin) ) {
				systemVars.ainputs_conf.imin[channel] = atoi(s_imin);
				change_ieqv = true;
			}
		}

		if ( s_imax != NULL ) {
			if ( systemVars.ainputs_conf.imax[channel] != atoi(s_imax) ) {
				systemVars.ainputs_conf.imax[channel] = atoi(s_imax);
				change_ieqv = true;
			}
		}

		// Ajusto los ieqv
		if ( change_ieqv ) {
			systemVars.ainputs_conf.ieq_min[channel] = systemVars.ainputs_conf.imin[channel];
			systemVars.ainputs_conf.ieq_max[channel] = systemVars.ainputs_conf.imax[channel];
			//xprintf_P( PSTR("DEBUG ANALOG Modifico IEQV\r\n\0"), channel );
		}

		if ( s_mmin != NULL ) { systemVars.ainputs_conf.mmin[channel] = atoi(s_mmin); }
		if ( s_mmax != NULL ) { systemVars.ainputs_conf.mmax[channel] = atof(s_mmax); }

		// Si el nombre es X deshabilito todo
/*		if ( strcmp ( systemVars.ainputs_conf.name[channel], "X" ) == 0 ) {
			systemVars.ainputs_conf.imin[channel] = 0;
			systemVars.ainputs_conf.imax[channel] = 0;
			systemVars.ainputs_conf.mmin[channel] = 0;
			systemVars.ainputs_conf.mmax[channel] = 0;
			systemVars.ainputs_conf.ieq_min[channel] = 0;
			systemVars.ainputs_conf.ieq_max[channel] = 0;
		}
*/
		retS = true;
	}

	return(retS);
}
//------------------------------------------------------------------------------------
bool ainputs_config_offset( char *s_channel, char *s_offset )
{
	// Configuro el parametro offset de un canal analogico.

uint8_t channel = 0;
float offset = 0.0;

	channel = atoi(s_channel);
	if ( ( channel >=  0) && ( channel < NRO_ANINPUTS ) ) {
		offset = atof(s_offset);
		systemVars.ainputs_conf.offset[channel] = offset;
		return(true);
	}

	return(false);
}
//------------------------------------------------------------------------------------
void ainputs_config_timepwrsensor ( char *s_sensortime )
{
	// Configura el tiempo de espera entre que prendo  la fuente de los sensores y comienzo el poleo.
	// Se utiliza solo desde el modo comando.
	// El tiempo de espera debe estar entre 1s y 15s

	systemVars.ainputs_conf.pwr_settle_time = atoi(s_sensortime);

	if ( systemVars.ainputs_conf.pwr_settle_time < 1 )
		systemVars.ainputs_conf.pwr_settle_time = 1;

	if ( systemVars.ainputs_conf.pwr_settle_time > 15 )
		systemVars.ainputs_conf.pwr_settle_time = 15;

	return;
}
//------------------------------------------------------------------------------------
void ainputs_config_span ( char *s_channel, char *s_span )
{
	// Configura el factor de correccion del span de canales delos INA.
	// Esto es debido a que las resistencias presentan una tolerancia entonces con
	// esto ajustamos que con 20mA den la excursión correcta.
	// Solo de configura desde modo comando.

uint8_t channel = 0;
uint16_t span = 0;

	channel = atoi(s_channel);
	if ( ( channel >=  0) && ( channel < NRO_ANINPUTS ) ) {
		span = atoi(s_span);
		systemVars.ainputs_conf.inaspan[channel] = span;
	}
	return;

}
//------------------------------------------------------------------------------------
bool ainputs_config_ical( char *s_channel, char *s_ieqv )
{
	// Para un canal dado, pongo una corriente de referecia en 4 o 20mA y
	// la mido. Este sera el valor equivalente.


uint8_t channel = 0;
uint16_t an_raw_val = 0;
float I = 0.0;
bool retS = false;

	channel = atoi(s_channel);

	if ( channel >= NRO_ANINPUTS ) {
		return(retS);
	}

	// Indico a la tarea analogica de no polear ni tocar los canales ni el pwr.
//	signal_tkData_poll_off();
	pv_ainputs_prender_12V_sensors();
	vTaskDelay( ( TickType_t)( ( 1000 * systemVars.ainputs_conf.pwr_settle_time ) / portTICK_RATE_MS ) );

	// Leo el canal del ina.
	ainputs_awake();
	an_raw_val = pv_ainputs_read_channel_raw( channel );
	ainputs_sleep();

	// Habilito a la tkData a volver a polear
//	signal_tkData_poll_on();

	// Convierto el raw_value a la magnitud
	I = (float)( an_raw_val) * 20 / ( systemVars.ainputs_conf.inaspan[channel] + 1);
	//xprintf_P( PSTR("ICAL: ch%d: ANRAW=%d, I=%.03f\r\n\0"), channel, an_raw_val, I );

	if  ( strcmp_P( strupr(s_ieqv), PSTR("IMIN\0")) == 0 ) {
		systemVars.ainputs_conf.ieq_min[channel] = I;
		xprintf_P( PSTR("ICALmin: ch%d: %d mA->%.03f\r\n\0"), channel, systemVars.ainputs_conf.imin[channel], systemVars.ainputs_conf.ieq_min[channel] );
		retS = true;
	} else if ( strcmp_P( strupr(s_ieqv), PSTR("IMAX\0")) == 0 ) {
		systemVars.ainputs_conf.ieq_max[channel] = I;
		xprintf_P( PSTR("ICALmax: ch%d: %d mA->%.03f\r\n\0"), channel, systemVars.ainputs_conf.imax[channel], systemVars.ainputs_conf.ieq_max[channel] );
		retS = true;
	}

	return(retS);

}
//------------------------------------------------------------------------------------
bool ainputs_config_autocalibrar( char *s_channel, char *s_mag_val )
{
	// Para un canal, toma como entrada el valor de la magnitud y ajusta
	// mag_offset para que la medida tomada coincida con la dada.


uint16_t an_raw_val = 0;
float an_mag_val = 0.0;
float I = 0.0;
float M = 0.0;
float P = 0.0;
uint16_t D = 0;
uint8_t channel = 0;

float an_mag_val_real = 0.0;
float offset = 0.0;

	channel = atoi(s_channel);

	if ( channel >= NRO_ANINPUTS ) {
		return(false);
	}

	// Indico a la tarea analogica de no polear ni tocar los canales ni el pwr.
	autocal_running = true;

	pv_ainputs_prender_12V_sensors();
	vTaskDelay( ( TickType_t)( ( 1000 * systemVars.ainputs_conf.pwr_settle_time ) / portTICK_RATE_MS ) );

	// Leo el canal del ina.
	ainputs_awake();
	an_raw_val = pv_ainputs_read_channel_raw( channel );
	ainputs_sleep();
//	xprintf_P( PSTR("ANRAW=%d\r\n\0"), an_raw_val );

	// Convierto el raw_value a la magnitud
	I = (float)( an_raw_val) * 20 / ( systemVars.ainputs_conf.inaspan[channel] + 1);
	P = 0;
	D = systemVars.ainputs_conf.imax[channel] - systemVars.ainputs_conf.imin[channel];

	// Habilito a la tkData a volver a polear
	autocal_running = false;

	an_mag_val = 0.0;
	if ( D != 0 ) {
		// Pendiente
		P = (float) ( systemVars.ainputs_conf.mmax[channel]  -  systemVars.ainputs_conf.mmin[channel] ) / D;
		// Magnitud
		M = (float) ( systemVars.ainputs_conf.mmin[channel] + ( I - systemVars.ainputs_conf.imin[channel] ) * P);

		// En este caso el offset que uso es 0 !!!.
		an_mag_val = M;

	} else {
		return(false);
	}

//	xprintf_P( PSTR("ANMAG=%.02f\r\n\0"), an_mag_val );

	an_mag_val_real = atof(s_mag_val);
//	xprintf_P( PSTR("ANMAG_T=%.02f\r\n\0"), an_mag_val_real );

	offset = an_mag_val_real - an_mag_val;
//	xprintf_P( PSTR("AUTOCAL offset=%.02f\r\n\0"), offset );

	systemVars.ainputs_conf.offset[channel] = offset;

	xprintf_P( PSTR("OFFSET=%.02f\r\n\0"), systemVars.ainputs_conf.offset[channel] );

	return(true);

}
//------------------------------------------------------------------------------------
void ainputs_config_defaults(void)
{
	// Realiza la configuracion por defecto de los canales digitales.

uint8_t channel = 0;

	systemVars.ainputs_conf.pwr_settle_time = 5;

	for ( channel = 0; channel < NRO_ANINPUTS; channel++) {
		systemVars.ainputs_conf.imin[channel] = 0;
		systemVars.ainputs_conf.imax[channel] = 20;
		systemVars.ainputs_conf.mmin[channel] = 0.0;
		systemVars.ainputs_conf.mmax[channel] = 6.0;
		systemVars.ainputs_conf.offset[channel] = 0.0;
		systemVars.ainputs_conf.inaspan[channel] = 3646;

		systemVars.ainputs_conf.ieq_min[channel] = 0.0;
		systemVars.ainputs_conf.ieq_max[channel] = 20.0;

		snprintf_P( systemVars.ainputs_conf.name[channel], PARAMNAME_LENGTH, PSTR("A%d\0"),channel );
	}

}
//------------------------------------------------------------------------------------
bool ainputs_read( float ain[], float *battery )
{

bool retS = false;

	ainputs_awake();
	//
	if ( ! sensores_prendidos ) {
		pv_ainputs_prender_12V_sensors();
		sensores_prendidos = true;
		// Normalmente espero 1s de settle time que esta bien para los sensores
		// pero cuando hay un caudalimetro de corriente, necesita casi 5s
		// vTaskDelay( ( TickType_t)( 1000 / portTICK_RATE_MS ) );
		vTaskDelay( ( TickType_t)( ( 1000 * systemVars.ainputs_conf.pwr_settle_time ) / portTICK_RATE_MS ) );
	}

	// Leo.
	// Los canales de IO no son los mismos que los canales del INA !! ya que la bateria
	// esta en el canal 1 del ina2
	// Lectura general.
	ain[0] = pv_ainputs_read_channel(0);
	ain[1] = pv_ainputs_read_channel(1);
	ain[2] = pv_ainputs_read_channel(2);
	ain[3] = pv_ainputs_read_channel(3);
	ain[4] = pv_ainputs_read_channel(4);

	if ( spx_io_board == SPX_IO8CH ) {
		ain[5] = pv_ainputs_read_channel(5);
		ain[6] = pv_ainputs_read_channel(6);
		ain[7] = pv_ainputs_read_channel(7);
	}

	if ( spx_io_board == SPX_IO5CH ) {
		// Leo la bateria
		// Convierto el raw_value a la magnitud ( 8mV por count del A/D)
		pv_ainputs_read_battery(battery);
	} else {
		*battery = 0.0;
	}

	// Apago los sensores y pongo a los INA a dormir si estoy con la board IO5.
	// Sino dejo todo prendido porque estoy en modo continuo
	//if ( (spx_io_board == SPX_IO5CH) && ( systemVars.timerPoll > 180 ) ) {
	if ( spx_io_board == SPX_IO5CH ) {
		pv_ainputs_apagar_12Vsensors();
		sensores_prendidos = false;
	}
	//
	ainputs_sleep();

	return(retS);

}
//------------------------------------------------------------------------------------
void ainputs_print(file_descriptor_t fd, float src[] )
{
	// Imprime los canales configurados ( no X ) en un fd ( tty_gprs,tty_xbee,tty_term) en
	// forma formateada.
	// Los lee de una estructura array pasada como src

uint8_t i = 0;

	for ( i = 0; i < NRO_ANINPUTS; i++) {
		if ( strcmp ( systemVars.ainputs_conf.name[i], "X" ) != 0 )
			xCom_printf_P(fd, PSTR("%s:%.02f;"), systemVars.ainputs_conf.name[i], src[i] );
	}

}
//------------------------------------------------------------------------------------
void ainputs_battery_print( file_descriptor_t fd, float battery )
{
	// bateria
	if ( spx_io_board == SPX_IO5CH ) {
		xCom_printf_P(fd, PSTR("bt:%.02f;"), battery );
	}
}
//------------------------------------------------------------------------------------
bool ainputs_autocal_running(void)
{
	return(autocal_running);
}
//------------------------------------------------------------------------------------
uint8_t ainputs_checksum(void)
{
 // https://portal.u-blox.com/s/question/0D52p00008HKDMyCAP/python-code-to-generate-checksums-so-that-i-may-validate-data-coming-off-the-serial-interface

uint16_t i;
uint8_t checksum = 0;
char dst[32];
char *p;
uint8_t j = 0;

	//	char name[MAX_ANALOG_CHANNELS][PARAMNAME_LENGTH];
	//	uint8_t imin[MAX_ANALOG_CHANNELS];	// Coeficientes de conversion de I->magnitud (presion)
	//	uint8_t imax[MAX_ANALOG_CHANNELS];
	//	float mmin[MAX_ANALOG_CHANNELS];
	//	float mmax[MAX_ANALOG_CHANNELS];

	// A0:A0,0,20,0.000,6.000;A1:A1,0,20,0.000,6.000;A2:A2,0,20,0.000,6.000;A3:A3,0,20,0.000,6.000;A4:A4,0,20,0.000,6.000;

	// calculate own checksum
	for(i=0;i<NRO_ANINPUTS;i++) {
		// Vacio el buffer temoral
		memset(dst,'\0', sizeof(dst));
		// Copio sobe el buffer una vista ascii ( imprimible ) de c/registro.
		j = 0;
		j += snprintf_P( &dst[j], sizeof(dst), PSTR("A%d:%s,"), i, systemVars.ainputs_conf.name[i] );
		j += snprintf_P(&dst[j], sizeof(dst), PSTR("%d,%d,"), systemVars.ainputs_conf.imin[i],systemVars.ainputs_conf.imax[i] );
		j += snprintf_P(&dst[j], sizeof(dst), PSTR("%.03f,%.03f;"), systemVars.ainputs_conf.mmin[i], systemVars.ainputs_conf.mmax[i] );

		//xprintf_P( PSTR("DEBUG: ACKS = [%s]\r\n\0"), dst );
		// Apunto al comienzo para recorrer el buffer
		p = dst;
		// Mientras no sea NULL calculo el checksum deol buffer
		while (*p != '\0') {
			checksum += *p++;
		}

	}
	//xprintf_P( PSTR("DEBUG: cks = [0x%02x]\r\n\0"), checksum );
	return(checksum);

}
//------------------------------------------------------------------------------------
// FUNCIONES PRIVADAS
//------------------------------------------------------------------------------------
static void pv_ainputs_prender_12V_sensors(void)
{
	IO_set_SENS_12V_CTL();
}
//------------------------------------------------------------------------------------
static void pv_ainputs_apagar_12Vsensors(void)
{
	IO_clr_SENS_12V_CTL();
}
//------------------------------------------------------------------------------------
static uint16_t pv_ainputs_read_battery_raw(void)
{
	if ( spx_io_board != SPX_IO5CH ) {
		return(-1);
	}

	return( pv_ainputs_read_channel_raw(99));
}
//------------------------------------------------------------------------------------
static uint16_t pv_ainputs_read_channel_raw(uint8_t channel_id )
{
	// Como tenemos 2 arquitecturas de dataloggers, SPX_5CH y SPX_8CH,
	// los canales estan mapeados en INA con diferentes id.

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// Aqui convierto de io_channel a (ina_id, ina_channel )
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	// ina_id es el parametro que se pasa a la funcion INA_id2busaddr para
	// que me devuelva la direccion en el bus I2C del dispositivo.


uint8_t ina_reg = 0;
uint8_t ina_id = 0;
uint16_t an_raw_val = 0;
uint8_t MSB = 0;
uint8_t LSB = 0;
char res[3] = { '\0','\0', '\0' };
int8_t xBytes = 0;
//float vshunt;

	//xprintf_P( PSTR("in->ACH: ch=%d, ina=%d, reg=%d, MSB=0x%x, LSB=0x%x, ANV=(0x%x)%d, VSHUNT = %.02f(mV)\r\n\0") ,channel_id, ina_id, ina_reg, MSB, LSB, an_raw_val, an_raw_val, vshunt );

	switch(spx_io_board) {

	case SPX_IO5CH:	// Datalogger SPX_5CH
		switch ( channel_id ) {
		case 0:
			ina_id = INA_A; ina_reg = INA3221_CH1_SHV;
			break;
		case 1:
			ina_id = INA_A; ina_reg = INA3221_CH2_SHV;
			break;
		case 2:
			ina_id = INA_A; ina_reg = INA3221_CH3_SHV;
			break;
		case 3:
			ina_id = INA_B; ina_reg = INA3221_CH2_SHV;
			break;
		case 4:
			ina_id = INA_B; ina_reg = INA3221_CH3_SHV;
			break;
		case 99:
			ina_id = INA_B; ina_reg = INA3221_CH1_BUSV;
			break;	// Battery
		default:
			return(-1);
			break;
		}
		break;

	case SPX_IO8CH:	// Datalogger SPX_8CH
		switch ( channel_id ) {
		case 0:
			ina_id = INA_B; ina_reg = INA3221_CH1_SHV;
			break;
		case 1:
			ina_id = INA_B; ina_reg = INA3221_CH2_SHV;
			break;
		case 2:
			ina_id = INA_B; ina_reg = INA3221_CH3_SHV;
			break;
		case 3:
			ina_id = INA_A; ina_reg = INA3221_CH1_SHV;
			break;
		case 4:
			ina_id = INA_A; ina_reg = INA3221_CH2_SHV;
			break;
		case 5:
			ina_id = INA_A; ina_reg = INA3221_CH3_SHV;
			break;
		case 6:
			ina_id = INA_C; ina_reg = INA3221_CH1_SHV;
			break;
		case 7:
			ina_id = INA_C; ina_reg = INA3221_CH2_SHV;
			break;
		case 8:
			ina_id = INA_C; ina_reg = INA3221_CH3_SHV;
			break;
		default:
			return(-1);
			break;
		}
		break;

	default:
		return(-1);
		break;
	}

	// Leo el valor del INA.
//	xprintf_P(PSTR("DEBUG: INAID = %d\r\n\0"), ina_id );
	xBytes = INA_read( ina_id, ina_reg, res ,2 );

	if ( xBytes == -1 )
		xprintf_P(PSTR("ERROR I2C: pv_ainputs_read_channel_raw.\r\n\0"));

	an_raw_val = 0;
	MSB = res[0];
	LSB = res[1];
	an_raw_val = ( MSB << 8 ) + LSB;
	an_raw_val = an_raw_val >> 3;

	//vshunt = (float) an_raw_val * 40 / 1000;
	//xprintf_P( PSTR("out->ACH: ch=%d, ina=%d, reg=%d, MSB=0x%x, LSB=0x%x, ANV=(0x%x)%d, VSHUNT = %.02f(mV)\r\n\0") ,channel_id, ina_id, ina_reg, MSB, LSB, an_raw_val, an_raw_val, vshunt );

	return( an_raw_val );
}
//------------------------------------------------------------------------------------
static float pv_ainputs_read_channel ( uint8_t io_channel )
{
	// Lee un canal analogico y devuelve el valor convertido a la magnitud configurada.
	// Es publico porque se utiliza tanto desde el modo comando como desde el modulo de poleo de las entradas.
	// Hay que corregir la correspondencia entre el canal leido del INA y el canal fisico del datalogger
	// io_channel. Esto lo hacemos en AINPUTS_read_ina.

uint16_t an_raw_val = 0;
float an_mag_val = 0.0;
float I = 0.0;
float M = 0.0;
float P = 0.0;
uint16_t D = 0;

	// Leo el valor del INA.
	an_raw_val = pv_ainputs_read_channel_raw( io_channel );

	// Convierto el raw_value a la magnitud
	// Calculo la corriente medida en el canal
	I = (float)( an_raw_val) * 20 / ( systemVars.ainputs_conf.inaspan[io_channel] + 1);
	if ( systemVars.debug == DEBUG_DATA ) {
		xprintf_P( PSTR("DEBUG ANALOG READ CHANNEL: A%d I=%.03f\r\n\0"), io_channel, I );
	}

	// Convierto a la corriente equivalente por el ajuste de offset y span
	I = pv_analog_convert_ieqv (I, io_channel);
	if ( systemVars.debug == DEBUG_DATA ) {
		xprintf_P( PSTR("DEBUG ANALOG READ CHANNEL: A%d Ieqv=%.03f\r\n\0"), io_channel, I );
	}

	// Calculo la magnitud
	P = 0;
	D = systemVars.ainputs_conf.imax[io_channel] - systemVars.ainputs_conf.imin[io_channel];

	an_mag_val = 0.0;
	if ( D != 0 ) {
		// Pendiente
		P = (float) ( systemVars.ainputs_conf.mmax[io_channel]  -  systemVars.ainputs_conf.mmin[io_channel] ) / D;
		// Magnitud
		M = (float) (systemVars.ainputs_conf.mmin[io_channel] + ( I - systemVars.ainputs_conf.imin[io_channel] ) * P);

		// Al calcular la magnitud, al final le sumo el offset.
		an_mag_val = M + systemVars.ainputs_conf.offset[io_channel];
	} else {
		// Error: denominador = 0.
		an_mag_val = -999.0;
	}

	return(an_mag_val);

}
//------------------------------------------------------------------------------------
static void pv_ainputs_read_battery(float *battery)
{

	if ( spx_io_board != SPX_IO5CH ) {
		*battery = -1;
	}

	// Convierto el raw_value a la magnitud ( 8mV por count del A/D)
	*battery =  0.008 * pv_ainputs_read_battery_raw();

}
//------------------------------------------------------------------------------------
static float pv_analog_convert_ieqv( float i_real, uint8_t io_channel )
{
float Ieq = 0.0;

	Ieq = ( systemVars.ainputs_conf.imax[io_channel] - systemVars.ainputs_conf.imin[io_channel] );
	Ieq *= ( i_real - systemVars.ainputs_conf.ieq_min[io_channel] );
	Ieq /= ( systemVars.ainputs_conf.ieq_max[io_channel] - systemVars.ainputs_conf.ieq_min[io_channel] );
	Ieq += systemVars.ainputs_conf.imin[io_channel];
	return(Ieq);
}
//------------------------------------------------------------------------------------
