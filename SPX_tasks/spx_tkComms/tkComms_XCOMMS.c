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

}
//------------------------------------------------------------------------------------
void xCOMMS_apagar_dispositivo(void)
{

	if ( systemVars.comms_channel == COMMS_CHANNEL_XBEE ) {
		xbee_apagar();
	} else if ( systemVars.comms_channel == COMMS_CHANNEL_GPRS ) {
		gprs_apagar();
	}

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
