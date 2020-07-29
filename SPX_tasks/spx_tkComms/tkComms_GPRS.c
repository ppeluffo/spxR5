/*
 * tkComms_GPRS.c
 *
 *  Created on: 5 mar. 2020
 *      Author: pablo
 */

#include "tkComms.h"


typedef enum { prender_RXRDY = 0, prender_HW, prender_SW, prender_CPAS, prender_CFUN, prender_EXIT } t_states_fsm_prender_gprs;

#define MAX_SW_TRYES_PRENDER	3
#define MAX_HW_TRYES_PRENDER	3
#define MAX_TRYES_CPAS			4
#define MAX_TRYES_CFUN			3
#define MAX_TRYES_CPIN			3
#define MAX_TRYES_CREG			3
#define MAX_TRYES_SOCKCONT		3
#define MAX_TRYES_SOCKSETPN		3
#define MAX_TRYES_NETCLOSE		1
#define MAX_TRYES_NETOPEN		1
#define MAX_TRYES_CGSN			3
#define MAX_TRYES_CCID			3
#define MAX_TRYES_CPSI			3
#define MAX_TRYES_CGATT			3
#define MAX_TRYES_SWITCHCMDMODE	3


#define TIMEOUT_MODEM_BOOT	30
#define TIMEOUT_CFUN		30
#define TIMEOUT_CPAS		15
#define TIMEOUT_CGMI		10
#define TIMEOUT_CGMM		10
#define TIMEOUT_CGMR		10
#define TIMEOUT_IFC			10
#define TIMEOUT_IMEI		30
#define TIMEOUT_CCID		10
#define TIMEOUT_CPIN		30
#define TIMEOUT_CREG		30
#define TIMEOUT_CIPMODE		10
#define TIMEOUT_CMGF		20
#define TIMEOUT_DCDMODE		10
#define TIMEOUT_CSQ			10
#define TIMEOUT_SOCKCONT	15
#define TIMEOUT_SOCKSETPN	15
#define TIMEOUT_PDP			15
#define TIMEOUT_NETCLOSE	120
#define TIMEOUT_NETOPEN		120
#define TIMEOUT_READIP		10
#define TIMEOUT_LINKCLOSE	120
#define TIMEOUT_LINKOPEN	120
#define TIMEOUT_CBC			10
#define TIMEOUT_CGSN		30
#define TIMEOUT_CCID		10
#define TIMEOUT_D1			5
#define TIMEOUT_CSUART		5
#define TIMEOUT_C1			5
#define TIMEOUT_CGATT		60
#define TIMEOUT_CPSI		15
#define TIMEOUT_SWITCHCMDMODE	30
#define TIMEOUT_CIPTIMEOUT	10


void gprs_START_SIMULATOR(void);
bool gprs_PBDONE( bool f_debug );
bool gprs_CFUN( bool f_debug );
bool gprs_CPAS( bool f_debug );
void gprs_CBC( bool f_debug );
void gprs_CPOF( void );
void gprs_AT( void );
bool gprs_ATI( bool f_debug );
void gprs_IFC( bool f_debug );
bool pv_get_token( char *p, char *buff, uint8_t size);
void gprs_CIPMODE(bool f_debug);
void gprs_CMGF( bool f_debug );
void gprs_D1(bool f_debug);
void gprs_CSUART(bool f_debug);
void gprs_C1(bool f_debug);
bool gprs_CCID(bool f_debug);
bool gprs_CPIN(  bool f_debug, char *pin );
bool gprs_CREG( bool f_debug );
bool gprs_CPSI( bool f_debug );
bool gprs_CGATT(bool f_debug);
bool gprs_CGDCONT( bool f_debug, char *apn);
uint8_t gprs_CSQ( bool f_debug );
void gprs_CIPTIMEOUT(bool f_debug);

void gprs_CGMI( bool f_debug );
void gprs_CGMM( bool f_debug );
void gprs_CGMR( bool f_debug );
void gprs_CFGRI (bool f_debug);
bool gprs_CGSN(bool f_debug);
bool gprs_CSOCKSETPN( bool f_debug );

//------------------------------------------------------------------------------------

struct {
	char buffer[GPRS_RXBUFFER_LEN];
	uint16_t ptr;
} gprsRxBuffer;

#define IMEIBUFFSIZE	24
#define CCIDBUFFSIZE	24

struct {
	char buff_gprs_imei[IMEIBUFFSIZE];
	char buff_gprs_ccid[CCIDBUFFSIZE];
} gprs_status;

//------------------------------------------------------------------------------------
void gprs_atcmd_preamble(void)
{
	// Espera antes de c/comando. ( ver recomendaciones de TELIT )
	vTaskDelay( (portTickType)( 50 / portTICK_RATE_MS ) );
}
//------------------------------------------------------------------------------------
void gprs_init(void)
{
	// GPRS
	IO_config_GPRS_SW();
	IO_config_GPRS_PWR();
	IO_config_GPRS_RTS();
	IO_config_GPRS_CTS();
	IO_config_GPRS_DCD();
	IO_config_GPRS_RI();
	IO_config_GPRS_RX();
	IO_config_GPRS_TX();
	IO_config_GPRS_DTR();

	IO_set_GPRS_DTR();
	IO_set_GPRS_RTS();

	memset(gprs_status.buff_gprs_ccid, '\0', IMEIBUFFSIZE );
	memset(gprs_status.buff_gprs_ccid, '\0', IMEIBUFFSIZE );
}
//------------------------------------------------------------------------------------
void gprs_hw_pwr_on(uint8_t delay_factor)
{
	/*
	 * Prendo la fuente del modem y espero que se estabilize la fuente.
	 */

	IO_clr_GPRS_SW();	// GPRS=0V, PWR_ON pullup 1.8V )
	IO_set_GPRS_PWR();	// Prendo la fuente ( alimento al modem ) HW

	vTaskDelay( (portTickType)( ( 2000 + 2000 * delay_factor) / portTICK_RATE_MS ) );

}
//------------------------------------------------------------------------------------
void gprs_sw_pwr(void)
{
	/*
	 * Genera un pulso en la linea PWR_SW. Como tiene un FET la senal se invierte.
	 * En reposo debe la linea estar en 0 para que el fet flote y por un pull-up del modem
	 * la entrada PWR_SW esta en 1.
	 * El PWR_ON se pulsa a 0 saturando el fet.
	 */
	IO_set_GPRS_SW();	// GPRS_SW = 3V, PWR_ON = 0V.
	vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
	IO_clr_GPRS_SW();	// GPRS_SW = 0V, PWR_ON = pullup, 1.8V

}
//------------------------------------------------------------------------------------
void gprs_rxBuffer_fill(char c)
{
	/*
	 * Guarda el dato en el buffer LINEAL de operacion del GPRS
	 * Si hay lugar meto el dato.
	 */

	if ( gprsRxBuffer.ptr < GPRS_RXBUFFER_LEN )
		gprsRxBuffer.buffer[ gprsRxBuffer.ptr++ ] = c;
}
//------------------------------------------------------------------------------------
void gprs_flush_RX_buffer(void)
{

	frtos_ioctl( fdGPRS,ioctl_UART_CLEAR_RX_BUFFER, NULL);
	memset( gprsRxBuffer.buffer, '\0', GPRS_RXBUFFER_LEN);
	gprsRxBuffer.ptr = 0;
}
//------------------------------------------------------------------------------------
void gprs_flush_TX_buffer(void)
{
	frtos_ioctl( fdGPRS,ioctl_UART_CLEAR_TX_BUFFER, NULL);
}
//------------------------------------------------------------------------------------
void gprs_print_RX_buffer( bool f_debug )
{

	if ( f_debug ) {
		// Imprimo todo el buffer local de RX. Sale por \0.
		xprintf_P( PSTR ("GPRS: rxbuff>\r\n\0"));

		// Uso esta funcion para imprimir un buffer largo, mayor al que utiliza xprintf_P. !!!
		xnprint( gprsRxBuffer.buffer, GPRS_RXBUFFER_LEN );

		xprintf_P( PSTR ("\r\n[%d]\r\n\0"), gprsRxBuffer.ptr );
	}
}
//------------------------------------------------------------------------------------
bool gprs_check_response( const char *rsp )
{
	// Modifico para solo compara en mayusculas
bool retS = false;

	if ( strstr( strupr(gprsRxBuffer.buffer), strupr(rsp) ) != NULL ) {
		retS = true;
	}
	return(retS);
}
//------------------------------------------------------------------------------------
bool gprs_check_response_with_to( const char *rsp, uint8_t timeout )
{
	// Espera una respuesta durante un tiempo dado.
	// Hay que tener cuidado que no expire el watchdog por eso lo kickeo aqui. !!!!

bool retS = false;

	while ( timeout > 0 ) {
		timeout--;
		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );

		// Veo si tengo la respuesta correcta.
		if ( strstr( gprsRxBuffer.buffer, rsp) != NULL ) {
			retS = true;
			break;
		}
	}

	return(retS);
}
//------------------------------------------------------------------------------------
void gprs_apagar(void)
{
	/*
	 * Apaga el dispositivo quitando la energia del mismo
	 *
	 */

	gprs_CPOF();

	IO_clr_GPRS_SW();	// Es un FET que lo dejo cortado
	vTaskDelay( (portTickType)( 100 / portTICK_RATE_MS ) );

	IO_clr_GPRS_PWR();	// Apago la fuente.
	vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );

}
//------------------------------------------------------------------------------------
void gprs_CPOF( void )
{
	// Apaga el modem en modo 'soft'

	xprintf_P( PSTR("COMMS: gprs CPOF\r\n"));
	gprs_flush_RX_buffer();
	gprs_flush_TX_buffer();
	gprs_atcmd_preamble();
	xfprintf_P( fdGPRS,PSTR("AT+CPOF\r\0"));
	vTaskDelay( (portTickType)( 5000 / portTICK_RATE_MS ) );
	xprintf_P( PSTR("COMMS: gprs CPOF OK.\r\n\0"));
	return;
}
//------------------------------------------------------------------------------------
bool gprs_prender(bool f_debug )
{
	/*
	 * Prende el modem.
	 * Implementa la FSM descripta en el archivo FSM_gprs_prender.
	 *
	 * EL stado de actividad del teléfono se obtiene con + CPAS que indica el general actual de actividad del ME.
	 * El comando + CFUN se usa para configurar el ME a diferentes potencias y estados de consumo.
	 * El comando + CPIN se usa para ingresar las contraseñas ME que se necesitan
	 */

uint8_t state;
uint8_t sw_tryes;
uint8_t hw_tryes;
bool retS = false;

	state = prender_RXRDY;

	xprintf_PD( f_debug, PSTR("COMMS: gprs Modem prendiendo...\r\n\0"));

	while(1) {

		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );

		switch (state ) {
		case prender_RXRDY:
			// Avisa a la tarea de rx que se despierte.
#ifdef BETA_TEST
			xprintf_PD( f_debug, PSTR("COMMS: fsm_prender pwr_on RXDDY.\r\n\0"));
#endif
			// Aviso a la tarea de RX que se despierte ( para leer las respuestas del AT ) !!!
			while ( xTaskNotify( xHandle_tkCommsRX, SGN_WAKEUP , eSetBits ) != pdPASS ) {
				vTaskDelay( ( TickType_t)( 100 / portTICK_RATE_MS ) );
			}
			hw_tryes = 0;
			state = prender_HW;
			break;

		case prender_HW:
#ifdef BETA_TEST
			xprintf_PD( f_debug, PSTR("COMMS: fsm_prender pwr_on HW(%d).\r\n\0"),hw_tryes);
#endif
			if ( hw_tryes >= MAX_HW_TRYES_PRENDER ) {
				state = prender_EXIT;
				retS = false;
			} else {
				hw_tryes++;
				sw_tryes = 0;
				// Prendo la fuente del modem (HW)
				gprs_hw_pwr_on(hw_tryes);
				state = prender_SW;
			}
			break;

		case prender_SW:
#ifdef BETA_TEST
			xprintf_PD( f_debug, PSTR("COMMS: fsm_prender pwr_on SW(%d).\r\n\0"),sw_tryes);
#endif
			if ( sw_tryes >= MAX_SW_TRYES_PRENDER ) {
				// Apago el HW y espero
				gprs_apagar();
				vTaskDelay( (portTickType)( 10000 / portTICK_RATE_MS ) );
				state = prender_HW;
			} else {
				// Genero un pulso en el pin SW para prenderlo logicamente
				sw_tryes++;
				gprs_sw_pwr();

#ifdef BETA_TEST
				gprs_START_SIMULATOR();
#endif

				if ( gprs_PBDONE(f_debug)) {
					// Paso a CFUN solo si respondio con PB DONE
					// Si no reintento en este estado
					// Espero un poco mas para asegurarme que todo esta bien.
					vTaskDelay( (portTickType)( 5000 / portTICK_RATE_MS ) );
					state = prender_CPAS;
				}
			}
			break;

		case prender_CPAS:
#ifdef BETA_TEST
			xprintf_PD( f_debug, PSTR("COMMS: fsm_prender CPAS.\r\n\0"));
#endif
			if ( gprs_CPAS(f_debug ) ) {
				state = prender_CFUN;
				retS = true;
			} else {
				hw_tryes++;
				gprs_apagar();
				vTaskDelay( (portTickType)( 10000 / portTICK_RATE_MS ) );
				state = prender_HW;
			}
			break;

		case prender_CFUN:
#ifdef BETA_TEST
			xprintf_PD( f_debug, PSTR("COMMS: fsm_prender CFUN.\r\n\0"));
#endif
			if ( gprs_CFUN(f_debug ) ) {
				state = prender_EXIT;
			} else {
				hw_tryes++;
				gprs_apagar();
				vTaskDelay( (portTickType)( 10000 / portTICK_RATE_MS ) );
				state = prender_HW;
			}
			break;

		case prender_EXIT:
			if ( retS ) {
				xprintf_PD( f_debug, PSTR("COMMS: gprs Modem on.\r\n\0"));
			} else {
				xprintf_PD( f_debug, PSTR("COMMS: ERROR gprs Modem NO prendio !!!.\r\n\0"));
			}
			goto EXIT;
			break;
		}
	}

