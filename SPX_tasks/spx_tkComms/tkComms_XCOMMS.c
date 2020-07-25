/*
 * tkComms_XCOMMS.c
 *
 *  Created on: 5 mar. 2020
 *      Author: pablo
 */

#include "tkComms.h"

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
bool xCOMMS_prender_dispositivo(bool f_debug )
{
	/*
	 * El modem necesita que se le mande un AT y que responda un OK para
	 * confirmar que esta listo.
	 */

bool retS = false;

	xCOMMS_stateVars.gprs_prendido = true;
	retS = gprs_prender( f_debug );
	if ( retS == false ) {
		// No prendio
		xCOMMS_stateVars.gprs_prendido = false;
	}

	return(retS);
}
//------------------------------------------------------------------------------------
bool xCOMMS_configurar_dispositivo(bool f_debug, char *pin, char *apn, uint8_t *err_code )
{
	/*
	 * El modem necesita que se le mande un AT y que responda un OK para
	 * confirmar que esta listo.
	 * El Xbee no necesita nada.
	 */

bool retS = false;

	retS = gprs_configurar_dispositivo( f_debug, pin, apn, err_code );
	return(retS);
}
//------------------------------------------------------------------------------------
void xCOMMS_mon_sqe(bool f_debug, bool modo_continuo, uint8_t *csq )
{
	/*
	 * Solo en GPRS monitoreo la calidad de señal.
	 */

	gprs_mon_sqe( f_debug, modo_continuo, csq);

}
//------------------------------------------------------------------------------------
bool xCOMMS_scan(t_scan_struct *scan_boundle )
{

	/*
	 * El proceso de SCAN de APN corresponde solo al GPRS
	 * El SERVER_IP y DLGID se aplica a ambos, gprs y xbee
	 *
	 */

bool retS = false;

	retS = gprs_scan(scan_boundle);
	return(retS);
}
//------------------------------------------------------------------------------------
bool xCOMMS_need_scan( t_scan_struct *scan_boundle )
{

bool retS = false;

	retS = gprs_need_scan(scan_boundle);
	return(retS);
}
//------------------------------------------------------------------------------------
bool xCOMMS_netopen(bool f_debug, char *ip_assigned )
{

	if ( gprs_NETOPEN(f_debug ) == false ) {
		return(false);
	}

	//Leo la ip asignada
	if ( gprs_IPADDR(f_debug, ip_assigned ) == false ) {
		return(false);
	}

	return(true);

}
//------------------------------------------------------------------------------------
bool xCOMMS_netclose(bool f_debug)
{
	return(gprs_NETCLOSE(f_debug));
}
//------------------------------------------------------------------------------------
bool xCOMMS_net_status(bool f_debug)
{
	return(gprs_check_NETOPEN_status(f_debug));
}
//------------------------------------------------------------------------------------
t_link_status xCOMMS_open_link(bool f_debug, char *ip, char *port)
{
	/*
	 * Intenta abrir el link hacia el servidor
	 * En caso de XBEE no hay que hacer nada
	 * En caso de GPRS se debe intentar abrir el socket
	 *
	 */

t_link_status lstatus = LINK_CLOSED;

	// Paso a modo comando
	gprs_switch_to_command_mode(f_debug);

	lstatus = gprs_open_connection(f_debug, ip, port);
	return(lstatus);

}
//------------------------------------------------------------------------------------
void xCOMMS_close_link(bool f_debug )
{

	// Paso a modo comando
	gprs_switch_to_command_mode(f_debug);
	gprs_close_connection(f_debug);
}
//------------------------------------------------------------------------------------
t_link_status xCOMMS_link_status( bool f_debug )
{

t_link_status lstatus = LINK_CLOSED;

	lstatus = gprs_check_connection_status( f_debug);
	return(lstatus);
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
	xprintf_PVD( xCOMMS_get_fd(), DF_COMMS, PSTR("GET %s?DLGID=%s&TYPE=%s&VER=%s\0" ), sVarsComms.serverScript, sVarsComms.dlgId, type, SPX_FW_REV );
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
void xCOMMS_print_RX_buffer(bool d_flag)
{
	gprs_print_RX_buffer(d_flag);

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
bool xCOMMS_process_frame (t_frame tipo_frame )
{
	/*
	 * Esta el la funcion que hace el trabajo de mandar un frame , esperar
	 * la respuesta y procesarla.
	 */

t_frame_states fr_state = frame_ENTRY;
t_link_status link_status;
int8_t tryes = 9;
int8_t timeout = 10 ;
t_responses rsp;
bool net_status;
bool retS = false;

	xprintf_PD( DF_COMMS, PSTR("COMMS: IN  pf_fsm (type=%d).\r\n\0"),tipo_frame );

	while (1) {

		switch(fr_state) {

		case frame_ENTRY:
			xprintf_P( PSTR("COMMS: pf_fsm ENTRY(tryes=%d).\r\n\0" ), tryes );

			if ( tryes <= 0 ) {
				retS = false;
				goto EXIT;
			}

			// Veo si el socket esta abierto.
			link_status = xCOMMS_link_status(DF_COMMS);
			if ( link_status == LINK_OPEN ) {
				// Envio el frame
				if (tipo_frame == DATA ) {
					xDATA_FRAME_send();
				} else {
					xINIT_FRAME_send(tipo_frame);
				}
				tryes--;
				timeout = 10;
				fr_state = frame_RESPONSE;
			} else {
				// Socket cerrado:
				fr_state = frame_NET;
				break;
			}
			break;

		case frame_NET:
			xprintf_P( PSTR("COMMS: pf_fsm NET (tryes=%d).\r\n\0" ), tryes );
			// El socket esta cerrado por lo tanto estoy en modo comando !!!
			// Veo si el servicio de sockets esta abierto.
			net_status = xCOMMS_net_status( DF_COMMS );
			tryes--;
			if ( net_status == true) {
				// NETOPEN: Intento abrir el socket
				link_status = xCOMMS_open_link(DF_COMMS, sVarsComms.server_ip_address, sVarsComms.server_tcp_port );
			} else {
				// NETCLOSE: intento abrir el servicio de sockets
				if ( xCOMMS_netopen( DF_COMMS, xCOMMS_stateVars.ip_assigned ) == false ) {
					// No puede abrir el servicio local de sockets o leer la ip.
					// Intento cerrarlo antes de seguir. Si no puedo salgo y me voy
					if ( xCOMMS_netclose(DF_COMMS) == false ) {
						retS = false;
						goto EXIT;
					}
				}
			}
			fr_state = frame_ENTRY;
 			break;

		case frame_RESPONSE:
			// Estoy con el socket abierto en modo transparente !!
			// Antes de dar un comando debo pasarlo a modo comando !!
			xprintf_P( PSTR("COMMS: pf_fsm RESPONSE(tryes=%d, to=%d).\r\n\0" ), tryes,timeout );

			vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );	// Espero 1s
			timeout--;

			// Estado del link
			link_status = xCOMMS_link_status(DF_COMMS);
			if ( link_status != LINK_OPEN ) {
				// Se cerro: Aseguro el socket cerrado, Si no puedo me voy.
				xCOMMS_close_link(DF_COMMS);
				if ( xCOMMS_netclose(DF_COMMS) == false ) {
					retS = false;
					goto EXIT;
				} else {
					fr_state = frame_ENTRY;
					break;
				}
			}

			// Timeout de espera de respuesta
			if ( timeout == 0 ) {
				xprintf_P( PSTR("COMMS: TIMEOUT !!.\r\n\0" ));
				xCOMMS_close_link(DF_COMMS);
				if ( xCOMMS_netclose(DF_COMMS) == false ) {
					retS = false;
					goto EXIT;
				} else {
					fr_state = frame_ENTRY;
					break;
				}
			}

			// Analizo posibles respuestas
			if (tipo_frame == DATA ) {
				rsp =  xDATA_FRAME_process_response();
			} else {
				rsp =  xINIT_FRAME_process_response();
			}

			if ( rsp == rsp_OK ) {
				// OK. Salgo
				xprintf_PD( DF_COMMS, PSTR("COMMS: pf_fsm type %d OK !!\r\n\0"),tipo_frame );
				retS = true;
				goto EXIT;
			}

			if ( rsp == rsp_ERROR ){
				// Error a nivel del servidor.
				retS = false;
				goto EXIT;
				break;
			}

			// En otro caso sigo esperando en el mismo estado.
			break;

		default:
			xprintf_P( PSTR("COMMS: pf_fsm ERROR not known !!!\r\n\0" ) );
			xprintf_PD( DF_COMMS, PSTR("COMMS: pf_fsm type %d failed. State error !!!\r\n\0"),tipo_frame );
			retS = false;
			goto EXIT;
		}
	}

EXIT:

	xprintf_PD( DF_COMMS, PSTR("COMMS: OUT pf_fsm.\r\n\0") );
	return(retS);
}
//------------------------------------------------------------------------------------
