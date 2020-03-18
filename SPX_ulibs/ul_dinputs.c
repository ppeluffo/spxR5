/*
 * spx_dinputs.c
 *
 *  Created on: 4 abr. 2019
 *      Author: pablo
 *
 *  Funcionamiento:
 *  Los canales digitales pueden funcionar midiendo el nivel o midiendo el tiempo que una entrada
 *  está en un nivel dado.
 *  Para esto cuando se configura se elige el modo NORMAL o TIMER.
 *  Cuando se reinicia el equipo, si alguno de los canales esta en modo TIMER se activa un
 *  timer con frecuencia 10hz cuya funcion de callback lee las entradas digitales (todas)
 *  por medio de la funcion pv_dinputs_poll().
 *  Esta funcion si una entrada está en 1 lógico, incrementa el contador correspondiente.
 *
 *
 *
 */

#include "spx.h"
#include "timers.h"

static uint16_t pv_din[MAX_DINPUTS_CHANNELS];

StaticTimer_t dinputs_xTimerBuffers;
TimerHandle_t dinputs_xTimer;

uint16_t ticks;

static bool pv_dinputs_poll(void);
static void pv_dinputs_TimerCallback( TimerHandle_t xTimer );

//------------------------------------------------------------------------------------
void dinputs_setup(void)
{
	// Configura el timer que va a hacer la llamada periodica
	// a la funcion de callback que atienda las inputs.
	// Se deben crear antes que las tarea y que arranque el scheduler

	dinputs_xTimer = xTimerCreateStatic ("TDIN",
			pdMS_TO_TICKS( 100 ),
			pdTRUE,
			( void * ) 0,
			pv_dinputs_TimerCallback,
			&dinputs_xTimerBuffers
			);

}
//------------------------------------------------------------------------------------
void dinputs_init(void)
{
	// En el caso del SPX_8CH se deberia inicializar el port de salidas del MCP
	// pero esto se hace en la funcion MCP_init(). Esta tambien inicializa el port
	// de entradas digitales.

bool start_timer = false;
uint8_t channel;

	switch (spx_io_board ) {
	case SPX_IO5CH:
		IO_config_PA0();	// D0
		IO_config_PB7();	// D1
		break;
	case SPX_IO8CH:
		MCP_init();
		break;
	}

	// Si alguna de las entradas esta en modo timer debemos arrancar el timer.
	// Si no solo esperamos para usar el tickless.
	for (channel=0; channel < NRO_DINPUTS; channel++) {
		if ( systemVars.dinputs_conf.wrk_modo[channel] == DIN_TIMER ) {
			start_timer = true;
		}
	}

	if ( start_timer ) {
		ticks = 0;
		xTimerStart( dinputs_xTimer, 0 );
		xprintf_P(PSTR("tkInputs: dinputs timer started...\r\n\0"));
	} else {
		xprintf_P(PSTR("tkInputs: dinputs timer stopped (modo normal)\r\n\0"));
	}

}
//------------------------------------------------------------------------------------
bool dinputs_config_channel( uint8_t channel,char *s_aname ,char *s_tmodo )
{

	// Configura los canales digitales. Es usada tanto desde el modo comando como desde el modo online por gprs.
	// config digital {0..N} dname {timer}

bool retS = false;

	//xprintf_P( PSTR("DEBUG DIGITAL CONFIG: D%d,name=%s,modo=%s\r\n\0"), channel, s_aname, s_tmodo );

// Aplicacion ALARMAS
/*
#ifdef APLICACION_PLANTAPOT
	return(true);
#endif
*/
	if ( u_control_string(s_aname) == 0 ) {
		//xprintf_P( PSTR("DEBUG DIGITAL ERROR: D%d\r\n\0"), channel );
		return( false );
	}

	if ( s_aname == NULL ) {
		return(retS);
	}

	if ( ( channel >=  0) && ( channel < NRO_DINPUTS ) ) {
		snprintf_P( systemVars.dinputs_conf.name[channel], PARAMNAME_LENGTH, PSTR("%s\0"), s_aname );

		systemVars.dinputs_conf.wrk_modo[channel] = DIN_NORMAL;
		if ( s_tmodo != NULL ) {
			if  ( strcmp_P( strupr(s_tmodo), PSTR("NORMAL\0")) == 0 ) {
				systemVars.dinputs_conf.wrk_modo[channel] = DIN_NORMAL;
			}
			if  ( strcmp_P( strupr(s_tmodo), PSTR("TIMER\0")) == 0 ) {
				systemVars.dinputs_conf.wrk_modo[channel] = DIN_TIMER;
			}
		}

		// En caso que sea X, el valor es siempre NORMAL
		if ( strcmp ( systemVars.dinputs_conf.name[channel], "X" ) == 0 ) {
			systemVars.dinputs_conf.wrk_modo[channel] = DIN_NORMAL;
		}

		retS = true;
	}

	return(retS);
}
//------------------------------------------------------------------------------------
void dinputs_config_defaults(void)
{
	// Realiza la configuracion por defecto de los canales digitales.

uint8_t channel = 0;

	for ( channel = 0; channel < NRO_DINPUTS; channel++ ) {
		snprintf_P( systemVars.dinputs_conf.name[channel], PARAMNAME_LENGTH, PSTR("DIN%d\0"), channel );
		systemVars.dinputs_conf.wrk_modo[channel] = DIN_NORMAL;
	}

	// Aplicacion ALARMAS
/*
#ifdef APLICACION_PLANTAPOT
		snprintf_P( systemVars.dinputs_conf.name[6], PARAMNAME_LENGTH, PSTR("MKEY\0") );
		snprintf_P( systemVars.dinputs_conf.name[7], PARAMNAME_LENGTH, PSTR("BTN\0") );
#endif
*/

}
//------------------------------------------------------------------------------------
static void pv_dinputs_TimerCallback( TimerHandle_t xTimer )
{
	// Funcion de callback que lee las entradas digitales para contar los
	// ticks que estan en 1.
	// Las entradas que son normales solo almacena su nivel.

	ticks++;
	pv_dinputs_poll();

}
//------------------------------------------------------------------------------------
void dinputs_clear(void)
{
	// Inicializo a 0 la estructura que guarda los valores.

uint8_t channel;

	ticks = 0;
	for (channel=0; channel < MAX_DINPUTS_CHANNELS; channel++)
		pv_din[channel] = 0;
}
//------------------------------------------------------------------------------------
static bool pv_dinputs_poll(void)
{
	// Leo las entradas digitales y actualizo la estructura.
	// Si las entradas son timers, si estan en 1 incremento
	// Si son normales, pongo su valor (0 o 1)
	// En las placas con OPTO, el estado de reposo de las señales ( visto desde el micro )
	// es 0.
	// Al activarlas contra GND se marca un '1' en el micro.


uint8_t channel;
uint8_t din_val;
uint8_t port = 0;
int8_t rdBytes = 0;
bool retS = false;

	switch (spx_io_board ) {

	case SPX_IO5CH:	// SPX_IO5
		// DIN0
		din_val = IO_read_PA0();
		if ( systemVars.dinputs_conf.wrk_modo[0] == DIN_NORMAL ) {
			// modo normal. Solo leo el valor de la entrada (0,1)
			pv_din[0] = din_val;
		} else {
			// modo timer
			if ( din_val == 1)
				pv_din[0]++;
		}

		// DIN1
		din_val = IO_read_PB7();
		if ( systemVars.dinputs_conf.wrk_modo[1] == DIN_NORMAL ) {
			// modo normal
			pv_din[1] = din_val;
		} else {
			// modo timer
			if ( din_val == 1)
				pv_din[1]++;
		}
		retS = true;
		break;

	case SPX_IO8CH:
		rdBytes = MCP_read( MCP_GPIOA, (char *)&port, 1 );
		if ( rdBytes == -1 ) {
			xprintf_P(PSTR("ERROR: IO_DIN_read_pin\r\n\0"));
			retS = false;
			break;
		}

		for (channel=0; channel < NRO_DINPUTS; channel++) {
			din_val = ( port & ( 1 << channel )) >> channel;
			if ( systemVars.dinputs_conf.wrk_modo[channel] == DIN_NORMAL ) {
				pv_din[channel] = din_val;
			} else {
				// modo timer
				if ( din_val == 1)
					pv_din[channel]++;
			}
		}
		retS = true;
		break;
	}

	return(retS);
}
//------------------------------------------------------------------------------------
bool dinputs_read( uint16_t dst[] )
{
	// Leo las entradas que estan configuradas en modo NORMAL y copio de pv_din
	// los valores de aquellas que estan en modo TIMER

uint8_t channel;
uint8_t din_val;
uint8_t port = 0;
int8_t rdBytes = 0;
bool retS = false;

	switch (spx_io_board ) {

	case SPX_IO5CH:	// SPX_IO5

		// DIN0
		if ( systemVars.dinputs_conf.wrk_modo[0] == DIN_NORMAL ) {
			// modo normal
			dst[0] = IO_read_PA0();
		} else {
			dst[0] = pv_din[0];
		}

		// DIN1
		if ( systemVars.dinputs_conf.wrk_modo[1] == DIN_NORMAL ) {
			// modo normal
			dst[1] = IO_read_PB7();
		} else {
			dst[1] = pv_din[1];
		}

		retS = true;
		break;

	case SPX_IO8CH:
		rdBytes = MCP_read( MCP_GPIOA, (char *)&port, 1 );
		if ( rdBytes == -1 ) {
			xprintf_P(PSTR("ERROR: IO_DIN_read_pin\r\n\0"));
			retS = false;
			break;
		}

		for (channel=0; channel < NRO_DINPUTS; channel++) {
			din_val = ( port & ( 1 << channel )) >> channel;
			if ( systemVars.dinputs_conf.wrk_modo[channel] == DIN_NORMAL ) {
				dst[channel] = din_val;
			} else {
				dst[channel] = pv_din[channel];
			}
		}

		retS = true;
		break;
	}

	//xprintf_P(PSTR("DEBUG DIN: tics=%d\r\n\0"), ticks);
	return(retS);
}
//------------------------------------------------------------------------------------
void dinputs_print(file_descriptor_t fd, uint16_t src[] )
{
	// Imprime los canales configurados ( no X ) en un fd ( tty_gprs,tty_xbee,tty_term) en
	// forma formateada.
	// Los lee de una estructura array pasada como src

uint8_t channel = 0;
float time_up;

	for ( channel = 0; channel < NRO_DINPUTS; channel++) {
		if ( ! strcmp ( systemVars.dinputs_conf.name[channel], "X" ) )
			continue;

		if ( systemVars.dinputs_conf.wrk_modo[channel] == DIN_NORMAL ) {
			xfprintf_P(fd, PSTR("%s:%d;"), systemVars.dinputs_conf.name[channel], src[channel] );
		} else {
			// Ajusto los ticks.
			// El maximo nro. de ticks esta dado por la variable ticks.
			// El time-up lo expreso en segundos. Como el tick es de 0.1s, divido por 10.
			time_up = src[channel] / 10;
			xfprintf_P(fd, PSTR("%s:%.01f;"), systemVars.dinputs_conf.name[channel], time_up );
		}
	}

}
//------------------------------------------------------------------------------------
uint8_t dinputs_checksum(void)
{

uint8_t channel;
uint8_t checksum = 0;
char dst[32];
char *p;
uint8_t j = 0;

	//	char name[MAX_DINPUTS_CHANNELS][PARAMNAME_LENGTH];
	//	bool modo_normal[MAX_DINPUTS_CHANNELS];

	// D0:D0,1;D1:D1,1;

	// calculate own checksum
	for(channel=0;channel<NRO_DINPUTS;channel++) {
		// Vacio el buffer temoral
		memset(dst,'\0', sizeof(dst));
		j = 0;
		// Copio sobe el buffer una vista ascii ( imprimible ) de c/registro.
		if ( systemVars.dinputs_conf.wrk_modo[channel] == DIN_NORMAL ) {
			j += snprintf_P(&dst[j], sizeof(dst), PSTR("D%d:%s,NORMAL;"), channel , systemVars.dinputs_conf.name[channel] );
		} else {
			j += snprintf_P(&dst[j], sizeof(dst), PSTR("D%d:%s,TIMER;"), channel, systemVars.dinputs_conf.name[channel] );
		}
		// Apunto al comienzo para recorrer el buffer
		p = dst;
		// Mientras no sea NULL calculo el checksum deol buffer
		while (*p != '\0') {
			checksum += *p++;
		}

	//	xprintf_P( PSTR("DEBUG: DCKS = [%s]\r\n\0"), dst );
	//	xprintf_P( PSTR("DEBUG: cks = [0x%02x]\r\n\0"), checksum );

	}

	return(checksum);

}
//------------------------------------------------------------------------------------
bool dinputs_service_read(void)
{
	/*
	 * Leo las entradas digitales y las imprimo
	 * Se usa para el modo test.
	 */

int8_t channel;
uint8_t din_val;
uint8_t port = 0;
int8_t rdBytes = 0;
uint8_t dst[IO8_DINPUTS_CHANNELS];
	switch (spx_io_board ) {

	case SPX_IO5CH:	// SPX_IO5

		// DIN0
		dst[0] = IO_read_PA0();
		// DIN1
		dst[1] = IO_read_PB7();
		xprintf_P(PSTR("Dinputs: din_1=%d,din_0=%d"), dst[1], dst[0]);
		break;

	case SPX_IO8CH:
		rdBytes = MCP_read( MCP_GPIOA, (char *)&port, 1 );
		if ( rdBytes == -1 ) {
			xprintf_P(PSTR("ERROR: IO_DIN_read_pin\r\n\0"));
			return(false);
			break;
		}

		for (channel=0; channel < NRO_DINPUTS; channel++) {
			din_val = ( port & ( 1 << channel )) >> channel;
			dst[channel] = din_val;
		}

		xprintf_P(PSTR("Dinputs: msb[ "));
		for (channel=(NRO_DINPUTS - 1); channel>=0 ; channel--) {
			xprintf_P(PSTR("%d "),dst[channel]);
		}
		xprintf_P(PSTR("]\r\n"));
		break;
	}

	return(false);
}