EXIT:

	return(retS);
}
//------------------------------------------------------------------------------------
void gprs_START_SIMULATOR(void)
{

	gprs_flush_RX_buffer();
	gprs_flush_TX_buffer();
	xprintf_P( PSTR("GPRS: gprs START PWRSIM\r\n\0"));
	xfprintf_P( fdGPRS , PSTR("START_PWRSIM\r\n\0"));
}
//------------------------------------------------------------------------------------
void gprs_AT( void )
{
	// En caso que algun comando no responda, envio un solo AT y veo que pasa.
	// Esto me sirve para ver si el modem esta respondiendo o no.

	xprintf_P( PSTR("GPRS: gprs AT TESTING START >>>>>>>>>>\r\n\0"));
	gprs_atcmd_preamble();
	xfprintf_P( fdGPRS , PSTR("AT\r\0"));
	vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
	gprs_print_RX_buffer(true);
	xprintf_P( PSTR("GPRS: gprs AT TESTING END <<<<<<<<<<<\r\n\0"));

}
//------------------------------------------------------------------------------------
bool gprs_PBDONE( bool f_debug )
{
	/*
	 * Espero por la linea PB DONE que indica que el modem se
	 * inicializo y termino el proceso de booteo.
	 */

uint8_t timeout;
bool retS = false;

	gprs_flush_RX_buffer();
	timeout = 0;
	while ( timeout++ < TIMEOUT_MODEM_BOOT ) {
		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
		if ( gprs_check_response("PB DONE\0") ) {
			retS = true;
			break;
		}
	}

	gprs_print_RX_buffer( f_debug);
	if ( retS ) {
		xprintf_PD( f_debug, PSTR("COMMS: gprs Modem boot en %d s.\r\n\0"), timeout );
	} else {
		gprs_AT();
		xprintf_PD( f_debug, PSTR("COMMS: ERROR gprs Modem boot failed !!!\r\n\0") );
	}
	return (retS);

}
 //------------------------------------------------------------------------------------
bool gprs_CFUN( bool f_debug )
{
	/*
	 * Doy el comando AT+CFUN? hasta 3 veces con timout de 10s esperando
	 * una respuesta OK/ERROR/Timeout
	 * La respuesta 1 indica: 1 – full functionality, online mode
	 * De acuerdo a QUECTEL Maximum Response Time 15s, determined by the network.
	 */

uint8_t tryes;
int8_t timeout;
bool retS = false;
t_responses cmd_rsp = rsp_NONE;

	xprintf_PD( f_debug,  PSTR("COMMS: gprs CFUN\r\n"));
	tryes = 0;
	while ( tryes++ < MAX_TRYES_CFUN ) {

		// Envio el comando
		gprs_flush_RX_buffer();
		gprs_flush_TX_buffer();
		xprintf_PD( f_debug, PSTR("GPRS: gprs send CFUN (%d)\r\n\0"),tryes );
		gprs_atcmd_preamble();
		xfprintf_P( fdGPRS , PSTR("AT+CFUN?\r\0"));

		timeout = 0;
		while ( timeout++ < TIMEOUT_CFUN ) {
			vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );

			if ( gprs_check_response("ERROR") ) {
				// Salgo y reintento el comando.
				cmd_rsp = rsp_ERROR;
				break;
			}

			if ( gprs_check_response("OK") ) {
				cmd_rsp = rsp_OK;
				if ( gprs_check_response("+CFUN: 1") ) {
					// Respuesta correcta. Salgo.
					retS = true;
					goto EXIT;
				}
				// OK pero no correcto: Reintento
				break;
			}
			// Sin respuesta aun. Espero
		}

		// Sali por timeout:
		gprs_print_RX_buffer( f_debug);
		gprs_AT();
	}

EXIT:

	gprs_print_RX_buffer( f_debug);
	switch(cmd_rsp) {
	case rsp_OK:
		xprintf_PD( f_debug,  PSTR("COMMS: gprs CFUN OK en [%d] secs\r\n\0"), (( 10 * (tryes-1) ) + timeout) );
		retS = true;
		break;
	case rsp_ERROR:
		xprintf_PD( f_debug,  PSTR("COMMS: ERROR gprs CFUN FAIL !!.\r\n\0"));
		retS = false;
		break;
	default:
		// Timeout
		gprs_AT();
		xprintf_PD( f_debug,  PSTR("COMMS: ERROR(TO) gprs CFUN FAIL !!.\r\n\0"));
		retS = false;
	}

	return(retS);

}
//------------------------------------------------------------------------------------
bool gprs_CPAS( bool f_debug )
{
	/*
	 * Doy el comando AT+CPAS hasta 3 veces con timout de 10s esperando
	 * una respuesta OK/ERROR/Timeout
	 * Nos devuelve el status de actividad del modem
	 *
	 */

uint8_t tryes;
int8_t timeout;
bool retS = false;
t_responses cmd_rsp = rsp_NONE;

	xprintf_PD( f_debug,  PSTR("COMMS: gprs CPAS\r\n"));
	tryes = 0;
	while ( tryes++ < MAX_TRYES_CPAS ) {

		// Envio el comando
		gprs_flush_RX_buffer();
		gprs_flush_TX_buffer();
		xprintf_PD( f_debug, PSTR("GPRS: gprs send CPAS (%d)\r\n\0"), tryes );
		gprs_atcmd_preamble();
		xfprintf_P( fdGPRS , PSTR("AT+CPAS\r\0"));

		timeout = 0;
		while ( timeout++ < TIMEOUT_CPAS ) {
			vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );

			if ( gprs_check_response("ERROR") ) {
				cmd_rsp = rsp_ERROR;
				// Salgo y reintento el comando.
				break;
			}

			if ( gprs_check_response("OK") ) {
				cmd_rsp = rsp_OK;
				if ( gprs_check_response("CPAS: 0") ) {
					// Respuesta correcta. Salgo.
					retS = true;
					goto EXIT;
				}
				// OK pero no correcto: Reintento
				break;
			}
			// Sin respuesta aun. Espero
		}

		// Sali por timeout:
		gprs_print_RX_buffer( f_debug);
		gprs_AT();
	}

