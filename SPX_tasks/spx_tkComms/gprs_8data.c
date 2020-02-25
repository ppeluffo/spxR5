/*
 * gprs_8data.c
 *
 *  Created on: 9 oct. 2019
 *      Author: pablo
 */

#include <comms.h>

// La tarea no puede demorar mas de 5m.
#define WDG_GPRS_DATA	300

static bool pv_hay_datos_para_trasmitir(void);
static bool pv_procesar_frame( void );
static bool pv_trasmitir_frame(void );

static void pv_tx_data_payload( void );

static bool pv_procesar_respuesta_server(void);
static void pv_process_response_RESET(void);
static void pv_process_response_MEMFORMAT(void);
static uint8_t pv_process_response_OK(void);

FAT_t gprs_fat;
st_dataRecord_t gprs_dr;

typedef enum { SST_MONITOREO_DATOS = 0, SST_TXMIT_DATOS, SST_NO_DATOS, SST_ESPERA, SST_EXIT } data_state_t;
data_state_t data_state;

static void sst_monitoreo_datos(void);
static bool sst_txmit_datos(void);
static bool sst_no_datos(void);
static bool sst_espera(void);

//------------------------------------------------------------------------------------
bool st_gprs_data(void)
{
	// Me quedo en un loop permanente revisando si hay datos y si hay los trasmito
	// Solo salgo en caso que hubiesen problemas para trasmitir
	// Esto es que no puedo abrir un socket mas de 3 veces o que no puedo trasmitir
	// el mismo frame mas de 3 veces.
	// En estos casos salgo con false de modo de hacer un ciclo completo apagando el modem.
	// Si por algun problema no puedo trasmitir, salgo asi me apago y reinicio.
	// Si pude trasmitir o simplemente no hay datos, en modo continuo retorno TRUE.

	// Si hay datos los trasmito todos
	// Solo salgo en caso que hubiesen problemas para trasmitir
	// Esto es que no puedo abrir un socket mas de 3 veces o que no puedo trasmitir
	// el mismo frame mas de 3 veces.
	// En estos casos salgo con false de modo de hacer un ciclo completo apagando el modem.

	// Mientras estoy trasmitiendo datos no atiendo las señales; solo durante la espera en modo
	// continuo.

bool exit_flag = bool_RESTART;

	GPRS_stateVars.state = G_DATA;

	xprintf_P( PSTR("GPRS: data.\r\n\0"));

	data_state = SST_MONITOREO_DATOS;

	//
	while ( 1 ) {

		// Si por algun problema no puedo trasmitir, salgo asi me apago y reinicio.
		// Si pude trasmitir o simplemente no hay datos, en modo continuo retorno TRUE.

		ctl_watchdog_kick(WDG_GPRSTX, WDG_GPRS_DATA );

		switch (data_state ) {
		case SST_MONITOREO_DATOS:
			sst_monitoreo_datos();
			break;
		case SST_TXMIT_DATOS:
			exit_flag = sst_txmit_datos();
			break;
		case SST_NO_DATOS:
			exit_flag = sst_no_datos();
			break;
		case SST_ESPERA:
			exit_flag = sst_espera();
			break;
		case SST_EXIT:
			goto EXIT;
		default:
			data_state = SST_EXIT;
		}

	}

	// Exit area:
EXIT:
	// No espero mas y salgo del estado prender.
	return(exit_flag);
}
//------------------------------------------------------------------------------------
static void sst_monitoreo_datos(void)
{

	//xprintf_P( PSTR("DEBUG GPRS: sst_monitoreo_datos.\r\n\0"));
	GPRS_stateVars.state = G_DATA;

	while ( data_state == SST_MONITOREO_DATOS ) {

		if ( pv_hay_datos_para_trasmitir() ) {
			data_state = SST_TXMIT_DATOS;
		} else {
			data_state = SST_NO_DATOS;
		}

	}
	return;
}
//------------------------------------------------------------------------------------
static bool sst_txmit_datos(void)
{

bool retS = bool_CONTINUAR;

	//xprintf_P( PSTR("DEBUG GPRS: sst_txmit_datos.\r\n\0"));
	GPRS_stateVars.state = G_DATA;

	while ( data_state == SST_TXMIT_DATOS ) {

		if ( pv_procesar_frame() ) {
			data_state = SST_MONITOREO_DATOS;
		} else {
			data_state = SST_EXIT;
			retS = bool_RESTART;
		}

	}
	return(retS);

}
//------------------------------------------------------------------------------------
static bool sst_no_datos(void)
{

bool retS = bool_CONTINUAR;

	//xprintf_P( PSTR("DEBUG GPRS: sst_no_datos.\r\n\0"));
	GPRS_stateVars.state = G_DATA;

	while ( data_state == SST_NO_DATOS ) {

		if ( MODO_DISCRETO ) {
			data_state = SST_EXIT;
			retS = bool_CONTINUAR;
		} else {
			data_state = SST_ESPERA;
		}
	}
	return(retS);
}
//------------------------------------------------------------------------------------
static bool sst_espera(void)
{

uint8_t sleep_time = 60;
bool retS = bool_CONTINUAR;
//t_socket_status socket_status = 0;

	//xprintf_P( PSTR("DEBUG GPRS: sst_espera.\r\n\0"));
	GPRS_stateVars.state = G_DATA_AWAITING;

	while ( data_state == SST_ESPERA ) {

		while( sleep_time-- > 0 ) {

			vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );

			// PROCESO LAS SEÑALES
			if ( GPRS_stateVars.signal_redial) {
				// Salgo a discar inmediatamente.
				GPRS_stateVars.signal_redial = false;
				data_state = SST_EXIT;
				retS = bool_RESTART;
				break;
			}

			if ( GPRS_stateVars.signal_frameReady) {
				GPRS_stateVars.signal_frameReady = false;
				data_state = SST_MONITOREO_DATOS;
				break;
			}

			// EXPIRO EL TIEMPO
			if ( sleep_time == 1 ) {
				data_state = SST_MONITOREO_DATOS;
				break;
			}

			//xprintf_P(PSTR("DEBUG check sms4tx=%d\r\n"), GPRS_stateVars.sms_for_tx);
			// SMS for TX
			if ( GPRS_stateVars.sms_for_tx == true ) {
				// Si el socket esta abierto lo cierro
				if ( u_gprs_close_socket() ) {
					u_gprs_sms_txcheckpoint();
				}
			}

		}
	}

	// SMS RX ?
	// Al final del ciclo de espera chequeo por sms recibidos.
	u_gprs_sms_rxcheckpoint();

	return(retS);
}
//------------------------------------------------------------------------------------
static bool pv_hay_datos_para_trasmitir(void)
{

	/* Veo si hay datos en memoria para trasmitir
	 * Memoria vacia: rcds4wr = MAX, rcds4del = 0;
	 * Memoria llena: rcds4wr = 0, rcds4del = MAX;
	 * Memoria toda leida: rcds4rd = 0;
	 */

//	gprs_fat.wrPTR,gprs_fat.rdPTR, gprs_fat.delPTR,gprs_fat.rcds4wr,gprs_fat.rcds4rd,gprs_fat.rcds4del );

bool retS = false;

	memset( &gprs_fat, '\0', sizeof ( FAT_t));
	FAT_read(&gprs_fat);

	// Si hay registros para leer
	if ( gprs_fat.rcds4rd > 0) {
		retS = true;
	} else {
		retS = false;
		if ( systemVars.debug == DEBUG_GPRS ) {
			xprintf_P( PSTR("GPRS: bd EMPTY\r\n\0"));
		}
	}

	return(retS);
}
//------------------------------------------------------------------------------------
static bool pv_procesar_frame( void )
{
	// Intento MAX_TRYES de trasmitir un frame y recibir la respuesta correctamente

uint8_t intentos = 0;

	for ( intentos = 0; intentos < MAX_INIT_TRYES; intentos++ ) {

		if ( pv_trasmitir_frame() && pv_procesar_respuesta_server() ) {
			// Intento madar el frame al servidor
			// Aqui es que anduvo todo bien y debo salir para pasar al modo DATA
				if ( systemVars.debug == DEBUG_GPRS ) {
					xprintf_P( PSTR("GPRS: data packet sent.\r\n\0" ));
				}
				return(true);

		} else {

			// Espero 3s antes de reintentar
			vTaskDelay( (portTickType)( 3000 / portTICK_RATE_MS ) );
			if ( systemVars.debug == DEBUG_GPRS ) {
				xprintf_P( PSTR("GPRS: data packey retry(%d)\r\n\0"),intentos);
			}
		}
	}
	// No pude transmitir despues de MAX_TRYES
	return(false);

}
//------------------------------------------------------------------------------------
static bool pv_trasmitir_frame(void )
{
	// Hay datos que intento trasmitir.
	// Leo los mismos hasta completar un TX_WINDOW
	// El socket lo chequeo antes de comenzar a trasmitir. Una vez que comienzo
	// trasmito todo y paso a esperar la respuesta donde chequeo el socket nuevamente.
	// Intento enviar el mismo paquete de datos hasta 3 veces.
	// Entro luego de haber chequeado que hay registros para trasmitir o sea que se que los hay !!

uint8_t registros_trasmitidos = 0;
uint8_t i = 0;
t_socket_status socket_status = 0;

	for ( i = 0; i < MAX_TRYES_OPEN_SOCKET; i++ ) {

		socket_status = u_gprs_check_socket_status();

		if (  socket_status == SOCK_OPEN ) {
			// Envio un window frame
			registros_trasmitidos = 0;
			FF_rewind();

			u_gprs_flush_RX_buffer();
			u_gprs_flush_TX_buffer();

			u_gprs_tx_header("DATA");

			xCom_printf_P( fdGPRS,PSTR("&PLOAD=\0"));
			if ( systemVars.debug ==  DEBUG_GPRS ) {
				xprintf_P( PSTR("&PLOAD=\0"));
			}

			while ( pv_hay_datos_para_trasmitir() && ( registros_trasmitidos < MAX_RCDS_WINDOW_SIZE ) ) {
				pv_tx_data_payload();
				registros_trasmitidos++;
			}

			u_gprs_tx_tail();
			return(true);

		} else {

			// El socket esta cerrado: Doy el comando para abrirlo y espero
			u_gprs_open_socket();
		}

	}

	return(false);
}
//------------------------------------------------------------------------------------
static void pv_tx_data_payload( void )
{
	// Primero recupero los datos de la memoria haciendo el proceso inverso de
	// cuando los grabe en spx_tkData::pv_data_guardar_en_BD

size_t bRead;

	memset ( &gprs_fat, '\0', sizeof(FAT_t));
	memset ( &gprs_dr, '\0', sizeof( st_dataRecord_t));

	// Paso1: Leo un registro de memoria
	bRead = FF_readRcd( &gprs_dr, sizeof(st_dataRecord_t));
	FAT_read(&gprs_fat);

	if ( bRead == 0) {
		return;
	}

	xCom_printf_P( fdGPRS,PSTR("CTL:%d;\0"),gprs_fat.rdPTR );
	data_print_inputs(fdGPRS, &gprs_dr);

	if ( systemVars.debug ==  DEBUG_GPRS ) {
		xprintf_P( PSTR("CTL:%d;\0"), gprs_fat.rdPTR );
		data_print_inputs(fdTERM, &gprs_dr);
	}

	vTaskDelay( (portTickType)( 250 / portTICK_RATE_MS ) );

}
//------------------------------------------------------------------------------------
static bool pv_procesar_respuesta_server(void)
{
	// Me quedo hasta 10s esperando la respuesta del server al paquete de datos.
	// Salgo por timeout, socket cerrado, error del server o respuesta correcta
	// Si la respuesta es correcta, ejecuto la misma

	// <html><body><h1>TYPE=DATA&PLOAD=RX_OK:3;</h1></body></html>

uint8_t timeout = 0;
bool exit_flag = false;

	for ( timeout = 0; timeout < 10; timeout++) {

		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );	// Espero 1s

		if ( u_gprs_check_socket_status() != SOCK_OPEN ) {		// El socket se cerro
			exit_flag = false;
			goto EXIT;
		}

		if ( u_gprs_check_response ("</h1>\0")) {	// Recibi una respuesta del server.
			// Log.

			if ( systemVars.debug == DEBUG_GPRS ) {
				u_gprs_print_RX_Buffer();
			} else {
				u_gprs_print_RX_response();
			}

			if ( u_gprs_check_response ("ERROR\0")) {
				// ERROR del server: salgo inmediatamente
				exit_flag = false;
				goto EXIT;
			}

			if ( u_gprs_check_response ("RESET\0")) {
				// El sever mando la orden de resetearse inmediatamente
				pv_process_response_RESET();
			}

			if ( u_gprs_check_response ("MFORMAT\0")) {
				// El sever mando la orden de formatear la memoria y resetearse
				pv_process_response_MEMFORMAT();
			}

			if ( u_gprs_check_response ("PERF_OUTS\0")) {
				// El sever mando actualizacion de las salidas
				perforacion_process_gprs_response( (const char *)&commsRxBuffer.buffer );
			}

			if ( u_gprs_check_response ("TQS\0")) {
				// El sever mando actualizacion de los datos aun tanque
				tanque_process_gprs_response( (const char *)&commsRxBuffer.buffer );
			}

			if ( u_gprs_check_response ("RX_OK\0")) {
				// Datos procesados por el server.
				pv_process_response_OK();
				exit_flag = true;
				goto EXIT;
			}
		}
	}

