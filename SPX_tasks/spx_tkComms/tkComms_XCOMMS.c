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
	case ST_IP:
		xprintf_P( PSTR("  state: ip"));
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
	xCOMMS_stateVars.reset_dlg = false;

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
	 * El Xbee no necesita nada.
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
bool xCOMMS_configurar_dispositivo(bool f_debug, char *pin, uint8_t *err_code )
{
	/*
	 * El modem necesita que se le mande un AT y que responda un OK para
	 * confirmar que esta listo.
	 * El Xbee no necesita nada.
	 */

bool retS = false;

	retS = gprs_configurar_dispositivo( f_debug, pin, err_code );
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
bool xCOMMS_ip(bool f_debug, char *apn, char *ip_assigned, uint8_t *err_code )
{

	/*
	 * El proceso de pedir una IP se aplica solo a GPRS
	 *
	 */

bool retS = false;

	retS = gprs_ip(f_debug, apn, ip_assigned, err_code);
	return(retS);
}
//------------------------------------------------------------------------------------
t_link_status xCOMMS_link_status( bool f_debug )
{

t_link_status lstatus = LINK_CLOSED;

	lstatus = gprs_check_socket_status( f_debug);
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
t_link_status xCOMMS_open_link(bool f_debug, char *ip, char *port)
{
	/*
	 * Intenta abrir el link hacia el servidor
	 * En caso de XBEE no hay que hacer nada
	 * En caso de GPRS se debe intentar abrir el socket
	 *
	 */

t_link_status lstatus = LINK_CLOSED;

	xCOMMS_flush_RX();
	lstatus = gprs_open_socket(f_debug, ip, port);
	return(lstatus);

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
bool xCOMMS_procesar_senales( t_comms_states state, t_comms_states *next_state )
{
	if ( SPX_SIGNAL( SGN_REDIAL )) {
		/*
		 * Debo apagar y prender el dispositivo. Como ya estoy apagado
		 * salgo para pasar al estado PRENDIENDO.
		 */
		xprintf_PD( DF_COMMS, PSTR("COMMS: SGN_REDIAL rcvd.\r\n\0"));
		SPX_CLEAR_SIGNAL( SGN_REDIAL );
		xCOMMS_apagar_dispositivo();
		*next_state = ST_PRENDER;
		return(true);
	}

	if ( SPX_SIGNAL( SGN_FRAME_READY )) {
		/*
		 * En ESPERA_PRENDIDO debo salir al modo DATAFRAME a procesar el FRAME
		 * En los otros casos solo la ignoro ( borro ) pero no tomo acciones.
		 * En ST_DATAFRAME no proceso esta señal.
		 */
		SPX_CLEAR_SIGNAL( SGN_FRAME_READY );
		xprintf_PD( DF_COMMS, PSTR("COMMS: SGN_FRAME_READY rcvd.\r\n\0"));

		switch (state) {
		case ST_ESPERA_PRENDIDO:
			if ( xCOMMS_stateVars.gprs_inicializado ) {
				*next_state = ST_DATAFRAME;
			} else {
				*next_state = ST_PRENDER;
			}
			return(true);
			break;
		case ST_DATAFRAME:
			// Ignoro la señal.
			return(false);
			break;
		default:
			return(true);
			break;
		}
	}

	if ( SPX_SIGNAL( SGN_MON_SQE )) {
		/*
		 * La señal de monitorear sqe no la borro nunca ya que es un
		 * estado en el que entro en modo diagnostico y no debo salir mas.
		 * Aqui lo que hago es salir a prender el dispositivo y entrar a monitorear el sqe
		 */
		xprintf_PD( DF_COMMS, PSTR("COMMS: SGN_MON_SQE rcvd.\r\n\0"));
		switch(state) {
		case ST_PRENDER:		// Ignoro
			break;
		case ST_CONFIGURAR:		// Ignoro
			break;
		case ST_MON_SQE:		// Ignoro
			break;
		default:
			xCOMMS_apagar_dispositivo();
			*next_state = ST_PRENDER;
		}
		return(true);
	}

	if ( SPX_SIGNAL( SGN_RESET_COMMS_DEV )) {
		/*
		 * Debo resetear el dispositivo.
		 * Esto implica apagarlo y prenderlo
		 */
		xprintf_PD( DF_COMMS, PSTR("COMMS: SGN_RESET_COMMS_DEV rcvd.\r\n\0"));
		SPX_CLEAR_SIGNAL( SGN_RESET_COMMS_DEV );
		*next_state = ST_PRENDER;
		return(true);
	}


	if ( SPX_SIGNAL( SGN_SMS )) {
		/*
		 * Solo las atiendo mientras estoy en modo espera.
		 * No borro la señal sino solo luego de haberlos procesado
		 */
		xprintf_PD( DF_COMMS, PSTR("COMMS: SGN_SMS rcvd.\r\n\0"));
		switch(state) {
		case ST_ESPERA_APAGADO:
			*next_state = ST_PRENDER;
			break;
		case ST_ESPERA_PRENDIDO:
			*next_state = ST_DATAFRAME;
			break;
		default:
			// Ignoro
			break;
		}
		return(true);
	}

	return(false);
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