EXIT:

	gprs_print_RX_buffer( f_debug);
	switch(cmd_rsp) {
	case rsp_OK:
		xprintf_PD( f_debug,  PSTR("COMMS: gprs CPAS OK en [%d] secs\r\n\0"), (( 10 * (tryes-1) ) + timeout) );
		retS = true;
		break;
	case rsp_ERROR:
		xprintf_PD( f_debug,  PSTR("COMMS: ERROR gprs CPAS FAIL !!.\r\n\0"));
		retS = false;
		break;
	default:
		// Timeout
		gprs_AT();
		xprintf_PD( f_debug,  PSTR("COMMS: ERROR(TO) gprs CPAS FAIL !!.\r\n\0"));
		retS = false;
	}

	return(retS);
}
//------------------------------------------------------------------------------------
void gprs_mon_sqe( bool f_debug,  bool forever, uint8_t *csq)
{

uint8_t timer = 10;

	// Recien despues de estar registrado puedo leer la calidad de señal.
	*csq = gprs_CSQ(f_debug);

	if ( forever == false ) {
		return;
	}

	// Salgo por watchdog (900s) o reset
	while ( true ) {

		vTaskDelay( ( TickType_t)( 1000 / portTICK_RATE_MS ) );

		if ( --timer == 0) {
			// Expiro: monitoreo el SQE y recargo el timer a 10 segundos
			gprs_CSQ(f_debug);
			timer = 10;
		}
	}
}
//------------------------------------------------------------------------------------
uint8_t gprs_CSQ( bool f_debug )
{
	// Veo la calidad de senal que estoy recibiendo

char csqBuffer[32] = { 0 };
char *ts = NULL;
uint8_t csq, dbm;
uint8_t timeout;
bool retS = false;

	csq = 0;
	dbm = 0;

	xprintf_PD( f_debug,  PSTR("COMMS: gprs CSQ\r\n"));
	gprs_flush_RX_buffer();
	gprs_flush_TX_buffer();
	gprs_atcmd_preamble();
	xfprintf_P( fdGPRS,PSTR("AT+CSQ\r\0"));
	timeout = 0;
	while ( timeout++ < TIMEOUT_CSQ ) {
	vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
		if ( gprs_check_response("OK") ) {
			gprs_print_RX_buffer( f_debug);
			memcpy(csqBuffer, &gprsRxBuffer.buffer[0], sizeof(csqBuffer) );
			if ( (ts = strchr(csqBuffer, ':')) ) {
				ts++;
				csq = atoi(ts);
				dbm = 113 - 2 * csq;
			}
			retS = true;
			break;
		}
	}

	if ( retS ) {
		xprintf_PD( f_debug,  PSTR("COMMS: gprs CSQ OK. [csq=%d,DBM=%d]\r\n\0"), csq, dbm );
	} else {
		xprintf_PD( f_debug,  PSTR("COMMS: ERROR gprs CSQ FAIL !!.\r\n\0") );
	}

	return(csq);
}
//------------------------------------------------------------------------------------
bool gprs_configurar_dispositivo( bool f_debug, char *pin, char *apn, uint8_t *err_code )
{
	/*
	 * Consiste en enviar los comandos AT de modo que el modem GPRS
	 * quede disponible para trabajar
	 * Doy primero los comandos que no tienen que ver con el sim ni con la network
	 * Estos son lo que demoran mas y que si no responden debo abortar.
	 */

	// PASO 1: Configuro el modem. ( respuestas locales inmediatas )
	gprs_CBC( f_debug );
	gprs_ATI(f_debug);
	gprs_IFC( f_debug );

	gprs_CIPMODE(f_debug);	// modo transparente.
	gprs_D1(f_debug);
	gprs_CSUART(f_debug);
	gprs_C1(f_debug);

	// PASO 2: Vemos que halla un SIM operativo.
	// AT+CPIN?
	// Vemos que halla un pin presente.
	if ( ! gprs_CPIN( f_debug, pin) ) {
		*err_code = ERR_CPIN_FAIL;
		return(false);
	}

	//
	// AT+CCID
	if ( gprs_CCID(f_debug) == false ) {
		xprintf_PD( f_debug, PSTR("COMMS: gprs ERROR: CCID not available!!\r\n\0"));
		//gprs_apagar();
		//return (false );
	}

	// PASO 3: Comandos que dependen del estado de la red ( demoran )
	// NETWORK GSM/GPRS (base de trasmision de datos)
	// Vemos que el modem este registrado en la red. Pude demorar hasta 1 minuto ( CLARO )
	// Con esto estoy conectado a la red movil.
	if ( ! gprs_CREG(f_debug) ) {
		*err_code = ERR_CREG_FAIL;
		return(false);
	}

	// Vemos que la red este operativa
	if ( ! gprs_CPSI(f_debug) ) {
		*err_code = ERR_CPSI_FAIL;
		return(false);
	}

	// PASO 4: Comandos de conexion a la red de datos ( PDP )
	// Dependen de la red por lo que pueden demorar hasta 3 minutos
	// Debo atachearme a la red de paquetes de datos.
	// Configuro el APN ( PDP context)
	if ( ! gprs_CGDCONT( f_debug, apn ) ) {
		*err_code = ERR_APN_FAIL;
		return(false);
	}

	// Activo el APN
	if ( ! gprs_CGATT(f_debug) ) {
		*err_code = ERR_NETATTACH_FAIL;
		return(false);
	}

	gprs_CMGF(f_debug);		// Configuro para mandar SMS en modo TEXTO

	gprs_CIPTIMEOUT(f_debug);

	*err_code = ERR_NONE;
	return(true);
}
//------------------------------------------------------------------------------------
void gprs_CBC( bool f_debug )
{
	// Lee el voltaje del modem
	// Solo espero un OK.
	// Es de respuesta inmediata ( 100 ms )

uint8_t timeout;
bool retS = false;

	xprintf_PD( f_debug,  PSTR("COMMS: gprs CBC\r\n"));
	gprs_flush_RX_buffer();
	gprs_flush_TX_buffer();
	gprs_atcmd_preamble();
	xfprintf_P( fdGPRS,PSTR("AT+CBC\r\0"));
	timeout = 0;
	while ( timeout++ < TIMEOUT_CBC ) {
		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
		if ( gprs_check_response("OK") ) {
			retS = true;
			break;
		}
	}

	gprs_print_RX_buffer(f_debug);
	if ( retS ) {
		xprintf_PD( f_debug, PSTR("COMMS: gprs CBC OK.\r\n\0"));
	} else {
		gprs_AT();
		xprintf_PD( f_debug, PSTR("COMMS: WARN gprs CBC FAIL !!.\r\n\0") );
	}

	return;
}
//------------------------------------------------------------------------------------
bool gprs_ATI( bool f_debug )
{
	// Mando el comando ATI que me sustituye al CGMM,CGMR, CGMI, IMEI
	// El IMEI se consigue con AT+CGSN y telit le da un timeout de 20s !!!
	// Si da error no tranca nada asi que intento una sola vez

bool retS = false;
char *p;
uint8_t timeout;

	xprintf_PD( f_debug,  PSTR("COMMS: gprs ATI\r\n"));
	gprs_flush_RX_buffer();
	gprs_flush_TX_buffer();
	gprs_atcmd_preamble();
	xfprintf_P( fdGPRS,PSTR("ATI\r\0"));

	timeout = 0;
	while ( timeout++ < TIMEOUT_IMEI ) {
		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
		if ( gprs_check_response("OK") ) {
			p = strstr(gprsRxBuffer.buffer,"IMEI:");
			retS = pv_get_token(p, gprs_status.buff_gprs_imei, IMEIBUFFSIZE );
			break;
		}
	}

	gprs_print_RX_buffer(f_debug);

	if (retS) {
		xprintf_PD( f_debug,  PSTR("COMMS: gprs ATI OK imei [%s] en [%d] secs\r\n\0"), gprs_status.buff_gprs_imei, timeout );
	} else {
		gprs_AT();
		xprintf_PD( f_debug,  PSTR("COMMS: ERROR gprs ATI FAIL !!.\r\n\0"));
	}

	return(retS);
}
//------------------------------------------------------------------------------------
void gprs_IFC( bool f_debug )
{
	// Lee si tiene configurado control de flujo.

uint8_t timeout;
bool retS = false;

	xprintf_PD( f_debug,  PSTR("COMMS: gprs IFC\r\n"));
	gprs_flush_RX_buffer();
	gprs_flush_TX_buffer();
	gprs_atcmd_preamble();
	xfprintf_P( fdGPRS,PSTR("AT+IFC?\r\0"));

	timeout = 0;
	while ( timeout++ < TIMEOUT_IFC ) {
		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );

		if ( gprs_check_response("OK") ) {
			if ( gprs_check_response("IFC: 0,0")) {
				retS = true;
			}
			break;
		}
	}

	gprs_print_RX_buffer(f_debug);
	if ( retS ) {
		xprintf_P( PSTR("COMMS: gprs IFC OK (flow control NONE)\r\n\0") );
	} else {
		gprs_AT();
		xprintf_P( PSTR("COMMS: WARN: gprs IFC FAIL !!.\r\n\0") );
	}

	return;

}
//------------------------------------------------------------------------------------
void gprs_CIPMODE(bool f_debug)
{
	// Funcion que configura el TCP/IP en modo transparente.

uint8_t timeout;
bool retS = false;

	xprintf_PD( f_debug,  PSTR("COMMS: gprs CIPMODE\r\n"));
	gprs_flush_RX_buffer();
	gprs_flush_TX_buffer();
	gprs_atcmd_preamble();
	xfprintf_P( fdGPRS,PSTR("AT+CIPMODE=1\r\0"));
	timeout = 0;
	while ( timeout++ < TIMEOUT_CIPMODE ) {
		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
		if ( gprs_check_response("OK") ) {
			retS = true;
			break;
		}
	}

	gprs_print_RX_buffer(f_debug);
	if ( retS ) {
		xprintf_PD( f_debug, PSTR("COMMS: gprs CIPMODE OK en [%d] secs\r\n\0"), timeout );
	} else {
		gprs_AT();
		xprintf_PD( f_debug, PSTR("COMMS: WARN gprs CIPMODE FAIL !!.\r\n\0") );
	}

	return;
}
//------------------------------------------------------------------------------------
void gprs_D1(bool f_debug)
{

uint8_t timeout;
bool retS = false;

	xprintf_PD( f_debug,  PSTR("COMMS: gprs AT&D1\r\n"));
	gprs_flush_RX_buffer();
	gprs_flush_TX_buffer();
	gprs_atcmd_preamble();
	xfprintf_P( fdGPRS,PSTR("AT&D1\r\0"));
	timeout = 0;
	while ( timeout++ < TIMEOUT_D1 ) {
		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
		if ( gprs_check_response("OK") ) {
			retS = true;
			break;
		}
	}

	gprs_print_RX_buffer(f_debug);
	if ( retS ) {
		xprintf_PD( f_debug, PSTR("COMMS: gprs &D1 OK.\r\n\0") );
	} else {
		gprs_AT();
		xprintf_PD( f_debug, PSTR("COMMS: WARN gprs &D1 FAIL !!\r\n\0") );
	}

	return;
}
//------------------------------------------------------------------------------------
void gprs_CSUART(bool f_debug)
{
	// Funcion que configura la UART con 7 lineas ( DCD/RTS/CTS/DTR)

uint8_t timeout;
bool retS = false;

	xprintf_PD( f_debug,  PSTR("COMMS: gprs CSUART\r\n"));
	gprs_flush_RX_buffer();
	gprs_flush_TX_buffer();
	gprs_atcmd_preamble();
	xfprintf_P( fdGPRS,PSTR("AT+CSUART=1\r\0"));
	timeout = 0;
	while ( timeout++ < TIMEOUT_CSUART ) {
		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
		if ( gprs_check_response("OK") ) {
			retS = true;
			break;
		}
	}

	gprs_print_RX_buffer(f_debug);
	if ( retS ) {
		xprintf_PD( f_debug, PSTR("COMMS: gprs CSUART OK.\r\n\0"));
	} else {
		gprs_AT();
		xprintf_PD( f_debug, PSTR("COMMS: WARN gprs CSUART FAIL !!\r\n\0") );
	}

	return;
}
//------------------------------------------------------------------------------------
void gprs_C1(bool f_debug)
{

uint8_t timeout;
bool retS = false;

	xprintf_PD( f_debug,  PSTR("COMMS: gprs AT&C1\r\n"));
	gprs_flush_RX_buffer();
	gprs_flush_TX_buffer();
	gprs_atcmd_preamble();
	xfprintf_P( fdGPRS,PSTR("AT&C1\r\0"));
	timeout = 0;
	while ( timeout++ < TIMEOUT_C1 ) {
		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
		if ( gprs_check_response("OK") ) {
			retS = true;
			break;
		}
	}

	gprs_print_RX_buffer(f_debug);
	if ( retS ) {
		xprintf_PD( f_debug, PSTR("COMMS: gprs &C1 OK.\r\n\0"));
	} else {
		gprs_AT();
		xprintf_PD( f_debug, PSTR("COMMS: WARN gprs &C1 FAIL !!\r\n\0") );
	}

	return;
}
//------------------------------------------------------------------------------------
bool gprs_CPIN( bool f_debug, char *pin )
{
	// Chequeo que el SIM este en condiciones de funcionar.
	// AT+CPIN?
	// No configuro el PIN !!!
	// TELIT le da un timeout de 20s.

uint8_t tryes;
int8_t timeout;
bool retS = false;
t_responses cmd_rsp = rsp_NONE;

	xprintf_PD( f_debug, PSTR("GPRS: gprs CPIN\r\n\0"));
	tryes = 0;
	while ( tryes++ < MAX_TRYES_CPIN ) {

		gprs_flush_RX_buffer();
		gprs_flush_TX_buffer();
		xprintf_PD( f_debug, PSTR("GPRS: gprs send CPIN (%d)\r\n\0"),tryes );
		gprs_atcmd_preamble();
		xfprintf_P( fdGPRS , PSTR("AT+CPIN?\r\0"));

		timeout = 0;
		while ( timeout++ < TIMEOUT_CPIN ) {
			vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );

			if ( gprs_check_response("ERROR") ) {
				// Salgo y reintento el comando.
				cmd_rsp = rsp_ERROR;
				break;
			}

			if ( gprs_check_response("OK") ) {
				cmd_rsp = rsp_OK;
				if ( gprs_check_response("READY") ) {
					// Respuesta correcta.
					retS = true;
				} else {
					// OK pero no correcto: Reintento
					retS = true;
				}
				goto EXIT;
			}

			// Sin respuesta aun. Espero
		}

		gprs_print_RX_buffer( f_debug);

		// Sali por timeout:
		if ( cmd_rsp == rsp_NONE) {
			gprs_AT();
		}
	}

