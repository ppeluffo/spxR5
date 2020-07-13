/*
 * tkComms_GPRS.c
 *
 *  Created on: 5 mar. 2020
 *      Author: pablo
 */

#include "tkComms.h"

const char apn_spy[] PROGMEM = "SPYMOVIL.VPNANTEL";		// SPYMOVIL | UTE | TAHONA
const char apn_ose[] PROGMEM = "STG1.VPNANTEL";			// OSE
const char apn_claro[] PROGMEM = "ipgrs.claro.com.uy";	// CLARO

const char ip_server_spy1[] PROGMEM = "192.168.0.9\0";		// SPYMOVIL
const char ip_server_spy2[] PROGMEM = "190.64.69.34\0";		// SPYMOVIL PUBLICA (CLARO)
const char ip_server_ose[] PROGMEM = "172.27.0.26\0";		// OSE
const char ip_server_ute[] PROGMEM = "192.168.1.9\0";		// UTE

//const char * const scan_list1[] PROGMEM = { apn_spy, ip_server_spy1 };

PGM_P const scan_list1[] PROGMEM = { apn_spy, ip_server_spy1 };
PGM_P const scan_list2[] PROGMEM = { apn_spy, ip_server_ute };
PGM_P const scan_list3[] PROGMEM = { apn_ose, ip_server_ose };
PGM_P const scan_list4[] PROGMEM = { apn_claro, ip_server_spy2 };

/*
 * Para testing
PGM_P const scan_list4[] PROGMEM = { apn_spy, ip_server_spy1 };
PGM_P const scan_list3[] PROGMEM = { apn_spy, ip_server_ute };
PGM_P const scan_list2[] PROGMEM = { apn_ose, ip_server_ose };
PGM_P const scan_list1[] PROGMEM = { apn_claro, ip_server_spy2 };
*/

typedef enum { prender_RXRDY = 0, prender_HW, prender_SW, prender_CPAS, prender_CFUN, prender_EXIT } t_states_fsm_prender_gprs;

#define MAX_SW_TRYES_PRENDER	3
#define MAX_HW_TRYES_PRENDER	3
#define MAX_TRYES_CPAS			4
#define MAX_TRYES_CFUN			3
#define MAX_TRYES_CPIN			3
#define MAX_TRYES_CREG			3
#define MAX_TRYES_PDPATTACH		5
#define MAX_TRYES_SOCKCONT		3
#define MAX_TRYES_SOCKSETPN		3
#define MAX_TRYES_NETCLOSE		3
#define MAX_TRYES_NETOPEN		3

#define TIMEOUT_MODEM_BOOT	30
#define TIMEOUT_CFUN		10
#define TIMEOUT_CPAS		15
#define TIMEOUT_CGMI		10
#define TIMEOUT_CGMM		10
#define TIMEOUT_CGMR		10
#define TIMEOUT_IFC			10
#define TIMEOUT_IMEI		10
#define TIMEOUT_CCID		10
#define TIMEOUT_CPIN		15
#define TIMEOUT_CREG		30
#define TIMEOUT_PDPATTACH	20
#define TIMEOUT_CIPMODE		10
#define TIMEOUT_CMGF		10
#define TIMEOUT_DCDMODE		10
#define TIMEOUT_CSQ			10
#define TIMEOUT_SOCKCONT	15
#define TIMEOUT_SOCKSETPN	15
#define TIMEOUT_PDP			15
#define TIMEOUT_NETCLOSE	10
#define TIMEOUT_NETOPEN		30
#define TIMEOUT_READIP		10
#define TIMEOUT_SOCKCLOSE	25
#define TIMEOUT_SOCK2OPEN	15


bool gprs_scan_try (t_scan_struct *scan_boundle, PGM_P *dlist );
bool gprs_send_scan_frame( t_scan_struct *scan_boundle, char *ip_tmp );
bool gprs_process_scan_response(t_scan_struct *scan_boundle);
void gprs_extract_dlgid(t_scan_struct *scan_boundle);

bool gprs_PBDONE( bool f_debug );
bool gprs_CFUN( bool f_debug );
bool gprs_CPAS( bool f_debug );

void gprs_CGMI( bool f_debug );
void gprs_CGMM( bool f_debug );
void gprs_CGMR( bool f_debug );
void gprs_IFC( bool f_debug );
bool pv_get_token( char *p, char *buff, uint8_t size);
void gprs_CIPMODE(bool f_debug);
void gprs_DCDMODE( bool f_debug );
void gprs_CMGF( bool f_debug );
void gprs_CFGRI (bool f_debug);

