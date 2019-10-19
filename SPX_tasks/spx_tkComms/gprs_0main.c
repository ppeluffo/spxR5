/*
 * sp5KV5_tkGprs_main.c
 *
 *  Created on: 26 de abr. de 2017
 *      Author: pablo
 *
 *
 *  Comandos AT para conectarse.
 *
 *  1) Chequeamos que halla un SIM presente.
 *  AT+CPIN?
 *  +CPIN: READY
 *
 *  2) Registacion.
 *  El dispositivo debe estar registrado en la red antes de poder usarla.
 *  Para chequear si esta registrado usamos
 *  AT+CGREG?
 *  +CGREG: 0,1
 *
 *  Normalmente el SIM esta para que el dispositivo se registre automaticamente
 *  Esto se puede ver con el comando AT+COPS? el cual tiene la red preferida y el modo 0
 *  indica que esta para registrarse automaticamente.
 *  Este comando se usa para de-registrar y registrar en la red.
 *  Conviene dejarlo automatico de modo que si el TE se puede registrar lo va a hacer.
 *  Solo chequeamos que este registrado con CGREG.
 *
 *  3) Si esta registrado, podemos obtener informacion de la red ( opcional )
 *  AT+COPS?
 *
 *  4) Una vez registrado consultamos la calidad de senal
 *  AT+CSQ
 *  +CSQ 10,99
 *
 *  5) Si estoy registrado, me debo atachear a la red
 *  AT+CGATT=1
 *
 *  6) APN
 * 	Incluso si GPRS Attach es exitoso, no significa que se haya establecido la llamada de datos.
 * 	En GPRS, un Protocolo de paquetes de datos (PDP) define la sesión de datos.
 * 	El contexto PDP establece la ruta de datos entre el dispositivo y el GGSN (Nodo de soporte Gateway GPRS).
 * 	GGSN actúa como una puerta de enlace entre el dispositivo y el resto del mundo.
 * 	Por lo tanto, debe establecer un contexto PDP antes de que pueda enviar / recibir datos en Internet.
 * 	El GGSN se identifica a través del nombre del punto de acceso (APN).
 * 	Cada proveedor tendrá sus propias APN y generalmente están disponibles en Internet.
 * 	El dispositivo puede definir múltiples contextos PDP que se almacenan de manera única
 * 	en los ID de contexto.
 *
 * 	7) Ahora que los contextos PDP están definidos, utilizo el contexto PDP correcto que coincida
 *  con la tarjeta SIM.
 *  Ahora para configurar la sesión, el contexto PDP apropiado debe estar activado.
 *  AT + CGACT = 1,1
 *  El primer parámetro (1) es activar el contexto y el segundo parámetro es el ID de contexto.
 *  Una vez que el Contexto PDP se activa con éxito, el dispositivo puede enviar / recibir
 *  datos en Internet.
 *  Para verificar si hubo algún problema con la activación PDP, envíe el comando
 *  AT + CEER.
 *
 *
 */

#include <spx_tkComms/gprs.h>

#define WDG_GPRSRX_TIMEOUT 60

static void pv_gprs_init_system(void);
static void pv_gprs_rxbuffer_poke(char c);

