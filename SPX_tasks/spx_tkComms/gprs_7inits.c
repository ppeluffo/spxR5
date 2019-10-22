/*
 * gprs_7ifup.c
 *
 *  Created on: 9 oct. 2019
 *      Author: pablo
 */

#include <spx_tkComms/gprs.h>
#include "spx.h"

// La tarea no puede demorar mas de 180s.
#define WDG_GPRS_INITS	180

bool f_send_init_base = false;
bool f_send_init_analog = false;
bool f_send_init_digital = false;
bool f_send_init_counters = false;
bool f_send_init_range = false;
bool f_send_init_psensor = false;
bool f_send_init_outputs = false;

typedef enum { GLOBAL=0, BASE, ANALOG, DIGITAL, COUNTERS, RANGE, PSENSOR, OUTPUTS } init_frames_t;

static bool pv_send_frame_init(init_frames_t tipo);
static bool pv_tx_frame_init(init_frames_t tipo);
static void pv_tx_init_payload(init_frames_t tipo);
static void pv_tx_init_payload_global(void);
static void pv_tx_init_payload_base(void);
static void pv_tx_init_payload_analog(void);
static void pv_tx_init_payload_digital(void);
static void pv_tx_init_payload_counters(void);
static void pv_tx_init_payload_range(void);
static void pv_tx_init_payload_psensor(void);
static void pv_tx_init_payload_outputs(void);

static t_frame_responses pv_init_process_response(void);
static void pv_init_reconfigure_params_global(void);
static void pv_init_reconfigure_params_base(void);
static void pv_init_reconfigure_params_analog(void);
static void pv_init_reconfigure_params_digital(void);
static void pv_init_reconfigure_params_counters(void);
static void pv_init_reconfigure_params_range(void);
static void pv_init_reconfigure_params_psensor(void);
static void pv_init_reconfigure_params_outputs(void);