EXIT:

	gprs_print_RX_buffer( f_debug);

	switch(cmd_rsp) {
	case rsp_OK:
		xprintf_PD( f_debug,  PSTR("COMMS: gprs CPIN OK en [%d] secs\r\n\0"), (( 10 * (tryes-1) ) + timeout) );
		retS = true;
		break;
	case rsp_ERROR:
		xprintf_PD( f_debug,  PSTR("COMMS: gprs CPIN FAIL !!.\r\n\0"));
		retS = false;
		break;
	default:
		// Timeout
		gprs_AT();
		xprintf_PD( f_debug,  PSTR("COMMS: ERROR(TO) gprs CPIN FAIL !!.\r\n\0"));
		retS = false;
	}

	return(retS);
}
//------------------------------------------------------------------------------------
bool gprs_CCID( bool f_debug )
{
	// Leo el ccid del sim para poder trasmitirlo al server y asi
	// llevar un control de donde esta c/sim
	// AT+CCID
	// +CCID: "8959801611637152574F"
	//
	// OK

uint8_t tryes;
int8_t timeout;
bool retS = false;
char *p;

	xprintf_PD( f_debug,  PSTR("COMMS: gprs CCID\r\n"));
	tryes = 0;
	while ( tryes++ < MAX_TRYES_CCID ) {
		// Envio el comando
		gprs_flush_RX_buffer();
		gprs_flush_TX_buffer();
		xprintf_PD( f_debug, PSTR("GPRS: gprs send CCID (%d)\r\n\0"),tryes );
		gprs_atcmd_preamble();
		xfprintf_P( fdGPRS , PSTR("AT+CCID\r\0"));

		timeout = 0;
		while ( timeout++ < TIMEOUT_CCID ) {
			vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );

			if ( gprs_check_response("ERROR") ) {
				// Salgo y reintento el comando.
				break;
			}

			if ( gprs_check_response("OK") ) {
				gprs_print_RX_buffer( f_debug);
				p = strstr(gprsRxBuffer.buffer,"CCID:");
				retS = pv_get_token(p, gprs_status.buff_gprs_ccid, CCIDBUFFSIZE );
				goto EXIT;
			}
		}

		// Sali por TO/ERROR/rsp.no correcta: Muestro la respuesta y reintento.
		gprs_print_RX_buffer( f_debug);
	}

EXIT:

	if (retS) {
		xprintf_PD( f_debug,  PSTR("COMMS: gprs CCID OK  ccid [%s] en [%d] secs\r\n\0"), gprs_status.buff_gprs_imei, timeout );
	} else {
		gprs_AT();
		xprintf_PD( f_debug,  PSTR("COMMS: ERROR gprs CCID FAIL !!.\r\n\0"));
	}

	return(retS);
}
//------------------------------------------------------------------------------------
bool gprs_CREG( bool f_debug )
{
	/* Chequeo que el TE este registrado en la red.
	 Esto deberia ser automatico.
	 Normalmente el SIM esta para que el dispositivo se registre automaticamente
	 Esto se puede ver con el comando AT+COPS? el cual tiene la red preferida y el modo 0
	 indica que esta para registrarse automaticamente.
	 Este comando se usa para de-registrar y registrar en la red.
	 Conviene dejarlo automatico de modo que si el TE se puede registrar lo va a hacer.
	 Solo chequeamos que este registrado con CGREG.
	 AT+CGREG?
	 +CGREG: 0,1
	 HAy casos como con los sims CLARO que puede llegar hasta 1 minuto a demorar conectarse
	 a la red, por eso esperamos mas.
	 En realidad el comando retorna en no mas de 5s, pero el registro puede demorar hasta 1 minuto
	 o mas, dependiendo de la calidad de señal ( antena ) y la red.
	 Para esto, el mon_sqe lo ponemos antes.
	*/

bool retS = false;
uint8_t tryes;
int8_t timeout;
t_responses cmd_rsp = rsp_NONE;

	xprintf_PD( f_debug,  PSTR("COMMS: gprs CREG (NET registation)\r\n\0"));
	tryes = 0;
	while ( tryes++ < MAX_TRYES_CREG ) {

		gprs_flush_RX_buffer();
		gprs_flush_TX_buffer();
		xprintf_PD( f_debug, PSTR("GPRS: gprs send CREG (%d)\r\n\0"),tryes );
		gprs_atcmd_preamble();
		xfprintf_P( fdGPRS, PSTR("AT+CREG?\r\0"));

		timeout = 0;
		while ( timeout++ < TIMEOUT_CREG ) {
			vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );

			if ( gprs_check_response("ERROR") ) {
				cmd_rsp = rsp_ERROR;
				// Salgo y reintento el comando.
				break;
			}

			if ( gprs_check_response("OK") ) {
				if ( gprs_check_response("CREG: 0,1") ) {
					// Respuesta correcta.
					cmd_rsp = rsp_OK;
					retS = true;
					goto EXIT;
				}
				// OK pero no correcto: Sigo esperando.
				break;
			}

			// Espero
		}

		gprs_print_RX_buffer( f_debug);

		// Sali por timeout:
		if ( cmd_rsp == rsp_NONE) {
			gprs_AT();
		}
	}