// Exit:
EXIT:

	return(exit_flag);

}
//------------------------------------------------------------------------------------
static void pv_process_response_RESET(void)
{
	// El server me pide que me resetee de modo de mandar un nuevo init y reconfigurarme

	xprintf_P( PSTR("GPRS: RESET...\r\n\0"));

	vTaskDelay( ( TickType_t)( 1000 / portTICK_RATE_MS ) );
	// RESET
	CCPWrite( &RST.CTRL, RST_SWRST_bm );   /* Issue a Software Reset to initilize the CPU */

}
//------------------------------------------------------------------------------------
static void pv_process_response_MEMFORMAT(void)
{
	// El server me pide que me reformatee la memoria y me resetee

	xprintf_P( PSTR("GPRS: MFORMAT...\r\n\0"));

	vTaskDelay( ( TickType_t)( 1000 / portTICK_RATE_MS ) );
	//
	// Primero reseteo a default
	u_load_defaults(NULL);
	u_save_params_in_NVMEE();

	// Nadie debe usar la memoria !!!
	ctl_watchdog_kick(WDG_CMD, 0x8000 );

	vTaskSuspend( xHandle_tkAplicacion );
	ctl_watchdog_kick(WDG_APP, 0x8000 );

	vTaskSuspend( xHandle_tkInputs );
	ctl_watchdog_kick(WDG_DIN, 0x8000 );

	vTaskSuspend( xHandle_tkGprsTx );
	ctl_watchdog_kick(WDG_GPRSRX, 0x8000 );

	// No suspendo esta tarea porque estoy dentro de ella. !!!
	//vTaskSuspend( xHandle_tkGprsTx );
	ctl_watchdog_kick(WDG_GPRSRX, 0x8000 );

	// Formateo
	FF_format(true);

	// Reset
	CCPWrite( &RST.CTRL, RST_SWRST_bm );   /* Issue a Software Reset to initilize the CPU */

}
//------------------------------------------------------------------------------------
static uint8_t pv_process_response_OK(void)
{
	// Retorno la cantidad de registros procesados ( y borrados )
	// Recibi un OK del server y el ultimo ID de registro a borrar.
	// Los borro de a uno.

uint8_t recds_borrados = 0;

	memset ( &gprs_fat, '\0', sizeof( FAT_t));

	// Borro los registros.
	while (  u_check_more_Rcds4Del(&gprs_fat) ) {

		FF_deleteRcd();
		recds_borrados++;
		FAT_read(&gprs_fat);

		if ( systemVars.debug == DEBUG_GPRS ) {
			xprintf_P( PSTR("GPRS: mem wrPtr=%d,rdPtr=%d,delPtr=%d,r4wr=%d,r4rd=%d,r4del=%d \r\n\0"), gprs_fat.wrPTR,gprs_fat.rdPTR, gprs_fat.delPTR,gprs_fat.rcds4wr,gprs_fat.rcds4rd,gprs_fat.rcds4del );
		}

	}

	return(recds_borrados);
}
//------------------------------------------------------------------------------------
