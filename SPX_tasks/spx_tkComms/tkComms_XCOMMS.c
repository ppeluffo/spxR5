/*
 * tkComms_XCOMMS.c
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

static st_dataRecord_t dataRecord;

//* Para testing
/*
PGM_P const scan_list4[] PROGMEM = { apn_spy, ip_server_spy1 };
PGM_P const scan_list3[] PROGMEM = { apn_spy, ip_server_ute };
PGM_P const scan_list2[] PROGMEM = { apn_ose, ip_server_ose };
PGM_P const scan_list1[] PROGMEM = { apn_claro, ip_server_spy2 };
*/

//------------------------------------------------------------------------------------
void xCOMMS_config_defaults( char *opt )
{

char l_data[10] = { 0 };

	memcpy(l_data, opt, sizeof(l_data));
	strupr(l_data);

	if ( spx_io_board == SPX_IO8CH ) {
		sVarsComms.timerDial = 0;
	} else if ( spx_io_board == SPX_IO5CH ) {
		sVarsComms.timerDial = 900;
	}

	if ( strcmp_P( l_data, PSTR("SPY\0")) == 0) {
		snprintf_P( sVarsComms.apn, APN_LENGTH, PSTR("SPYMOVIL.VPNANTEL\0") );
		strncpy_P(sVarsComms.server_ip_address, PSTR("192.168.0.9\0"),16);

	} else if (strcmp_P( l_data, PSTR("UTE\0")) == 0) {
		snprintf_P( sVarsComms.apn, APN_LENGTH, PSTR("SPYMOVIL.VPNANTEL\0") );
		strncpy_P(sVarsComms.server_ip_address, PSTR("192.168.1.9\0"),16);

	} else if (strcmp_P( l_data, PSTR("OSE\0")) == 0) {
		snprintf_P( sVarsComms.apn, APN_LENGTH, PSTR("STG1.VPNANTEL\0") );
		strncpy_P(sVarsComms.server_ip_address, PSTR("172.27.0.26\0"),16);

	} else if (strcmp_P( l_data, PSTR("CLARO\0")) == 0) {
		snprintf_P( sVarsComms.apn, APN_LENGTH, PSTR("ipgrs.claro.com.uy\0") );
		strncpy_P(sVarsComms.server_ip_address, PSTR("190.64.69.34\0"),16);

	} else {
		snprintf_P( sVarsComms.apn, APN_LENGTH, PSTR("DEFAULT\0") );
		strncpy_P(sVarsComms.server_ip_address, PSTR("DEFAULT\0"),16);
	}

	snprintf_P( sVarsComms.dlgId, DLGID_LENGTH, PSTR("DEFAULT\0") );
	//strncpy_P(systemVars.gprs_conf.serverScript, PSTR("/cgi-bin/PY/spy.py\0"),SCRIPT_LENGTH);
	strncpy_P(sVarsComms.serverScript, PSTR("/cgi-bin/SPY/spy.py\0"),SCRIPT_LENGTH);
	strncpy_P(sVarsComms.server_tcp_port, PSTR("80\0"),PORT_LENGTH	);
    snprintf_P(sVarsComms.simpwd, sizeof(sVarsComms.simpwd), PSTR("%s\0"), SIMPIN_DEFAULT );

	// PWRSAVE
	if ( spx_io_board == SPX_IO5CH ) {
		sVarsComms.pwrSave.pwrs_enabled = true;
	} else if ( spx_io_board == SPX_IO8CH ) {
		sVarsComms.pwrSave.pwrs_enabled = false;
	}

	sVarsComms.pwrSave.hora_start.hour = 23;
	sVarsComms.pwrSave.hora_start.min = 30;
	sVarsComms.pwrSave.hora_fin.hour = 5;
	sVarsComms.pwrSave.hora_fin.min = 30;

}
//------------------------------------------------------------------------------------
void xCOMMS_status(void)
{

uint8_t dbm;

	xprintf_P( PSTR(">Device Gprs:\r\n\0"));
	xprintf_P( PSTR("  apn: %s\r\n\0"), sVarsComms.apn );
	xprintf_P( PSTR("  server ip:port: %s:%s\r\n\0"), sVarsComms.server_ip_address, sVarsComms.server_tcp_port );
	xprintf_P( PSTR("  server script: %s\r\n\0"), sVarsComms.serverScript );
	xprintf_P( PSTR("  simpwd: %s\r\n\0"), sVarsComms.simpwd );

	dbm = 113 - 2 * xCOMMS_stateVars.csq;
	xprintf_P( PSTR("  signalQ: csq=%d, dBm=%d\r\n\0"), xCOMMS_stateVars.csq, dbm );
	xprintf_P( PSTR("  ip address: %s\r\n\0"), xCOMMS_stateVars.ip_assigned) ;

	// TASK STATE
	switch (tkComms_state) {
	case ST_ESPERA_APAGADO:
		xprintf_P( PSTR("  state: await_OFF"));
		break;
	case ST_ESPERA_PRENDIDO:
		xprintf_P( PSTR("  state: await_ON"));
		break;
	case ST_PRENDER:
		xprintf_P( PSTR("  state: prendiendo"));
		break;
	case ST_CONFIGURAR:
		xprintf_P( PSTR("  state: configurando"));
		break;
	case ST_MON_SQE:
		xprintf_P( PSTR("  state: mon_sqe"));
		break;
	case ST_SCAN:
		xprintf_P( PSTR("  state: scanning"));
		break;
	case ST_INITFRAME:
		xprintf_P( PSTR("  state: link up: inits"));
		break;
	case ST_DATAFRAME:
		xprintf_P( PSTR("  state: link up: data"));
		break;
	default:
		xprintf_P( PSTR("  state: ERROR\r\n"));
		break;
	}

	xprintf_P( PSTR(" [%d,%d,%d]\r\n"),xCOMMS_stateVars.gprs_prendido, xCOMMS_stateVars.gprs_inicializado,xCOMMS_stateVars.errores_comms);
}
//------------------------------------------------------------------------------------
void xCOMMS_init(void)
{
	gprs_init();
	xCOMMS_stateVars.gprs_prendido = false;
	xCOMMS_stateVars.gprs_inicializado = false;
	xCOMMS_stateVars.errores_comms = 0;
}
//------------------------------------------------------------------------------------
void xCOMMS_apagar_dispositivo(void)
{
	gprs_apagar();
	xCOMMS_stateVars.gprs_prendido = false;
	xCOMMS_stateVars.gprs_inicializado = false;
}
//------------------------------------------------------------------------------------
bool xCOMMS_prender_dispositivo(void)
{
	/*
	 * El modem necesita que se le mande un AT y que responda un OK para
	 * confirmar que esta listo.
	 */

bool retS = false;

	xCOMMS_stateVars.gprs_prendido = true;
	retS = gprs_prender();
	if ( retS == false ) {
		// No prendio
		xCOMMS_stateVars.gprs_prendido = false;
	}

	return(retS);
}
//------------------------------------------------------------------------------------
bool xCOMMS_configurar_dispositivo(char *pin, char *apn, uint8_t *err_code )
{
	/*
	 * El modem necesita que se le mande un AT y que responda un OK para
	 * confirmar que esta listo.
	 * El Xbee no necesita nada.
	 */

bool retS = false;

	retS = gprs_configurar_dispositivo( pin, apn, err_code );
	return(retS);
}
//------------------------------------------------------------------------------------
void xCOMMS_mon_sqe( bool forever, uint8_t *csq )
{
	/*
	 * Solo en GPRS monitoreo la calidad de señal.
	 */

	gprs_mon_sqe( forever, csq);

}
//------------------------------------------------------------------------------------
bool xCOMMS_need_scan( void )
{

	// Veo si es necesario hacer un SCAN de la IP del server
	if ( ( strcmp_P( sVarsComms.apn, PSTR("DEFAULT\0")) == 0 ) ||
			( strcmp_P( sVarsComms.dlgId, PSTR("DEFAULT\0")) == 0 ) ||
			( strcmp_P( sVarsComms.dlgId, PSTR("DEFAULT\0")) == 0 ) ) {
		// Alguno de los parametros estan en DEFAULT.
		return(true);
	}

	return(false);

}
//------------------------------------------------------------------------------------
bool xCOMMS_scan(void)
{

	/*
	 * El proceso de SCAN de APN corresponde solo al GPRS
	 * El SERVER_IP y DLGID se aplica a ambos, gprs y xbee
	 *
	 */
	// Inicio un ciclo de SCAN
	// Pruebo con c/boundle de datos: el que me de OK es el correcto

	xprintf_PD( DF_COMMS, PSTR("COMMS: starting to SCAN...\r\n\0" ));

	// scan_list1: datos de Spymovil.
	if ( xCOMMS_scan_try( (PGM_P *)scan_list1 ))
		return(true);

	// scan_list2: datos de UTE.
	if ( xCOMMS_scan_try( (PGM_P *)scan_list2 ))
		return(true);

	// scan_list3: datos de OSE.
	if ( xCOMMS_scan_try( (PGM_P *)scan_list3 ))
		return(true);

	// scan_list4: datos de SPY PUBLIC_IP.
	if ( xCOMMS_scan_try( (PGM_P *)scan_list4 ))
		return(true);

	return(false);
}
//------------------------------------------------------------------------------------
bool xCOMMS_scan_try ( PGM_P *dlist )
{
	/*
	 * Recibe una lista de PGM_P cuyo primer elemento es un APN y el segundo una IP.
	 * Intenta configurar el APN y abrir un socket a la IP.
	 * Si lo hace manda un frame de SCAN en el que le devuelven el dlgid.
	 * Si todo esta bien, guarda los datos descubiertos en sVars. !!
	 */

char apn_tmp[APN_LENGTH];
char ip_tmp[IP_LENGTH];

	strcpy_P( apn_tmp, (PGM_P)pgm_read_word( &dlist[0]));
	strcpy_P( ip_tmp, (PGM_P)pgm_read_word( &dlist[1]));

	xprintf_PD( DF_COMMS, PSTR("COMMS: GPRS_SCAN trying APN:%s, IP:%s\r\n\0"), apn_tmp, ip_tmp );
	ctl_watchdog_kick( WDG_COMMS, WDG_TO600 );

	// Apago
	gprs_apagar();
	vTaskDelay( (portTickType)( 5000 / portTICK_RATE_MS ) );

	// Prendo
	if ( ! gprs_prender() )
		return(false);

	// Configuro
	// EL pin es el de default ya que si estoy aqui es porque no tengo configuracion valida.
	if (  ! gprs_configurar_dispositivo( sVarsComms.simpwd , apn_tmp, NULL ) ) {
		return(false);
	}

	// Envio un frame de SCAN al servidor.
	if ( xCOMMS_process_frame(SCAN, ip_tmp,"80") ) {
		// Resultado OK. Los parametros son correctos asi que los salvo en el systemVars. !!!
		// que es a donde esta apuntando el scan_boundle
		// El dlgid quedo salvado al procesar la respuesta.
		memset( sVarsComms.apn,'\0', APN_LENGTH );
		strncpy(sVarsComms.apn, apn_tmp, APN_LENGTH);
		memset( sVarsComms.server_ip_address,'\0', IP_LENGTH );
		strncpy(sVarsComms.server_ip_address, ip_tmp, IP_LENGTH);
		return(true);
	} else {
		return (false);
	}
}
//------------------------------------------------------------------------------------
bool xCOMMS_ipaddr( char *ip_assigned )
{
	//Leo la ip asignada
	return( gprs_IPADDR( ip_assigned ) == false );
}
//------------------------------------------------------------------------------------
void xCOMMS_flush_RX(void)
{
	/*
	 * Inicializa todos los buffers de recepcion para el canal activo.
	 * Reinicia el buffer que recibe de la uart del dispositivo
	 * de comunicaciones, y el buffer comun commsRxBuffer
	 */

	gprs_flush_RX_buffer();
	vTaskDelay( (portTickType)( 10 / portTICK_RATE_MS ) );
}
//------------------------------------------------------------------------------------
void xCOMMS_flush_TX(void)
{
	/*
	 * Inicializa todos los buffers de trasmision para el canal activo.
	 * Reinicia el buffer que transmite en la uart del dispositivo
	 * de comunicaciones
	 */

	gprs_flush_TX_buffer();
	vTaskDelay( (portTickType)( 10 / portTICK_RATE_MS ) );

}
//------------------------------------------------------------------------------------
int xCOMMS_check_response( uint16_t start,const char *pattern )
{
	return( gprs_check_response(start, pattern));
}
//------------------------------------------------------------------------------------
void xCOMMS_print_RX_buffer(void)
{
	gprs_print_RX_buffer();
}
//------------------------------------------------------------------------------------
void xCOMMS_rxbuffer_copy_to(char *dst, uint16_t start, uint16_t size )
{
	gprs_rxbuffer_copy_to( dst, start, size );
}
//------------------------------------------------------------------------------------
uint16_t xCOMMS_datos_para_transmitir(void)
{
/* Veo si hay datos en memoria para trasmitir
 * Memoria vacia: rcds4wr = MAX, rcds4del = 0;
 * Memoria llena: rcds4wr = 0, rcds4del = MAX;
 * Memoria toda leida: rcds4rd = 0;
 * gprs_fat.wrPTR, gprs_fat.rdPTR, gprs_fat.delPTR,gprs_fat.rcds4wr,gprs_fat.rcds4rd,gprs_fat.rcds4del
 */

uint16_t nro_recs_pendientes;
FAT_t fat;

	memset( &fat, '\0', sizeof ( FAT_t));
	FAT_read(&fat);

	nro_recs_pendientes = fat.rcds4rd;
	// Si hay registros para leer
	if ( nro_recs_pendientes == 0) {
		xprintf_PD( DF_COMMS, PSTR("COMMS: bd EMPTY\r\n\0"));
	}

	return(nro_recs_pendientes);
}
//------------------------------------------------------------------------------------
bool xCOMMS_SGN_FRAME_READY(void)
{
	if ( SPX_SIGNAL( SGN_FRAME_READY )) {
		SPX_CLEAR_SIGNAL( SGN_FRAME_READY );
		xprintf_PD( DF_COMMS, PSTR("COMMS: SGN_FRAME_READY rcvd.\r\n\0"));
		return (true);
	}
	return (false);
}
//------------------------------------------------------------------------------------
bool xCOMMS_SGN_REDIAL(void)
{
	if ( SPX_SIGNAL( SGN_REDIAL )) {
		SPX_CLEAR_SIGNAL( SGN_REDIAL );
		xprintf_PD( DF_COMMS, PSTR("COMMS: SGN_REDIAL rcvd.\r\n\0"));
		return (true);
	}
	return (false);
}
//------------------------------------------------------------------------------------
/*
 * Los comandos de xCOMMS son solo wrappers de los comandos de gprs de modo que al
 * implementar las maquinas de estado es este nivel podamos abstraernos de la
 * tecnologia de comunicaciones que implementemos ( gprs, xbee, lora, etc )
 */