EXIT:

	gprs_print_RX_buffer( f_debug);

	switch(cmd_rsp) {
	case rsp_OK:
		xprintf_PD( f_debug,  PSTR("COMMS: gprs gprs CREG OK en [%d] secs\r\n\0"), (( 10 * (tryes-1) ) + timeout) );
		retS = true;
		break;
	case rsp_ERROR:
		xprintf_PD( f_debug,  PSTR("COMMS: ERROR gprs gprs CREG FAIL !!.\r\n\0"));
		retS = false;
		break;
	default:
		// Timeout
		gprs_AT();
		xprintf_PD( f_debug,  PSTR("COMMS: ERROR(TO) gprs CREG FAIL !!.\r\n\0"));
		retS = false;
	}

	return(retS);

}
//------------------------------------------------------------------------------------
bool gprs_CPSI( bool f_debug )
{
	/* Chequeo que la red este operativa
	*/

bool retS = false;
uint8_t tryes;
int8_t timeout;
t_responses cmd_rsp = rsp_NONE;

	xprintf_PD( f_debug,  PSTR("COMMS: gprs CPSI( service )\r\n\0"));
	tryes = 0;
	while ( tryes++ < MAX_TRYES_CPSI ) {

		gprs_flush_RX_buffer();
		gprs_flush_TX_buffer();
		xprintf_PD( f_debug, PSTR("GPRS: gprs send CPSI (%d)\r\n\0"),tryes );
		gprs_atcmd_preamble();
		xfprintf_P( fdGPRS, PSTR("AT+CPSI?\r\0"));

		timeout = 0;
		while ( timeout++ < TIMEOUT_CPSI ) {
			vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );

			if ( gprs_check_response("ERROR") ) {
				cmd_rsp = rsp_ERROR;
				// Salgo y reintento el comando.
				break;
			}

			if ( gprs_check_response("OK") ) {
				cmd_rsp = rsp_OK;
				if ( gprs_check_response("NO SERVICE") ) {
					// Sin servicio.
					retS = false;
				} else {
					retS = true;
				}
				goto EXIT;
			}

			// Espero

		}

		gprs_print_RX_buffer( f_debug);

		// Sali por timeout:
		if ( cmd_rsp == rsp_NONE) {
			gprs_AT();
		}
	}

EXIT:

	gprs_print_RX_buffer( f_debug);

	switch(cmd_rsp) {
	case rsp_OK:
		if ( retS ) {
			xprintf_PD( f_debug,  PSTR("COMMS: gprs gprs CPSI OK (SERVICE) en [%d] secs\r\n\0"), (( 10 * (tryes-1) ) + timeout) );
		} else {
			xprintf_PD( f_debug,  PSTR("COMMS: gprs gprs CPSI FAIL (NO SERVICE) !!.\r\n\0"));
		}
	break;
	case rsp_ERROR:
		xprintf_PD( f_debug,  PSTR("COMMS: ERROR gprs gprs CPSI FAIL !!.\r\n\0"));
		break;
	default:
		// Timeout
		gprs_AT();
		xprintf_PD( f_debug,  PSTR("COMMS: ERROR(TO) gprs CPSI FAIL !!.\r\n\0"));
	}

	return(retS);

}
//------------------------------------------------------------------------------------
bool gprs_CGDCONT( bool f_debug, char *apn)
{

uint8_t tryes;
int8_t timeout;
bool retS = false;
t_responses cmd_rsp = rsp_NONE;

	xprintf_PD( f_debug, PSTR("COMMS: gprs CGDCONT (define PDP)\r\n\0") );
	//Defino el PDP indicando cual es el APN.
	tryes = 0;
	while (tryes++ <  MAX_TRYES_SOCKCONT ) {
		gprs_flush_RX_buffer();
		gprs_flush_TX_buffer();
		xprintf_P(PSTR("GPRS: gprs send CGDCONT (%d)\r\n\0"),tryes);
		gprs_atcmd_preamble();
		// xfprintf_P( fdGPRS, PSTR("AT+CGSOCKCONT=1,\"IP\",\"%s\"\r\0"), apn);
		xfprintf_P( fdGPRS, PSTR("AT+CGDCONT=1,\"IP\",\"%s\"\r\0"), apn);

		timeout = 0;
		while ( timeout++ < TIMEOUT_SOCKCONT ) {
			vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
			if ( gprs_check_response("OK") ) {
				cmd_rsp = rsp_OK;
				goto EXIT;
			}
			if ( gprs_check_response("ERROR") ) {
				cmd_rsp = rsp_ERROR;
				goto EXIT;
			}

			// Espero
		}

		gprs_print_RX_buffer( f_debug);
		// Sali por timeout:
		if ( cmd_rsp == rsp_NONE) {
			gprs_AT();
		}
	}

EXIT:

	gprs_print_RX_buffer( f_debug);

	switch(cmd_rsp) {
	case rsp_OK:
		xprintf_PD( f_debug,  PSTR("COMMS: gprs CGDCONT OK en [%d] secs\r\n\0"), (( 10 * (tryes-1) ) + timeout) );
		retS = true;
		break;
	case rsp_ERROR:
		xprintf_PD( f_debug,  PSTR("COMMS: ERROR gprs CGDCONT FAIL !!.\r\n\0"));
		retS = false;
		break;
	default:
		// Timeout
		gprs_AT();
		xprintf_PD( f_debug,  PSTR("COMMS: ERROR(TO) gprs CGDCONT FAIL !!.\r\n\0"));
		retS = false;
	}

	return(retS);
}
//------------------------------------------------------------------------------------
bool gprs_CGATT(bool f_debug)
{
	/* Una vez registrado, me atacheo a la red
	 AT+CGATT=1
	 AT+CGATT?
	 +CGATT: 1
	 Puede demorar mucho, hasta 75s. ( 1 min en CLARO )
	*/

int8_t timeout;
bool retS = false;
uint8_t tryes = 0;
t_responses cmd_rsp = rsp_NONE;

	xprintf_PD( f_debug,  PSTR("COMMS: gprs CGATT (NET attach)\r\n\0"));

	// Indico que active el PDP #1
	while ( tryes++ < MAX_TRYES_CGATT ) {
		gprs_flush_RX_buffer();
		gprs_flush_TX_buffer();
		gprs_atcmd_preamble();
		xfprintf_P( fdGPRS,PSTR("AT+CGATT=1\r\0"));
		timeout = 0;
		while ( timeout++ < TIMEOUT_CGATT ) {
			vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
			if ( gprs_check_response("OK") ) {
				cmd_rsp = rsp_OK;
				goto AWAIT;
			}
			if ( gprs_check_response("ERROR") ) {
				// Salgo a repetir el comando
				cmd_rsp = rsp_ERROR;
				break;
			}
			// Espero
		}
		// Sali por timeout:
		gprs_print_RX_buffer( f_debug);
		gprs_AT();
	}

	// Aqui es que nunca me dio OK. Salgo
	retS = false;
	xprintf_PD( f_debug, PSTR("COMMS: ERROR CGATT (NET attach) FAIL !!\r\n\0") );
	return(retS);

AWAIT:

	// Espero hasta 3 minutos que quede atacheado
	// Monitoreo estar attacheado.
	tryes = 0;
	while ( tryes++ < MAX_TRYES_CGATT ) {
		// Vemos si necesita SIMPIN
		gprs_flush_RX_buffer();
		gprs_flush_TX_buffer();
		xprintf_PD( f_debug, PSTR("GPRS: gprs send CGATT? (%d)\r\n\0"),tryes );
		gprs_atcmd_preamble();
		xfprintf_P( fdGPRS , PSTR("AT+CGATT?\r\0"));
		timeout = 0;
		while ( timeout++ < TIMEOUT_CGATT ) {
			vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );

			if ( gprs_check_response("ERROR") ) {
				// Salgo a repetir el comando
				cmd_rsp = rsp_ERROR;
			}

			if ( gprs_check_response("OK") ) {
				cmd_rsp = rsp_OK;
				if ( gprs_check_response("CGATT: 1") ) {
					// Respuesta correcta.
					retS = true;
					goto EXIT;
				}
				// OK pero no correcto: Reintento
				break;
			}

			// Espero
		}

		// Sali por timeout:
		gprs_print_RX_buffer( f_debug);
		gprs_AT();
	}