bool gprs_CGSOCKCONT( bool f_debug, char *apn);
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
bool retS = false;

	if ( strstr( gprsRxBuffer.buffer, rsp) != NULL ) {
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
	IO_clr_GPRS_SW();	// Es un FET que lo dejo cortado
	vTaskDelay( (portTickType)( 100 / portTICK_RATE_MS ) );

	IO_clr_GPRS_PWR();	// Apago la fuente.
	vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );

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

	while(1) {

		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );

		switch (state ) {
		case prender_RXRDY:
			xprintf_PD( f_debug, PSTR("COMMS: gprs modem pwr_on RXDDY.\r\n\0"));
			// Aviso a la tarea de RX que se despierte ( para leer las respuestas del AT ) !!!
			while ( xTaskNotify( xHandle_tkCommsRX, SGN_WAKEUP , eSetBits ) != pdPASS ) {
				vTaskDelay( ( TickType_t)( 100 / portTICK_RATE_MS ) );
			}
			// Terminarlo

			hw_tryes = 0;
			state = prender_HW;
			break;

		case prender_HW:
			xprintf_PD( f_debug, PSTR("COMMS: gprs modem pwr_on HW(%d).\r\n\0"),hw_tryes);
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
			xprintf_PD( f_debug, PSTR("COMMS: gprs modem pwr_on SW(%d).\r\n\0"),sw_tryes);
			if ( sw_tryes >= MAX_SW_TRYES_PRENDER ) {
				// Apago el HW y espero
				gprs_apagar();
				vTaskDelay( (portTickType)( 10000 / portTICK_RATE_MS ) );
				state = prender_HW;
			} else {
				// Genero un pulso en el pin SW para prenderlo logicamente
				sw_tryes++;
				gprs_sw_pwr();
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
			xprintf_PD( f_debug, PSTR("COMMS: gprs modem CPAS.\r\n\0"));
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
			xprintf_PD( f_debug, PSTR("COMMS: gprs modem CFUN.\r\n\0"));
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
	 */

uint8_t tryes;
int8_t timeout;
bool retS = false;

	tryes = 0;
	while ( tryes++ < MAX_TRYES_CFUN ) {

		// Envio el comando
		gprs_flush_RX_buffer();
		gprs_flush_TX_buffer();
		xprintf_PD( f_debug, PSTR("GPRS: gprs send CFUN (%d)\r\n\0"),tryes );
		xfprintf_P( fdGPRS , PSTR("AT+CFUN?\r\0"));

		timeout = 0;
		while ( timeout++ < TIMEOUT_CFUN ) {
			vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
			if ( gprs_check_response("ERROR") ) {
				// Salgo y reintento el comando.
				break;
			}
			if ( gprs_check_response("+CFUN: 1") ) {
				// Respuesta correcta.
				gprs_print_RX_buffer( f_debug);
				retS = true;
				goto EXIT;
			}
		}
		// Sali por TO. Muestro la respuesta y reintento.
		gprs_print_RX_buffer( f_debug);
	}

	EXIT:

	if (retS) {
		xprintf_PD( f_debug,  PSTR("COMMS: gprs CFUN OK en [%d] secs\r\n\0"), (( 10 * (tryes-1) ) + timeout) );
	} else {
		xprintf_PD( f_debug,  PSTR("COMMS: gprs CFUN FAIL !!.\r\n\0"));
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

	tryes = 0;
	while ( tryes++ < MAX_TRYES_CPAS ) {

		// Envio el comando
		gprs_flush_RX_buffer();
		gprs_flush_TX_buffer();
		xprintf_PD( f_debug, PSTR("GPRS: gprs send CPAS (%d)\r\n\0"), tryes );
		xfprintf_P( fdGPRS , PSTR("AT+CPAS\r\0"));

		timeout = 0;
		while ( timeout++ < TIMEOUT_CPAS ) {
			vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
			if ( gprs_check_response("ERROR") ) {
				// Salgo y reintento el comando.
				break;
			}
			if ( gprs_check_response("+CPAS: 0") ) {
				// Respuesta correcta.
				gprs_print_RX_buffer( f_debug);
				retS = true;
				goto EXIT;
			}
		}
		// Sali por TO. Muestro la respuesta y reintento.
		gprs_print_RX_buffer( f_debug);
	}

	EXIT:

	if (retS) {
		xprintf_PD( f_debug,  PSTR("COMMS: gprs CPAS OK en [%d] secs\r\n\0"), (( 10 * (tryes-1) ) + timeout) );
	} else {
		xprintf_PD( f_debug,  PSTR("COMMS: gprs CPAS FAIL !!.\r\n\0"));
	}

	return(retS);
}
//------------------------------------------------------------------------------------
bool gprs_configurar_dispositivo( bool f_debug, char *pin, uint8_t *err_code )
{
	/*
	 * Consiste en enviar los comandos AT de modo que el modem GPRS
	 * quede disponible para trabajar
	 */

	gprs_CGMI( f_debug );
	gprs_CGMM( f_debug );
	gprs_CGMR( f_debug );
	gprs_IFC( f_debug );

	// AT+CGSN
	if ( gprs_readImei(f_debug) == false ) {
		gprs_apagar();
		xprintf_PD( f_debug, PSTR("COMMS: gprs ERROR: Imei not available!!\r\n\0"));
		//return (false );
	}

	// AT+CCID
	if ( gprs_readCcid(f_debug) == false ) {
		gprs_apagar();
		xprintf_PD( f_debug, PSTR("COMMS: gprs ERROR: Ccid not available!!\r\n\0"));
		//return (false );
	}

	gprs_CIPMODE(f_debug);	// modo transparente.
	gprs_DCDMODE(f_debug);	// UART para utilizar las 7 lineas
	gprs_CMGF(f_debug);		// Configuro para mandar SMS en modo TEXTO
	//gprs_CFGRI(f_debug);	// Configuro el RI para que sea ON y al llegar un SMS sea OFF


	// AT+CPIN?
	// Vemos que halla un pin presente.
	if ( ! gprs_CPIN( f_debug, pin) ) {
		*err_code = ERR_CPIN_FAIL;
		return(false);
	}

	// Vemos que el modem este registrado en la red. Pude demorar hasta 1 minuto ( CLARO )
	if ( ! gprs_CGREG(f_debug) ) {
		*err_code = ERR_CREG_FAIL;
		return(false);
	}

	// Me debo atachear a la red. Con esto estoy conectado a la red movil pero
	// aun no puedo trasmitir datos.
	if ( ! gprs_CGATT(f_debug) ) {
		*err_code = ERR_NETATTACH_FAIL;
		return(false);
	}

	*err_code = ERR_NONE;
	return(true);
}
//--------------------------------------------------------------------------------------
void gprs_CGMI( bool f_debug )
{
	// Pide identificador del fabricante

uint8_t timeout;
bool retS = false;

	xprintf_PD( f_debug,  PSTR("COMMS: gprs CGMI\r\n"));
	gprs_flush_RX_buffer();
	gprs_flush_TX_buffer();
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
		xprintf_PD( f_debug, PSTR("COMMS: WARN gprs CGMI failed !!!\r\n\0") );
	}

	return;
}
//------------------------------------------------------------------------------------
void gprs_CGMM( bool f_debug )
{
	// Pide identificador del modelo

uint8_t timeout;
bool retS = false;

	xprintf_PD( f_debug,  PSTR("COMMS: gprs CGMI\r\n"));
	gprs_flush_RX_buffer();
	gprs_flush_TX_buffer();
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
		xprintf_PD( f_debug, PSTR("COMMS: WARN gprs CGMM failed !!!\r\n\0") );
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
		xprintf_PD( f_debug, PSTR("COMMS: WARN gprs CGMR failed !!!\r\n\0") );
	}

	return;


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
	xfprintf_P( fdGPRS,PSTR("AT+IFC?\r\0"));
	timeout = 0;
	while ( timeout++ < TIMEOUT_IFC ) {
		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
		if ( gprs_check_response("+IFC: 0,0")) {
			retS = true;
			break;
		}
	}

	gprs_print_RX_buffer(f_debug);
	if ( retS ) {
		xprintf_P( PSTR("COMMS: gprs IFC OK (flow control NONE)\r\n\0") );
	} else {
		xprintf_P( PSTR("COMMS: WARN: gprs IFC flow control FAIL !!!\r\n\0") );
	}

	return;

}
//------------------------------------------------------------------------------------
bool gprs_readImei( bool f_debug )
{
	// Leo el imei del modem para poder trasmitirlo al server y asi
	// llevar un control de donde esta c/sim.
	// Solo lo intento una vez porque no tranca nada el no tenerlo.

bool retS = false;
char *p;
uint8_t timeout;

	// Envio un AT+CGSN para leer el IMEI
	gprs_flush_RX_buffer();
	gprs_flush_TX_buffer();
	xfprintf_P( fdGPRS,PSTR("AT+CGSN\r\0"));
	timeout = 0;
	while ( timeout++ < TIMEOUT_IMEI ) {
		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
		if ( gprs_check_response("OK") ) {
			p = strstr(gprsRxBuffer.buffer,"AT+CGSN");
			retS = pv_get_token(p, gprs_status.buff_gprs_imei, IMEIBUFFSIZE );
			break;
		}
	}

	gprs_print_RX_buffer(f_debug);

	if (retS) {
		xprintf_PD( f_debug,  PSTR("COMMS: gprs IMEI [%s] OK en [%d] secs\r\n\0"), gprs_status.buff_gprs_imei, timeout );
	} else {
		xprintf_PD( f_debug,  PSTR("COMMS: ERROR gprs IMEI Failed\r\n\0"));
	}

	return(retS);
}
//------------------------------------------------------------------------------------
bool gprs_readCcid( bool f_debug )
{
	// Leo el ccid del sim para poder trasmitirlo al server y asi
	// llevar un control de donde esta c/sim
	// AT+CCID
	// +CCID: "8959801611637152574F"
	//
	// OK

bool retS = false;
char *p;
uint8_t timeout;

	// Envio un AT+CCID para leer el CCID
	gprs_flush_RX_buffer();
	gprs_flush_TX_buffer();
	xfprintf_P( fdGPRS,PSTR("AT+CCID\r\0"));
	timeout = 0;
	while ( timeout++ < TIMEOUT_CCID ) {
		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
		if ( gprs_check_response("OK") ) {
			p = strstr(gprsRxBuffer.buffer,"AT+CCID");
			retS = pv_get_token(p, gprs_status.buff_gprs_ccid, CCIDBUFFSIZE );
			break;
		}
	}

	gprs_print_RX_buffer(f_debug);

	if (retS) {
		xprintf_PD( f_debug,  PSTR("COMMS: gprs CCID OK [%s] OK en [%d] secs\r\n\0"), gprs_status.buff_gprs_ccid, timeout );
	} else {
		xprintf_PD( f_debug,  PSTR("COMMS: ERROR gprs CCID Failed\r\n\0"));
	}

	return(retS);
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

	memset( buff, '\0', IMEIBUFFSIZE);

	// Start. Busco el primer caracter.
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
void gprs_CIPMODE(bool f_debug)
{
	// Funcion que configura el modo transparente.

uint8_t timeout;
bool retS = false;

	xprintf_PD( f_debug,  PSTR("COMMS: gprs CIPMODE\r\n"));
	gprs_flush_RX_buffer();
	gprs_flush_TX_buffer();
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
		xprintf_PD( f_debug, PSTR("COMMS: WARN gprs CIPMODE failed !!!\r\n\0") );
	}

	return;
}
//------------------------------------------------------------------------------------
void gprs_DCDMODE( bool f_debug )
{
	// Funcion que configura el UART para utilizar las 7 lineas y el DCD indicando
	// el estado del socket.

uint8_t timeout;
bool retS = false;

	/*
	 * Esto permite que la linea DTR permita switchear entre modo comando y modo
	 * datos en el modo transparente de la uart.
	 * Con DTR=0 salgo del modo data y paso al modo comandos.
	 */

	gprs_flush_RX_buffer();
	gprs_flush_TX_buffer();
	xfprintf_P( fdGPRS,PSTR("AT&D1\r\0"));
	timeout = 0;
	while ( timeout++ < TIMEOUT_DCDMODE ) {
		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
		if ( gprs_check_response("OK") ) {
			retS = true;
			break;
		}
	}

	gprs_print_RX_buffer(f_debug);
	if ( retS ) {
		xprintf_PD( f_debug, PSTR("COMMS: gprs &D1 OK en [%d] secs\r\n\0"), timeout );
	} else {
		xprintf_PD( f_debug, PSTR("COMMS: WARN gprs &D1 failed !!!\r\n\0") );
	}

	/*
	 * Pongo la uart en modo 7 lineas.
	 */
	gprs_flush_RX_buffer();
	gprs_flush_TX_buffer();
	xfprintf_P( fdGPRS,PSTR("AT+CSUART=1\r\0"));
	timeout = 0;
	while ( timeout++ < TIMEOUT_DCDMODE ) {
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
		xprintf_PD( f_debug, PSTR("COMMS: WARN gprs CSUART failed !!!\r\n\0") );
	}

	/*
	 * Uso el pin DCD para indicar la conexion.
	 * DCD = 0: link up
	 * DCD = 1: link down
	 */
	gprs_flush_RX_buffer();
	gprs_flush_TX_buffer();
	xfprintf_P( fdGPRS,PSTR("AT&C1\r\0"));
	timeout = 0;
	while ( timeout++ < TIMEOUT_DCDMODE ) {
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
		xprintf_PD( f_debug, PSTR("COMMS: WARN gprs &C1 failed !!!\r\n\0") );
	}

	return;
}
//------------------------------------------------------------------------------------
void gprs_CMGF( bool f_debug )
{
	// Configura para mandar SMS en modo texto

uint8_t timeout;
bool retS = false;

	xprintf_PD( f_debug,  PSTR("COMMS: gprs CMGF\r\n"));
	gprs_flush_RX_buffer();
	gprs_flush_TX_buffer();
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
		xprintf_PD( f_debug, PSTR("COMMS: gprs CMGF OK en [%d] secs\r\n\0"), timeout );
	} else {
		xprintf_PD( f_debug, PSTR("COMMS: WARN gprs CMGFE failed !!!\r\n\0") );
	}

	return;
}
//------------------------------------------------------------------------------------
void gprs_CFGRI (bool f_debug)
{
	// Configura para que RI sea ON y al recibir un SMS haga un pulso

//uint8_t pin;

	gprs_flush_RX_buffer();
	gprs_flush_TX_buffer();
	xfprintf_P( fdGPRS,PSTR("AT+CFGRI=1,1\r\0"));
	vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
	gprs_print_RX_buffer(f_debug);

	// Reseteo el RI
	gprs_flush_RX_buffer();
	gprs_flush_TX_buffer();
	xfprintf_P( fdGPRS,PSTR("AT+CRIRS\r\0"));
	vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
	gprs_print_RX_buffer(f_debug);

	vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );

	// Leo el RI
	//pin = IO_read_RI();
	//xprintf_P( PSTR("RI=%d\r\n\0"),pin);

}
//------------------------------------------------------------------------------------
bool gprs_CPIN( bool f_debug, char *pin )
{
	// Chequeo que el SIM este en condiciones de funcionar.
	// AT+CPIN?
	// No configuro el PIN !!!
	// Debe responder en 5s.

uint8_t tryes;
int8_t timeout;
bool retS = false;

	xprintf_PD( f_debug, PSTR("GPRS: gprs CPIN\r\n\0"));

	tryes = 0;
	while ( tryes++ < MAX_TRYES_CPIN ) {
		// Vemos si necesita SIMPIN
		gprs_flush_RX_buffer();
		gprs_flush_TX_buffer();
		xfprintf_P( fdGPRS , PSTR("AT+CPIN?\r\0"));

		timeout = 0;
		while ( timeout++ < TIMEOUT_CPIN ) {
			vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
			if ( gprs_check_response("READY") ) {
				retS = true;
				goto EXIT;
			}
			if ( gprs_check_response("ERROR") ) {
				retS = false;
				goto EXIT;
			}
		}

		gprs_print_RX_buffer( f_debug);
		xprintf_P(PSTR("GPRS: gprs CPIN retry (%d)\r\n\0"),tryes);
	}

EXIT:

	gprs_print_RX_buffer( f_debug);

	if (retS) {
		xprintf_PD( f_debug,  PSTR("COMMS: gprs CPIN OK en [%d] secs\r\n\0"), (( 10 * (tryes-1) ) + timeout) );
	} else {
		xprintf_PD( f_debug,  PSTR("COMMS: gprs CPIN FAIL !!.\r\n\0"));
	}

	return(retS);
}
//------------------------------------------------------------------------------------
bool gprs_CGREG( bool f_debug )
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
	*/

