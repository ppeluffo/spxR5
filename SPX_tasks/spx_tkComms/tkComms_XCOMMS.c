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
		systemVars.comms_conf.timerDial = 0;
	} else if ( spx_io_board == SPX_IO5CH ) {
		systemVars.comms_conf.timerDial = 900;
	}

	if (!strcmp_P( l_data, PSTR("SPY\0"))) {
		snprintf_P( systemVars.comms_conf.apn, APN_LENGTH, PSTR("SPYMOVIL.VPNANTEL\0") );
		strncpy_P(systemVars.comms_conf.server_ip_address, PSTR("192.168.0.9\0"),16);

	} else if (!strcmp_P( l_data, PSTR("UTE\0"))) {
		snprintf_P( systemVars.comms_conf.apn, APN_LENGTH, PSTR("SPYMOVIL.VPNANTEL\0") );
		strncpy_P(systemVars.comms_conf.server_ip_address, PSTR("192.168.1.9\0"),16);

	} else if (!strcmp_P( l_data, PSTR("OSE\0"))) {
		snprintf_P( systemVars.comms_conf.apn, APN_LENGTH, PSTR("STG1.VPNANTEL\0") );
		strncpy_P(systemVars.comms_conf.server_ip_address, PSTR("172.27.0.26\0"),16);

	} else if (!strcmp_P( l_data, PSTR("CLARO\0"))) {
		snprintf_P( systemVars.comms_conf.apn, APN_LENGTH, PSTR("ipgrs.claro.com.uy\0") );
		strncpy_P(systemVars.comms_conf.server_ip_address, PSTR("190.64.69.34\0"),16);

	} else {
		snprintf_P( systemVars.comms_conf.apn, APN_LENGTH, PSTR("DEFAULT\0") );
		strncpy_P(systemVars.comms_conf.server_ip_address, PSTR("DEFAULT\0"),16);
	}

	snprintf_P( systemVars.comms_conf.dlgId, DLGID_LENGTH, PSTR("DEFAULT\0") );
	//strncpy_P(systemVars.gprs_conf.serverScript, PSTR("/cgi-bin/PY/spy.py\0"),SCRIPT_LENGTH);
	strncpy_P(systemVars.comms_conf.serverScript, PSTR("/cgi-bin/SPY/spy.py\0"),SCRIPT_LENGTH);
	strncpy_P(systemVars.comms_conf.server_tcp_port, PSTR("80\0"),PORT_LENGTH	);
    snprintf_P(systemVars.comms_conf.simpwd, sizeof(systemVars.comms_conf.simpwd), PSTR("%s\0"), SIMPIN_DEFAULT );

	// PWRSAVE
	if ( spx_io_board == SPX_IO5CH ) {
		systemVars.comms_conf.pwrSave.pwrs_enabled = true;
	} else if ( spx_io_board == SPX_IO8CH ) {
		systemVars.comms_conf.pwrSave.pwrs_enabled = false;
	}

	systemVars.comms_conf.pwrSave.hora_start.hour = 23;
	systemVars.comms_conf.pwrSave.hora_start.min = 30;
	systemVars.comms_conf.pwrSave.hora_fin.hour = 5;
	systemVars.comms_conf.pwrSave.hora_fin.min = 30;

}
//------------------------------------------------------------------------------------
void xCOMMS_status(void)
{

uint8_t dbm;

	if ( systemVars.comms_channel == COMMS_CHANNEL_XBEE ) {
		xprintf_P( PSTR(">Device Xbee:\r\n\0"));
		return;

	} else if ( systemVars.comms_channel == COMMS_CHANNEL_GPRS ) {

		// Imprime todo lo referente al GPRS
		// MODEM
		xprintf_P( PSTR(">Device Gprs:\r\n\0"));
		dbm = 113 - 2 * xCOMMS_stateVars.csq;
		xprintf_P( PSTR("  signalQ: csq=%d, dBm=%d\r\n\0"), xCOMMS_stateVars.csq, dbm );
		xprintf_P( PSTR("  ip address: %s\r\n\0"), xCOMMS_stateVars.ip_assigned) ;

		// TASK STATE
		switch (tkComms_state) {
		case ST_ESPERA_APAGADO:
			xprintf_P( PSTR("  state: await_OFF\r\n"));
			break;
		case ST_ESPERA_PRENDIDO:
			xprintf_P( PSTR("  state: await_ON\r\n"));
			break;
		case ST_PRENDER:
			xprintf_P( PSTR("  state: prendiendo\r\n"));
			break;
		case ST_CONFIGURAR:
			xprintf_P( PSTR("  state: configurando\r\n"));
			break;
		case ST_MON_SQE:
			xprintf_P( PSTR("  state: mon_sqe\r\n"));
			break;
		case ST_SCAN:
			xprintf_P( PSTR("  state: scanning\r\n"));
			break;
		case ST_IP:
			xprintf_P( PSTR("  state: ip\r\n"));
			break;
		case ST_INITFRAME:
			xprintf_P( PSTR("  state: link up: inits\r\n"));
			break;
		case ST_DATAFRAME:
			xprintf_P( PSTR("  state: link up: data\r\n"));
			break;
		default:
			xprintf_P( PSTR("  state: ERROR\r\n"));
			break;
		}

	}

}
//------------------------------------------------------------------------------------
void xCOMMS_init(void)
{

	if ( systemVars.comms_channel == COMMS_CHANNEL_XBEE ) {
		xbee_init();
	} else if ( systemVars.comms_channel == COMMS_CHANNEL_GPRS ) {
		gprs_init();
	}

	xCOMMS_stateVars.dispositivo_prendido = false;
}
//------------------------------------------------------------------------------------
void xCOMMS_apagar_dispositivo(void)
{

	if ( systemVars.comms_channel == COMMS_CHANNEL_XBEE ) {
		xbee_apagar();
	} else if ( systemVars.comms_channel == COMMS_CHANNEL_GPRS ) {
		gprs_apagar();
	}

	xCOMMS_stateVars.dispositivo_prendido = false;
}
//------------------------------------------------------------------------------------
bool xCOMMS_prender_dispositivo(bool f_debug, uint8_t delay_factor)
{
	/*
	 * El modem necesita que se le mande un AT y que responda un OK para
	 * confirmar que esta listo.
	 * El Xbee no necesita nada.
	 */

bool retS = false;

	if ( systemVars.comms_channel == COMMS_CHANNEL_XBEE ) {
		retS = xbee_prender( f_debug, delay_factor);
	} else if ( systemVars.comms_channel == COMMS_CHANNEL_GPRS ) {
		retS = gprs_prender( f_debug, delay_factor);
	}

	if (retS )
		xCOMMS_stateVars.dispositivo_prendido = true;

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

	if ( systemVars.comms_channel == COMMS_CHANNEL_XBEE ) {
		retS = xbee_configurar_dispositivo(err_code);
	} else if ( systemVars.comms_channel == COMMS_CHANNEL_GPRS ) {
		retS = gprs_configurar_dispositivo( f_debug, pin, err_code );
	}

	return(retS);
}
//------------------------------------------------------------------------------------
void xCOMMS_mon_sqe(bool f_debug, bool modo_continuo, uint8_t *csq )
{
	/*
	 * Solo en GPRS monitoreo la calidad de señal.
	 */


	if ( systemVars.comms_channel == COMMS_CHANNEL_XBEE ) {
		xbee_mon_sqe();
	} else if ( systemVars.comms_channel == COMMS_CHANNEL_GPRS ) {
		gprs_mon_sqe( f_debug, modo_continuo, csq);
	}
}
//------------------------------------------------------------------------------------
bool xCOMMS_scan(bool f_debug, char *apn, char *ip_server, char *dlgid, uint8_t *err_code )
{

	/*
	 * El proceso de SCAN de APN corresponde solo al GPRS
	 * El SERVER_IP y DLGID se aplica a ambos, gprs y xbee
	 *
	 */

bool retS = false;

		if ( systemVars.comms_channel == COMMS_CHANNEL_XBEE ) {
			retS = xbee_scan(f_debug, ip_server, dlgid, err_code);
		} else if ( systemVars.comms_channel == COMMS_CHANNEL_GPRS ) {
			retS = gprs_scan(f_debug, apn, ip_server, dlgid, err_code);
		}

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

		if ( systemVars.comms_channel == COMMS_CHANNEL_XBEE ) {
			retS = xbee_ip();
		} else if ( systemVars.comms_channel == COMMS_CHANNEL_GPRS ) {
			retS = gprs_ip(f_debug, apn, ip_assigned, err_code);
		}

		return(retS);
}
//------------------------------------------------------------------------------------
t_link_status xCOMMS_link_status(void)
{

t_link_status lstatus = LINK_CLOSED;

	if ( systemVars.comms_channel == COMMS_CHANNEL_XBEE ) {
		lstatus = xbee_check_socket_status();
	} else if ( systemVars.comms_channel == COMMS_CHANNEL_GPRS ) {
		lstatus = gprs_check_socket_status();
	}

	return(lstatus);
}
//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
void xCOMMS_flush_RX(void)
{
	/*
	 * Inicializa todos los buffers de recepcion para el canal activo.
	 * Reinicia el buffer que recibe de la uart del dispositivo
	 * de comunicaciones, y el buffer comun commsRxBuffer
	 */

	if ( systemVars.comms_channel == COMMS_CHANNEL_XBEE ) {
		xbee_flush_RX_buffer();
	} else	if ( systemVars.comms_channel == COMMS_CHANNEL_GPRS ) {
		gprs_flush_RX_buffer();
	}
}
//------------------------------------------------------------------------------------
void xCOMMS_flush_TX(void)
{
	/*
	 * Inicializa todos los buffers de trasmision para el canal activo.
	 * Reinicia el buffer que transmite en la uart del dispositivo
	 * de comunicaciones
	 */

	if ( systemVars.comms_channel == COMMS_CHANNEL_XBEE ) {
		xbee_flush_TX_buffer();
	} else	if ( systemVars.comms_channel == COMMS_CHANNEL_GPRS ) {
		gprs_flush_TX_buffer();
	}
}
//------------------------------------------------------------------------------------
void xCOMMS_send_header(char *type)
{

	xprintf_PVD( xCOMMS_get_fd(), DF_COMMS, PSTR("GET %s?DLGID=%s&TYPE=%s&VER=%s\0" ), systemVars.comms_conf.serverScript, systemVars.comms_conf.dlgId, type, SPX_FW_REV );
}
//------------------------------------------------------------------------------------
void xCOMMS_send_tail(void)
{

	// ( No mando el close ya que espero la respuesta y no quiero que el socket se cierre )
	xprintf_PVD(  xCOMMS_get_fd(), DF_COMMS, PSTR(" HTTP/1.1\r\nHost: www.spymovil.com\r\n\r\n\r\n\0") );

	vTaskDelay( (portTickType)( 250 / portTICK_RATE_MS ) );
}
//------------------------------------------------------------------------------------
t_link_status xCOMMS_open_link(void)
{
	/*
	 * Intenta abrir el link hacia el servidor
	 * En caso de XBEE no hay que hacer nada
	 * En caso de GPRS se debe intentar abrir el socket
	 *
	 */

t_link_status lstatus = LINK_CLOSED;

	xCOMMS_flush_RX();

	if ( systemVars.comms_channel == COMMS_CHANNEL_XBEE ) {
		lstatus = xbee_open_socket();
	} else if ( systemVars.comms_channel == COMMS_CHANNEL_GPRS ) {
		lstatus = gprs_open_socket();
	}

	return(lstatus);

}
//------------------------------------------------------------------------------------
file_descriptor_t xCOMMS_get_fd(void)
{

file_descriptor_t fd = fdGPRS;

	if ( systemVars.comms_channel == COMMS_CHANNEL_XBEE ) {
		fd = fdXBEE;
	} else if ( systemVars.comms_channel == COMMS_CHANNEL_GPRS ) {
		fd = fdGPRS;
	}
	return(fd);

}
//------------------------------------------------------------------------------------
void xCOMM_send_global_params(void)
{
	if ( systemVars.comms_channel == COMMS_CHANNEL_XBEE ) {
		xprintf_PVD(  xCOMMS_get_fd(), DF_COMMS, PSTR("IMEI:0000;SIMID:000;CSQ:0;WRST:%02X;" ),wdg_resetCause );

	} else if ( systemVars.comms_channel == COMMS_CHANNEL_GPRS ) {
		xprintf_PVD(  xCOMMS_get_fd(), DF_COMMS, PSTR("IMEI:%s;" ), gprs_get_imei() );
		xprintf_PVD(  xCOMMS_get_fd(), DF_COMMS, PSTR("SIMID:%s;CSQ:%d;WRST:%02X;" ), gprs_get_ccid(), xCOMMS_stateVars.csq, wdg_resetCause );

	}

}
//------------------------------------------------------------------------------------
bool xCOMMS_check_response( const char *pattern )
{

	if ( systemVars.comms_channel == COMMS_CHANNEL_XBEE ) {
		return( xbee_check_response(pattern)) ;
	} else if ( systemVars.comms_channel == COMMS_CHANNEL_GPRS ) {
		return( gprs_check_response(pattern));
	}

	return(false);
}
//------------------------------------------------------------------------------------
void xCOMMS_print_RX_buffer(bool d_flag)
{
	if ( d_flag ) {
		if ( systemVars.comms_channel == COMMS_CHANNEL_XBEE ) {
			xbee_print_RX_buffer();
		} else if ( systemVars.comms_channel == COMMS_CHANNEL_GPRS ) {
			gprs_print_RX_buffer();
		}
	}
}
//------------------------------------------------------------------------------------
char *xCOMM_get_buffer_ptr( char *pattern)
{

	if ( systemVars.comms_channel == COMMS_CHANNEL_XBEE ) {
		return( xbee_get_buffer_ptr(pattern));
	} else if ( systemVars.comms_channel == COMMS_CHANNEL_GPRS ) {
		return( gprs_get_buffer_ptr(pattern));
	}

	return(NULL);
}
//------------------------------------------------------------------------------------
void xCOMMS_send_dr(bool d_flag, st_dataRecord_t *dr)
{
	/*
	 * Imprime un datarecord en un file descriptor dado.
	 * En caso de debug, lo muestra en xTERM.
	 */

	if ( systemVars.comms_channel == COMMS_CHANNEL_XBEE ) {
		data_print_inputs(fdXBEE, dr);
	} else if ( systemVars.comms_channel == COMMS_CHANNEL_GPRS ) {
		data_print_inputs(fdGPRS, dr);
	} else {
		return;
	}

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
		SPX_CLEAR_SIGNAL( SGN_REDIAL );
		xCOMMS_apagar_dispositivo();
		xprintf_PD( DF_COMMS, PSTR("COMMS: SGN_REDIAL rcvd.\r\n\0"));
		*next_state = ST_PRENDER;
		return(true);
	}

	if ( SPX_SIGNAL( SGN_FRAME_READY )) {
		/*
		 * En ESPERA_PRENDIDO debo salir al modo DATAFRAME a procesar el FRAME
		 * En los otros casos solo la ignoro ( borro ) pero no tomo acciones.
		 */
		SPX_CLEAR_SIGNAL( SGN_FRAME_READY );
		if ( state == ST_ESPERA_PRENDIDO ) {
			xprintf_PD( DF_COMMS, PSTR("COMMS: SGN_FRAME_READY rcvd.\r\n\0"));
			*next_state = ST_DATAFRAME;
			return(true);
		}
	}

	if ( SPX_SIGNAL( SGN_MON_SQE )) {
		/*
		 * La señal de monitorear sqe no la borro nunca ya que es un
		 * estado en el que entro en modo diagnostico y no debo salir mas.
		 * Aqui lo que hago es salir a prender el dispositivo y entrar a monitorear el sqe
		 */
		switch(state) {
		case ST_PRENDER:		// Ignoro
			break;
		case ST_CONFIGURAR:		// Ignoro
			break;
		case ST_MON_SQE:		// Ignoro
			break;
		default:
			xCOMMS_apagar_dispositivo();
			xprintf_PD( DF_COMMS, PSTR("COMMS: SGN_MON_SQE rcvd.\r\n\0"));
			*next_state = ST_PRENDER;
			return(true);
		}
	}

	if ( SPX_SIGNAL( SGN_RESET_COMMS_DEV )) {
		/*
		 * Debo resetear el dispositivo.
		 * Esto implica apagarlo y prenderlo
		 */
		SPX_CLEAR_SIGNAL( SGN_RESET_COMMS_DEV );
		xprintf_PD( DF_COMMS, PSTR("COMMS: SGN_RESET_COMMS_DEV rcvd.\r\n\0"));
		*next_state = ST_PRENDER;
		return(true);
	}

	return(false);
}
//------------------------------------------------------------------------------------