EXIT:

	gprs_print_RX_buffer( f_debug);

	switch(cmd_rsp) {
	case rsp_OK:
		xprintf_PD( f_debug,  PSTR("COMMS: gprs gprs CGATT OK (NET attach) en [%d] secs\r\n\0"), (( 10 * (tryes-1) ) + timeout) );
		retS = true;
		break;
	case rsp_ERROR:
		xprintf_PD( f_debug,  PSTR("COMMS: gprs ERROR: CGATT FAIL !!.\r\n\0"));
		retS = false;
		break;
	default:
		// Timeout
		gprs_AT();
		xprintf_PD( f_debug,  PSTR("COMMS: ERROR(TO) gprs CGATT FAIL !!.\r\n\0"));
		retS = false;
	}

	return(retS);

}
//------------------------------------------------------------------------------------
void gprs_CMGF( bool f_debug )
{
	// Configura para mandar SMS en modo texto
	// Telit le da un timeout de 5s

uint8_t timeout;
bool retS = false;

	xprintf_PD( f_debug,  PSTR("COMMS: gprs CMGF\r\n"));
	gprs_flush_RX_buffer();
	gprs_flush_TX_buffer();
	gprs_atcmd_preamble();
	xfprintf_P( fdGPRS,PSTR("AT+CMGF=1\r\0"));
	timeout = 0;
	while ( timeout++ < TIMEOUT_CMGF ) {
		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
		if ( gprs_check_response("OK") ) {
			retS = true;
			break;
		}
	}

	gprs_print_RX_buffer(f_debug);
	if ( retS ) {
		xprintf_PD( f_debug, PSTR("COMMS: gprs CMGF OK.\r\n\0") );
	} else {
		gprs_AT();
		xprintf_PD( f_debug, PSTR("COMMS: WARN gprs CMGF FAIL !!.\r\n\0") );
	}

	return;
}
//------------------------------------------------------------------------------------
void gprs_CIPTIMEOUT(bool f_debug)
{

uint8_t timeout;
bool retS = false;

	xprintf_PD( f_debug,  PSTR("COMMS: gprs CIPTIMEOUT\r\n"));
	gprs_flush_RX_buffer();
	gprs_flush_TX_buffer();
	gprs_atcmd_preamble();
	xfprintf_P( fdGPRS,PSTR("AT+CIPTIMEOUT=60000,60000,60000\r\0"));
	timeout = 0;
	while ( timeout++ < TIMEOUT_CIPTIMEOUT ) {
		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
		if ( gprs_check_response("OK") ) {
			retS = true;
			break;
		}
	}

	gprs_print_RX_buffer(f_debug);
	if ( retS ) {
		xprintf_PD( f_debug, PSTR("COMMS: gprs CIPTIMEOUT OK.\r\n\0") );
	} else {
		gprs_AT();
		xprintf_PD( f_debug, PSTR("COMMS: WARN gprs CIPTIMEOUT FAIL !!\r\n\0") );
	}

	return;
}
//------------------------------------------------------------------------------------
char *gprs_get_imei(void)
{
	return( gprs_status.buff_gprs_imei );
}
//------------------------------------------------------------------------------------
char *gprs_get_ccid(void)
{
	return( gprs_status.buff_gprs_ccid );

}
//------------------------------------------------------------------------------------
bool pv_get_token( char *p, char *buff, uint8_t size)
{

uint8_t i = 0;
bool retS = false;
char c;

	if ( p == NULL ) {
		// No lo pude leer.
		retS = false;
		goto EXIT;
	}

	memset( buff, '\0', size);

	// Start. Busco el primer caracter en rxbuffer
	for (i=0; i<64; i++) {
		c = *p;
		if ( isdigit(c) ) {
			break;
		}
		if (i==63) {
			retS = false;
			goto EXIT;
		}
		p++;
	}

	// Copio hasta un \r
	for (i=0;i<size;i++) {
		c = *p;
		if (( c =='\r' ) || ( c == '"')) {
			retS = true;
			break;
		}
		buff[i] = c;
		p++;
	}


EXIT:

	return(retS);

}
//------------------------------------------------------------------------------------
t_net_status gprs_NETCLOSE(bool f_debug)
{
	// Paso a modo comando por las dudas

//t_responses cmd_rsp = rsp_NONE;
t_net_status net_status = NET_UNKNOWN;
uint8_t tryes;
int8_t timeout;

	xprintf_PD( f_debug,  PSTR("COMMS: gprs NETCLOSE\r\n\0"));
	tryes = 0;
	while (tryes++ <  MAX_TRYES_NETCLOSE ) {
		gprs_flush_RX_buffer();
		gprs_flush_TX_buffer();
		xprintf_P(PSTR("GPRS: gprs send NETCLOSE (%d)\r\n\0"),tryes);
		gprs_atcmd_preamble();
		xfprintf_P( fdGPRS,PSTR("AT+NETCLOSE\r\0"));

		timeout = 0;
		while ( timeout++ < TIMEOUT_NETCLOSE ) {

			vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );

			if ( gprs_check_response("Network closed")) {
				//cmd_rsp = rsp_OK;
				net_status = NET_CLOSE;
				goto EXIT;
			}

			if ( gprs_check_response("OK") ) {
				//cmd_rsp = rsp_OK;
				net_status = NET_CLOSE;
				goto EXIT;
			}

			if ( gprs_check_response("Network is already closed")) {
				//cmd_rsp = rsp_ERROR;
				net_status = NET_CLOSE;
				goto EXIT;
			}

			// +IP ERROR / +CME ERROR
			if ( gprs_check_response("ERROR") ) {
				//cmd_rsp = rsp_ERROR;
				net_status = NET_UNKNOWN;
				goto EXIT;
			}

			// Sin respuesta aun ( hasta 2 minutos.
		}

		// Sali por timeout
		gprs_print_RX_buffer( f_debug);
	}

	xprintf_PD( f_debug,  PSTR("COMMS: ERROR(TO) gprs NETCLOSE !!.\r\n\0"));

EXIT:

	gprs_print_RX_buffer( f_debug);
	if ( net_status == NET_CLOSE ) {
		xprintf_PD( f_debug,  PSTR("COMMS: gprs NETCLOSE OK en [%d] secs\r\n\0"), (( 10 * (tryes-1) ) + timeout) );
	} else {
		xprintf_PD( f_debug,  PSTR("COMMS: ERROR gprs NETCLOSE FAIL !!.\r\n\0"));
	}

	return(net_status);
}
//------------------------------------------------------------------------------------
t_net_status gprs_NETOPEN(bool f_debug)
{
	// Inicia el servicio de sockets abriendo un socket. ( no es una conexion sino un
	// socket local por el cual se va a comunicar. )
	// Activa el contexto y crea el socket local
	// La red asigna una IP.
	// Puede demorar unos segundos por lo que espero para chequear el resultado
	// y reintento varias veces.

//t_responses cmd_rsp = rsp_NONE;
t_net_status net_status = NET_UNKNOWN;
uint8_t tryes;
int8_t timeout;

	xprintf_PD( f_debug,  PSTR("COMMS: gprs NETOPEN\r\n\0"));
	tryes = 0;
	while (tryes++ <  MAX_TRYES_NETOPEN ) {

		gprs_flush_RX_buffer();
		gprs_flush_TX_buffer();
		xprintf_P(PSTR("GPRS: gprs send NETOPEN (%d)\r\n\0"),tryes);
		gprs_atcmd_preamble();
		xfprintf_P( fdGPRS,PSTR("AT+NETOPEN=\"TCP\"\r\0"));

		timeout = 0;
		while ( timeout++ < TIMEOUT_NETOPEN ) {
			vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );

			// Evaluo las respuestas del modem.
			if ( gprs_check_response("Connect ok")) {
				//cmd_rsp = rsp_OK;
				net_status = NET_OPEN;
				goto EXIT;
			}

			if ( gprs_check_response("Network opened")) {
				//cmd_rsp = rsp_ERROR;
				net_status = NET_OPEN;
				goto EXIT;
			}

			// +IP ERROR: Network is already opened
			if ( gprs_check_response("Network is already opened")) {
				//cmd_rsp = rsp_OK;
				net_status = NET_OPEN;
				goto EXIT;
			}

			// +IP ERROR: Network not opened
			if ( gprs_check_response("Network not opened")) {
				//cmd_rsp = rsp_ERROR;
				net_status = NET_CLOSE;
				goto EXIT;
			}

			// +IP ERROR / +CME ERROR
			if ( gprs_check_response("ERROR") ) {
				//cmd_rsp = rsp_ERROR;
				net_status = NET_UNKNOWN;
				goto EXIT;
			}

			// Espero
		}

		// Sali por timeout:
		gprs_print_RX_buffer( f_debug);
	}

	xprintf_PD( f_debug,  PSTR("COMMS: ERROR(TO) gprs NETOPEN !!.\r\n\0"));

EXIT:

	// Muestro el resultado del comando (OK/ERR/TO)
	gprs_print_RX_buffer( f_debug);

	if ( net_status == NET_OPEN ) {
		xprintf_PD( f_debug,  PSTR("COMMS: gprs NETOPEN OK en [%d] secs\r\n\0"), (( 10 * (tryes-1) ) + timeout) );
	} else {
		xprintf_PD( f_debug,  PSTR("COMMS: ERROR gprs NETOPEN FAIL !!.\r\n\0"));
	}

	// Retorno el resultado de la operacion
	return(net_status);

}
//------------------------------------------------------------------------------------
t_net_status gprs_NET_status( bool f_debug)
{
	// Consulto el estado del servicio. Solo espero 30s
	// Si me da timeout asumo que el servicio esta apagado


//t_responses cmd_rsp = rsp_NONE;
t_net_status net_status = NET_UNKNOWN;
uint8_t timeout;

	xprintf_PD( f_debug, PSTR("COMMS: gprs NETstatus.\r\n\0"));
	gprs_flush_RX_buffer();
	gprs_flush_TX_buffer();
	gprs_atcmd_preamble();
	xfprintf_P( fdGPRS, PSTR("AT+NETOPEN?\r\0"));
	timeout = 0;
	while ( timeout++ < TIMEOUT_NETOPEN ) {
		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );

		if ( gprs_check_response("OK")) {
			if ( gprs_check_response("NETOPEN: 0,0")) {
				//cmd_rsp = rsp_OK;
				net_status = NET_CLOSE;
				goto EXIT;
			}
			if ( gprs_check_response("NETOPEN: 1,0")) {
				//cmd_rsp = rsp_OK;
				net_status = NET_OPEN;
				goto EXIT;
			}

		} else if ( gprs_check_response("ERROR")) {
			//cmd_rsp = rsp_ERROR;
			net_status = NET_UNKNOWN;
			goto EXIT;
		}
		// Espero
	}

	// Sali por timeout:
	gprs_print_RX_buffer( f_debug);
	xprintf_PD( f_debug,  PSTR("COMMS: ERROR(TO) gprs NETstatus !!.\r\n\0"));

