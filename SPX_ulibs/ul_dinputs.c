/*
 * spx_dinputs.c
 *
 *  Created on: 4 abr. 2019
 *      Author: pablo
 */

#include "spx.h"
#include "timers.h"

static uint16_t din[MAX_DINPUTS_CHANNELS];

StaticTimer_t dinputs_xTimerBuffers;
TimerHandle_t dinputs_xTimer;

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
uint8_t i;

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
	for (i=0; i < NRO_DINPUTS; i++) {
		if ( ! systemVars.dinputs_conf.modo_normal[i] ) {
			start_timer = true;
		}

	}
	if ( start_timer ) {
		xTimerStart( dinputs_xTimer, 0 );
		xprintf_P(PSTR("tkDada: dinputs timer started...\r\n\0"));
	} else {
		xprintf_P(PSTR("tkDada: dinputs timer stopped (modo normal)\r\n\0"));
	}

}
//------------------------------------------------------------------------------------
bool dinputs_config_channel( uint8_t channel,char *s_aname ,char *s_tmodo )
{

	// Configura los canales digitales. Es usada tanto desde el modo comando como desde el modo online por gprs.
	// config digital {0..N} dname {timer}

bool retS = false;

	xprintf_P( PSTR("DEBUG DIGITAL CONFIG: D%d,name=%s,modo=%s\r\n\0"), channel, s_aname, s_tmodo );


	if ( u_control_string(s_aname) == 0 ) {
		//xprintf_P( PSTR("DEBUG DIGITAL ERROR: D%d\r\n\0"), channel );
		return( false );
	}

	if ( s_aname == NULL ) {
		return(retS);
	}

	if ( ( channel >=  0) && ( channel < NRO_DINPUTS ) ) {
		snprintf_P( systemVars.dinputs_conf.name[channel], PARAMNAME_LENGTH, PSTR("%s\0"), s_aname );
		systemVars.dinputs_conf.modo_normal[channel] = true;
		if ( ( s_tmodo != NULL ) && ( !strcmp_P( strupr(s_tmodo), PSTR("TIMER\0")))) {
			systemVars.dinputs_conf.modo_normal[channel] = false;
		}

		// En caso que sea X, el valor es siempre NORMAL
		if ( strcmp ( systemVars.dinputs_conf.name[channel], "X" ) == 0 ) {
			systemVars.dinputs_conf.modo_normal[channel] = true;
		}

		retS = true;
	}

	return(retS);
}
//------------------------------------------------------------------------------------
void dinputs_config_defaults(void)
{
	// Realiza la configuracion por defecto de los canales digitales.

uint8_t i = 0;

	for ( i = 0; i < NRO_DINPUTS; i++ ) {
		snprintf_P( systemVars.dinputs_conf.name[i], PARAMNAME_LENGTH, PSTR("D%d\0"), i );
		systemVars.dinputs_conf.modo_normal[i] = true;
	}

}
//------------------------------------------------------------------------------------
static void pv_dinputs_TimerCallback( TimerHandle_t xTimer )
{
	// Funcion de callback que lee las entradas digitales para contar los
	// ticks que estan en 1.
	// Las entradas que son normales solo almacena su nivel.

	pv_dinputs_poll();

}
//------------------------------------------------------------------------------------
void dinputs_clear(void)
{
	// Inicializo a 0 la estructura que guarda los valores.

uint8_t i;

	for (i=0; i < MAX_DINPUTS_CHANNELS; i++)
		din[i] = 0;
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


uint8_t i;
uint8_t din_val;
uint8_t port = 0;
int8_t rdBytes = 0;
bool retS = false;

	switch (spx_io_board ) {

	case SPX_IO5CH:	// SPX_IO5
		// DIN0
		din_val = IO_read_PA0();
		if ( systemVars.dinputs_conf.modo_normal[0] == true ) {
			// modo normal
			din[0] = din_val;
		} else {
			// modo timer
			if ( din_val == 1)
				din[0]++;
		}

		// DIN1
		din_val = IO_read_PB7();
		if ( systemVars.dinputs_conf.modo_normal[1] == true ) {
			// modo normal
			din[1] = din_val;
		} else {
			// modo timer
			if ( din_val == 1)
				din[1]++;
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

		for (i=0; i<NRO_DINPUTS;i++) {
			din_val = ( port & ( 1 << i )) >> i;
			if ( systemVars.dinputs_conf.modo_normal[1] == true ) {
				din[i] = din_val;
			} else {
				// modo timer
				if ( din_val == 1)
					din[i]++;
			}
		}
		retS = true;
		break;
	}

	return(retS);
}
//------------------------------------------------------------------------------------
bool dinputs_read(uint16_t dst[] )
{
	// Actualiza las entradas que estan en modo normal y copia
	// igual a dinputs_copy.

uint8_t i;
uint8_t din_val;
uint8_t port = 0;
int8_t rdBytes = 0;
bool retS = false;

	switch (spx_io_board ) {

	case SPX_IO5CH:	// SPX_IO5

		// DIN0
		if ( systemVars.dinputs_conf.modo_normal[0] == true ) {
			// modo normal
			dst[0] = IO_read_PA0();
		}

		// DIN1
		if ( systemVars.dinputs_conf.modo_normal[1] == true ) {
			// modo normal
			dst[1] = IO_read_PB7();
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

		for (i=0; i<NRO_DINPUTS;i++) {
			din_val = ( port & ( 1 << i )) >> i;
			if ( systemVars.dinputs_conf.modo_normal[1] == true ) {
				dst[i] = din_val;
			}
		}
		retS = true;
		break;
	}

	return(retS);
}
//------------------------------------------------------------------------------------
void dinputs_print(file_descriptor_t fd, uint16_t src[] )
{
	// Imprime los canales configurados ( no X ) en un fd ( tty_gprs,tty_xbee,tty_term) en
	// forma formateada.
	// Los lee de una estructura array pasada como src

uint8_t i = 0;

	for ( i = 0; i < NRO_DINPUTS; i++) {
		if ( ! strcmp ( systemVars.dinputs_conf.name[i], "X" ) )
			continue;

		xCom_printf_P(fd, PSTR(",%s=%d"), systemVars.dinputs_conf.name[i], src[i] );
	}

}
//------------------------------------------------------------------------------------
uint8_t dinputs_checksum(void)
{

uint16_t i;
uint8_t checksum = 0;
char dst[32];
char *p;

	//	char name[MAX_DINPUTS_CHANNELS][PARAMNAME_LENGTH];
	//	bool modo_normal[MAX_DINPUTS_CHANNELS];

	// D0:D0,1;D1:D1,1;

	// calculate own checksum
	for(i=0;i<NRO_DINPUTS;i++) {
		// Vacio el buffer temoral
		memset(dst,'\0', sizeof(dst));
		// Copio sobe el buffer una vista ascii ( imprimible ) de c/registro.
		snprintf_P(dst, sizeof(dst), PSTR("D%d:%s,%d;"), i, systemVars.dinputs_conf.name[i],systemVars.dinputs_conf.modo_normal[i] );
		//xprintf_P( PSTR("DEBUG: DCKS = [%s]\r\n\0"), dst );
		// Apunto al comienzo para recorrer el buffer
		p = dst;
		// Mientras no sea NULL calculo el checksum deol buffer
		while (*p != '\0') {
			checksum += *p++;
		}
		//xprintf_P( PSTR("DEBUG: cks = [0x%02x]\r\n\0"), checksum );
	}

	return(checksum);

}
//------------------------------------------------------------------------------------


