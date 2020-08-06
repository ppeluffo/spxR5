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
t_net_status xCOMMS_netopen( void )
{
	return( gprs_NETOPEN());
}
//------------------------------------------------------------------------------------
bool xCOMMS_ipaddr( char *ip_assigned )
{
	//Leo la ip asignada
	return( gprs_IPADDR( ip_assigned ) == false );
}
//------------------------------------------------------------------------------------
t_net_status  xCOMMS_netclose( void )
{
	return(gprs_NETCLOSE());
}
//------------------------------------------------------------------------------------
t_net_status  xCOMMS_netstatus( void )
{
	return(gprs_NET_status());
}
//------------------------------------------------------------------------------------
t_link_status xCOMMS_linkopen( char *ip, char *port)
{
	 // Intenta abrir el link hacia el servidor
	return( gprs_LINK_open( ip, port));
}
//------------------------------------------------------------------------------------
t_link_status xCOMMS_linkclose( void )
{

	return(gprs_LINK_close());
}
//------------------------------------------------------------------------------------
t_link_status xCOMMS_linkstatus( bool dcd_mode )
{
	return( gprs_LINK_status( dcd_mode) );
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
	vTaskDelay( (portTickType)( 100 / portTICK_RATE_MS ) );
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
	vTaskDelay( (portTickType)( 100 / portTICK_RATE_MS ) );

}
//------------------------------------------------------------------------------------
void xCOMMS_send_header(char *type)
{
	if ( strcmp(type,"SCAN") == 0 ) {
		xprintf_PVD( xCOMMS_get_fd(), DF_COMMS, PSTR("GET %s?DLGID=DEFAULT&TYPE=CTL&VER=%s\0" ), sVarsComms.serverScript, SPX_FW_REV );
	} else {
		// INIT, DATA
		xprintf_PVD( xCOMMS_get_fd(), DF_COMMS, PSTR("GET %s?DLGID=%s&TYPE=%s&VER=%s\0" ), sVarsComms.serverScript, sVarsComms.dlgId, type, SPX_FW_REV );
	}
}
//------------------------------------------------------------------------------------
void xCOMMS_send_tail(void)
{

	// ( No mando el close ya que espero la respuesta y no quiero que el socket se cierre )
	xprintf_PVD(  xCOMMS_get_fd(), DF_COMMS, PSTR(" HTTP/1.1\r\nHost: www.spymovil.com\r\n\r\n\r\n\0") );
	vTaskDelay( (portTickType)( 250 / portTICK_RATE_MS ) );
}
//------------------------------------------------------------------------------------
file_descriptor_t xCOMMS_get_fd(void)
{

file_descriptor_t fd = fdGPRS;

	fd = fdGPRS;
	return(fd);

}
//------------------------------------------------------------------------------------
bool xCOMMS_check_response( const char *pattern )
{
	return( gprs_check_response(pattern));
}
//------------------------------------------------------------------------------------
void xCOMMS_print_RX_buffer(void)
{
	gprs_print_RX_buffer();
}
//------------------------------------------------------------------------------------
char *xCOMM_get_buffer_ptr( char *pattern)
{
	return( gprs_get_buffer_ptr(pattern));
}
//------------------------------------------------------------------------------------
void xCOMMS_send_dr(bool d_flag, st_dataRecord_t *dr)
{
	/*
	 * Imprime un datarecord en un file descriptor dado.
	 * En caso de debug, lo muestra en xTERM.
	 */

	data_print_inputs(fdGPRS, dr);
	if (d_flag ) {
		data_print_inputs(fdTERM, dr);
	}

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
bool xCOMMS_process_frame (t_frame tipo_frame, char *dst_ip, char *dst_port )
{
	/*
	 * Esta el la funcion que hace el trabajo de mandar un frame , esperar
	 * la respuesta y procesarla.
	 */

t_frame_states fr_state = frame_ENTRY;
t_link_status link_status;
t_net_status net_status;
int8_t tryes;
int8_t timeout = 10 ;
t_responses frame_response = rsp_NONE;
bool retS = false;

#ifdef BETA_TEST
	xprintf_PD( DF_COMMS, PSTR("COMMS: IN  pf_fsm (type=%d).\r\n\0"),tipo_frame );
#endif

	// Ajusto los intentos en SCAN ya que sino repito los errores.
	if ( tipo_frame == SCAN ) {
		tryes = 4;
	} else {
		tryes = 9;
	}

	while (1) {

		switch(fr_state) {

		case frame_ENTRY:
#ifdef BETA_TEST
			xprintf_P( PSTR("COMMS: pf_fsm ENTRY(tryes=%d).\r\n\0" ), tryes );
#endif
			// Maximo esfuerzo. Salgo.
			if ( tryes-- <= 0 ) {
				retS = false;
				goto EXIT;
			}

			// Veo si el socket esta abierto( por dcd).
			link_status = xCOMMS_linkstatus(true );

			// Enlace TCP abierto ( socket )
			if ( link_status == LINK_OPEN ) {
				// Envio el frame
				if (tipo_frame == DATA ) {
					xDATA_FRAME_send();
				} else if (tipo_frame == SCAN ) {
					xSCAN_FRAME_send();
				} else {
					xINIT_FRAME_send(tipo_frame);
				}
				timeout = 10;
				fr_state = frame_RESPONSE;
				break;
			}

			// Enlace TCP cerrado ( socket )
			if ( link_status == LINK_CLOSE ) {
				fr_state = frame_NET;
				break;
			}

			break;

		case frame_NET:
#ifdef BETA_TEST
			xprintf_P( PSTR("COMMS: pf_fsm NET (tryes=%d).\r\n\0" ), tryes );
#endif
			// Siempre vulevo a ENTRY
			fr_state = frame_ENTRY;

			// El socket esta cerrado por lo tanto estoy en modo comando !!!
			gprs_switch_to_command_mode(true);

			// Veo si el servicio de sockets esta abierto.
			net_status = xCOMMS_netstatus();

			// NET open: Intento abrir el link.
			if ( net_status == NET_OPEN ) {
				link_status = xCOMMS_linkopen( dst_ip, dst_port );
				if ( link_status == LINK_OPEN ) {
					 break;
				}
				if ( link_status == LINK_CLOSE ) {
					gprs_switch_to_command_mode(true);
					break;
				}
				if ( link_status == LINK_UNKNOWN ) {
					gprs_switch_to_command_mode( true);
					break;
				}
				break;
			}

			// NET close: Intento abrir el servicio de sockets local.
			if ( net_status == NET_CLOSE ) {
				net_status = xCOMMS_netopen();
				if ( net_status == NET_OPEN ) {
					xCOMMS_ipaddr( xCOMMS_stateVars.ip_assigned );
					break;
				}
				// Algo paso que no pude abrir el servicio de NET
				// Dejo el sistema en modo comando
				gprs_switch_to_command_mode(true);
				break;
			}

			// NET unknown. Timeout ?.
			if ( net_status == NET_UNKNOWN ) {
				gprs_switch_to_command_mode( true);
				break;
			}

 			break;

		case frame_RESPONSE:
			// Estoy con el socket abierto en modo transparente !!
			// Antes de dar un comando debo pasarlo a modo comando !!
#ifdef BETA_TEST
			xprintf_P( PSTR("COMMS: pf_fsm RESPONSE(tryes=%d, to=%d).\r\n\0" ), tryes,timeout );
#endif

			// Sleep: espero 1s.
			vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
			timeout--;

			// Timeout de espera de respuesta
			// El socket esta abierto aún !!. Puedo reenviar la query.
			if ( timeout == 0 ) {
				xprintf_P( PSTR("COMMS: TIMEOUT !!.\r\n\0" ));
				fr_state = frame_ENTRY;
				break;
			}

			// Estado del link ( por DCD )
			link_status = xCOMMS_linkstatus( true );
			if ( link_status == LINK_CLOSE ) {
				// Se cerro: Aseguro el socket cerrado, Si no puedo me voy.
				gprs_switch_to_command_mode(true);
				xCOMMS_linkclose();
				net_status = xCOMMS_netclose();
				if ( net_status == NET_CLOSE ) {
					fr_state = frame_ENTRY;
					break;
				} else {
					// No puedo resetear el estado. Salgo a reintentar todo de nuevo.
					retS = false;
					goto EXIT;
				}
				break;
			}

			// Analizo posibles respuestas
			if (tipo_frame == DATA ) {
				frame_response =  xDATA_FRAME_process_response();
			} else if (tipo_frame == SCAN ) {
				frame_response =  xSCAN_FRAME_process_response();
			} else {
				frame_response =  xINIT_FRAME_process_response();
			}

			if ( frame_response == rsp_OK ) {
				// OK. Salgo
#ifdef BETA_TEST
				xprintf_PD( DF_COMMS, PSTR("COMMS: pf_fsm type %d OK !!\r\n\0"),tipo_frame );
#endif
				retS = true;
				goto EXIT;
			}

			if ( frame_response == rsp_ERROR ){
				// Error a nivel del servidor.
				retS = false;
				goto EXIT;
				break;
			}

			// En otro caso sigo esperando en el mismo estado.
			// frame_response == rsp_NONE
			break;

		default:
			xprintf_P( PSTR("COMMS: pf_fsm ERROR not known !!!\r\n\0" ) );
			xprintf_PD( DF_COMMS, PSTR("COMMS: pf_fsm type %d failed. State error !!!\r\n\0"),tipo_frame );
			retS = false;
			goto EXIT;
		}
	}

EXIT:

#ifdef BETA_TEST
	xprintf_PD( DF_COMMS, PSTR("COMMS: OUT pf_fsm.\r\n\0") );
#endif

	return(retS);
}
//------------------------------------------------------------------------------------