// NET COMMANDS R2
//------------------------------------------------------------------------------------
bool xCOMMS_netopen(void)
{
	return( gprs_cmd_netopen() );
}
//------------------------------------------------------------------------------------
bool xCOMMS_netclose(void)
{
	return( gprs_cmd_netclose() );
}
//------------------------------------------------------------------------------------
bool xCOMMS_netstatus( t_net_status *net_status )
{
	return( gprs_cmd_netstatus(net_status) );
}
//------------------------------------------------------------------------------------
bool xCOMMS_linkopen( char *ip, char *port)
{
	return( gprs_cmd_linkopen(ip, port));
}
//------------------------------------------------------------------------------------
bool xCOMMS_linkclose( void )
{
	return( gprs_cmd_linkclose());
}
//------------------------------------------------------------------------------------
bool xCOMMS_linkstatus( t_link_status *link_status )
{
	return( gprs_cmd_linkstatus( link_status));
}
//------------------------------------------------------------------------------------
/*
 * Implemento las maquinas de estado de NET, LINK, DATA que van en el process_frame
 *
 */
//------------------------------------------------------------------------------------
bool xCOMMS_process_frame (t_frame tipo_frame, char *dst_ip, char *dst_port )
{
	/*
	 * Esta el la funcion que hace el trabajo de mandar un frame , esperar
	 * la respuesta y procesarla.
	 */

typedef enum { st_LINK, st_NET, st_SEND, st_RECEIVE, st_EXIT } t_frame_states;
t_frame_states state = st_LINK;
t_link_status link_status;
t_net_status net_status;
t_send_status send_status;
bool retS = false;
int8_t tryes;

#define MAXTRYES_SCAN	6
#define MAXTRYES_DATA	12
#define reset_tryes() tryes=MAXTRYES_DATA

	// Ajusto los intentos en SCAN ya que sino repito los errores.
	if ( tipo_frame == SCAN ) {
		tryes = 6;
	} else {
		reset_tryes();
	}

	while ( tryes-- > 0) {

		vTaskDelay( (portTickType)( 100 / portTICK_RATE_MS ) );
		xprintf_P(PSTR("DEBUG: xCOMMS_process_frame (%d)\r\n"), tryes);
		ctl_watchdog_kick(WDG_COMMS, WDG_COMMS_TO_PROCESSFRAME );

		switch(state) {

		case st_LINK:
			// Trigger
			link_status = fsm_LINK( dst_ip, dst_port );
			// Actions
			if ( link_status == LINK_OPEN ) {
				state = st_SEND;
			} else {
				state = st_NET;	// LINK UNKNOWN
			}
			break;

		case st_NET:
			// Trigger
			net_status = fsm_NET();
			// Actions
			if ( net_status == NET_OPEN ) {
				state = st_LINK;
			} else {
				retS = false;
				xCOMMS_netclose();
				state = st_EXIT;
			}
			break;

		case st_SEND:
			// Trigger
			send_status = fsm_SEND( tipo_frame );
			if ( send_status == SEND_NODATA ) {
				retS = true;
				state = st_EXIT;
			} else if (send_status == SEND_OK ) {
				state = st_RECEIVE;
			} else if ( send_status == SEND_FAIL ) {
				state = st_LINK;
				xCOMMS_linkclose();
			}
			break;

		case st_RECEIVE:
			if ( fsm_RECEIVE(tipo_frame) ) {
				// Vuelvo por si hay mas datos
				state = st_SEND;
				reset_tryes();
			} else {
				state = st_LINK;
				xCOMMS_linkclose();
			}
			break;

		case st_EXIT:
			return(retS);
		}
	}
	// Sali por reintentos.
	xCOMMS_linkclose();
	return(false);

}
//------------------------------------------------------------------------------------
t_net_status fsm_NET(void)
{
	/*
	 * Intento abrir el servicio de sockets.
	 * Reintento 3 veces abrirlo.
	 * En c/caso puede demorar hasta 30s en abrirse.
	 * En caso que algun comando no responda no salgo inmediatamente
	 * sino que espero 5s y lo reintento. Hacemos el mayor esfuerzo por conectarnos.
	 */

t_net_status net_status = NET_UNKNOWN;
int8_t net_tryes;
typedef enum { stNET_ENTRY, stNET_OPENING } t_net_states;
t_net_states net_state = stNET_ENTRY;
int8_t timeout;

	net_tryes = 4;
	while ( net_tryes-- > 0 ) {

		switch(net_state) {
		case stNET_ENTRY:
			xprintf_PD( DF_COMMS, PSTR("COMMS: fsm_NET-> ENTRY\r\n\0") );
			// Trigger
			xCOMMS_flush_RX();
			if ( xCOMMS_netstatus( &net_status) == false ) {
				// El comando NO respondio. Espero y reintento
				vTaskDelay( (portTickType)( 2000 / portTICK_RATE_MS ) );
				break;
			}
			// Actions
			if ( net_status == NET_OPEN ) {
				// El servicio de sockets esta abierto
				xprintf_PD( DF_COMMS, PSTR("COMMS: fsm_NET->is OPEN.\r\n\0") );
				return( NET_OPEN );
			}

			if ( net_status == NET_UNKNOWN ) {
				xprintf_PD( DF_COMMS, PSTR("COMMS: fsm_NET-> is UNKNOWN.\r\n\0") );
				// Cerramos el servicio volvemos a reintentar. No cambio de estado.
				if ( xCOMMS_netclose() == false) {
					xprintf_PD( DF_COMMS, PSTR("COMMS: fsm_NET-> gprs cmd not respond !!.\r\n\0") );
				}
				vTaskDelay( (portTickType)( 5000 / portTICK_RATE_MS ) );
				break;
			}

			if ( net_status == NET_CLOSE) {
				xprintf_PD( DF_COMMS, PSTR("COMMS: fsm_NET-> is CLOSE.\r\n\0") );
				if ( xCOMMS_netopen() == false ) {
					xprintf_PD( DF_COMMS, PSTR("COMMS: fsm_NET-> gprs cmd not respond !!.\r\n\0") );
					// No repondio. No cambio de estado
					vTaskDelay( (portTickType)( 5000 / portTICK_RATE_MS ) );
				} else {
					vTaskDelay( (portTickType)( 100 / portTICK_RATE_MS ) );
					timeout = 30;	// Espero hasta 30s que abra.
					net_state = stNET_OPENING;
				}
				break;
			}

			break;

		case stNET_OPENING:
			xprintf_PD( DF_COMMS, PSTR("COMMS: fsm_NET-> OPENING\r\n\0") );
			// Control
			// Entro con NET_CLOSE !!
			while ( timeout-- > 0) {
				xCOMMS_flush_RX();

				if ( xCOMMS_netstatus( &net_status) == false ) {
					// El comando NO respondio. Espero y reintento
					vTaskDelay( (portTickType)( 2000 / portTICK_RATE_MS ) );
					break;
				}

				if ( net_status == NET_OPEN ) {
					// El servicio de sockets esta abierto
					xprintf_PD( DF_COMMS, PSTR("COMMS: fsm_NET-> is OPEN.\r\n\0") );
					xCOMMS_ipaddr( xCOMMS_stateVars.ip_assigned );
					return(NET_OPEN);

				} else if ( net_status == NET_UNKNOWN ) {
					xprintf_PD( DF_COMMS, PSTR("COMMS: fsm_NET-> is UNKNOWN.\r\n\0") );
					// Cerramos el servicio.
					if ( xCOMMS_netclose() == false) {
						xprintf_PD( DF_COMMS, PSTR("COMMS: fsm_NET-> gprs cmd not respond !!.\r\n\0") );
					}
					vTaskDelay( (portTickType)( 5000 / portTICK_RATE_MS ) );
					net_state = stNET_ENTRY;

				} else if ( net_status == NET_CLOSE) {
					// Espero hasta 30s que abra. Me quedo en este estado ST2
					vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
				}
			} // End while timeout

			break;

		} // End switch

	} // End while.

	// Reintente muchas veces.
	xprintf_PD( DF_COMMS, PSTR("COMMS: fsm_NET Timeout.!!!\r\n\0") );
	return(NET_UNKNOWN);

}
//------------------------------------------------------------------------------------
t_link_status fsm_LINK(char *dst_ip, char *dst_port)
{
	/*
	 * Intento abrir el  socket.
	 * Reintento 3 veces abrirlo.
	 * En c/caso puede demorar hasta 30s en abrirse.
	 * En caso que algun comando no responda no salgo inmediatamente
	 * sino que espero 5s y lo reintento. Hacemos el mayor esfuerzo por conectarnos.
	 */

t_link_status link_status = LINK_UNKNOWN;
int8_t link_tryes;
typedef enum { stLINK_ENTRY, stLINK_OPENING } t_link_states;
t_link_states link_state = stLINK_ENTRY;
int8_t timeout;

	link_tryes = 4;
	while ( link_tryes-- > 0 ) {

		switch(link_state) {
		case stLINK_ENTRY:
			xprintf_PD( DF_COMMS, PSTR("COMMS: fsm_LINK-> ENTRY\r\n\0") );
			// Trigger
			xCOMMS_flush_RX();
			if ( xCOMMS_linkstatus( &link_status) == false ) {
				// El comando NO respondio. Espero y reintento
				vTaskDelay( (portTickType)( 2000 / portTICK_RATE_MS ) );
				break;
			}
			// Actions
			if ( link_status == LINK_OPEN ) {
				// El servicio de sockets esta abierto
				xprintf_PD( DF_COMMS, PSTR("COMMS: fsm_LINK->is OPEN.\r\n\0") );
				return( LINK_OPEN );
			}

			if ( link_status == LINK_UNKNOWN ) {
				xprintf_PD( DF_COMMS, PSTR("COMMS: fsm_LINK-> is UNKNOWN.\r\n\0") );
				// Cerramos el servicio volvemos a reintentar. No cambio de estado.
				if ( xCOMMS_linkclose() == false) {
					xprintf_PD( DF_COMMS, PSTR("COMMS: fsm_LINK-> gprs cmd not respond !!.\r\n\0") );
				}
				//vTaskDelay( (portTickType)( 5000 / portTICK_RATE_MS ) );
				return( LINK_UNKNOWN );
			}

			if ( link_status == LINK_CLOSE) {
				xprintf_PD( DF_COMMS, PSTR("COMMS: fsm_LINK-> is CLOSE.\r\n\0") );
				if ( xCOMMS_linkopen(dst_ip, dst_port) == false ) {
					xprintf_PD( DF_COMMS, PSTR("COMMS: fsm_LINK-> gprs cmd not respond !!.\r\n\0") );
					// No repondio. No cambio de estado
					vTaskDelay( (portTickType)( 5000 / portTICK_RATE_MS ) );
				} else {
					vTaskDelay( (portTickType)( 100 / portTICK_RATE_MS ) );
					timeout = 30;	// Espero hasta 30s que abra.
					link_state = stLINK_OPENING;
				}
				break;
			}

			break;

		case stLINK_OPENING:
			xprintf_PD( DF_COMMS, PSTR("COMMS: fsm_LINK-> OPENING\r\n\0") );
			// Control
			// Entro con NET_CLOSE !!
			while ( timeout-- > 0) {
				xCOMMS_flush_RX();

				if ( xCOMMS_linkstatus( &link_status) == false ) {
					// El comando NO respondio. Espero y reintento
					vTaskDelay( (portTickType)( 2000 / portTICK_RATE_MS ) );
					break;
				}

				if ( link_status == LINK_OPEN ) {
					// El servicio de sockets esta abierto
					xprintf_PD( DF_COMMS, PSTR("COMMS: fsm_LINK-> is OPEN.\r\n\0") );
					return(LINK_OPEN);

				} else if ( link_status == LINK_UNKNOWN ) {
					xprintf_PD( DF_COMMS, PSTR("COMMS: fsm_LINK-> is UNKNOWN.\r\n\0") );
					// Cerramos el servicio.
					if ( xCOMMS_linkclose() == false) {
						xprintf_PD( DF_COMMS, PSTR("COMMS: fsm_LINK-> gprs cmd not respond !!.\r\n\0") );
					}
					vTaskDelay( (portTickType)( 5000 / portTICK_RATE_MS ) );
					link_state = stLINK_ENTRY;

				} else if ( link_status == LINK_CLOSE) {
					// Espero hasta 30s que abra. Me quedo en este estado ST2
					vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
				}
			} // End while timeout

			break;

		} // End switch

	} // End while.

	// Reintente muchas veces.
	xprintf_PD( DF_COMMS, PSTR("COMMS: fsm_LINK Timeout.!!!\r\n\0") );
	return(LINK_UNKNOWN);

}
//------------------------------------------------------------------------------------
t_send_status fsm_SEND( t_frame tipo_frame )
{

t_send_status send_status = SEND_FAIL;

	// Envio el frame
	xprintf_P( PSTR("fsm_SEND.\r\n\0" ));

	switch(tipo_frame) {
	case DATA:
		send_status = xDATA_FRAME_send();
		break;
	case SCAN:
		send_status = xSCAN_FRAME_send();
		break;
	default:
		// Todos los tipos de init
		send_status = xINIT_FRAME_send(tipo_frame);
	}

	switch(send_status) {
	case SEND_NODATA:
		xprintf_PD( DF_COMMS, PSTR("COMMS: fsm_SEND No data->closing link.\r\n\0") );
		if ( xCOMMS_linkclose() == false) {
			xprintf_PD( DF_COMMS, PSTR("COMMS: fsm_LINK-> gprs cmd not respond !!.\r\n\0") );
		}
		break;
	case SEND_FAIL:
		xprintf_PD( DF_COMMS, PSTR("COMMS: fsm_SEND Fail.\r\n\0") );
		break;
	case SEND_OK:
		xprintf_PD( DF_COMMS, PSTR("COMMS: fsm_SEND ok.\r\n\0") );
		break;
	}

	return( send_status );
}
//------------------------------------------------------------------------------------
bool fsm_RECEIVE( t_frame tipo_frame )
{
	/*
	 * Se encarga de procesar resultados
	 */

t_responses response;
int8_t timeout;

	xprintf_P( PSTR("\r\nfsm_RECEIVE.\r\n\0" ));
	// Espero la respuesta
	timeout = 10;
	while(timeout-- > 0) {

		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );

		// Veo que no se halla cerrado el link
//		if ( xCOMMS_linkstatus( &link_status) == false ) {
			// El comando NO respondio. Espero y reintento
//			vTaskDelay( (portTickType)( 5000 / portTICK_RATE_MS ) );
//			return(false);
//		}

//		if ( link_status != LINK_OPEN ) {
//			return(false);
//		}

		// Analizo posibles respuestas
		if (tipo_frame == DATA ) {
			response =  xDATA_FRAME_process_response();
		} else if (tipo_frame == SCAN ) {
			response =  xSCAN_FRAME_process_response();
		} else {
			response =  xINIT_FRAME_process_response();
		}

		// Evaluo las respuestas
		if ( response == rsp_OK ) {
			// OK. Salgo
			return(true);
		} else 	if ( response == rsp_ERROR ){
			// Error a nivel del servidor.
			return(false);
		}
		// Espero porque la respuesta fue rsp_NONE

	}

	xprintf_P( PSTR("COMMS: fsm_RECEIVE->Timeout.\r\n\0" ));
	return(false);
}
//------------------------------------------------------------------------------------
void xCOMMS_xbuffer_init(void)
{
	memset(xcomms_buff.txbuff,'\0',XCOMMS_BUFFER_SIZE);
	xcomms_buff.ptr = 0;
}
//------------------------------------------------------------------------------------
void xCOMMS_xbuffer_load_dataRecord(void)
{

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

	xCOMMS_flush_RX();
	xCOMMS_flush_TX();
	xCOMMS_xbuffer_init();

	data_print_inputs(fdFILE, &dataRecord, fat.rdPTR );

}
//------------------------------------------------------------------------------------
int xCOMMS_xbuffer_send( bool dflag )
{
	/*
	 * envia un string por el comms port
	 */

int8_t timeout = 10;
int i = -1;
uint16_t size;

	// Doy el comando CIPSEND para enviar al modem.
	xCOMMS_flush_RX();
	xCOMMS_flush_TX();
	size = strlen(xcomms_buff.txbuff);
	//
	xfprintf_P( fdGPRS, PSTR("AT+CIPSEND=0,%d\r"),size);
	// Espero el prompt '>'
	vTaskDelay( (portTickType)( 100 / portTICK_RATE_MS ) );
	if ( xCOMMS_check_response(0, ">")  == -1 ) {
		xprintf_P(PSTR("SEND ERROR No prompt\r\n"));
		return(-1);
	}
	// Transmito
	if ( dflag ) {
		frtos_write(fdTERM, (char *)xcomms_buff.txbuff, size );
	}
	i = frtos_write(fdGPRS, (char *)xcomms_buff.txbuff, size );
	// Borro el buffer de rx.
	xCOMMS_flush_RX();
	// Espero la confirmacion del modem
	timeout = 10;
	while(timeout-- > 0) {
		vTaskDelay( (portTickType)( 10 / portTICK_RATE_MS ) );
		if ( xCOMMS_check_response(0, "+CIPSEND: 0") > 0) {
			if ( DF_COMMS ) {
				xCOMMS_print_RX_buffer();
			}
			return(i);
		}
	}

	// Sali por timeout
	xprintf_P(PSTR("SEND ERROR timeout\r\n"));
	if ( DF_COMMS) {

		xCOMMS_print_RX_buffer();
	}
	return(-1);

}
//------------------------------------------------------------------------------------
// FUNCIONES DE FRTOS-IO
// Funcion write File.
// La defino aqui porque es donde tengo el xbuffer.
//------------------------------------------------------------------------------------
int frtos_file_write( const char *pvBuffer, const uint16_t xBytes )
{
	// Append de *pvBuffer en xcomms_buff.txbuff

	xcomms_buff.ptr = strlen( xcomms_buff.txbuff );
	memcpy( &xcomms_buff.txbuff[xcomms_buff.ptr],  pvBuffer, xBytes );
	return(xBytes);
}
//------------------------------------------------------------------------------------