//------------------------------------------------------------------------------------
bool st_gprs_inits(void)
{

	// Send Init Frame
	// GET /cgi-bin/spx/SPYR2.pl?DLGID=TEST02&TYPE=INIT&VER=2.0.6&PLOAD=[SIMPWD:DEFAULT;IMEI:860585004367917;
	//                                                                 SIMID:895980161423091055;BASE:0x32;AN:0xCB;DG:0x1A;CNT:0x47;RG:0xF7;PSE:0x73;OUT:0xB2]
	// HTTP/1.1
	// Host: www.spymovil.com
	// Connection: close\r\r ( no mando el close )


	GPRS_stateVars.state = G_INITS;

	f_send_init_base = false;
	f_send_init_analog = false;
	f_send_init_digital = false;
	f_send_init_counters = false;
	f_send_init_range = false;
	f_send_init_psensor = false;
	f_send_init_outputs = false;

	// El resultado del frame GLOBAL es el que prende las flags de que
	// otras inits deben enviarse.
	if ( ! pv_send_frame_init(GLOBAL) ) {
		return(bool_RESTART);
	}

	// Configuracion BASE
	if ( f_send_init_base ) {
		if ( ! pv_send_frame_init(BASE) ) {
			return(bool_RESTART);
		}
	}

	// Configuracion ANALOG
	if ( f_send_init_analog ) {
		if ( ! pv_send_frame_init(ANALOG) ) {
			return(bool_RESTART);
		}
	}

	// Configuracion DIGITAL
	if ( f_send_init_digital ) {
		if ( ! pv_send_frame_init(DIGITAL) ) {
			return(bool_RESTART);
		}
	}

	// Configuracion COUNTERS
	if ( f_send_init_counters ) {
		if ( ! pv_send_frame_init(COUNTERS) ) {
			return(bool_RESTART);
		}
	}

	// Configuracion RANGE
	if ( f_send_init_range ) {
		if ( ! pv_send_frame_init(RANGE) ) {
			return(bool_RESTART);
		}
	}

	// Configuracion PSENSOR
	if ( f_send_init_psensor ) {
		if ( ! pv_send_frame_init(PSENSOR) ) {
			return(bool_RESTART);
		}
	}

	return(bool_CONTINUAR);

	// Configuracion OUTPUTS
	if ( f_send_init_outputs ) {
		if ( ! pv_send_frame_init(OUTPUTS) ) {
			return(bool_RESTART);
		}
	}

	return(bool_CONTINUAR);
}
//------------------------------------------------------------------------------------
void gprs_init_test(void)
{
	//xprintf_P( PSTR("Mensaje de prueba PAblo Tomas Peluffo Fleitas.\r\r\0"));
	// nacido en PAndo el 30 de mayo del 1964
	//xCom_printf_P( fdTERM, PSTR("PLOAD=CLASS:BASE;TDIAL:%d;TPOLL:%d;PWRS_MODO:ON;PWRS_START:0630;PWRS_END:1230\r\n\0"), systemVars.gprs_conf.timerDial,systemVars.timerPoll );
	xprintf_P( PSTR("TDIAL:%d;"), systemVars.gprs_conf.timerDial);
	xprintf_P( PSTR("TPOLL:%d;"), systemVars.gprs_conf.timerDial );
	xprintf_P( PSTR("PWRS_START:%02d:%02d;"), systemVars.gprs_conf.pwrSave.hora_start.hour, systemVars.gprs_conf.pwrSave.hora_start.min );
	xprintf_P( PSTR("PWRS_END:%02d:%02d"), systemVars.gprs_conf.pwrSave.hora_fin.hour, systemVars.gprs_conf.pwrSave.hora_fin.min);

	xprintf_P( PSTR("\r\n"));
//	xprintf_P(  PSTR("  TDIAL:%d;TPOLL:%d; pwrsave: ON, start[%02d:%02d], end[%02d:%02d]\r\n\0"), systemVars.gprs_conf.timerDial, systemVars.gprs_conf.pwrSave.hora_start.hour, systemVars.gprs_conf.pwrSave.hora_start.min, systemVars.gprs_conf.pwrSave.hora_fin.hour, systemVars.gprs_conf.pwrSave.hora_fin.min);

//
}
//------------------------------------------------------------------------------------
static bool pv_send_frame_init(init_frames_t tipo)
{
	// Debo mandar el frame de init al server, esperar la respuesta, analizarla
	// y reconfigurarme.
	// Intento 3 veces antes de darme por vencido.
	// El socket puede estar abierto o cerrado. Lo debo determinar en c/caso y
	// si esta cerrado abrirlo.
	// Mientras espero la respuesta debo monitorear que el socket no se cierre

uint8_t intentos = 0;
bool exit_flag = bool_RESTART;

// Entry:

	GPRS_stateVars.state = G_INITS;

	// En open_socket uso la IP del GPRS_stateVars asi que antes debo copiarla.
	strcpy( GPRS_stateVars.server_ip_address, systemVars.gprs_conf.server_ip_address );

	ctl_watchdog_kick(WDG_GPRSTX, WDG_GPRS_INITS );

	xprintf_P( PSTR("GPRS: iniframe(%d)\r\n\0"),tipo);

	// Intenteo MAX_INIT_TRYES procesar correctamente el INIT
	for ( intentos = 0; intentos < MAX_INIT_TRYES; intentos++ ) {

		if ( pv_tx_frame_init( tipo ) ) {

			switch( pv_init_process_response() ) {

			case FRAME_ERROR:
				// Reintento
				break;
			case FRAME_SOCK_CLOSE:
				// Reintento
				break;
			case FRAME_RETRY:
				// Reintento
				break;
			case FRAME_OK:
				// Aqui es que anduvo todo bien y continuo.
				if ( systemVars.debug == DEBUG_GPRS ) {
					xprintf_P( PSTR("\r\nGPRS: Init frame OK.\r\n\0" ));
				}
				exit_flag = bool_CONTINUAR;
				goto EXIT;
				break;
			case FRAME_NOT_ALLOWED:
				// Respondio bien pero debo salir a apagarme
				exit_flag = bool_RESTART;
				goto EXIT;
				break;
			case FRAME_ERR404:
				// No existe el recurso en el server
				exit_flag = bool_RESTART;
				goto EXIT;
				break;
			case FRAME_SRV_ERR:
				// Problemas en el servidor
				exit_flag = bool_RESTART;
				goto EXIT;
				break;
			}

		} else {

			if ( systemVars.debug == DEBUG_GPRS ) {
				xprintf_P( PSTR("GPRS: iniframe retry(%d)\r\n\0"),intentos);
			}

			// Espero 3s antes de reintentar
			vTaskDelay( (portTickType)( 3000 / portTICK_RATE_MS ) );
		}
	}

	// Aqui es que no puede enviar/procesar el INIT correctamente
	if ( systemVars.debug == DEBUG_GPRS ) {
		xprintf_P( PSTR("GPRS: Init frame FAIL !!.\r\n\0" ));
	}

// Exit
EXIT:

	return(exit_flag);

}
//------------------------------------------------------------------------------------
static bool pv_tx_frame_init(init_frames_t tipo)
{
	//  Aqui es donde trasmito el frame.
	//	GET /cgi-bin/spx/SPYR2.pl?DLGID=TEST02&TYPE=INIT&VER=2.0.6&PLOAD=[....] HTTP/1.1
	//	Host: www.spymovil.com
	//  Connection: close\r\r ( no mando el close )

uint8_t intentos = 0;
t_socket_status socket_status = 0;

	if ( systemVars.debug == DEBUG_GPRS  ) {
		xprintf_P( PSTR("GPRS: initframe: Sent\r\n\0"));
	}

	for ( intentos = 0; intentos < MAX_TRYES_OPEN_SOCKET; intentos++ ) {

		socket_status = u_gprs_check_socket_status();

		// Si el socket esta abierto trasmito y salgo a esperar la respuesta.
		if (  socket_status == SOCK_OPEN ) {
			u_gprs_flush_RX_buffer();
			u_gprs_flush_TX_buffer();

			u_gprs_tx_header("INIT");
			pv_tx_init_payload(tipo);
			u_gprs_tx_tail();

			return(true);
		}

		// Si el socket dio falla, debo reiniciar la conexion.
		if ( socket_status == SOCK_FAIL ) {
			return(false);
		}

		// El socket esta cerrado: Doy el comando para abrirlo y espero
		u_gprs_open_socket();

	}

	return(false);

}
//------------------------------------------------------------------------------------
static void pv_tx_init_payload(init_frames_t tipo)
{

	switch(tipo) {
	case GLOBAL:
		pv_tx_init_payload_global();
		break;
	case BASE:
		pv_tx_init_payload_base();
		break;
	case ANALOG:
		pv_tx_init_payload_analog();
		break;
	case DIGITAL:
		pv_tx_init_payload_digital();
		break;
	case COUNTERS:
		pv_tx_init_payload_counters();
		break;
	case RANGE:
		pv_tx_init_payload_range();
		break;
	case PSENSOR:
		pv_tx_init_payload_psensor();
		break;
	case OUTPUTS:
		pv_tx_init_payload_outputs();
		break;
	}
}
//------------------------------------------------------------------------------------
static void pv_tx_init_payload_global(void)
{
uint8_t base_cks, an_cks, dig_cks, cnt_cks, range_cks, psens_cks, out_cks;

	base_cks = u_base_checksum();
	an_cks = ainputs_checksum();
	dig_cks = dinputs_checksum();
	cnt_cks = counters_checksum();
	range_cks = range_checksum();
	psens_cks = psensor_checksum();
	out_cks = outputs_checksum();

	xCom_printf_P( fdGPRS,PSTR("&PLOAD=CLASS:GLOBAL;NACH:%d;NDCH:%d;NCNT:%d;" ),NRO_ANINPUTS,NRO_DINPUTS,NRO_COUNTERS );
	xCom_printf_P( fdGPRS,PSTR("SIMPWD:%s;IMEI:%s;" ), systemVars.gprs_conf.simpwd, &buff_gprs_imei );
	xCom_printf_P( fdGPRS,PSTR("SIMID:%s;CSQ:%d;WRST:%02X;" ), &buff_gprs_ccid,GPRS_stateVars.dbm, wdg_resetCause );
	xCom_printf_P( fdGPRS,PSTR("BASE:0x%02X;AN:0x%02X;DG:0x%02X;" ), base_cks,an_cks,dig_cks );
	xCom_printf_P( fdGPRS,PSTR("CNT:0x%02X;RG:0x%02X;PSE:0x%02X;OUT:0x%02X" ),cnt_cks,range_cks,psens_cks,out_cks );

	// DEBUG & LOG
	if ( systemVars.debug ==  DEBUG_GPRS ) {
		xprintf_P( PSTR("&PLOAD=CLASS:GLOBAL;NACH:%d;NDCH:%d;NCNT:%d;" ),NRO_ANINPUTS,NRO_DINPUTS,NRO_COUNTERS );
		xprintf_P( PSTR("SIMPWD:%s;IMEI:%s;" ), systemVars.gprs_conf.simpwd, &buff_gprs_imei );
		xprintf_P( PSTR("SIMID:%s;CSQ:%d;WRST:%02X;" ), &buff_gprs_ccid,GPRS_stateVars.dbm, wdg_resetCause );
		xprintf_P( PSTR("BASE:0x%02X;AN:0x%02X;DG:0x%02X;" ), base_cks,an_cks,dig_cks );
		xprintf_P( PSTR("CNT:0x%02X;RG:0x%02X;PSE:0x%02X;OUT:0x%02X" ),cnt_cks,range_cks,psens_cks,out_cks );
	}
}
//------------------------------------------------------------------------------------
static void pv_tx_init_payload_base(void)
{

	xCom_printf_P( fdGPRS,PSTR("&PLOAD=CLASS:BASE;"));
	xCom_printf_P( fdGPRS,PSTR("TDIAL:%d;"), systemVars.gprs_conf.timerDial);
	xCom_printf_P( fdGPRS,PSTR("TPOLL:%d;"), systemVars.gprs_conf.timerDial );
	if ( systemVars.gprs_conf.pwrSave.pwrs_enabled ) {
		xCom_printf_P( fdGPRS,PSTR("PWRS_MODO:ON;"));
	} else {
		xCom_printf_P( fdGPRS,PSTR("PWRS_MODO:OFF;"));
	}
	xCom_printf_P( fdGPRS,PSTR("PWRS_START:%02d%02d;"), systemVars.gprs_conf.pwrSave.hora_start.hour, systemVars.gprs_conf.pwrSave.hora_start.min );
	xCom_printf_P( fdGPRS,PSTR("PWRS_END:%02d%02d;"), systemVars.gprs_conf.pwrSave.hora_fin.hour, systemVars.gprs_conf.pwrSave.hora_fin.min);

	// DEBUG & LOG
	if ( systemVars.debug ==  DEBUG_GPRS ) {
		xprintf_P( PSTR("&PLOAD=CLASS:BASE;"));
		xprintf_P( PSTR("TDIAL:%d;"), systemVars.gprs_conf.timerDial);
		xprintf_P( PSTR("TPOLL:%d;"), systemVars.gprs_conf.timerDial );
		if ( systemVars.gprs_conf.pwrSave.pwrs_enabled ) {
			xprintf_P( PSTR("PWRS_MODO:ON;"));
		} else {
			xprintf_P( PSTR("PWRS_MODO:OFF;"));
		}
		xprintf_P( PSTR("PWRS_START:%02d%02d;"), systemVars.gprs_conf.pwrSave.hora_start.hour, systemVars.gprs_conf.pwrSave.hora_start.min );
		xprintf_P( PSTR("PWRS_END:%02d%02d;"), systemVars.gprs_conf.pwrSave.hora_fin.hour, systemVars.gprs_conf.pwrSave.hora_fin.min);

	}

}
//------------------------------------------------------------------------------------
static void pv_tx_init_payload_analog(void)
{
	// Mandamos todos los canales analogicos, esten o no configurados (X)

	//  PLOAD=CLASS:ANALOG;A0:PA,0,20,0.00,6.00;A1:PB,4,20,0.00,10.00;A2:X,4,20,0.00,10.00;A3:PC,4,20,0.00,10.00;A4:X,4,20,0.00,10.00

uint8_t i;

	xCom_printf_P( fdGPRS,PSTR("&PLOAD=CLASS:ANALOG\0"));
	if ( systemVars.debug ==  DEBUG_GPRS ) {
		xprintf_P( PSTR("&PLOAD=CLASS:ANALOG\0"));
	}

	for (i=0; i < NRO_ANINPUTS; i++) {
		xCom_printf_P( fdGPRS,PSTR(";A%d:%s,"), i, systemVars.ainputs_conf.name[i] );
		xCom_printf_P( fdGPRS,PSTR("%d,%d,"), systemVars.ainputs_conf.imin[i],systemVars.ainputs_conf.imax[i] );
		xCom_printf_P( fdGPRS,PSTR("%.03f,%.03f"), systemVars.ainputs_conf.mmin[i], systemVars.ainputs_conf.mmax[i] );
		if ( systemVars.debug ==  DEBUG_GPRS ) {
			xprintf_P( PSTR(";A%d:%s,"), i, systemVars.ainputs_conf.name[i] );
			xprintf_P( PSTR("%d,%d,"), systemVars.ainputs_conf.imin[i],systemVars.ainputs_conf.imax[i] );
			xprintf_P( PSTR("%.03f,%.03f"), systemVars.ainputs_conf.mmin[i], systemVars.ainputs_conf.mmax[i] );
		}
	}
	xprintf_P( PSTR(";D%d:%s,%d"), i, systemVars.dinputs_conf.name[i],systemVars.dinputs_conf.modo_normal[i] );

	xCom_printf_P( fdGPRS,PSTR("\0"));
	if ( systemVars.debug ==  DEBUG_GPRS ) {
		xprintf_P( PSTR("\r\n\0"));
	}
}
//------------------------------------------------------------------------------------
static void pv_tx_init_payload_digital(void)
{
uint8_t i;

	// PLOAD=CLASS:DIGITAL;D0:DINA,1;D1:DINB,0

	xCom_printf_P( fdGPRS,PSTR("&PLOAD=CLASS:DIGITAL\0"));
	if ( systemVars.debug ==  DEBUG_GPRS ) {
		xprintf_P( PSTR("&PLOAD=CLASS:DIGITAL\0"));
	}

	for (i=0; i < NRO_DINPUTS; i++) {
		if ( systemVars.dinputs_conf.modo_normal[i] == true ) {
			xCom_printf_P( fdGPRS,PSTR(";D%d:%s,NORMAL"), i, systemVars.dinputs_conf.name[i] );
		} else {
			xCom_printf_P( fdGPRS,PSTR(";D%d:%s,TIMER"), i, systemVars.dinputs_conf.name[i] );
		}
		xCom_printf_P( fdGPRS,PSTR("%d"), systemVars.dinputs_conf.modo_normal[i] );
		if ( systemVars.debug ==  DEBUG_GPRS ) {
			if ( systemVars.dinputs_conf.modo_normal[i] == true ) {
				xprintf_P( PSTR(";D%d:%s,NORMAL"), i, systemVars.dinputs_conf.name[i] );
			} else {
				xprintf_P( PSTR(";D%d:%s,TIMER"), i, systemVars.dinputs_conf.name[i] );
			}
		}
	}

	xCom_printf_P( fdGPRS,PSTR("\0"));
	if ( systemVars.debug ==  DEBUG_GPRS ) {
		xprintf_P( PSTR("\0"));
	}
}
//------------------------------------------------------------------------------------
static void pv_tx_init_payload_counters(void)
{
uint8_t i;

	// PLOAD=CLASS:COUNTER;C0:CNTA,1.000,100,10,0;C1:CNT1,1.000,100,10,0

	xCom_printf_P( fdGPRS,PSTR("&PLOAD=CLASS:COUNTER\0"));
	if ( systemVars.debug ==  DEBUG_GPRS ) {
		xprintf_P( PSTR("&PLOAD=CLASS:COUNTER\0"));
	}

	for (i=0; i < NRO_COUNTERS; i++) {
		xCom_printf_P( fdGPRS,PSTR(";C%d:%s,"), i, systemVars.counters_conf.name[i] );
		xCom_printf_P( fdGPRS,PSTR("%.03f,%d,"), systemVars.counters_conf.magpp[i], systemVars.counters_conf.period[i] );
		xCom_printf_P( fdGPRS,PSTR("%d,"), systemVars.counters_conf.pwidth[i] );
		if ( systemVars.counters_conf.speed[i] == CNT_LOW_SPEED ) {
			xCom_printf_P( fdGPRS,PSTR("LS"));
		} else {
			xCom_printf_P( fdGPRS,PSTR("HS"));
		}

		if ( systemVars.debug ==  DEBUG_GPRS ) {
			xprintf_P( PSTR(";C%d:%s,"), i, systemVars.counters_conf.name[i] );
			xprintf_P( PSTR("%.03f,%d,"), systemVars.counters_conf.magpp[i], systemVars.counters_conf.period[i] );
			xprintf_P( PSTR("%d,"), systemVars.counters_conf.pwidth[i] );
			if ( systemVars.counters_conf.speed[i] == CNT_LOW_SPEED ) {
				xprintf_P( PSTR("LS"));
			} else {
				xprintf_P( PSTR("HS"));
			}
		}
	}

	xCom_printf_P( fdGPRS,PSTR("\0"));
	if ( systemVars.debug ==  DEBUG_GPRS ) {
		xprintf_P( PSTR("\0"));
	}

}
//------------------------------------------------------------------------------------
static void pv_tx_init_payload_range(void)
{

	// PLOAD=CLASS:RANGE;R0:DIST1

	xCom_printf_P( fdGPRS,PSTR("&PLOAD=CLASS:RANGE;R0:%s"), systemVars.range_name);
	if ( systemVars.debug ==  DEBUG_GPRS ) {
		xprintf_P( PSTR("&PLOAD=CLASS:RANGE;R0:%s"), systemVars.range_name);
	}
}
//------------------------------------------------------------------------------------
static void pv_tx_init_payload_psensor(void)
{

	// PLOAD=CLASS:PSENSOR;PS0:CLORO,0,0.7,0.002

	xCom_printf_P( fdGPRS,PSTR("&PLOAD=CLASS:PSENSOR;"));
	xCom_printf_P( fdGPRS,PSTR("PS0:%s,%.03f,"), systemVars.psensor_conf.name,systemVars.psensor_conf.pmin );
	xCom_printf_P( fdGPRS,PSTR("%.03f,%.03f"), systemVars.psensor_conf.pmax, systemVars.psensor_conf.poffset );

	if ( systemVars.debug ==  DEBUG_GPRS ) {
		xprintf_P( PSTR("&PLOAD=CLASS:PSENSOR;"));
		xprintf_P( PSTR("PS0:%s,%.03f,"), systemVars.psensor_conf.name,systemVars.psensor_conf.pmin );
		xprintf_P( PSTR("%.03f,%.03f"), systemVars.psensor_conf.pmax, systemVars.psensor_conf.poffset );
	}
}
//------------------------------------------------------------------------------------
static void pv_tx_init_payload_outputs(void)
{

uint8_t i;

	switch(systemVars.doutputs_conf.modo) {
	case OFF:
		xCom_printf_P( fdGPRS,PSTR("&PLOAD=CLASS:OUTPUTS;MODO:OFF"));
		if ( systemVars.debug ==  DEBUG_GPRS ) {
			xprintf_P( PSTR("&PLOAD=CLASS:OUTPUTS;MODO:OFF"));
		}
		break;
	case CONSIGNA:
		xCom_printf_P( fdGPRS,PSTR("&PLOAD=CLASS:OUTPUTS;MODO:CONSIGNA;CHH1:%02d;CMM1:%02d;CHH2:%02d;CMM2:%02d"), systemVars.doutputs_conf.consigna.hhmm_c_diurna.hour, systemVars.doutputs_conf.consigna.hhmm_c_diurna.min, systemVars.doutputs_conf.consigna.hhmm_c_nocturna.hour, systemVars.doutputs_conf.consigna.hhmm_c_nocturna.min  );
		if ( systemVars.debug ==  DEBUG_GPRS ) {
			xprintf_P( PSTR("&PLOAD=CLASS:OUTPUTS;MODO:CONSIGNA;CHH1:%02d;CMM1:%02d;CHH2:%02d;CMM2:%02d"), systemVars.doutputs_conf.consigna.hhmm_c_diurna.hour, systemVars.doutputs_conf.consigna.hhmm_c_diurna.min, systemVars.doutputs_conf.consigna.hhmm_c_nocturna.hour, systemVars.doutputs_conf.consigna.hhmm_c_nocturna.min  );
		}
		break;
	case PERFORACIONES:
		xCom_printf_P( fdGPRS,PSTR("&PLOAD=CLASS:OUTPUTS;MODO:PERF"));
		if ( systemVars.debug ==  DEBUG_GPRS ) {
			xprintf_P( PSTR("&PLOAD=CLASS:OUTPUTS;MODO:PERF"));
		}
		break;
	case PILOTOS:
		xCom_printf_P( fdGPRS,PSTR("&PLOAD=CLASS:OUTPUTS;MODO:PILOTO;STEPS:%d;BAND:%.03f;"), systemVars.doutputs_conf.piloto.max_steps, systemVars.doutputs_conf.piloto.band );
		if ( systemVars.debug ==  DEBUG_GPRS ) {
			xprintf_P( PSTR("&PLOAD=CLASS:OUTPUTS;MODO:PILOTO;STEPS:%d;BAND:%.03f;"), systemVars.doutputs_conf.piloto.max_steps, systemVars.doutputs_conf.piloto.band );
		}

		for(i=0;i<MAX_PILOTO_PSLOTS;i++) {
			xCom_printf_P( fdGPRS,PSTR("SLOT%d:%02d,%02d,%.03f;"), i, systemVars.doutputs_conf.piloto.pSlots[i].hhmm.hour, systemVars.doutputs_conf.piloto.pSlots[i].hhmm.min, systemVars.doutputs_conf.piloto.pSlots[i].pout );
			if ( systemVars.debug ==  DEBUG_GPRS ) {
				xprintf_P( PSTR("SLOT%d:%02d,%02d,%.03f;"), i, systemVars.doutputs_conf.piloto.pSlots[i].hhmm.hour, systemVars.doutputs_conf.piloto.pSlots[i].hhmm.min, systemVars.doutputs_conf.piloto.pSlots[i].pout );
			}
		}

		xCom_printf_P( fdGPRS,PSTR("\0"));
		if ( systemVars.debug ==  DEBUG_GPRS ) {
			xprintf_P( PSTR("\0"));
		}
		break;
	}
}
//------------------------------------------------------------------------------------
static t_frame_responses pv_init_process_response(void)
{

	// Espero la respuesta al frame de INIT.
	// Si la recibo la proceso.
	// Salgo por timeout 10s o por socket closed.

uint8_t timeout = 0;
uint8_t exit_code = FRAME_ERROR;

	for ( timeout = 0; timeout < 10; timeout++) {

		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );	// Espero 1s

		if ( u_gprs_check_socket_status() != SOCK_OPEN ) {		// El socket se cerro
			exit_code = FRAME_SOCK_CLOSE;
			goto EXIT;
		}

		if ( u_gprs_check_response("ERROR") ) {	// Recibi un ERROR de respuesta
			u_gprs_print_RX_Buffer();
			exit_code = FRAME_ERROR;
			goto EXIT;
		}

		if ( u_gprs_check_response("</h1>") ) {	// Respuesta completa del server

			if ( systemVars.debug == DEBUG_GPRS  ) {
				u_gprs_print_RX_Buffer();
			} else {
				u_gprs_print_RX_response();
			}

			// Analizo las respuestas.
			if ( u_gprs_check_response("GLOBAL") ) {	// Respuesta correcta:
				// Borro la causa del reset
				wdg_resetCause = 0x00;
				pv_init_reconfigure_params_global();
				exit_code = FRAME_OK;
				goto EXIT;
			}

			if ( u_gprs_check_response("BASE") ) {	// Respuesta correcta: Reconfiguro parametros base.
				// Borro la causa del reset
				wdg_resetCause = 0x00;
				pv_init_reconfigure_params_base();
				exit_code = FRAME_OK;
				goto EXIT;
			}

			if ( u_gprs_check_response("ANALOG") ) {
				// Borro la causa del reset
				wdg_resetCause = 0x00;
				pv_init_reconfigure_params_analog();
				exit_code = FRAME_OK;
				goto EXIT;
			}

			if ( u_gprs_check_response("DIGITAL") ) {
				// Borro la causa del reset
				wdg_resetCause = 0x00;
				pv_init_reconfigure_params_digital();
				exit_code = FRAME_OK;
				goto EXIT;
			}

			if ( u_gprs_check_response("COUNTER") ) {
				// Borro la causa del reset
				wdg_resetCause = 0x00;
				pv_init_reconfigure_params_counters();
				exit_code = FRAME_OK;
				goto EXIT;
			}

			if ( u_gprs_check_response("RANGE") ) {
				// Borro la causa del reset
				wdg_resetCause = 0x00;
				pv_init_reconfigure_params_range();
				exit_code = FRAME_OK;
				goto EXIT;
			}

			if ( u_gprs_check_response("PSENSOR") ) {
				// Borro la causa del reset
				wdg_resetCause = 0x00;
				pv_init_reconfigure_params_psensor();
				exit_code = FRAME_OK;
				goto EXIT;
			}

			if ( u_gprs_check_response("OUTPUTS") ) {
				// Borro la causa del reset
				wdg_resetCause = 0x00;
				pv_init_reconfigure_params_outputs();
				exit_code = FRAME_OK;
				goto EXIT;
			}

			if ( u_gprs_check_response("SRV_ERR") ) {	// El servidor no pudo procesar el frame. Problema del server
				// Borro la causa del reset
				wdg_resetCause = 0x00;
				exit_code = FRAME_SRV_ERR;
				xprintf_P( PSTR("GPRS: SERVER ERROR !!.\r\n\0" ));
				goto EXIT;
			}

			if ( u_gprs_check_response("NOT_ALLOWED") ) {	// Datalogger esta usando un script incorrecto
				xprintf_P( PSTR("GPRS: SCRIPT ERROR !!.\r\n\0" ));
				exit_code = FRAME_NOT_ALLOWED;
				goto EXIT;
			}
		}
	}