bool retS = false;
uint8_t tryes;
int8_t timeout;

	xprintf_PD( f_debug,  PSTR("COMMS: gprs NET registation\r\n\0"));

	tryes = 0;
	while ( tryes++ < MAX_TRYES_CREG ) {

		gprs_flush_RX_buffer();
		gprs_flush_TX_buffer();
		xfprintf_P( fdGPRS, PSTR("AT+CREG?\r\0"));

		timeout = 0;
		while ( timeout++ < TIMEOUT_CREG ) {
			vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
			gprs_print_RX_buffer( f_debug);
			if ( gprs_check_response("+CREG: 0,1") ) {
				retS = true;
				goto EXIT;
			}
			if ( gprs_check_response("ERROR") ) {
				retS = false;
				goto EXIT;
			}
		}

		xprintf_P(PSTR("GPRS: gprs CREG retry (%d)\r\n\0"),tryes);
	}

EXIT:
	if (retS) {
		xprintf_PD( f_debug,  PSTR("COMMS: gprs NET registation OK en [%d] secs\r\n\0"), (( 10 * (tryes-1) ) + timeout) );
	} else {
		xprintf_PD( f_debug,  PSTR("COMMS: gprs ERROR: NET registation FAIL !!.\r\n\0"));
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
uint8_t tryes;

	xprintf_PD( f_debug,  PSTR("COMMS: gprs NET attach\r\n\0"));

	gprs_flush_RX_buffer();
	gprs_flush_TX_buffer();
	xfprintf_P( fdGPRS,PSTR("AT+CGATT=1\r\0"));

	tryes = 0;
	while ( tryes++ < MAX_TRYES_PDPATTACH ) {
		// Vemos si necesita SIMPIN
		gprs_flush_RX_buffer();
		gprs_flush_TX_buffer();
		xfprintf_P( fdGPRS , PSTR("AT+CGATT?\r\0"));

		timeout = 0;
		while ( timeout++ < TIMEOUT_PDPATTACH ) {
			vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
			if ( gprs_check_response("+CGATT: 1") ) {
				retS = true;
				goto EXIT;
			}
			if ( gprs_check_response("ERROR") ) {
				retS = false;
				goto EXIT;
			}
		}

		gprs_print_RX_buffer( f_debug);
		xprintf_P(PSTR("GPRS: gprs CGATT retry (%d)\r\n\0"),tryes);
	}

EXIT:

	gprs_print_RX_buffer( f_debug);
	if ( retS ) {
		xprintf_PD( f_debug,  PSTR("COMMS: gprs NET attached OK en [%d] secs\r\n\0"), (( 10 * (tryes-1) ) + timeout) );
	} else {
		xprintf_PD( f_debug,  PSTR("COMMS: gprs ERROR: NET attach FAIL !!.\r\n\0"));
	}
	return(retS);

}
//------------------------------------------------------------------------------------
uint8_t gprs_read_sqe( bool f_debug )
{
	// Veo la calidad de senal que estoy recibiendo

char csqBuffer[32] = { 0 };
char *ts = NULL;
uint8_t csq, dbm;
uint8_t timeout;
bool retS = false;

	csq = 0;
	dbm = 0;

	gprs_flush_RX_buffer();
	gprs_flush_TX_buffer();
	xfprintf_P( fdGPRS,PSTR("AT+CSQ\r\0"));
	timeout = 0;
	while ( timeout++ < TIMEOUT_CSQ ) {
	vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
		if ( gprs_check_response("OK") ) {
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
		xprintf_PD( f_debug,  PSTR("COMMS: gprs signalQ CSQ=%d,DBM=%d\r\n\0"), csq, dbm );
	} else {
		xprintf_PD( f_debug,  PSTR("COMMS: ERROR gprs CSQ\r\n\0") );
	}

	return(csq);
}
//------------------------------------------------------------------------------------
void gprs_mon_sqe( bool f_debug,  bool modo_continuo, uint8_t *csq)
{

uint8_t timer = 1;

	// Recien despues de estar registrado puedo leer la calidad de señal.
	*csq = gprs_read_sqe(f_debug);

	if ( ! modo_continuo ) {
		return;
	}

	// Mientras la senal este prendida, monitoreo. Salgo por watchdog.
	while ( true ) {

		vTaskDelay( ( TickType_t)( 1000 / portTICK_RATE_MS ) );

		if ( timer > 0) {	// Espero 1s contando
			timer--;
		} else {
			// Expiro: monitoreo el SQE y recargo el timer a 10 segundos
			gprs_read_sqe(f_debug);
			timer = 10;
		}
	}
}
//------------------------------------------------------------------------------------
bool gprs_net_connect(bool f_debug, char *apn, char *ip_assigned, uint8_t *err_code )
{

	/*
	 * El pedir IP es un concepto solo de GPRS
	 * - Setea el APN
	 * - Se conecta ( se le asigna una IP )
	 * - Lee la IP asignada.
	 */

	// Paso 1: Setea el APN ( Configuro el PDP context )
	if ( gprs_set_apn( f_debug, apn ) == false ) {
		return(false);
	}

	// Paso 2: Me conecto a la red ( Activo el PDP context )
	// Siempre antes de un NETOPEN doy este que cierra la conexion TCP y desactiva el PDP context.
	gprs_netclose(f_debug);

	if ( gprs_netopen(f_debug ) == false ) {
		return(false);
	}

	// Paso 3: Leo la ip asignada
	if ( gprs_read_ip_assigned(f_debug, ip_assigned ) == false ) {
		return(false);
	}

	return(true);

}
//------------------------------------------------------------------------------------
bool gprs_set_apn(bool f_debug, char *apn)
{

	// Configura el APN de trabajo.
	// Define el contexto PDP y lo activa

	xprintf_PD( f_debug, PSTR("COMMS: gprs set APN\r\n\0") );

	//Defino el PDP indicando cual es el APN.
	if ( gprs_CGSOCKCONT( f_debug, apn)) {

		if ( gprs_CSOCKSETPN(f_debug)) {
			return(true);
		}
	}

	return(false);

}
//------------------------------------------------------------------------------------
bool gprs_CGSOCKCONT( bool f_debug, char *apn)
{

uint8_t tryes;
int8_t timeout;
bool retS = false;

	xprintf_PD( f_debug, PSTR("COMMS: gprs define PDP\r\n\0") );

	//Defino el PDP indicando cual es el APN.
	tryes = 0;
	while (tryes++ <  MAX_TRYES_SOCKCONT ) {
		gprs_flush_RX_buffer();
		gprs_flush_TX_buffer();
		xfprintf_P( fdGPRS, PSTR("AT+CGSOCKCONT=1,\"IP\",\"%s\"\r\0"), apn);
		timeout = 0;
		while ( timeout++ < TIMEOUT_SOCKCONT ) {
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
		xprintf_P(PSTR("GPRS: gprs define PDP retry (%d)\r\n\0"),tryes);
	}

EXIT:

	gprs_print_RX_buffer( f_debug);
	if (retS) {
		xprintf_PD( f_debug,  PSTR("COMMS: gprs define PDP OK en [%d] secs\r\n\0"), (( 10 * (tryes-1) ) + timeout) );
	} else {
		xprintf_PD( f_debug,  PSTR("COMMS: gprs define PDP FAIL !!.\r\n\0"));
	}

	return(retS);
}
//------------------------------------------------------------------------------------
bool gprs_CSOCKSETPN( bool f_debug )
{

	// Como puedo tener varios PDP definidos, indico cual va a ser el que se deba activar
	// al usar el comando NETOPEN.

uint8_t tryes;
int8_t timeout;
bool retS = false;

	xprintf_PD( f_debug, PSTR("COMMS: gprs set PDP\r\n\0") );

	//Defino el PDP indicando cual es el APN.
	tryes = 0;
	while (tryes++ <  MAX_TRYES_SOCKSETPN ) {
		gprs_flush_RX_buffer();
		gprs_flush_TX_buffer();
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
		xprintf_P(PSTR("GPRS: gprs set PDP retry (%d)\r\n\0"),tryes);
	}

EXIT:

	gprs_print_RX_buffer( f_debug);
	if (retS) {
		xprintf_PD( f_debug,  PSTR("COMMS: gprs set PDP OK en [%d] secs\r\n\0"), (( 10 * (tryes-1) ) + timeout) );
	} else {
		xprintf_PD( f_debug,  PSTR("COMMS: gprs set PDP FAIL !!.\r\n\0"));
	}

	return(retS);
}
//------------------------------------------------------------------------------------
bool gprs_netclose(bool f_debug)
{
	// Paso a modo comando por las dudas

bool retS = false;
uint8_t tryes;
int8_t timeout;

	xprintf_PD( f_debug,  PSTR("COMMS: gprs NETclose\r\n\0"));

	gprs_switch_to_command_mode();

	tryes = 0;
	while (tryes++ <  MAX_TRYES_NETCLOSE ) {
		gprs_flush_RX_buffer();
		gprs_flush_TX_buffer();
		xfprintf_P( fdGPRS,PSTR("AT+NETCLOSE\r\0"));

		timeout = 0;
		while ( timeout++ < TIMEOUT_NETCLOSE ) {
			vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );

			if ( gprs_check_response("Network closed")) {
				retS = true;
				goto EXIT;

			} else if ( gprs_check_response("Network is already closed")) {
				retS = true;
				goto EXIT;

			} else if ( gprs_check_response("OK") ) {
				retS = true;
				goto EXIT;

			} else if ( gprs_check_response("ERROR") ) {
				retS = false;
				goto EXIT;
			}
		}

		gprs_print_RX_buffer( f_debug);
		xprintf_P(PSTR("GPRS: gprs NETclose retry (%d)\r\n\0"),tryes);
	}

EXIT:

	gprs_print_RX_buffer( f_debug);
	if (retS) {
		xprintf_PD( f_debug,  PSTR("COMMS: gprs NET closed OK en [%d] secs\r\n\0"), (( 10 * (tryes-1) ) + timeout) );
	} else {
		xprintf_PD( f_debug,  PSTR("COMMS: gprs ERROR: NET close FAIL !!.\r\n\0"));
	}

	return(retS);
}
//------------------------------------------------------------------------------------
bool gprs_netopen(bool f_debug)
{
	// Abre una conexion a la red..
	// Activa el contexto y crea el socket
	// La red asigna una IP.
	// Doy el comando para atachearme a la red
	// Puede demorar unos segundos por lo que espero para chequear el resultado
	// y reintento varias veces.

bool retS = false;
uint8_t tryes;
int8_t timeout;

	xprintf_PD( f_debug,  PSTR("COMMS: gprs NETclose\r\n\0"));

	gprs_switch_to_command_mode();

	tryes = 0;
	while (tryes++ <  MAX_TRYES_NETOPEN ) {

		xprintf_PD( f_debug,  PSTR("COMMS: gprs netopen sent(%d)\r\n\0"), tryes );
		gprs_flush_RX_buffer();
		gprs_flush_TX_buffer();
		xfprintf_P( fdGPRS,PSTR("AT+NETOPEN=\"TCP\"\r\0"));

		timeout = 0;
		while ( timeout++ < TIMEOUT_NETOPEN ) {
			vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );

			// Evaluo las respuestas del modem.
			if ( gprs_check_response("Network opened")) {
				retS = true;
				goto EXIT;

			} else if ( gprs_check_response("+IP ERROR: Network is already opened")) {
				retS = true;
				goto EXIT;

			} else if ( gprs_check_response("+NETOPEN: 0")) {
				retS = true;
				goto EXIT;

			} else if ( gprs_check_response("+NETOPEN: 1")) {
				retS = false;
				goto EXIT;

			} else if ( gprs_check_response("ERROR")) {
				retS = false;
				goto EXIT;
			}
		}

		gprs_print_RX_buffer( f_debug);
		xprintf_P(PSTR("GPRS: gprs NETopen retry (%d)\r\n\0"),tryes);
	}

EXIT:

	gprs_print_RX_buffer( f_debug);
	if (retS) {
		xprintf_PD( f_debug,  PSTR("COMMS: gprs NET open OK en [%d] secs\r\n\0"), (( 10 * (tryes-1) ) + timeout) );
	} else {
		xprintf_PD( f_debug,  PSTR("COMMS: gprs ERROR: NET open FAIL !!.\r\n\0"));
	}

	return(retS);

}
//------------------------------------------------------------------------------------
bool gprs_read_ip_assigned(bool f_debug, char *ip_assigned )
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

	strcpy(ip_assigned, "0.0.0.0\0");
	gprs_flush_RX_buffer();
	gprs_flush_TX_buffer();
	xfprintf_P( fdGPRS,PSTR("AT+IPADDR\r\0"));
	timeout = 0;
	while ( timeout++ < TIMEOUT_READIP ) {
		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );

		if ( gprs_check_response("+IP ERROR: Network not opened")) {
			// No tengo la IP.
			retS = false;
			break;
		}

		if ( gprs_check_response("+IPADDR:")) {
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

	gprs_print_RX_buffer(f_debug);

	if ( retS ) {
		xprintf_PD( f_debug,  PSTR("COMMS: gprs ip address=[%s]\r\n\0"), ip_assigned );
	} else {
		xprintf_P( PSTR("COMMS: gprs ERROR: ip no asignada !!.\r\n\0") );
	}

	return(retS);

}
//------------------------------------------------------------------------------------
t_link_status gprs_open_socket(bool f_debug, char *ip, char *port)
{

uint8_t timeout = 0;
t_link_status link_status = LINK_FAIL;

	xprintf_PD( f_debug, PSTR("COMMS: gprs try to open socket\r\n\0"));

	// Paso a modo comando
	gprs_switch_to_command_mode();

	// El socket debe estar cerrado.
	// Espero hasta 30s que este cerrado.
	timeout = 0;
	while ( timeout++ < TIMEOUT_SOCKCLOSE ) {
		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
		if ( IO_read_DCD() == 1)
			break;
	}

	// Intento abrir el socket una sola vez
	gprs_flush_RX_buffer();
	gprs_flush_TX_buffer();
	xfprintf_P( fdGPRS, PSTR("AT+TCPCONNECT=\"%s\",%s\r\n\0"), ip, port);
	vTaskDelay( (portTickType)( 1500 / portTICK_RATE_MS ) );

	// Espero hasta 15s la respuesta
	timeout = 0;
	while ( timeout++ < TIMEOUT_SOCK2OPEN ) {

		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );

		if ( gprs_check_response("CONNECT 115200")) {
			link_status = LINK_OPEN;
			//xprintf_PD( f_debug , PSTR("COMMS: gprs sckt is open (connect 115200)\r\n\0"));
			break;

		} else if ( gprs_check_response("Operation not supported")) {
			link_status = LINK_ERROR;
			//xprintf_PD( f_debug , PSTR("COMMS: gprs sckt ERROR\r\n\0"));
			break;

		} else if ( gprs_check_response("ERROR")) {
			link_status = LINK_ERROR;
			//xprintf_PD( f_debug, PSTR("COMMS: gprs sckt ERROR\r\n\0"));
			break;

		} else if ( gprs_check_response("FAIL")) {
			link_status = LINK_ERROR;
			//xprintf_PD( f_debug, PSTR("COMMS: gprs sckt CONNECT FAIL\r\n\0"));
			break;

		} else if ( gprs_check_response("+CIPEVENT:")) {
			// El socket no se va a recuperar. Hay que cerrar el net y volver a abrirlo.
			link_status = LINK_FAIL;
			//xprintf_PD( f_debug, PSTR("COMMS: gprs sckt FAIL +CIPEVENT:\r\n\0"));
			break;

		} else if ( gprs_check_response("+CIPOPEN: 0,2")) {
			// El socket no se va a recuperar. Hay que cerrar el net y volver a abrirlo.
			link_status = LINK_FAIL;
			//xprintf_PD( f_debug, PSTR("COMMS: gprs sckt FAIL +CIPOPEN: 0,2\r\n\0"));
			break;

		} else if ( gprs_check_response("+IP ERROR: Operation not supported")) {
			// El socket no se va a recuperar. Hay que cerrar el net y volver a abrirlo.
			link_status = LINK_FAIL;
			//xprintf_PD( f_debug, PSTR("COMMS: gprs sckt FAIL: Operation not supported\r\n\0"));
			break;
		}

	}

	gprs_print_RX_buffer(f_debug);

	if (link_status == LINK_OPEN ) {
		xprintf_PD( f_debug,  PSTR("COMMS: gprs Socket open OK en [%d] secs\r\n\0"), timeout );
	} else {
		xprintf_PD( f_debug,  PSTR("COMMS: gprs ERROR: Socket open FAIL !!.\r\n\0"));
	}

	return(link_status);

}
//------------------------------------------------------------------------------------
void gprs_close_socket(bool f_debug)
{
	xprintf_PD( f_debug, PSTR("COMMS: gprs try to close socket\r\n\0"));

	// Paso a modo comando y cierro el socket por las dudas
	gprs_switch_to_command_mode();

uint8_t timeout;
bool retS = false;

	xprintf_PD( f_debug, PSTR("COMMS: gprs try to close socket\r\n\0"));
	gprs_flush_RX_buffer();
	gprs_flush_TX_buffer();
	xfprintf_P( fdGPRS, PSTR("AT+TCPCLOSE\n\0"));
	timeout = 0;
	while ( timeout++ < TIMEOUT_SOCKCLOSE ) {
		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
		if ( IO_read_DCD() == 1 ) {
			retS = true;
			break;
		}
	}

	gprs_print_RX_buffer(f_debug);
	if ( retS ) {
		xprintf_PD( f_debug, PSTR("COMMS: gprs SOCK closed OK en [%d] secs\r\n\0"), timeout );
	} else {
		xprintf_PD( f_debug, PSTR("COMMS: WARN gprs SOCK closed failed !!!\r\n\0") );
	}

	return;

}
//------------------------------------------------------------------------------------
t_link_status gprs_check_socket_status(bool f_debug)
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
t_link_status socket_status = LINK_CLOSED;

	pin_dcd = IO_read_DCD();

	if (  pin_dcd == 0 ) {
		socket_status = LINK_OPEN;
		xprintf_PD( f_debug, PSTR("COMMS: gprs sckt is open (dcd=%d)\r\n\0"),pin_dcd);

	} else {
		socket_status = LINK_CLOSED;
		xprintf_PD( f_debug, PSTR("COMMS: gprs sckt is close (dcd=%d)\r\n\0"),pin_dcd);

	}

	return(socket_status);

}
//------------------------------------------------------------------------------------
char *gprs_get_buffer_ptr( char *pattern)
{

	return( strstr( gprsRxBuffer.buffer, pattern) );
}
//------------------------------------------------------------------------------------
bool gprs_scan( t_scan_struct *scan_boundle )
{

	// Inicio un ciclo de SCAN
	xprintf_PD( scan_boundle->f_debug, PSTR("COMMS: GPRS_SCAN starting to scan...\r\n\0" ));

	// scan_list1: datos de Spymovil.
	if ( gprs_scan_try( scan_boundle , (PGM_P *)scan_list1 ))
		return(true);

	// scan_list2: datos de UTE.
	if ( gprs_scan_try( scan_boundle , (PGM_P *)scan_list2 ))
		return(true);

	// scan_list3: datos de OSE.
	if ( gprs_scan_try( scan_boundle , (PGM_P *)scan_list3 ))
		return(true);

	// scan_list4: datos de SPY PUBLIC_IP.
	if ( gprs_scan_try( scan_boundle , (PGM_P *)scan_list4 ))
		return(true);

	return(false);

}
//------------------------------------------------------------------------------------
bool gprs_need_scan( t_scan_struct *scan_boundle )
{

	// Veo si es necesario hacer un SCAN de la IP del server
	if ( ( strcmp_P( scan_boundle->apn, PSTR("DEFAULT\0")) == 0 ) ||
			( strcmp_P( scan_boundle->server_ip, PSTR("DEFAULT\0")) == 0 ) ||
			( strcmp_P( scan_boundle->dlgid, PSTR("DEFAULT\0")) == 0 ) ) {
		// Alguno de los parametros estan en DEFAULT.
		return(true);
	}

	return(false);

}
//------------------------------------------------------------------------------------
bool gprs_scan_try (t_scan_struct *scan_boundle, PGM_P *dlist )
{
	/*
	 * Recibe una lista de PGM_P cuyo primer elemento es un APN y el segundo una IP.
	 * Intenta configurar el APN y abrir un socket a la IP.
	 *
	 */
char apn_tmp[APN_LENGTH];
char ip_tmp[IP_LENGTH];
uint8_t intentos;

	strcpy_P( apn_tmp, (PGM_P)pgm_read_word( &dlist[0]));
	strcpy_P( ip_tmp, (PGM_P)pgm_read_word( &dlist[1]));

	xprintf_PD( scan_boundle->f_debug, PSTR("COMMS: GPRS_SCAN trying APN:%s, IP:%s\r\n\0"), apn_tmp, ip_tmp );

	// Apago
	gprs_apagar();
	vTaskDelay( (portTickType)( 5000 / portTICK_RATE_MS ) );

	// Prendo
	if ( ! gprs_prender( scan_boundle->f_debug ) )
		return(false);

	// Configuro
	// EL pin es el de default ya que si estoy aqui es porque no tengo configuracion valida.
	if (  ! gprs_configurar_dispositivo( scan_boundle->f_debug, scan_boundle->cpin, NULL ) ) {
		return(false);
	}

	// Apn
	gprs_set_apn(scan_boundle->f_debug, apn_tmp );

	// Intento pedir una IP
	if ( ! gprs_netopen(scan_boundle->f_debug) ) {
		return(false);
	}

	// Tengo la IP. La leo.
	gprs_read_ip_assigned(scan_boundle->f_debug, NULL);

	// Envio un frame de SCAN al servidor.
	for( intentos= 0; intentos < 3 ; intentos++ )
	{
		if ( gprs_send_scan_frame(scan_boundle, ip_tmp) ) {
			if ( gprs_process_scan_response( scan_boundle )) {
				// Resultado OK. Los parametros son correctos asi que los salvo en el systemVars. !!!
				// que es a donde esta apuntando el scan_boundle
				memset( scan_boundle->apn,'\0', APN_LENGTH );
				strncpy(scan_boundle->apn, apn_tmp, APN_LENGTH);
				memset( scan_boundle->server_ip,'\0', IP_LENGTH );
				strncpy(scan_boundle->server_ip, ip_tmp, IP_LENGTH);
				return(true);
			}
		}
	}

	return(false);

}
//------------------------------------------------------------------------------------
bool gprs_send_scan_frame(t_scan_struct *scan_boundle, char *ip_tmp )
{
	//  DLGID=DEFAULT&TYPE=CTL&VER=2.9.9k&PLOAD=CLASS:SCAN;UID:3046323334331907001300

uint8_t i = 0;

	// Loop
	for ( i = 0; i < 3; i++ ) {

		if (  gprs_check_socket_status( scan_boundle->f_debug) == LINK_OPEN ) {

			gprs_flush_RX_buffer();
			gprs_flush_TX_buffer();
			xprintf_PVD( fdGPRS, scan_boundle->f_debug, PSTR("GET %s?DLGID=%s&TYPE=CTL&VER=%s\0" ), scan_boundle->script, scan_boundle->dlgid, SPX_FW_REV );
			xprintf_PVD(  fdGPRS, scan_boundle->f_debug, PSTR("&PLOAD=CLASS:SCAN;UID:%s\0" ), NVMEE_readID() );
			xprintf_PVD(  xCOMMS_get_fd(), scan_boundle->f_debug, PSTR(" HTTP/1.1\r\nHost: www.spymovil.com\r\n\r\n\r\n\0") );
			vTaskDelay( (portTickType)( 250 / portTICK_RATE_MS ) );
			return(true);

		} else {
			// No tengo enlace al server. Intento abrirlo
			vTaskDelay( (portTickType)( 3000 / portTICK_RATE_MS ) );
			gprs_open_socket(scan_boundle->f_debug, ip_tmp, scan_boundle->tcp_port );
		}
	}

	/*
	 * Despues de varios reintentos no logre enviar el frame
	 */
	return(false);

}
//------------------------------------------------------------------------------------
bool gprs_process_scan_response(t_scan_struct *scan_boundle)
{
uint8_t timeout = 0;

	for ( timeout = 0; timeout < 10; timeout++) {

		vTaskDelay( (portTickType)( 2000 / portTICK_RATE_MS ) );	// Espero 1s

		// El socket se cerro
		if ( gprs_check_socket_status( scan_boundle->f_debug) != LINK_OPEN ) {
			return(false);
		}

		//xCOMMS_print_RX_buffer(true);

		// Recibi un ERROR de respuesta
		if ( gprs_check_response("ERROR") ) {
			gprs_print_RX_buffer(true);
			return(false);
		}

		// Recibi un ERROR http 401: No existe el recurso
		if ( gprs_check_response("404 Not Found") ) {
			gprs_print_RX_buffer(true);
			return(false);
		}

		// Recibi un ERROR http 500: No existe el recurso
		if ( gprs_check_response("Internal Server Error") ) {
			gprs_print_RX_buffer(true);
			return(false);
		}

		// Respuesta completa del server
		if ( gprs_check_response("</h1>") ) {

			gprs_print_RX_buffer( scan_boundle->f_debug );

			// Respuesta correcta. El dlgid esta definido en la BD
			if ( gprs_check_response ("STATUS:OK")) {
				return(true);

			} else if ( gprs_check_response ("STATUS:RECONF")) {
				// Respuesta correcta
				// Configure el DLGID correcto y la SERVER_IP usada es la correcta.
				gprs_extract_dlgid(scan_boundle);
				return(true);

			} else if ( gprs_check_response ("STATUS:UNDEF")) {
				// Datalogger esta usando un script incorrecto
				xprintf_P( PSTR("COMMS: GPRS_SCAN SCRIPT ERROR !!.\r\n\0" ));
				return(false);

			} else if ( gprs_check_response ("NOTDEFINED")) {
				// Datalogger esta usando un script incorrecto
				xprintf_P( PSTR("COMMS: GPRS_SCAN dlg not defined in BD\r\n\0" ));
				return(false);

			} else {
				// Frame no reconocido !!!
				xprintf_P( PSTR("COMMS: GPRS_SCAN frame desconocido\r\n\0" ));
				return(false);
			}
		}
	}

	return(false);
}
//------------------------------------------------------------------------------------
void gprs_extract_dlgid(t_scan_struct *scan_boundle)
{
	// La linea recibida es del tipo: <h1>TYPE=CTL&PLOAD=CLASS:SCAN;STATUS:RECONF;DLGID:TEST01</h1>
	// Es la respuesta del server a un frame de SCAN.
	// Indica 2 cosas: - El server es el correcto por lo que debo salvar la IP
	//                 - Me pasa el DLGID correcto.


char *p = NULL;
char localStr[32] = { 0 };
char *stringp = NULL;
char *token = NULL;
char *delim = ",;:=><";

	p = gprs_get_buffer_ptr("DLGID");
	if ( p == NULL ) {
		return;
	}

	// Copio el mensaje enviado a un buffer local porque la funcion strsep lo modifica.
	memset(localStr,'\0',32);
	memcpy(localStr,p,sizeof(localStr));

	stringp = localStr;
	token = strsep(&stringp,delim);	// DLGID
	token = strsep(&stringp,delim);	// TH001

	// Copio el dlgid recibido al systemVars.dlgid que esta en el scan_boundle
	memset( scan_boundle->dlgid,'\0', DLGID_LENGTH );
	strncpy(scan_boundle->dlgid, token, DLGID_LENGTH);
	xprintf_P( PSTR("COMMS: GPRS_SCAN discover DLGID to %s\r\n\0"), scan_boundle->dlgid );
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
		xfprintf_P( fdGPRS , PSTR("AT+STK=0\r\0"));
		vTaskDelay( (portTickType)( 5000 / portTICK_RATE_MS ) );
		break;
	case 1:
		// Enable
		gprs_flush_RX_buffer();
		gprs_flush_TX_buffer();
		xfprintf_P( fdGPRS , PSTR("AT+STK=1\r\0"));
		vTaskDelay( (portTickType)( 5000 / portTICK_RATE_MS ) );
		break;
	case 2:
		// Check. Query STK status ?
		xprintf_P(PSTR("GPRS: query STK status ?\r\n\0"));
		gprs_flush_RX_buffer();
		gprs_flush_TX_buffer();
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
void gprs_switch_to_command_mode(void)
{
	/*
	 * Para switchear de data mode a command mode, DTR=0 al menos 1s.
	 */
	IO_clr_GPRS_DTR();
	vTaskDelay( (portTickType)( 1500 / portTICK_RATE_MS ) );
	IO_set_GPRS_DTR();
}
//------------------------------------------------------------------------------------