//-------------------------------------------------------------------------------------
void tkGprsTx(void * pvParameters)
{

( void ) pvParameters;

	while ( !startTask )
		vTaskDelay( ( TickType_t)( 100 / portTICK_RATE_MS ) );

	vTaskDelay( ( TickType_t)( 1000 / portTICK_RATE_MS ) );

	pv_gprs_init_system();

	xprintf_P( PSTR("starting tkGprsTx..\r\n\0"));

	for( ;; )
	{

RESTART:

		if ( st_gprs_esperar_apagado() != bool_CONTINUAR ) {	// Espero con el modem apagado
			goto RESTART;
		}

		if ( st_gprs_prender() != bool_CONTINUAR )  {	// Intento prenderlo y si no salgo con false
			goto RESTART;								// y voy directo a APAGARLO
		}

		if ( st_gprs_configurar() != bool_CONTINUAR ) {	// Intento configurarlo y si no puedo o debo reiniciarlo ( cambio de
			goto RESTART;								// banda ) salgo con false y vuelvo a APAGAR
		}

		if ( st_gprs_monitor_sqe() != bool_CONTINUAR ) {	// Salgo por signal o reset o timeout de control task
			goto RESTART;
		}

		if ( st_gprs_get_ip() != bool_CONTINUAR  ) {	// Si no logro una IP debo reiniciarme. Salgo en este caso
			goto RESTART;								// con false. Aqui si no tengo el APN hago un SCAN
		}

		if ( st_gprs_scan_frame() != bool_CONTINUAR ) {	// Si la IP esta en DEFAULT hago un scan de varios servidores para
			goto RESTART;								// descubrir la server IP correcta.
		}

		// El modem prendio. Paso a reconfigurar con inits.
		if ( st_gprs_inits() != bool_CONTINUAR ) {
			goto RESTART;
		}

		st_gprs_data();		// Estado que quedo con la interfase gprs up en envio de datos.

	}
}
//------------------------------------------------------------------------------------
void tkGprsRx(void * pvParameters)
{
	// Esta tarea lee y procesa las respuestas del GPRS. Lee c/caracter recibido y lo va
	// metiendo en un buffer circular propio del GPRS que permite luego su impresion,
	// analisis, etc.

( void ) pvParameters;
char c;
uint32_t ulNotifiedValue;
BaseType_t xResult;

	// Espero la notificacion para arrancar
	while ( !startTask )
		vTaskDelay( ( TickType_t)( 100 / portTICK_RATE_MS ) );

	u_gprs_rxbuffer_flush();

	xprintf_P( PSTR("starting tkGprsRX..\r\n\0"));

	// loop
	for( ;; )
	{

		ctl_watchdog_kick(WDG_GPRSRX, WDG_GPRSRX_TIMEOUT);

		// Si el modem NO esta prendido, espero de a 5s antes de reintentar
		// lo que me da tiempo de entrar en tickless.
		if ( GPRS_stateVars.modem_prendido ) {

			GPRS_stateVars.dcd = IO_read_DCD();

			// Monitoreo las señales y solo prendo las flags correspondientes.
			xResult = xTaskNotifyWait( 0x00, ULONG_MAX, &ulNotifiedValue, ((TickType_t) 100 / portTICK_RATE_MS ) );
			if ( xResult == pdTRUE ) {
				if ( ( ulNotifiedValue & TK_FRAME_READY ) != 0 ) {
					GPRS_stateVars.signal_frameReady = true;
				}
			}

			while ( frtos_read( fdGPRS, &c, 1 ) == 1 ) {
				pv_gprs_rxbuffer_poke(c);
			}

		} else {

			GPRS_stateVars.dcd = 1;
			//vTaskDelay( ( TickType_t)( 5000 / portTICK_RATE_MS ) );
			xResult = xTaskNotifyWait( 0x00, ULONG_MAX, &ulNotifiedValue, ((TickType_t) 25000 / portTICK_RATE_MS ) );
		}

	}
}
//------------------------------------------------------------------------------------
/*
ISR(PORTD_INT0_vect)
{

	GPRS_stateVars.dcd = IO_read_DCD();
	PORTA.INTFLAGS = PORT_INT0IF_bm;
}
*/
//------------------------------------------------------------------------------------
static void pv_gprs_init_system(void)
{
	// Aca hago toda la inicializacion que requiere el sistema del gprs.

	//u_gprs_init_pines();	// Lo paso a tkCTL()

	memset(buff_gprs_imei,'\0',IMEIBUFFSIZE);
	strncpy_P(GPRS_stateVars.dlg_ip_address, PSTR("000.000.000.000\0"),16);

	if ( spx_io_board == SPX_IO8CH ) {
		systemVars.gprs_conf.timerDial = 0;
	}

	// Configuracion inicial de la tarea
	// Configuro la interrupcion del DCD ( PORTD.6)
	// La variable GPRS_stateVars.dcd

	// Los pines ya estan configurados como entradas.
	//
//	PORTD.PIN2CTRL = PORT_OPC_PULLUP_gc | PORT_ISC_BOTHEDGES_gc;	// Sensa both edge
//	PORTD.INT0MASK = PIN6_bm;
//	PORTD.INTCTRL = PORT_INT0LVL0_bm;


}
//------------------------------------------------------------------------------------
static void pv_gprs_rxbuffer_poke(char c)
{

	// Si hay lugar meto el dato.
	if ( pv_gprsRxCbuffer.ptr < UART_GPRS_RXBUFFER_LEN )
		pv_gprsRxCbuffer.buffer[ pv_gprsRxCbuffer.ptr++ ] = c;
}
//------------------------------------------------------------------------------------