EXIT:

	gprs_print_RX_buffer( f_debug);

	switch(net_status) {
	case NET_OPEN:
		xprintf_PD( f_debug, PSTR("COMMS: gprs NETstatus is open.\r\n\0"));
		break;
	case NET_CLOSE:
		xprintf_PD( f_debug, PSTR("COMMS: gprs NETstatus is close.\r\n\0") );
		break;
	default:
		// UNKNOWN
		xprintf_PD( f_debug, PSTR("COMMS: gprs NETstatus is unknown.\r\n\0") );
		break;
	}

	return(net_status);

}
//------------------------------------------------------------------------------------
bool gprs_IPADDR(bool f_debug, char *ip_assigned )
{

	/*
	 * Tengo la IP asignada: la leo para actualizar systemVars.ipaddress
	 * La respuesta normal seria del tipo:
	 * 		AT+IPADDR
	 * 		+IPADDR: 10.204.2.115
	 * Puede llegar a responder
	 * 		AT+IPADDR
	 * 		+IP ERROR: Network not opened
	 * lo que sirve para reintentar.
	 */

char *ts = NULL;
char c = '\0';
char *ptr = NULL;
uint8_t timeout;
bool retS = false;

	xprintf_PD( f_debug,  PSTR("COMMS: gprs IPADDR\r\n\0"));
	strcpy(ip_assigned, "0.0.0.0\0");
	gprs_flush_RX_buffer();
	gprs_flush_TX_buffer();
	gprs_atcmd_preamble();
	xfprintf_P( fdGPRS,PSTR("AT+IPADDR\r\0"));
	timeout = 0;
	while ( timeout++ < TIMEOUT_READIP ) {
		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );

		// Evaluo las respuestas del modem.
		if ( gprs_check_response("OK") ) {
			if ( gprs_check_response("IPADDR:")) {
				// Tengo la IP: extraigo la IP del token.
				ptr = ip_assigned;
				ts = strchr( gprsRxBuffer.buffer, ':');
				ts++;
				while ( (c= *ts) != '\r') {
					*ptr++ = c;
					ts++;
				}
				*ptr = '\0';
				retS = true;
				break;
			}
		}

		if ( gprs_check_response("ERROR") ) {
			// +IP ERROR: Network not opened
			retS = false;
			break;
		}
		// Sin respuesta aun
	}

	gprs_print_RX_buffer(f_debug);

	if ( retS ) {
		xprintf_PD( f_debug,  PSTR("COMMS: gprs IPADDR OK [%s]\r\n\0"), ip_assigned );
	} else {
		gprs_AT();
		xprintf_P( PSTR("COMMS: ERROR gprs IPADDR FAIL !!.\r\n\0") );
	}

	return(retS);

}
//------------------------------------------------------------------------------------
t_link_status gprs_LINK_open(bool f_debug, char *ip, char *port)
{

	// only <link_num>=0 is allowed to operate with transparent mode.
	// Cuando el link queda abierto en modo transparente, lo que escribo en el puerto serial
	// se va por el link.
	// En caso de problemas da un timeout de 120s.

uint8_t timeout = 0;
t_link_status link_status = LINK_UNKNOWN;

	xprintf_PD( f_debug, PSTR("COMMS: gprs LINK open.\r\n\0"));

#ifdef MODEM_SIMULATOR
		break;
#endif

	// Intento abrir el socket una sola vez
	gprs_flush_RX_buffer();
	gprs_flush_TX_buffer();
	gprs_atcmd_preamble();
	xfprintf_P( fdGPRS, PSTR("AT+TCPCONNECT=\"%s\",%s\r\n\0"), ip, port);
	//xfprintf_P( fdGPRS, PSTR("AT+CIPOPEN=0,\"TCP\",\"%s\",%s\r\0"), ip, port);

	// Espero hasta 120s la respuesta
	timeout = 0;
	while ( timeout++ < TIMEOUT_LINKOPEN ) {

		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );

		if ( gprs_check_response("CONNECT 115200")) {
			link_status = LINK_OPEN;
			goto EXIT;

		} else if ( gprs_check_response("Connect ok")) {
			link_status = LINK_OPEN;
			goto EXIT;

		} else if ( gprs_check_response("OK")) {
			link_status = LINK_OPEN;
			goto EXIT;

		} else if ( gprs_check_response("Connection is already created")) {
			// Me respondio que la conexion esta abierta pero estoy en modo comando
			link_status = LINK_OPEN;
			goto EXIT;

		} else if ( gprs_check_response("ERROR")) {
			link_status = LINK_UNKNOWN;
			goto EXIT;
		}
		// Espero
	}

	// Sali por timeout:
	gprs_print_RX_buffer( f_debug);
	xprintf_PD( f_debug,  PSTR("COMMS: ERROR(TO) gprs LINK open !!.\r\n\0"));

EXIT:

	gprs_print_RX_buffer( f_debug);
	if (link_status == LINK_OPEN ) {
		xprintf_PD( f_debug,  PSTR("COMMS: gprs LINK open OK en [%d] secs\r\n\0"), timeout );
	} else {
		xprintf_PD( f_debug,  PSTR("COMMS: ERROR gprs LINK open FAIL !!.\r\n\0"));
	}

	return(link_status);

}
//------------------------------------------------------------------------------------
t_link_status gprs_LINK_close(bool f_debug )
{

uint8_t timeout;
t_link_status link_status = LINK_UNKNOWN;

	xprintf_PD( f_debug, PSTR("COMMS: gprs LINK close.\r\n\0"));
	gprs_flush_RX_buffer();
	gprs_flush_TX_buffer();
	gprs_atcmd_preamble();
	//xfprintf_P( fdGPRS, PSTR("AT+TCPCLOSE\n\0"));
	xfprintf_P( fdGPRS, PSTR("AT+CIPCLOSE=0\r\0"));
	timeout = 0;
	while ( timeout++ < TIMEOUT_LINKCLOSE ) {
		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );

		if ( gprs_check_response("OK")) {
			link_status = LINK_CLOSE;
			goto EXIT;

		}

		if ( gprs_check_response("Operation not supported")) {
			// Puede dar +IP ERROR: Operation not supported lo que indica que esta cerrado.
			// Es cuando no se abrio nunca y por eso el cerrarlo no esta soportado
			link_status = LINK_UNKNOWN;
			goto EXIT;
		}

		if ( gprs_check_response("ERROR")) {
			link_status = LINK_UNKNOWN;
			goto EXIT;
		}
		// Espero
	}

	// Sali por timeout:
	gprs_print_RX_buffer( f_debug);
	xprintf_PD( f_debug,  PSTR("COMMS: ERROR(TO) gprs LINK close !!.\r\n\0"));