// Exit:
EXIT:

	return(exit_code);

}
//------------------------------------------------------------------------------------
static void pv_init_reconfigure_params_global(void)
{
	// Recibimos un frame que trae la fecha y hora y parametros que indican
	// que otras flags de configuraciones debemos prender.
	// GLOBAL;CLOCK:1910120345;BASE;ANALOG;DIGITAL;COUNTERS;RANGE;PSENSOR;OUTS

char *p = NULL;
char localStr[32] = { 0 };
char *stringp = NULL;
char *token = NULL;
char *delim = ",=:;><";
char rtcStr[12];
uint8_t i = 0;
char c = '\0';
RtcTimeType_t rtc;
int8_t xBytes = 0;


	// CLOCK
	p = strstr( (const char *)&pv_gprsRxCbuffer.buffer, "CLOCK");
	if ( p != NULL ) {
		// Copio el mensaje enviado a un buffer local porque la funcion strsep lo modifica.
		memset( &localStr, '\0', sizeof(localStr) );
		memcpy(localStr,p,sizeof(localStr));

		stringp = localStr;
		token = strsep(&stringp,delim);			// CLOCK

		token = strsep(&stringp,delim);			// 1910120345
		memset(rtcStr, '\0', sizeof(rtcStr));
		memcpy(rtcStr,token, sizeof(rtcStr));	// token apunta al comienzo del string con la hora
		for ( i = 0; i<12; i++) {
			c = *token;
			rtcStr[i] = c;
			c = *(++token);
			if ( c == '\0' )
				break;

		}

		memset( &rtc, '\0', sizeof(rtc) );
		RTC_str2rtc(rtcStr, &rtc);	// Convierto el string YYMMDDHHMM a RTC.
		xBytes = RTC_write_dtime(&rtc);		// Grabo el RTC
		if ( xBytes == -1 )
			xprintf_P(PSTR("ERROR: I2C:RTC:pv_process_server_clock\r\n\0"));

		if ( systemVars.debug == DEBUG_GPRS ) {
			xprintf_P( PSTR("GPRS: Update rtc to: %s\r\n\0"), rtcStr );
		}

	}

	// Flags de configuraciones particulares: BASE;ANALOG;DIGITAL;COUNTERS;RANGE;PSENSOR;OUTS
	p = strstr( (const char *)&pv_gprsRxCbuffer.buffer, "BASE");
	if ( p != NULL ) {
		f_send_init_base = true;
		if ( systemVars.debug == DEBUG_GPRS ) {
			xprintf_P( PSTR("GPRS: Reconfig BASE\r\n\0"));
		}
	}

	p = strstr( (const char *)&pv_gprsRxCbuffer.buffer, "ANALOG");
	if ( p != NULL ) {
		f_send_init_analog = true;
		if ( systemVars.debug == DEBUG_GPRS ) {
			xprintf_P( PSTR("GPRS: Reconfig ANALOG\r\n\0"));
		}
	}

	p = strstr( (const char *)&pv_gprsRxCbuffer.buffer, "DIGITAL");
	if ( p != NULL ) {
		f_send_init_digital = true;
		if ( systemVars.debug == DEBUG_GPRS ) {
			xprintf_P( PSTR("GPRS: Reconfig DIGITAL\r\n\0"));
		}
	}

	p = strstr( (const char *)&pv_gprsRxCbuffer.buffer, "COUNTERS");
	if ( p != NULL ) {
		f_send_init_counters = true;
		if ( systemVars.debug == DEBUG_GPRS ) {
			xprintf_P( PSTR("GPRS: Reconfig COUNTERS\r\n\0"));
		}
	}

	p = strstr( (const char *)&pv_gprsRxCbuffer.buffer, "RANGE");
	if ( p != NULL ) {
		f_send_init_range = true;
		if ( systemVars.debug == DEBUG_GPRS ) {
			xprintf_P( PSTR("GPRS: Reconfig RANGE\r\n\0"));
		}
	}

	p = strstr( (const char *)&pv_gprsRxCbuffer.buffer, "PSENSOR");
	if ( p != NULL ) {
		f_send_init_psensor = true;
		if ( systemVars.debug == DEBUG_GPRS ) {
			xprintf_P( PSTR("GPRS: Reconfig PSENSOR\r\n\0"));
		}
	}

	p = strstr( (const char *)&pv_gprsRxCbuffer.buffer, "OUTPUTS");
	if ( p != NULL ) {
		f_send_init_outputs = true;
		if ( systemVars.debug == DEBUG_GPRS ) {
			xprintf_P( PSTR("GPRS: Reconfig OUTPUTS\r\n\0"));
		}
	}

}
//------------------------------------------------------------------------------------
static void pv_init_reconfigure_params_base(void)
{

	//	TYPE=INIT&PLOAD=CLASS:BASE;TPOLL:60;TDIAL:60;PWRS:1,2330,630

char *p = NULL;
char localStr[32] = { 0 };
char *stringp = NULL;
char *token = NULL;
char *tk_pws_modo = NULL;
char *tk_pws_start = NULL;
char *tk_pws_end = NULL;
char *delim = ",=:;><";
bool save_flag = false;

	// TDIAL
	p = strstr( (const char *)&pv_gprsRxCbuffer.buffer, "TDIAL");
	if ( p != NULL ) {

		// Copio el mensaje enviado a un buffer local porque la funcion strsep lo modifica.
		memset( &localStr, '\0', sizeof(localStr) );
		memcpy(localStr,p,sizeof(localStr));

		stringp = localStr;
		token = strsep(&stringp,delim);		// TDIAL
		token = strsep(&stringp,delim);		// timerDial
		u_gprs_config_timerdial(token);
		save_flag = true;

		if ( systemVars.debug == DEBUG_GPRS ) {
			xprintf_P( PSTR("GPRS: Reconfig TDIAL\r\n\0"));
		}
	}

	// TPOLL
	p = strstr( (const char *)&pv_gprsRxCbuffer.buffer, "TPOLL");
	if ( p != NULL ) {

		memset( &localStr, '\0', sizeof(localStr) );
		memcpy(localStr,p,sizeof(localStr));

		stringp = localStr;
		token = strsep(&stringp,delim);		// TPOLL
		token = strsep(&stringp,delim);		// timerPoll
		u_config_timerpoll(token);
		save_flag = true;

		if ( systemVars.debug == DEBUG_GPRS ) {
			xprintf_P( PSTR("GPRS: Reconfig TPOLL\r\n\0"));
		}
	}

	// PWRS
	p = strstr( (const char *)&pv_gprsRxCbuffer.buffer, "PWRS");
	if ( p != NULL ) {
		memset( &localStr, '\0', sizeof(localStr) );
		memcpy(localStr,p,sizeof(localStr));

		stringp = localStr;
		tk_pws_modo = strsep(&stringp,delim);		//PWRS
		tk_pws_modo = strsep(&stringp,delim);		// modo ( ON / OFF ).
		tk_pws_start = strsep(&stringp,delim);		// startTime
		tk_pws_end  = strsep(&stringp,delim); 		// endTime
		u_gprs_configPwrSave(tk_pws_modo, tk_pws_start, tk_pws_end );
		save_flag = true;

		if ( systemVars.debug == DEBUG_GPRS ) {
			xprintf_P( PSTR("GPRS: Reconfig PWRSAVE\r\n\0"));
		}
	}

	if ( save_flag ) {
		u_save_params_in_NVMEE();
	}

}
//------------------------------------------------------------------------------------
static void pv_init_reconfigure_params_analog(void)
{
	//	CLASS:ANALOG;A0:PA,4,20,0,100;A1:X,0,0,0,0;A2:OPAC,0,20,0,100,A3:NOX,4,20,0,2000;A4:SO2,4,20,0,1000;A5:CO,4,20,0,25;A6:NO2,4,20,0,000;A7:CAU,4,20,0,1000
	// 	PLOAD=CLASS:ANALOG;A0:PA,4,20,0.0,10.0;A1:X,4,20,0.0,10.0;A3:X,4,20,0.0,10.0;
	//  TYPE=INIT&PLOAD=CLASS:ANALOG;A0:PA,4,20,0.0,10.0;A1:X,4,20,0.0,10.0;A2:X,4,20,0.0,10.0;A3:X,4,20,0.0,10.0;A4:X,4,20,0.0,10.0;

char *p = NULL;
char localStr[32] = { 0 };
char *stringp = NULL;
char *tk_name= NULL;
char *tk_iMin= NULL;
char *tk_iMax = NULL;
char *tk_mMin = NULL;
char *tk_mMax = NULL;
char *delim = ",=:;><";
bool save_flag = false;
uint8_t ch;
char str_base[8];

	// A?
	for (ch=0; ch < NRO_ANINPUTS; ch++ ) {
		memset( &str_base, '\0', sizeof(str_base) );
		snprintf_P( str_base, sizeof(str_base), PSTR("A%d\0"), ch );
		//xprintf_P( PSTR("DEBUG str_base: %s\r\n\0"), str_base);

		p = strstr( (const char *)&pv_gprsRxCbuffer.buffer, str_base);
		//xprintf_P( PSTR("DEBUG str_p: %s\r\n\0"), p);
		if ( p != NULL ) {
			memset(localStr,'\0',sizeof(localStr));
			memcpy(localStr,p,sizeof(localStr));
			stringp = localStr;
			//xprintf_P( PSTR("DEBUG local_str: %s\r\n\0"), localStr );
			tk_name = strsep(&stringp,delim);		//A0
			tk_name = strsep(&stringp,delim);		//name
			tk_iMin = strsep(&stringp,delim);		//iMin
			tk_iMax = strsep(&stringp,delim);		//iMax
			tk_mMin = strsep(&stringp,delim);		//mMin
			tk_mMax = strsep(&stringp,delim);		//mMax

			ainputs_config_channel( ch, tk_name ,tk_iMin, tk_iMax, tk_mMin, tk_mMax );
			if ( systemVars.debug == DEBUG_GPRS ) {
				xprintf_P( PSTR("GPRS: Reconfig A%d\r\n\0"), ch);
			}

			save_flag = true;
		}
	}

	if ( save_flag ) {
		u_save_params_in_NVMEE();
	}

}
//------------------------------------------------------------------------------------
static void pv_init_reconfigure_params_digital(void)
{

	//	PLOAD=CLASS:DIGITAL;D0:DIN0,NORMAL;D1:DIN1,TIMER;

char *p = NULL;
char localStr[32] = { 0 };
char *stringp = NULL;
char *tk_name= NULL;
char *tk_type= NULL;
char *delim = ",=:;><";
bool save_flag = false;
uint8_t ch;
char str_base[8];

	// A?
	for (ch=0; ch < NRO_DINPUTS; ch++ ) {
		memset( &str_base, '\0', sizeof(str_base) );
		snprintf_P( str_base, sizeof(str_base), PSTR("D%d\0"), ch );
		p = strstr( (const char *)&pv_gprsRxCbuffer.buffer, str_base);
		if ( p != NULL ) {
			memset(localStr,'\0',sizeof(localStr));
			memcpy(localStr,p,sizeof(localStr));
			stringp = localStr;
			tk_name = strsep(&stringp,delim);		//D0
			tk_name = strsep(&stringp,delim);		//name
			tk_type = strsep(&stringp,delim);		//type

			dinputs_config_channel( ch,tk_name ,tk_type );

			if ( systemVars.debug == DEBUG_GPRS ) {
				xprintf_P( PSTR("GPRS: Reconfig D%d\r\n\0"), ch);
			}
			save_flag = true;
		}
	}

	if ( save_flag ) {
		u_save_params_in_NVMEE();
	}

}
//------------------------------------------------------------------------------------
static void pv_init_reconfigure_params_counters(void)
{
	//	PLOAD=CLASS:COUNTER;C0:CNT0,1.0,15,1000,0;C1:X,1.0,10,100,1;
	//  PLOAD=CLASS:COUNTER;C0:CNT0,1.0,15,1000,0;C1:X,1.0,10,100,1;

char *p = NULL;
char localStr[32] = { 0 };
char *stringp = NULL;
char *tk_name = NULL;
char *tk_magpp = NULL;
char *tk_pwidth = NULL;
char *tk_period = NULL;
char *tk_speed = NULL;
char *delim = ",=:;><";
bool save_flag = false;
uint8_t ch;
char str_base[8];

	// C?
	for (ch=0; ch < NRO_COUNTERS; ch++ ) {
		memset( &str_base, '\0', sizeof(str_base) );
		snprintf_P( str_base, sizeof(str_base), PSTR("C%d\0"), ch );
		p = strstr( (const char *)&pv_gprsRxCbuffer.buffer, str_base);
		if ( p != NULL ) {
			memset(localStr,'\0',sizeof(localStr));
			memcpy(localStr,p,sizeof(localStr));
			stringp = localStr;
			tk_name = strsep(&stringp,delim);		//C0
			tk_name = strsep(&stringp,delim);		//name
			tk_magpp = strsep(&stringp,delim);		//magpp
			tk_pwidth = strsep(&stringp,delim);
			tk_period = strsep(&stringp,delim);
			tk_speed = strsep(&stringp,delim);

			counters_config_channel( ch,tk_name ,tk_magpp, tk_pwidth, tk_period, tk_speed );

			if ( systemVars.debug == DEBUG_GPRS ) {
				xprintf_P( PSTR("GPRS: Reconfig C%d\r\n\0"), ch);
			}
			save_flag = true;
		}
	}

	if ( save_flag ) {
		u_save_params_in_NVMEE();
	}

}
//------------------------------------------------------------------------------------
static void pv_init_reconfigure_params_range(void)
{
	// TYPE=INIT&PLOAD=CLASS:RANGE;R0:DIST;

char *p = NULL;
char localStr[32] = { 0 };
char *stringp = NULL;
char *token = NULL;
char *delim = ",=:;><";

	// RANGE
	p = strstr( (const char *)&pv_gprsRxCbuffer.buffer, "R0");
	if ( p != NULL ) {

		// Copio el mensaje enviado a un buffer local porque la funcion strsep lo modifica.
		memset( &localStr, '\0', sizeof(localStr) );
		memcpy(localStr,p,sizeof(localStr));
		stringp = localStr;
		token = strsep(&stringp,delim);		// R0
		token = strsep(&stringp,delim);		// Range name
		range_config(token);

		if ( systemVars.debug == DEBUG_GPRS ) {
			xprintf_P( PSTR("GPRS: Reconfig RANGE\r\n\0"));
		}

		u_save_params_in_NVMEE();
	}

}
//------------------------------------------------------------------------------------
static void pv_init_reconfigure_params_psensor(void)
{
	// TYPE=INIT&PLOAD=CLASS:PSENSOR;PS0:CLORO,0.000,100.000,1.000;

char *p = NULL;
char localStr[32] = { 0 };
char *stringp = NULL;
char *tk_name = NULL;
char *tk_pmin = NULL;
char *tk_pmax = NULL;
char *tk_poffset = NULL;
char *delim = ",=:;><";

	p = strstr( (const char *)&pv_gprsRxCbuffer.buffer, "PS0");
	if ( p != NULL ) {
		memset( &localStr, '\0', sizeof(localStr) );
		memcpy(localStr,p,sizeof(localStr));

		stringp = localStr;
		tk_name = strsep(&stringp,delim);
		tk_name = strsep(&stringp,delim);
		tk_pmin = strsep(&stringp,delim);
		tk_pmax  = strsep(&stringp,delim);
		tk_poffset  = strsep(&stringp,delim);
		psensor_config(tk_name, tk_pmin, tk_pmax, tk_poffset );

		if ( systemVars.debug == DEBUG_GPRS ) {
			xprintf_P( PSTR("GPRS: Reconfig PSENSOR\r\n\0"));
		}

		u_save_params_in_NVMEE();

	}

}
//------------------------------------------------------------------------------------
static void pv_init_reconfigure_params_outputs(void)
{

}
//------------------------------------------------------------------------------------
