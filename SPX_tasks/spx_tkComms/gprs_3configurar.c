/*
 * sp5KV5_tkGprs_configurar.c
 *
 *  Created on: 27 de abr. de 2017
 *      Author: pablo
 */

#include <spx_tkComms/gprs.h>

static bool pv_gprs_CPIN(void);
static bool pv_gprs_CGREG(void);
static bool pv_gprs_CGATT(void);
//static void pg_gprs_APN(void);
static void pg_gprs_CIPMODE(void);
static void pg_gprs_DCDMODE(void);
static void pg_gprs_CMGF(void);
static void pg_gprs_CFGRI(void);
static void pg_gprs_CMGD(void);

// La tarea no puede demorar mas de 180s.
#define WDG_GPRS_TO_CONFIG	180
//------------------------------------------------------------------------------------
bool st_gprs_configurar(void)
{

	// Configuro los parametros opertativos, la banda GSM y pido una IP de modo que el
	// modem quede listo para
	// No atiendo mensajes ya que no requiero parametros operativos.
	// WATCHDOG: No demoro mas de 2 minutos en este estado

bool exit_flag = bool_RESTART;

// Entry:

	ctl_watchdog_kick(WDG_GPRSTX, WDG_GPRS_TO_CONFIG);

	GPRS_stateVars.state = G_CONFIGURAR;

	xprintf_P( PSTR("GPRS: configurar.\r\n\0"));

	// Vemos que halla un pin presente.
	if ( !pv_gprs_CPIN() ) {
		exit_flag = bool_RESTART;
		goto EXIT;
	}

	// Vemos que el modem este registrado en la red
	if ( !pv_gprs_CGREG() ) {
		exit_flag = bool_RESTART;
		goto EXIT;
	}

	// Recien despues de estar registrado puedo leer la calidad de señal.
	u_gprs_ask_sqe();

	// Me debo atachear a la red. Con esto estoy conectado a la red movil pero
	// aun no puedo trasmitir datos.
	if ( !pv_gprs_CGATT() ) {
		exit_flag = bool_RESTART;
		goto EXIT;
	}

	// Configuro parametros operativos
	//pv_gprs_CSPN();
	//pv_gprs_CNSMOD();
	//pv_gprs_CCINFO();
	//pv_gprs_CNTI();

	pg_gprs_CIPMODE();	// modo transparente.
	pg_gprs_DCDMODE();	// UART para utilizar las 7 lineas

	// Configuro para mandar SMS en modo TEXTO
	pg_gprs_CMGF();
	// Configuro el RI para que sea ON y al llegar un SMS sea OFF
	pg_gprs_CFGRI();
//	pg_gprs_CMGD();

//	pg_gprs_APN();		// Configuro el APN.
// !! Lo paso al modulo de scan_apn

	exit_flag = bool_CONTINUAR;

EXIT:

	u_gprs_sms_txcheckpoint();
	u_gprs_sms_rxcheckpoint();

	return(exit_flag);

}
//------------------------------------------------------------------------------------
// FUNCIONES PRIVADAS
//------------------------------------------------------------------------------------
static bool pv_gprs_CPIN(void)
{
	// Chequeo que el SIM este en condiciones de funcionar.
	// AT+CPIN?

uint8_t tryes = 3;

	if ( systemVars.debug == DEBUG_GPRS ) {
		xprintf_P( PSTR("GPRS: CPIN\r\n\0"));
	}

	while ( tryes > 0 ) {
		// Vemos si necesita SIMPIN
		u_gprs_flush_RX_buffer();
		xCom_printf_P( fdGPRS,PSTR("AT+CPIN?\r\0"));
		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
		if ( systemVars.debug == DEBUG_GPRS ) {
			u_gprs_print_RX_Buffer();
		}

		// Pin desbloqueado
		if ( u_gprs_check_response("+CPIN: READY\0") ) {
			if ( tryes == 1 ) {
				// Se destrabo con el pin x defecto. Lo grabo en la EE.
			    snprintf_P(systemVars.gprs_conf.simpwd, sizeof(systemVars.gprs_conf.simpwd), PSTR("%s\0"), SIMPIN_DEFAULT );
			    u_save_params_in_NVMEE();
			}
			vTaskDelay( (portTickType)( 5000 / portTICK_RATE_MS ) );
			return(true);
		}

		// Requiere PIN
		if ( u_gprs_check_response("+CPIN: SIM PIN\0") ) {
			u_gprs_flush_RX_buffer();

			if ( tryes == 3 ) {
				// Ingreso el pin configurado en el systemVars.
				xCom_printf_P( fdGPRS,PSTR("AT+CPIN=%s\r"), systemVars.gprs_conf.simpwd);
			} else if ( tryes == 2 ) {
				// Ingreso el pin por defecto
				xCom_printf_P( fdGPRS,PSTR("AT+CPIN=%s\r"), SIMPIN_DEFAULT );
			}

			vTaskDelay( (portTickType)( 3000 / portTICK_RATE_MS ) );
			if ( systemVars.debug == DEBUG_GPRS ) {
				u_gprs_print_RX_Buffer();
			}
		}

		tryes--;

	}

	// Pin BLOQUEADO
	// Configuro para esperar 1h.
	systemVars.gprs_conf.timerDial = 3600;
	return(false);

}
//------------------------------------------------------------------------------------
static bool pv_gprs_CGREG(void)
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

	xprintf_P( PSTR("GPRS: NET registation\r\n\0"));

	for ( tryes = 0; tryes < 5; tryes++ ) {
		u_gprs_flush_RX_buffer();
		xCom_printf_P( fdGPRS,PSTR("AT+CREG?\r\0"));
		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
		if ( systemVars.debug == DEBUG_GPRS ) {
			u_gprs_print_RX_Buffer();
		}

		vTaskDelay( ( TickType_t)( 5000 / portTICK_RATE_MS ) );

		u_gprs_check_response("+CREG: 0,1\0") ? (retS = true ): (retS = false) ;

		if ( retS ) {
			break;
		}

	}
	return(retS);

}
//------------------------------------------------------------------------------------
static bool pv_gprs_CGATT(void)
{
	/* Una vez registrado, me atacheo a la red
	 AT+CGATT=1

	 AT+CGATT?
	 +CGATT: 1
	*/

bool retS = false;
uint8_t tryes = 0;

	xprintf_P( PSTR("GPRS: NET attach\r\n\0"));

	u_gprs_flush_RX_buffer();
	xCom_printf_P( fdGPRS,PSTR("AT+CGATT=1\r\0"));
	vTaskDelay( (portTickType)( 2000 / portTICK_RATE_MS ) );
	if ( systemVars.debug == DEBUG_GPRS ) {
		u_gprs_print_RX_Buffer();
	}

	for ( tryes = 0; tryes < 3; tryes++ ) {
		u_gprs_flush_RX_buffer();
		xCom_printf_P( fdGPRS,PSTR("AT+CGATT?\r\0"));
		vTaskDelay( (portTickType)( 2000 / portTICK_RATE_MS ) );
		if ( systemVars.debug == DEBUG_GPRS ) {
			u_gprs_print_RX_Buffer();
		}

		u_gprs_check_response("+CGATT: 1\0") ? (retS = true ): (retS = false) ;

		if ( retS ) {
			break;
		}

	}
	return(retS);

}
//------------------------------------------------------------------------------------
static void pg_gprs_CIPMODE(void)
{
	// Funcion que configura el modo transparente.

	u_gprs_flush_RX_buffer();
	xCom_printf_P( fdGPRS,PSTR("AT+CIPMODE=1\r\0"));
	vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
	if ( systemVars.debug == DEBUG_GPRS ) {
		u_gprs_print_RX_Buffer();
	}

}
//------------------------------------------------------------------------------------
static void pg_gprs_DCDMODE(void)
{
	// Funcion que configura el UART para utilizar las 7 lineas y el DCD indicando
	// el estado del socket.

	u_gprs_flush_RX_buffer();
	xCom_printf_P( fdGPRS,PSTR("AT&D1\r\0"));
	vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
	if ( systemVars.debug == DEBUG_GPRS ) {
		u_gprs_print_RX_Buffer();
	}

	u_gprs_flush_RX_buffer();
	xCom_printf_P( fdGPRS,PSTR("AT+CSUART=1\r\0"));
	vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
	if ( systemVars.debug == DEBUG_GPRS ) {
		u_gprs_print_RX_Buffer();
	}
/*
	u_gprs_flush_RX_buffer();
	xCom_printf_P( fdGPRS,PSTR("AT+CDCDMD=0\r\0"));
	vTaskDelay( (portTickType)( 100 / portTICK_RATE_MS ) );
	u_gprs_print_RX_Buffer();
*/
	u_gprs_flush_RX_buffer();
	xCom_printf_P( fdGPRS,PSTR("AT&C1\r\0"));
	vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
	if ( systemVars.debug == DEBUG_GPRS ) {
		u_gprs_print_RX_Buffer();
	}


}
//------------------------------------------------------------------------------------
static void pg_gprs_CMGF(void)
{
	// Configura para mandar SMS en modo texto

	u_gprs_flush_RX_buffer();
	xCom_printf_P( fdGPRS,PSTR("AT+CMGF=1\r\0"));
	vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
	if ( systemVars.debug == DEBUG_GPRS ) {
		u_gprs_print_RX_Buffer();
	}

}
//------------------------------------------------------------------------------------
static void pg_gprs_CFGRI(void)
{
	// Configura para que RI sea ON y al recibir un SMS haga un pulso

uint8_t pin;

	u_gprs_flush_RX_buffer();
	xCom_printf_P( fdGPRS,PSTR("AT+CFGRI=1,1\r\0"));
	vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
	if ( systemVars.debug == DEBUG_GPRS ) {
		u_gprs_print_RX_Buffer();
	}

	// Reseteo el RI
	u_gprs_flush_RX_buffer();
	xCom_printf_P( fdGPRS,PSTR("AT+CRIRS\r\0"));
	vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
	if ( systemVars.debug == DEBUG_GPRS ) {
		u_gprs_print_RX_Buffer();
	}

	vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );

	// Leo el RI
	pin = IO_read_RI();
	xprintf_P( PSTR("RI=%d\r\n\0"),pin);

}
//------------------------------------------------------------------------------------
static void pg_gprs_CMGD(void)
{
	// Borro todos los mensajes SMS de la memoria

	u_gprs_flush_RX_buffer();
	xCom_printf_P( fdGPRS,PSTR("AT+CMGD=,4\r\0"));
	vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
	if ( systemVars.debug == DEBUG_GPRS ) {
		u_gprs_print_RX_Buffer();
	}

}
//------------------------------------------------------------------------------------