EXIT:

	if (link_status == LINK_CLOSE ) {
		xprintf_PD( f_debug, PSTR("COMMS: gprs LINK close OK en [%d] secs\r\n\0"), timeout );
	} else {
		xprintf_PD( f_debug, PSTR("COMMS: WARN gprs LINK close FAIL !!\r\n\0") );
	}

	return(link_status);

}
//------------------------------------------------------------------------------------
t_link_status gprs_LINK_status(bool f_debug, bool dcd_mode )
{
	/* En el caso de un link GPRS, el socket puede estar abierto
	 * o no.
	 * El socket esta abierto si el modem esta prendido y
	 * el DCD esta en 0.
	 * Cuando el modem esta apagado pin_dcd = 0
	 * Cuando el modem esta prendido y el socket cerrado pin_dcd = 1
	 * Cuando el modem esta prendido y el socket abierto pin_dcd = 0.
	 */


uint8_t pin_dcd = 0;
t_link_status link_status = LINK_UNKNOWN;
uint8_t timeout = 0;


#ifdef MODEM_SIMULATOR
	pin_dcd = 0;
#endif

	if ( dcd_mode == true ) {
		// Hardware mode
		pin_dcd = IO_read_DCD();
		if (  pin_dcd == 0 ) {
			link_status = LINK_OPEN;
			xprintf_PD( f_debug, PSTR("COMMS: gprs LINK open. (dcd=%d)\r\n\0"),pin_dcd);
		} else {
			link_status = LINK_CLOSE;
			xprintf_PD( f_debug, PSTR("COMMS: gprs LINK close. (dcd=%d)\r\n\0"),pin_dcd);
		}
		return(link_status);

	} else {
		// Software mode
		gprs_atcmd_preamble();
		xfprintf_P( fdGPRS, PSTR("AT+CIPCLOSE?\r\0"));
		timeout = 0;
		while ( timeout++ < TIMEOUT_LINKCLOSE ) {
			vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );

			if ( gprs_check_response("OK")) {
				// link cerrado
				link_status = LINK_CLOSE;
				goto EXIT;
			}

			if ( gprs_check_response("CIPCLOSE: 0")) {
				// link cerrado
				link_status = LINK_CLOSE;
				goto EXIT;
			}

			if ( gprs_check_response("ERROR")) {
				link_status = LINK_UNKNOWN;
				goto EXIT;
			}
			// Await
		}
		// Sali por timeout:
		gprs_print_RX_buffer( f_debug);
		xprintf_PD( f_debug,  PSTR("COMMS: ERROR(TO) gprs LINK status !!.\r\n\0"));
EXIT:

		if (link_status == LINK_CLOSE ) {
			xprintf_PD( f_debug, PSTR("COMMS: gprs LINK status OK (CLOSE) en [%d] secs\r\n\0"), timeout );
		} else {
			xprintf_PD( f_debug, PSTR("COMMS: WARN gprs LINK status FAIL (UNKNOWN) !!\r\n\0") );
		}

		return(link_status);
	}

	return(link_status);
}
//------------------------------------------------------------------------------------
char *gprs_get_buffer_ptr( char *pattern)
{

	return( strstr( gprsRxBuffer.buffer, pattern) );
}
//------------------------------------------------------------------------------------
bool gprs_SAT_set(uint8_t modo)
{
/*
 * Seguimos viendo que luego de algún CPIN se cuelga el modem y ya aunque lo apague, luego al encenderlo
 * no responde al PIN.
 * En https://www.libelium.com/forum/viewtopic.php?t=21623 reportan algo parecido.
 * https://en.wikipedia.org/wiki/SIM_Application_Toolkit
 * Parece que el problema es que al enviar algun comando al SIM, este interactua con el STK (algun menu ) y lo bloquea.
 * Hasta no conocer bien como se hace lo dejamos sin usar.
 * " la tarjeta SIM es un ordenador diminuto con sistema operativo y programa propios.
 *   STK responde a comandos externos, por ejemplo, al presionar un botón del menú del operador,
 *   y hace que el teléfono ejecute ciertas acciones
 * "
 * https://www.techopedia.com/definition/30501/sim-toolkit-stk
 *
 * El mensaje +STIN: 25 es un mensaje no solicitado que emite el PIN STK.
 *
 * Esta rutina lo que hace es interrogar al SIM para ver si tiene la funcion SAT habilitada
 * y dar el comando de deshabilitarla
 *
 */


	xprintf_P(PSTR("GPRS: gprs SAT.(modo=%d)\r\n\0"),modo);

	switch(modo) {
	case 0:
		// Disable
		gprs_flush_RX_buffer();
		gprs_flush_TX_buffer();
		gprs_atcmd_preamble();
		xfprintf_P( fdGPRS , PSTR("AT+STK=0\r\0"));
		vTaskDelay( (portTickType)( 5000 / portTICK_RATE_MS ) );
		break;
	case 1:
		// Enable
		gprs_flush_RX_buffer();
		gprs_flush_TX_buffer();
		gprs_atcmd_preamble();
		xfprintf_P( fdGPRS , PSTR("AT+STK=1\r\0"));
		vTaskDelay( (portTickType)( 5000 / portTICK_RATE_MS ) );
		break;
	case 2:
		// Check. Query STK status ?
		xprintf_P(PSTR("GPRS: query STK status ?\r\n\0"));
		gprs_flush_RX_buffer();
		gprs_flush_TX_buffer();
		gprs_atcmd_preamble();
		xfprintf_P( fdGPRS , PSTR("AT+STK?\r\0"));
		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
		gprs_print_RX_buffer(true);
		break;
	default:
		return(false);
	}

	return (true);

}
//------------------------------------------------------------------------------------
void gprs_switch_to_command_mode(bool f_debug)
{
	/*
	 * Para switchear de data mode a command mode, DTR=0 al menos 1s.
	 */

//uint8_t tryes;
int8_t timeout;

	gprs_flush_RX_buffer();
	gprs_flush_TX_buffer();

	xprintf_PD( f_debug,  PSTR("COMMS: back to CMD.\r\n\0"));

	gprs_atcmd_preamble();
	xfprintf_P( fdGPRS, PSTR("AT\r"));
	vTaskDelay( (portTickType)( 500 / portTICK_RATE_MS ) );
	if ( gprs_check_response("OK")) {
		xprintf_PD( f_debug,  PSTR("COMMS: back to CMD OK.\r\n\0"));
		return;
	}

	// No esto en modo comando. Lo fuerzo.
	timeout = 0;
	while ( timeout++ < TIMEOUT_SWITCHCMDMODE ) {

		vTaskDelay( (portTickType)( 1500 / portTICK_RATE_MS ) );
		xfprintf_P( fdGPRS , PSTR("+++"));
		IO_clr_GPRS_DTR();
		vTaskDelay( (portTickType)( 1500 / portTICK_RATE_MS ) );
		IO_set_GPRS_DTR();
		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
		xfprintf_P( fdGPRS, PSTR("AT\r"));
		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
		if ( gprs_check_response("OK")) {
			xprintf_PD( f_debug,  PSTR("COMMS: back to CMD OK en [%d] secs.\r\n\0"), timeout * 4);
			return;
		}
	}
	xprintf_PD( f_debug,  PSTR("COMMS: back to CMD FAIL !!.\r\n\0"));

	/*
	tryes = 0;
	while (tryes++ <  MAX_TRYES_SWITCHCMDMODE ) {

		gprs_flush_RX_buffer();
		gprs_flush_TX_buffer();

		vTaskDelay( (portTickType)( 1500 / portTICK_RATE_MS ) );
		xfprintf_P( fdGPRS , PSTR("+++"));
		IO_clr_GPRS_DTR();
		vTaskDelay( (portTickType)( 1500 / portTICK_RATE_MS ) );
		IO_set_GPRS_DTR();

		timeout = 0;
		while ( timeout++ < TIMEOUT_NETOPEN ) {
			if ( gprs_check_response("OK")) {
				xprintf_PD( f_debug,  PSTR("COMMS: back to command mode OK.\r\n\0"));
				return;
			}
			vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
		}
	}
	xprintf_PD( f_debug,  PSTR("COMMS: back to command mode FAIL !!.\r\n\0"));
	return;
	*/

}
//------------------------------------------------------------------------------------
// COMANDOS NO USADOS
/*
void gprs_CGMI( bool f_debug )
{
	// Pide identificador del fabricante

uint8_t timeout;
bool retS = false;

	xprintf_PD( f_debug,  PSTR("COMMS: gprs CGMI\r\n"));
	gprs_flush_RX_buffer();
	gprs_flush_TX_buffer();
	gprs_atcmd_preamble();
	xfprintf_P( fdGPRS,PSTR("AT+CGMI\r\0"));

	timeout = 0;
	while ( timeout++ < TIMEOUT_CGMI ) {
		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
		if ( gprs_check_response("OK") ) {
			retS = true;
			break;
		}
	}

	gprs_print_RX_buffer(f_debug);
	if ( retS ) {
		xprintf_PD( f_debug, PSTR("COMMS: gprs CGMI OK.\r\n\0"));
	} else {
		gprs_AT();
		xprintf_PD( f_debug, PSTR("COMMS: WARN gprs CGMI FAIL !!.\r\n\0") );
	}

	return;
}
//------------------------------------------------------------------------------------
void gprs_CGMM( bool f_debug )
{
	// Pide identificador del modelo

uint8_t timeout;
bool retS = false;

	xprintf_PD( f_debug,  PSTR("COMMS: gprs CGMM\r\n"));
	gprs_flush_RX_buffer();
	gprs_flush_TX_buffer();
	gprs_atcmd_preamble();
	xfprintf_P( fdGPRS,PSTR("AT+CGMM\r\0"));

	timeout = 0;
	while ( timeout++ < TIMEOUT_CGMM ) {
		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
		if ( gprs_check_response("OK") ) {
			retS = true;
			break;
		}
	}

	gprs_print_RX_buffer(f_debug);
	if ( retS ) {
		xprintf_PD( f_debug, PSTR("COMMS: gprs CGMM OK.\r\n\0"));
	} else {
		gprs_AT();
		xprintf_PD( f_debug, PSTR("COMMS: WARN gprs CGMM FAIL !!.\r\n\0") );
	}

	return;

}
//------------------------------------------------------------------------------------
void gprs_CGMR( bool f_debug )
{
	// Pide identificador de revision


uint8_t timeout;
bool retS = false;

	xprintf_PD( f_debug,  PSTR("COMMS: gprs CGMR\r\n"));
	gprs_flush_RX_buffer();
	gprs_flush_TX_buffer();
	gprs_atcmd_preamble();
	xfprintf_P( fdGPRS,PSTR("AT+CGMR\r\0"));

	timeout = 0;
	while ( timeout++ < TIMEOUT_CGMR ) {
		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
		if ( gprs_check_response("OK") ) {
			retS = true;
			break;
		}
	}

	gprs_print_RX_buffer(f_debug);
	if ( retS ) {
		xprintf_PD( f_debug, PSTR("COMMS: gprs CGMR OK.\r\n\0"));
	} else {
		gprs_AT();
		xprintf_PD( f_debug, PSTR("COMMS: WARN gprs CGMR FAIL !!.\r\n\0") );
	}

	return;


}
//------------------------------------------------------------------------------------
bool gprs_CGSN( bool f_debug )
{
	// Leo el imei del modem para poder trasmitirlo al server y asi
	// llevar un control de donde esta c/sim.
	// Solo lo intento una vez porque no tranca nada el no tenerlo.

uint8_t tryes;
int8_t timeout;
bool retS = false;
char *p;

	xprintf_PD( f_debug,  PSTR("COMMS: gprs CGSN\r\n"));
	tryes = 0;
	while ( tryes++ < MAX_TRYES_CGSN ) {
		// Envio el comando
		gprs_flush_RX_buffer();
		gprs_flush_TX_buffer();
		xprintf_PD( f_debug, PSTR("GPRS: gprs send CGSN (%d)\r\n\0"),tryes );
		gprs_atcmd_preamble();
		xfprintf_P( fdGPRS , PSTR("AT+CGSN\r\0"));

		timeout = 0;
		while ( timeout++ < TIMEOUT_CGSN ) {
			vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );

			if ( gprs_check_response("ERROR") ) {
				// Salgo y reintento el comando.
				break;
			}

			if ( gprs_check_response("OK") ) {
				gprs_print_RX_buffer( f_debug);
				p = strstr(gprsRxBuffer.buffer,"AT+CGSN");
				retS = pv_get_token(p, gprs_status.buff_gprs_imei, IMEIBUFFSIZE );
				goto EXIT;
			}
		}

		// Sali por TO/ERROR/rsp.no correcta: Muestro la respuesta y reintento.
		gprs_print_RX_buffer( f_debug);
	}

EXIT:

	if (retS) {
		xprintf_PD( f_debug,  PSTR("COMMS: gprs CGSN OK  imei [%s] en [%d] secs\r\n\0"), gprs_status.buff_gprs_imei, timeout );
	} else {
		gprs_AT();
		xprintf_PD( f_debug,  PSTR("COMMS: ERROR gprs CGSN FAIL !!.\r\n\0"));
	}

	return(retS);
}
//------------------------------------------------------------------------------------
void gprs_CFGRI (bool f_debug)
{
	// DEPRECATED !!!
	// Configura para que RI sea ON y al recibir un SMS haga un pulso

//uint8_t pin;

	gprs_flush_RX_buffer();
	gprs_flush_TX_buffer();
	gprs_atcmd_preamble();
	xfprintf_P( fdGPRS,PSTR("AT+CFGRI=1,1\r\0"));
	vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
	gprs_print_RX_buffer(f_debug);

	// Reseteo el RI
	gprs_flush_RX_buffer();
	gprs_flush_TX_buffer();
	gprs_atcmd_preamble();
	xfprintf_P( fdGPRS,PSTR("AT+CRIRS\r\0"));
	vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
	gprs_print_RX_buffer(f_debug);

	vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );

	// Leo el RI
	//pin = IO_read_RI();
	//xprintf_P( PSTR("RI=%d\r\n\0"),pin);

}
//------------------------------------------------------------------------------------
bool gprs_CSOCKSETPN( bool f_debug )
{

	// Como puedo tener varios PDP definidos, indico cual va a ser el que se deba activar
	// al usar el comando NETOPEN.

uint8_t tryes;
int8_t timeout;
bool retS = false;

	xprintf_PD( f_debug, PSTR("COMMS: gprs CSOCKSETPN (set PDP)\r\n\0") );

	//Defino el PDP indicando cual es el APN.
	tryes = 0;
	while (tryes++ <  MAX_TRYES_SOCKSETPN ) {
		gprs_flush_RX_buffer();
		gprs_flush_TX_buffer();
		xprintf_P(PSTR("GPRS: gprs send CSOCKSETPN (%d)\r\n\0"),tryes);
		gprs_atcmd_preamble();
		xfprintf_P( fdGPRS,PSTR("AT+CSOCKSETPN=1\r\0"));
		timeout = 0;
		while ( timeout++ < TIMEOUT_SOCKSETPN ) {
			vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
			if ( gprs_check_response("OK") ) {
				retS = true;
				goto EXIT;
			}
			if ( gprs_check_response("ERROR") ) {
				retS = false;
				goto EXIT;
			}
		}

		gprs_print_RX_buffer( f_debug);
		gprs_AT();
	}

EXIT:

	gprs_print_RX_buffer( f_debug);
	if (retS) {
		xprintf_PD( f_debug,  PSTR("COMMS: gprs CSOCKSETPN OK en [%d] secs\r\n\0"), (( 10 * (tryes-1) ) + timeout) );
	} else {
		gprs_AT();
		xprintf_PD( f_debug,  PSTR("COMMS: ERROR gprs CSOCKSETPN FAIL !!.\r\n\0"));
	}

	return(retS);
}
//------------------------------------------------------------------------------------
*/

