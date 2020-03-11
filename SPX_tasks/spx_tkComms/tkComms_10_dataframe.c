/*
 * tkComms_10_dataframe.c
 *
 *  Created on: 10 mar. 2020
 *      Author: pablo
 */

#include <tkComms.h>

// La tarea no puede demorar mas de 300s.

#define WDG_COMMS_TO_DATAFRAME	300

typedef enum { SST_ENTRY = 0, SST_ENVIAR_FRAME, SST_PROCESAR_RESPUESTA, SST_EXIT } t_data_state;

static t_data_state data_state;
static uint8_t intentos;
static st_dataRecord_t dataRecord;

#define MAX_INTENTOS_ENVIAR_DATA_FRAME	4

static t_data_state data_init(void);
static t_data_state data_enviar_frame(void);
static t_data_state data_procesar_respuesta(void);

static bool EV_hay_datos_para_transmitir(void);
static bool EV_envio_frame(void);
static bool EV_procesar_respuesta(void);

static void ac_send_data_record( void );
static void ac_process_response_RESET(void);
static void ac_process_response_MEMFORMAT(void);
static uint8_t ac_process_response_OK(void);

//------------------------------------------------------------------------------------
t_comms_states tkComms_st_dataframe(void)
{
	/* Estado en que procesa los frames de datos, los transmite y procesa
	 * las respuestas
	 * Si no hay datos para transmitir sale
	 * Si luego de varios reintentos no pudo, sale
	 */

t_comms_states next_state = ST_ENTRY;

// ENTRY

	xprintf_PD( DF_COMMS, PSTR("COMMS: IN st_dataframe.\r\n\0"));
	xprintf_P( PSTR("COMMS: dataframe.\r\n\0"));
	data_state = SST_ENTRY;

// LOOP
	for( ;; )
	{

		ctl_watchdog_kick(WDG_COMMS, WDG_COMMS_TO_DATAFRAME);

		// Proceso las señales:
		if ( xCOMMS_procesar_senales( ST_DATAFRAME , &next_state ) )
			goto EXIT;

		switch ( data_state ) {
		case SST_ENTRY:
			// Determina si hay datos o no para transmitir
			data_state = data_init();
			break;
		case  SST_ENVIAR_FRAME:
			data_state = data_enviar_frame();
			break;
		case SST_PROCESAR_RESPUESTA:
			data_state = data_procesar_respuesta();
			break;
		case SST_EXIT:
			// Salgo del estado.
			next_state = ST_ENTRY;
			goto EXIT;
			break;
		default:
			xprintf_P( PSTR("COMMS: data state ERROR !!.\r\n\0"));
			next_state = ST_ENTRY;
			goto EXIT;
		}
	}

// EXIT:
EXIT:

	xprintf_PD( DF_COMMS, PSTR("COMMS: OUT st_dataframe.\r\n\0"));
	return(next_state);

}
//------------------------------------------------------------------------------------
// ESTADOS LOCALES
//------------------------------------------------------------------------------------
static t_data_state data_init(void)
{
	/* Estado en que se determina si hay o no datos para transmitir.
	 * Si hay, inicializa las variables de intentos.
	 * So no hay, sale y pasa al estado superior de standby
	 */

	if ( EV_hay_datos_para_transmitir() ) {
		/* Inicializo el contador de errores y paso
		 * al estado que intento trasmitir los frames
		 */
		intentos =  MAX_INTENTOS_ENVIAR_DATA_FRAME;
		return(SST_ENVIAR_FRAME);

	} else {
		/* Al no haber datos, voy al estado local de EXIT
		 * y actualizo la variable de salida que voy a retornar al nivel superior
		 */
		return(SST_EXIT);
	}

	return(SST_EXIT);
}
//------------------------------------------------------------------------------------
static t_data_state data_enviar_frame(void)
{

t_data_state next_state;

	if ( EV_envio_frame() ) {

		next_state = SST_PROCESAR_RESPUESTA;

	} else {

		/* Despues de varios reintentos no pude trasmitir el bloque
		 * Debo mandar apagar y prender el dispositivo
		 * Salgo y fijo el estado del nivel superior
		 */
		SPX_SEND_SIGNAL(SGN_RESET_COMMS_DEV);
		next_state = SST_EXIT;
	}

	return(next_state);
}
//------------------------------------------------------------------------------------
static t_data_state data_procesar_respuesta(void)
{

t_data_state next_state;

	if ( EV_procesar_respuesta() ) {
		// Procese correctamente la respuesta. Veo si hay mas datos para transmitir.
		next_state = SST_ENTRY;

	} else {
		// Error al procesar: reintento enviar el frame
		next_state = SST_ENVIAR_FRAME;
	}

	return(next_state);
}
//------------------------------------------------------------------------------------
// FUNCIONES AUXILIARES
//------------------------------------------------------------------------------------
static bool EV_hay_datos_para_transmitir(void)
{
/* Veo si hay datos en memoria para trasmitir
 * Memoria vacia: rcds4wr = MAX, rcds4del = 0;
 * Memoria llena: rcds4wr = 0, rcds4del = MAX;
 * Memoria toda leida: rcds4rd = 0;
 * gprs_fat.wrPTR, gprs_fat.rdPTR, gprs_fat.delPTR,gprs_fat.rcds4wr,gprs_fat.rcds4rd,gprs_fat.rcds4del
 */

bool retS = false;
FAT_t fat;

	memset( &fat, '\0', sizeof ( FAT_t));
	FAT_read(&fat);

	// Si hay registros para leer
	if ( fat.rcds4rd > 0) {
		retS = true;
	} else {
		retS = false;
		xprintf_PD( DF_COMMS, PSTR("COMMS: bd EMPTY\r\n\0"));
	}

	return(retS);
}
//------------------------------------------------------------------------------------
static bool EV_envio_frame(void)
{
	/* Intenta enviar un frame.
	 * Un frame esta compuesto por varias lineas de datos
	 * Si se genera algun problema debo esperar 3secs antes de reintentar
	 */

uint8_t registros_trasmitidos = 0;
uint8_t i = 0;

	// Loop
	for ( i = 0; i < MAX_TRYES_OPEN_COMMLINK; i++ ) {

		if (  xCOMMS_link_status() == LINK_OPEN ) {
			// Envio un window frame
			registros_trasmitidos = 0;
			FF_rewind();

			xCOMMS_flush_RX();
			xCOMMS_flush_TX();

			xCOMMS_send_header("DATA");
			xprintf_PVD(  xCOMMS_get_fd(), DF_COMMS, PSTR("&PLOAD=\0") );

			while ( EV_hay_datos_para_transmitir() && ( registros_trasmitidos < MAX_RCDS_WINDOW_SIZE ) ) {
				ac_send_data_record();
				registros_trasmitidos++;
				// Espero 250ms entre records
				vTaskDelay( (portTickType)( 250 / portTICK_RATE_MS ) );
			}

			xCOMMS_send_tail();
			//  El bloque se trasmition OK. Paso a esperar la respuesta
			//
			return(true);

		} else {
			// No tengo enlace al server. Intento abrirlo
			vTaskDelay( (portTickType)( 3000 / portTICK_RATE_MS ) );
			xCOMMS_open_link();
		}
	}
	/*
	 * Despues de varios reintentos no logre enviar el frame
	 */
	return(false);

}
//------------------------------------------------------------------------------------
static bool EV_procesar_respuesta(void)
{
	/*
	 * Me quedo hasta 10s esperando la respuesta del server al paquete de datos.
	 * Salgo por timeout, socket cerrado, error del server o respuesta correcta
	 * Si la respuesta es correcta, ejecuto la misma
	 * Ej:
	 * <html><body><h1>TYPE=DATA&PLOAD=RX_OK:3;</h1></body></html>
	 */

uint8_t timeout = 0;

	for ( timeout = 0; timeout < 10; timeout++) {

		vTaskDelay( (portTickType)( 2000 / portTICK_RATE_MS ) );	// Espero 1s

		// El socket se cerro
		if ( xCOMMS_link_status() != LINK_OPEN ) {
			return(false);
		}

		//xCOMMS_print_RX_buffer(true);

		// Recibi un ERROR de respuesta
		if ( xCOMMS_check_response("ERROR") ) {
			xCOMMS_print_RX_buffer(true);
			return(false);
		}

		// Respuesta completa del server
		if ( xCOMMS_check_response("</h1>") ) {

			xCOMMS_print_RX_buffer( DF_COMMS );

			if ( xCOMMS_check_response ("ERROR\0")) {
				// ERROR del server: salgo inmediatamente
				return(false);
			}

			if ( xCOMMS_check_response ("RESET\0")) {
				// El sever mando la orden de resetearse inmediatamente
				ac_process_response_RESET();
			}

			if ( xCOMMS_check_response ("MFORMAT\0")) {
				// El sever mando la orden de formatear la memoria y resetearse
				ac_process_response_MEMFORMAT();
			}

			/*
			if ( xCOMMS_check_response ("PERF_OUTS\0")) {
				// El sever mando actualizacion de las salidas
				perforacion_process_gprs_response( (const char *)&commsRxBuffer.buffer );
			}

			if ( xCOMMS_check_response ("TQS\0")) {
				// El sever mando actualizacion de los datos aun tanque
				tanque_process_gprs_response( (const char *)&commsRxBuffer.buffer );
			}
			*/
			/*
			 * Lo ultimo que debo procesar es el OK !!!
			 */
			if ( xCOMMS_check_response ("RX_OK\0")) {
				// Datos procesados por el server.
				ac_process_response_OK();
				return(true);
			}
		}
	}

// Exit:

	return(false);

}
//------------------------------------------------------------------------------------
static void ac_send_data_record( void )
{
	/* Leo un registro de la memoria haciendo el proceso inverso de
	 * cuando los grabe en spx_tkData::pv_data_guardar_en_BD y lo
	 * mando por el canal de comunicaciones
	 */

size_t bRead;
FAT_t fat;

	memset ( &fat, '\0', sizeof(FAT_t));
	memset ( &dataRecord, '\0', sizeof( st_dataRecord_t));

	// Paso1: Leo un registro de memoria
	bRead = FF_readRcd( &dataRecord, sizeof(st_dataRecord_t));
	FAT_read(&fat);

	if ( bRead == 0) {
		return;
	}

	xprintf_PVD(  xCOMMS_get_fd(), DF_COMMS, PSTR("CTL:%d;\0"),fat.rdPTR );
	xCOMMS_send_dr(DF_COMMS, &dataRecord);

}
//------------------------------------------------------------------------------------
static void ac_process_response_RESET(void)
{
	// El server me pide que me resetee de modo de mandar un nuevo init y reconfigurarme

	xprintf_P( PSTR("COMMS: RESET...\r\n\0"));

	vTaskDelay( ( TickType_t)( 1000 / portTICK_RATE_MS ) );
	// RESET
	CCPWrite( &RST.CTRL, RST_SWRST_bm );   /* Issue a Software Reset to initilize the CPU */

}
//------------------------------------------------------------------------------------
static void ac_process_response_MEMFORMAT(void)
{
	// El server me pide que me reformatee la memoria y me resetee

	xprintf_P( PSTR("COMMS: MFORMAT...\r\n\0"));

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

	vTaskSuspend( xHandle_tkComms );
	ctl_watchdog_kick(WDG_COMMSRX, 0x8000 );

	// No suspendo esta tarea porque estoy dentro de ella. !!!
	//vTaskSuspend( xHandle_tkGprsTx );
	ctl_watchdog_kick(WDG_COMMSRX, 0x8000 );

	// Formateo
	FF_format(true);

	// Reset
	CCPWrite( &RST.CTRL, RST_SWRST_bm );   /* Issue a Software Reset to initilize the CPU */

}
//------------------------------------------------------------------------------------
static uint8_t ac_process_response_OK(void)
{
	/*
	 * Retorno la cantidad de registros procesados ( y borrados )
	 * Recibi un OK del server y el ultimo ID de registro a borrar.
	 * Los borro de a uno.
	 */

uint8_t recds_borrados = 0;
FAT_t fat;

	memset ( &fat, '\0', sizeof( FAT_t));

	// Borro los registros.
	while (  u_check_more_Rcds4Del(&fat) ) {
		FF_deleteRcd();
		recds_borrados++;
		FAT_read(&fat);
		xprintf_PD( DF_COMMS, PSTR("COMMS: mem wrPtr=%d,rdPtr=%d,delPtr=%d,r4wr=%d,r4rd=%d,r4del=%d \r\n\0"), fat.wrPTR, fat.rdPTR, fat.delPTR, fat.rcds4wr, fat.rcds4rd, fat.rcds4del );
	}

	return(recds_borrados);
}
//------------------------------------------------------------------------------------


