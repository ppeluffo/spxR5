/*
 * tkComms_GPRS.c
 *
 *  Created on: 5 mar. 2020
 *      Author: pablo
 */

#include "tkComms.h"

//------------------------------------------------------------------------------------

struct {
	char buffer[GPRS_RXBUFFER_LEN];
	uint16_t ptr;
} gprsRxBuffer;

#define IMEIBUFFSIZE	24

char buff_gprs_imei[IMEIBUFFSIZE];
char buff_gprs_ccid[IMEIBUFFSIZE];

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
bool gprs_prender(bool f_debug, uint8_t delay_factor )
{
	/*
	 * Intenta prender el dispositivo GPRS.
	 * Debe suministrar energia y luego activar el pin sw.
	 * Finalmente manda un comando AT y debe recibir un OK.
	 * Si es asi lee el imei y el ccid y retorna true.
	 */


uint8_t tries;

	gprs_hw_pwr_on(delay_factor);

// Loop:
	/*
	 * El GPRS requiere un pulso en  el pin de pwr_sw
	 */
	for ( tries = 0; tries < 3; tries++ ) {

		if ( f_debug ) {
			xprintf_P(PSTR("COMMS:gprs intentos: SW=%d\r\n\0"), tries);
		}


		// Genero un pulso en el pin SW para prenderlo logicamente
		gprs_sw_pwr();

		// Espero 10s para interrogarlo
		vTaskDelay( (portTickType)( ( 10000 + ( 5000 * tries ) ) / portTICK_RATE_MS ) );

		// Mando un AT y espero un OK para ver si prendio y responde.
		gprs_flush_RX_buffer();
		gprs_flush_TX_buffer();
		xfprintf_P( fdGPRS, PSTR("AT\r\0"));
		vTaskDelay( (portTickType)( 250 / portTICK_RATE_MS ) );

		if ( f_debug ) {
			gprs_print_RX_buffer();
		}

		if ( gprs_check_response("OK\0") ) {
			// Respondio OK. Esta prendido; salgo
			xprintf_P( PSTR("COMMS:gprs Modem on.\r\n\0"));
			gprs_readImei(f_debug);
			gprs_readCcid(f_debug);
			return(true);

		} else {
			if ( f_debug ) {
				xprintf_P(PSTR("COMMS:gprs Modem No prendio!!\r\n\0"));
			}
		}

	}

	// No prendio luego de 3 intentos SW.
	// Apago y prendo de nuevo
	// Espero 10s antes de reintentar
	gprs_apagar();
	vTaskDelay( (portTickType)( 10000 / portTICK_RATE_MS ) );

	return(false);
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
void gprs_print_RX_buffer(void)
{

	// Imprimo todo el buffer local de RX. Sale por \0.
	xprintf_P( PSTR ("GPRS: rxbuff>\r\n\0"));

	// Uso esta funcion para imprimir un buffer largo, mayor al que utiliza xprintf_P. !!!
	xnprint( gprsRxBuffer.buffer, GPRS_RXBUFFER_LEN );
	xprintf_P( PSTR ("\r\n[%d]\r\n\0"), gprsRxBuffer.ptr );
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
void gprs_readImei( bool f_debug )
{
	// Leo el imei del modem para poder trasmitirlo al server y asi
	// llevar un control de donde esta c/sim

uint8_t i = 0;
uint8_t j = 0;
uint8_t start = 0;
uint8_t end = 0;

	// Envio un AT+CGSN para leer el IMEI
	gprs_flush_RX_buffer();
	gprs_flush_TX_buffer();

	xfprintf_P( fdGPRS,PSTR("AT+CGSN\r\0"));
	vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );

	if ( f_debug ) {
		gprs_print_RX_buffer();
	}

	// Leo y Evaluo la respuesta al comando AT+CGSN
	if ( gprs_check_response("OK\0") ) {
		// Extraigo el IMEI del token.
		// Guardo el IMEI
		start = 0;
		end = 0;
		j = 0;
		// Busco el primer digito
		for ( i = 0; i < 64; i++ ) {
			if ( isdigit( gprsRxBuffer.buffer[i]) ) {
				start = i;
				break;
			}
		}

		if ( start == end )		// No lo pude leer.
			goto EXIT;

		// Busco el ultimo digito y copio todos
		for ( i = start; i < 64; i++ ) {
			if ( isdigit( gprsRxBuffer.buffer[i]) ) {
				buff_gprs_imei[j++] = gprsRxBuffer.buffer[i];
			} else {
				break;
			}
		}
	}

// Exit
EXIT:

	xprintf_P( PSTR("COMMS:gprs IMEI[%s]\r\n\0"),buff_gprs_imei);


}
//--------------------------------------------------------------------------------------
void gprs_readCcid( bool f_debug )
{
	// Leo el ccid del sim para poder trasmitirlo al server y asi
	// llevar un control de donde esta c/sim
	// AT+CCID
	// +CCID: "8959801611637152574F"
	//
	// OK


uint8_t i = 0;
uint8_t j = 0;
uint8_t start = 0;
uint8_t end = 0;

	// Envio un AT+CGSN para leer el SIM ID
	gprs_flush_RX_buffer();
	gprs_flush_TX_buffer();

	xfprintf_P( fdGPRS,PSTR("AT+CCID\r\0"));
	vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );

	if ( f_debug ) {
		gprs_print_RX_buffer();
	}

	// Leo y Evaluo la respuesta al comando AT+CCID
	if ( gprs_check_response("OK\0") ) {
		// Extraigoel CCID del token. Voy a usar el buffer  de print ya que la respuesta
		// Guardo
		start = 0;
		end = 0;
		j = 0;
		// Busco el primer digito
		for ( i = 0; i < 64; i++ ) {
			if ( isdigit( gprsRxBuffer.buffer[i]) ) {
				start = i;
				break;
			}
		}
		if ( start == end )		// No lo pude leer.
			goto EXIT;

		// Busco el ultimo digito y copio todos
		for ( i = start; i < 64; i++ ) {
			if ( isdigit( gprsRxBuffer.buffer[i]) ) {
				buff_gprs_ccid[j++] = gprsRxBuffer.buffer[i];
				if ( j > 18) break;
			} else {
				break;
			}
		}

		// El CCID que usa ANTEL es de 18 digitos.
		buff_gprs_ccid[18] = '\0';
	}

// Exit
EXIT:

	xprintf_P( PSTR("COMMS:gprs CCID[%s]\r\n\0"),buff_gprs_ccid);


}
//--------------------------------------------------------------------------------------
bool gprs_configurar_dispositivo( bool f_debug, char *pin, uint8_t *err_code )
{
	/*
	 * Consiste en enviar los comandos AT de modo que el modem GPRS
	 * quede disponible para trabajar
	 */

	// Vemos que halla un pin presente.
	if ( ! gprs_CPIN( f_debug, pin) ) {
		*err_code = ERR_CPIN_FAIL;
		return(false);
	}

	// Vemos que el modem este registrado en la red
	if ( ! gprs_CGREG(f_debug) ) {
		return(false);
	}

	// Me debo atachear a la red. Con esto estoy conectado a la red movil pero
	// aun no puedo trasmitir datos.
	if ( ! gprs_CGATT(f_debug) ) {
		*err_code = ERR_NETATTACH_FAIL;
		return(false);
	}

	gprs_CIPMODE(f_debug);	// modo transparente.
	gprs_DCDMODE(f_debug);	// UART para utilizar las 7 lineas
	gprs_CMGF(f_debug);		// Configuro para mandar SMS en modo TEXTO
	gprs_CFGRI(f_debug);	// Configuro el RI para que sea ON y al llegar un SMS sea OFF

	*err_code = ERR_NONE;
	return(true);
}
//--------------------------------------------------------------------------------------
bool gprs_CPIN( bool f_debug, char *pin )
{
	// Chequeo que el SIM este en condiciones de funcionar.
	// AT+CPIN?

uint8_t tryes = 3;

	if ( f_debug ) {
		xprintf_P( PSTR("GPRS:gprs CPIN\r\n\0"));
	}

	while ( tryes > 0 ) {
		// Vemos si necesita SIMPIN
		gprs_flush_RX_buffer();
		xfprintf_P( fdGPRS , PSTR("AT+CPIN?\r\0"));
		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
		if ( f_debug ) {
			gprs_print_RX_buffer();
		}

		// Pin desbloqueado
		if ( gprs_check_response("+CPIN: READY\0") ) {
			if ( tryes == 1 ) {
				// Se destrabo con el pin x defecto. Lo grabo en la EE.
			    snprintf_P( pin, SIM_PASSWD_LENGTH, PSTR("%s\0"), SIMPIN_DEFAULT );
			    u_save_params_in_NVMEE();
			}
			vTaskDelay( (portTickType)( 5000 / portTICK_RATE_MS ) );
			return(true);
		}

		// Requiere PIN
		if ( gprs_check_response("+CPIN: SIM PIN\0") ) {
			gprs_flush_RX_buffer();

			if ( tryes == 3 ) {
				// Ingreso el pin configurado en el systemVars.
				xfprintf_P( fdGPRS, PSTR("AT+CPIN=%s\r"), pin );
			} else if ( tryes == 2 ) {
				// Ingreso el pin por defecto
				xfprintf_P( fdGPRS,PSTR("AT+CPIN=%s\r"), SIMPIN_DEFAULT );
			}

			vTaskDelay( (portTickType)( 3000 / portTICK_RATE_MS ) );
			if ( f_debug ) {
				gprs_print_RX_buffer();
			}
		}

		tryes--;

	}

	// Pin BLOQUEADO
	return(false);

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
	*/

bool retS = false;
uint8_t tryes = 0;

	xprintf_P( PSTR("COMMS:gprs NET registation\r\n\0"));

	for ( tryes = 0; tryes < 5; tryes++ ) {

		gprs_flush_RX_buffer();
		xfprintf_P( fdGPRS, PSTR("AT+CREG?\r\0"));
		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
		if ( f_debug ) {
			gprs_print_RX_buffer();
		}

		vTaskDelay( ( TickType_t)( 5000 / portTICK_RATE_MS ) );

		if ( gprs_check_response("+CREG: 0,1\0") ) {
			retS = true;
			break;
		}

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
	*/

uint8_t tryes = 0;

	xprintf_P( PSTR("COMMS:gprs NET attach\r\n\0"));

	gprs_flush_RX_buffer();
	xfprintf_P( fdGPRS,PSTR("AT+CGATT=1\r\0"));
	vTaskDelay( (portTickType)( 2000 / portTICK_RATE_MS ) );
	if ( f_debug ) {
		gprs_print_RX_buffer();
	}

	for ( tryes = 0; tryes < 3; tryes++ ) {
		gprs_flush_RX_buffer();
		xfprintf_P( fdGPRS,PSTR("AT+CGATT?\r\0"));
		vTaskDelay( (portTickType)( 2000 / portTICK_RATE_MS ) );
		if ( f_debug ) {
			gprs_print_RX_buffer();
		}

		if ( gprs_check_response("+CGATT: 1\0") ) {
			return(true);
		}
	}

	return(false);

}
//------------------------------------------------------------------------------------
void gprs_CIPMODE(bool f_debug)
{
	// Funcion que configura el modo transparente.

	gprs_flush_RX_buffer();
	xfprintf_P( fdGPRS,PSTR("AT+CIPMODE=1\r\0"));
	vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
	if ( f_debug ) {
		gprs_print_RX_buffer();
	}

}
//------------------------------------------------------------------------------------
void gprs_DCDMODE( bool f_debug )
{
	// Funcion que configura el UART para utilizar las 7 lineas y el DCD indicando
	// el estado del socket.

	gprs_flush_RX_buffer();
	xfprintf_P( fdGPRS,PSTR("AT&D1\r\0"));
	vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
	if ( f_debug ) {
		gprs_print_RX_buffer();
	}

	gprs_flush_RX_buffer();
	xfprintf_P( fdGPRS,PSTR("AT+CSUART=1\r\0"));
	vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
	if ( f_debug ) {
		gprs_print_RX_buffer();
	}
/*
	gprs_flush_RX_buffer();
	xfprintf_P( fdGPRS,PSTR("AT+CDCDMD=0\r\0"));
	vTaskDelay( (portTickType)( 100 / portTICK_RATE_MS ) );
	gprs_print_RX_buffer();
*/
	gprs_flush_RX_buffer();
	xfprintf_P( fdGPRS,PSTR("AT&C1\r\0"));
	vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
	if ( f_debug ) {
		gprs_print_RX_buffer();
	}

}
//------------------------------------------------------------------------------------
void gprs_CMGF( bool f_debug )
{
	// Configura para mandar SMS en modo texto

	gprs_flush_RX_buffer();
	xfprintf_P( fdGPRS,PSTR("AT+CMGF=1\r\0"));
	vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
	if ( f_debug ) {
		gprs_print_RX_buffer();
	}

}
//------------------------------------------------------------------------------------
void gprs_CFGRI (bool f_debug)
{
	// Configura para que RI sea ON y al recibir un SMS haga un pulso

uint8_t pin;

	gprs_flush_RX_buffer();
	xfprintf_P( fdGPRS,PSTR("AT+CFGRI=1,1\r\0"));
	vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
	if ( f_debug ) {
		gprs_print_RX_buffer();
	}

	// Reseteo el RI
	gprs_flush_RX_buffer();
	xfprintf_P( fdGPRS,PSTR("AT+CRIRS\r\0"));
	vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
	if ( f_debug ) {
		gprs_print_RX_buffer();
	}

	vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );

	// Leo el RI
	pin = IO_read_RI();
	xprintf_P( PSTR("RI=%d\r\n\0"),pin);

}
//------------------------------------------------------------------------------------
uint8_t gprs_read_sqe( bool f_debug )
{
	// Veo la calidad de senal que estoy recibiendo

char csqBuffer[32] = { 0 };
char *ts = NULL;
uint8_t csq, dbm;

	csq = 0;
	dbm = 0;

	// AT+CSQ
	gprs_flush_RX_buffer();
	xfprintf_P( fdGPRS, PSTR("AT+CSQ\r\0"));
	vTaskDelay( (portTickType)( 500 / portTICK_RATE_MS ) );

	if ( f_debug ) {
		gprs_print_RX_buffer();
	}

	memcpy(csqBuffer, &gprsRxBuffer.buffer[0], sizeof(csqBuffer) );
	if ( (ts = strchr(csqBuffer, ':')) ) {
		ts++;
		csq = atoi(ts);
		dbm = 113 - 2 * csq;
	}

	// LOG
	xprintf_P ( PSTR("COMMS:gprs signalQ CSQ=%d,DBM=%d\r\n\0"), csq, dbm );
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
bool gprs_scan(bool f_debug, char *apn, char *ip_server, char *dlgid, uint8_t *err_code )
{

	return(true);
}
//------------------------------------------------------------------------------------
bool gprs_ip(bool f_debug, char *apn, char *ip_assigned, uint8_t *err_code )
{

	/*
	 *  El pedir IP es un concepto solo de GPRS
	 */

	// Configuramos en el modem el APN
	gprs_set_apn( f_debug, apn );

	strcpy(ip_assigned, "0.0.0.0\0");

	// Intento pedir una IP.
	if ( gprs_netopen(f_debug ) ) {
		gprs_read_ip_assigned(f_debug, ip_assigned );
		return(true);

	} else {
		// Aqui es que luego de tantos reintentos no consegui la IP.
		xprintf_P( PSTR("GPRS: ERROR: ip no asignada !!.\r\n\0") );
		return(false);
	}

	return(false);
}
//------------------------------------------------------------------------------------
void gprs_set_apn(bool f_debug, char *apn)
{

	// Configura el APN de trabajo.

	xprintf_P( PSTR("COMMS:gprs set APN\r\n\0") );

	//Defino el PDP indicando cual es el APN.
	// AT+CGDCONT
	gprs_flush_RX_buffer();
	xfprintf_P( fdGPRS, PSTR("AT+CGSOCKCONT=1,\"IP\",\"%s\"\r\0"), apn);
	vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
	if ( f_debug ) {
		gprs_print_RX_buffer();
	}

	// Como puedo tener varios PDP definidos, indico cual va a ser el que se deba activar
	// al usar el comando NETOPEN.
	gprs_flush_RX_buffer();
	xfprintf_P( fdGPRS,PSTR("AT+CSOCKSETPN=1\0"));
	vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
	if ( f_debug ) {
		gprs_print_RX_buffer();
	}

}
//------------------------------------------------------------------------------------
bool gprs_netopen(bool f_debug)
{
	// Abre una conexion a la red.
	// La red asigna una IP.
	// Doy el comando para atachearme a la red
	// Puede demorar unos segundos por lo que espero para chequear el resultado
	// y reintento varias veces.

uint8_t reintentos = MAX_TRYES_NET_ATTCH;
uint8_t checks = 0;

	xprintf_P( PSTR("COMMS:gprs netopen (get IP).\r\n\0") );
	// Espero 2s para dar el comando
	vTaskDelay( ( TickType_t)( 2000 / portTICK_RATE_MS ) );

	while ( reintentos-- > 0 ) {

		if ( f_debug ) {
			xprintf_P( PSTR("COMMS:gprs send NETOPEN cmd (%d)\r\n\0"),reintentos );
		}

		// Envio el comando.
		// AT+NETOPEN
		gprs_flush_RX_buffer();
		xfprintf_P( fdGPRS,PSTR("AT+NETOPEN\r\0"));
		vTaskDelay( ( TickType_t)( 2000 / portTICK_RATE_MS ) );

		// Intento 5 veces ver si respondio correctamente.
		for ( checks = 0; checks < 5; checks++) {

			if ( f_debug ) {
				xprintf_P( PSTR("COMMS:gprs netopen check.(%d)\r\n\0"),checks );
			}

			if ( f_debug ) {
				gprs_print_RX_buffer();
			}

			// Evaluo las respuestas del modem.
			if ( gprs_check_response("+NETOPEN: 0")) {
				xprintf_P( PSTR("COMMS:gprs NETOPEN OK !.\r\n\0") );
				return(true);

			} else if ( gprs_check_response("Network opened")) {
				xprintf_P( PSTR("COMMS:gprs NETOPEN OK !.\r\n\0") );
				return(true);

			} else if ( gprs_check_response("+IP ERROR: Network is already opened")) {
				return(true);

			} else if ( gprs_check_response("+NETOPEN: 1")) {
				xprintf_P( PSTR("COMMS:gprs NETOPEN FAIL !!.\r\n\0") );
				return(false);

			} else if ( gprs_check_response("ERROR")) {
				break;	// Salgo de la espera

			}
			// Aun no tengo ninguna respuesta esperada.
			// espero 5s para re-evaluar la respuesta.
			vTaskDelay( (portTickType)( 5000 / portTICK_RATE_MS ) );

		}

		// No pude atachearme. Debo mandar de nuevo el comando
		// Pasaron 25s.
	}

	// Luego de varios reintentos no pude conectarme a la red.
	xprintf_P( PSTR("COMMS:gprs NETOPEN FAIL !!.\r\n\0"));
	return(false);

}
//------------------------------------------------------------------------------------
void gprs_read_ip_assigned(bool f_debug, char *ip_assigned )
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

	// AT+CGPADDR para leer la IP
	gprs_flush_RX_buffer();
	//xfprintf_P( fdGPRS,PSTR("AT+CGPADDR\r\0"));
	xfprintf_P( fdGPRS,PSTR("AT+IPADDR\r\0"));
	vTaskDelay( (portTickType)( 2000 / portTICK_RATE_MS ) );
	if ( f_debug ) {
		gprs_print_RX_buffer();
	}

	if ( gprs_check_response("+IP ERROR: Network not opened")) {
		// No tengo la IP.

	} else if ( gprs_check_response("+IPADDR:")) {
		// Tengo la IP: extraigo la IP del token.
		ptr = ip_assigned;
		ts = strchr( gprsRxBuffer.buffer, ':');
		ts++;
		while ( (c= *ts) != '\r') {
			*ptr++ = c;
			ts++;
		}
		*ptr = '\0';

		xprintf_P( PSTR("COMMS:gprs ip address=[%s]\r\n\0"), ip_assigned );
	} else {
		// Asumo algun errro pero no tengo la IP
	}
}
//------------------------------------------------------------------------------------
