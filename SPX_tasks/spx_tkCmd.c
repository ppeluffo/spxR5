/*
 * sp5K_tkCmd.c
 *
 *  Created on: 27/12/2013
 *      Author: root
 */

#include "spx.h"
#include "gprs.h"

//----------------------------------------------------------------------------------------
// FUNCIONES DE USO PRIVADO
//----------------------------------------------------------------------------------------
static void pv_snprintfP_OK(void );
static void pv_snprintfP_ERR(void);
static void pv_cmd_read_fuses(void);

static void pv_cmd_print_stack_watermarks(void);
static void pv_cmd_read_battery(void);
static void pv_cmd_read_analog_channel(void);
static void pv_cmd_read_digital_channels(void);
static void pv_cmd_read_memory(void);
static void pv_cmd_read_debug(void);
static void pv_cmd_write_valve(void);
static void pv_cmd_rwGPRS(uint8_t cmd_mode );

//----------------------------------------------------------------------------------------
// FUNCIONES DE CMDMODE
//----------------------------------------------------------------------------------------
static void cmdHelpFunction(void);
static void cmdClearScreen(void);
static void cmdResetFunction(void);
static void cmdWriteFunction(void);
static void cmdReadFunction(void);
static void cmdStatusFunction(void);
static void cmdConfigFunction(void);
static void cmdKillFunction(void);

#define WR_CMD 0
#define RD_CMD 1

#define WDG_CMD_TIMEOUT	30

static usuario_t tipo_usuario;
RtcTimeType_t rtc;

st_dataRecord_t dr;
dataframe_s df;

//------------------------------------------------------------------------------------
void tkCmd(void * pvParameters)
{

uint8_t c;
uint8_t ticks;

( void ) pvParameters;

	// Espero la notificacion para arrancar
	while ( !startTask )
		vTaskDelay( ( TickType_t)( 100 / portTICK_RATE_MS ) );

	vTaskDelay( ( TickType_t)( 500 / portTICK_RATE_MS ) );

	FRTOS_CMD_init();

	// Registro los comandos y los mapeo al cmd string.
	FRTOS_CMD_register( "cls\0", cmdClearScreen );
	FRTOS_CMD_register( "reset\0", cmdResetFunction);
	FRTOS_CMD_register( "write\0", cmdWriteFunction);
	FRTOS_CMD_register( "read\0", cmdReadFunction);
	FRTOS_CMD_register( "help\0", cmdHelpFunction );
	FRTOS_CMD_register( "status\0", cmdStatusFunction );
	FRTOS_CMD_register( "config\0", cmdConfigFunction );
	FRTOS_CMD_register( "kill\0", cmdKillFunction );

	// Fijo el timeout del READ
	ticks = 5;
	frtos_ioctl( fdTERM,ioctl_SET_TIMEOUT, &ticks );

	tipo_usuario = USER_TECNICO;

	xprintf_P( PSTR("starting tkCmd..\r\n\0") );

	//FRTOS_CMD_regtest();
	// loop
	for( ;; )
	{

		// Con la terminal desconectada paso c/5s plt 30s es suficiente.
		ctl_watchdog_kick(WDG_CMD, WDG_CMD_TIMEOUT);

	//	PORTF.OUTTGL = 0x02;	// Toggle F1 Led Comms

		// Si no tengo terminal conectada, duermo 5s lo que me permite entrar en tickless.
		if ( ! ctl_terminal_connected() ) {
			vTaskDelay( ( TickType_t)( 25000 / portTICK_RATE_MS ) );

		} else {

			c = '\0';	// Lo borro para que luego del un CR no resetee siempre el timer.
			// el read se bloquea 50ms. lo que genera la espera.
			//while ( CMD_read( (char *)&c, 1 ) == 1 ) {
			while ( frtos_read( fdTERM, (char *)&c, 1 ) == 1 ) {
				FRTOS_CMD_process(c);
			}
		}
	}
}
//------------------------------------------------------------------------------------
static void cmdStatusFunction(void)
{

FAT_t l_fat;
uint8_t channel;

	xprintf_P( PSTR("\r\nSpymovil %s %s %s %s \r\n\0"), SPX_HW_MODELO, SPX_FTROS_VERSION, SPX_FW_REV, SPX_FW_DATE);
	xprintf_P( PSTR("Clock %d Mhz, Tick %d Hz\r\n\0"),SYSMAINCLK, configTICK_RATE_HZ );
	if ( spx_io_board == SPX_IO5CH ) {
		xprintf_P( PSTR("IOboard SPX5CH\r\n\0") );
	} else if ( spx_io_board == SPX_IO8CH ) {
		xprintf_P( PSTR("IOboard SPX8CH\r\n\0") );
	}

	// SIGNATURE ID
	xprintf_P( PSTR("uID=%s\r\n\0"), NVMEE_readID() );

	// Last reset cause
	xprintf_P( PSTR("WRST=0x%02X\r\n\0") ,wdg_resetCause );

//	xprintf_P( PSTR("sVars Size: %d\r\n\0"), sizeof(systemVars) );
//	xprintf_P( PSTR("dr Size: %d\r\n\0"), sizeof(st_dataRecord_t) );

	RTC_read_time();

	// DlgId
	xprintf_P( PSTR("dlgid: %s\r\n\0"), systemVars.gprs_conf.dlgId );

	// Memoria
	FAT_read(&l_fat);
	xprintf_P( PSTR("memory: rcdSize=%d, wrPtr=%d,rdPtr=%d,delPtr=%d,r4wr=%d,r4rd=%d,r4del=%d \r\n\0"), sizeof(st_dataRecord_t), l_fat.wrPTR,l_fat.rdPTR, l_fat.delPTR,l_fat.rcds4wr,l_fat.rcds4rd,l_fat.rcds4del );

	// SERVER
	xprintf_P( PSTR(">Server:\r\n\0"));
	xprintf_P( PSTR("  apn: %s\r\n\0"), systemVars.gprs_conf.apn );
	xprintf_P( PSTR("  server ip:port: %s:%s\r\n\0"), systemVars.gprs_conf.server_ip_address, systemVars.gprs_conf.server_tcp_port );
	xprintf_P( PSTR("  server script: %s\r\n\0"), systemVars.gprs_conf.serverScript );
	xprintf_P( PSTR("  simpwd: %s\r\n\0"), systemVars.gprs_conf.simpwd );

	// MODEM
	xprintf_P( PSTR(">Modem:\r\n\0"));
	xprintf_P( PSTR("  signalQ: csq=%d, dBm=%d\r\n\0"), GPRS_stateVars.csq, GPRS_stateVars.dbm );
	xprintf_P( PSTR("  ip address: %s\r\n\0"), GPRS_stateVars.dlg_ip_address);

	// GPRS STATE
	switch (GPRS_stateVars.state) {
	case G_ESPERA_APAGADO:
		xprintf_P( PSTR("  state: await_off\r\n"));
		break;
	case G_PRENDER:
		xprintf_P( PSTR("  state: prendiendo\r\n"));
		break;
	case G_CONFIGURAR:
		xprintf_P( PSTR("  state: configurando\r\n"));
		break;
	case G_MON_SQE:
		xprintf_P( PSTR("  state: mon_sqe\r\n"));
		break;
	case G_SCAN_APN:
		xprintf_P( PSTR("  state: scan apn\r\n"));
		break;
	case G_GET_IP:
		xprintf_P( PSTR("  state: ip\r\n"));
		break;
	case G_INIT_FRAME:
		xprintf_P( PSTR("  state: init frame\r\n"));
		break;
	case G_DATA:
		xprintf_P( PSTR("  state: data\r\n"));
		break;
	default:
		xprintf_P( PSTR("  state: ERROR\r\n"));
		break;
	}

	// CONFIG
	xprintf_P( PSTR(">Config:\r\n\0"));

	// debug
	switch(systemVars.debug) {
	case DEBUG_NONE:
		xprintf_P( PSTR("  debug: none\r\n\0") );
		break;
	case DEBUG_COUNTER:
		xprintf_P( PSTR("  debug: counter\r\n\0") );
		break;
	case DEBUG_DATA:
		xprintf_P( PSTR("  debug: data\r\n\0") );
		break;
	case DEBUG_GPRS:
		xprintf_P( PSTR("  debug: gprs\r\n\0") );
		break;
	default:
		xprintf_P( PSTR("  debug: ???\r\n\0") );
		break;
	}

	// Digital inputs timers. Solo en SPX_8CH ( para UTE )
	if ( spx_io_board == SPX_IO8CH ) {
		if ( systemVars.dtimers_enabled == true ) {
			xprintf_P( PSTR("  dinputsAsTimers: ON\r\n\0"));
		} else {
			xprintf_P( PSTR("  dinputsAsTimers: OFF\r\n\0"));
		}
	}

	// Timerpoll
	xprintf_P( PSTR("  timerPoll: [%d s]/ %d\r\n\0"),systemVars.timerPoll, ctl_readTimeToNextPoll() );

	// Timerdial
	xprintf_P( PSTR("  timerDial: [%d s]/\0"), systemVars.gprs_conf.timerDial );
	xprintf_P( PSTR(" %d\r\n\0"), u_gprs_read_timeToNextDial() );

	// Sensor Pwr Time
	xprintf_P( PSTR("  timerPwrSensor: [%d s]\r\n\0"), systemVars.rangeMeter_enabled );

	// Counters debounce time
	xprintf_P( PSTR("  timerDebounceCnt: [%d s]\r\n\0"), systemVars.counters_conf.debounce_time );

	if ( spx_io_board == SPX_IO5CH ) {

		// PWR SAVE:
		if ( systemVars.gprs_conf.pwrSave.pwrs_enabled ==  false ) {
			xprintf_P(  PSTR("  pwrsave: OFF\r\n\0"));
		} else {
			xprintf_P(  PSTR("  pwrsave: ON, start[%02d:%02d], end[%02d:%02d]\r\n\0"), systemVars.gprs_conf.pwrSave.hora_start.hour, systemVars.gprs_conf.pwrSave.hora_start.min, systemVars.gprs_conf.pwrSave.hora_fin.hour, systemVars.gprs_conf.pwrSave.hora_fin.min);
		}

		// RangeMeter: PULSE WIDTH
		if ( systemVars.rangeMeter_enabled ) {
			xprintf_P( PSTR("  rangeMeter: ON\r\n"));
		} else {
			xprintf_P( PSTR("  rangeMeter: OFF\r\n"));
		}

		// Consignas
		if ( systemVars.doutputs_conf.consigna.c_enabled ) {
			if ( systemVars.doutputs_conf.consigna.c_aplicada == CONSIGNA_DIURNA ) {
				xprintf_P( PSTR("  consignas: (DIURNA) (c_dia=%02d:%02d, c_noche=%02d:%02d)\r\n"), systemVars.doutputs_conf.consigna.hhmm_c_diurna.hour, systemVars.doutputs_conf.consigna.hhmm_c_diurna.min, systemVars.doutputs_conf.consigna.hhmm_c_nocturna.hour, systemVars.doutputs_conf.consigna.hhmm_c_nocturna.min );
			} else {
				xprintf_P( PSTR("  consignas: (NOCTURNA) (c_dia=%02d:%02d, c_noche=%02d:%02d)\r\n"), systemVars.doutputs_conf.consigna.hhmm_c_diurna.hour, systemVars.doutputs_conf.consigna.hhmm_c_diurna.min, systemVars.doutputs_conf.consigna.hhmm_c_nocturna.hour, systemVars.doutputs_conf.consigna.hhmm_c_nocturna.min );
			}
		} else {
			xprintf_P( PSTR("  consignas: OFF\r\n"));
		}

	}

	// Salidas digitales
	if ( spx_io_board == SPX_IO8CH ) {
		xprintf_P( PSTR("  outputs: 0x%02x [%c%c%c%c%c%c%c%c]\r\n\0"), systemVars.doutputs_conf.d_outputs ,  BYTE_TO_BINARY( systemVars.doutputs_conf.d_outputs ) );
	}

	// aninputs
	for ( channel = 0; channel < NRO_ANINPUTS; channel++) {
		xprintf_P( PSTR("  a%d( ) [%d-%d mA/ %.02f,%.02f | %.02f | %.02f | %s]\r\n\0"),channel, systemVars.ainputs_conf.imin[channel], systemVars.ainputs_conf.imax[channel], systemVars.ainputs_conf.mmin[channel], systemVars.ainputs_conf.mmax[channel], systemVars.ainputs_conf.inaspan[channel], systemVars.ainputs_conf.offset[channel], systemVars.ainputs_conf.name[channel] );
	}

	// dinputs
	for ( channel = 0; channel < NRO_DINPUTS; channel++) {
		if (  ( spx_io_board == SPX_IO8CH  ) &&  ( systemVars.dtimers_enabled == true )  && ( channel > 3 ) ) {
			xprintf_P( PSTR("  d%d( ) [ %s ] (T)\r\n\0"),channel,  systemVars.dinputs_conf.name[channel] );
		} else {
			xprintf_P( PSTR("  d%d( ) [ %s ]\r\n\0"),channel, systemVars.dinputs_conf.name[channel] );
		}
	}

	// contadores
	for ( channel = 0; channel < NRO_COUNTERS; channel++) {
		xprintf_P( PSTR("  c%d [ %s | %.02f ]\r\n\0"),channel, systemVars.counters_conf.name[channel], systemVars.counters_conf.magpp[channel] );
	}

	//data_get_name ( false );
}
//-----------------------------------------------------------------------------------
static void cmdResetFunction(void)
{
	// Resetea al micro prendiendo el watchdog

	FRTOS_CMD_makeArgv();

	// Reset memory ??
	if (!strcmp_P( strupr(argv[1]), PSTR("MEMORY\0"))) {

		// Nadie debe usar la memoria !!!
		ctl_watchdog_kick(WDG_CMD, 0x8000 );

		vTaskSuspend( xHandle_tkData );
		ctl_watchdog_kick(WDG_DAT, 0x8000 );

		vTaskSuspend( xHandle_tkCounter );
		ctl_watchdog_kick(WDG_COUNT, 0x8000 );

		vTaskSuspend( xHandle_tkData );
		ctl_watchdog_kick(WDG_DAT, 0x8000 );

		vTaskSuspend( xHandle_tkDtimers );
		ctl_watchdog_kick(WDG_DTIM, 0x8000 );

		vTaskSuspend( xHandle_tkDoutputs );
		ctl_watchdog_kick(WDG_DOUT, 0x8000 );

		vTaskSuspend( xHandle_tkGprsTx );
		ctl_watchdog_kick(WDG_GPRSRX, 0x8000 );

		if (!strcmp_P( strupr(argv[2]), PSTR("SOFT\0"))) {
			FF_format(false );
		} else if (!strcmp_P( strupr(argv[2]), PSTR("HARD\0"))) {
			FF_format(true);
		} else {
			xprintf_P( PSTR("ERROR\r\nUSO: reset memory {hard|soft}\r\n\0"));
			return;
		}
	}

	cmdClearScreen();

//	while(1)
//		;

	CCPWrite( &RST.CTRL, RST_SWRST_bm );   /* Issue a Software Reset to initilize the CPU */


}
//------------------------------------------------------------------------------------
static void cmdWriteFunction(void)
{

uint8_t pin;

	FRTOS_CMD_makeArgv();

	// RTC
	// write rtc YYMMDDhhmm
	if (!strcmp_P( strupr(argv[1]), PSTR("RTC\0")) ) {
		( RTC_write_time( argv[2]) > 0)?  pv_snprintfP_OK() : 	pv_snprintfP_ERR();
		return;
	}

	// EE
	// write ee pos string
	if (!strcmp_P( strupr(argv[1]), PSTR("EE\0")) && ( tipo_usuario == USER_TECNICO) ) {
		( EE_test_write ( argv[2], argv[3] ) > 0)?  pv_snprintfP_OK() : 	pv_snprintfP_ERR();
		return;
	}

	// NVMEE
	// write nvmee pos string
	if (!strcmp_P( strupr(argv[1]), PSTR("NVMEE\0")) && ( tipo_usuario == USER_TECNICO) ) {
		NVMEE_test_write ( argv[2], argv[3] );
		pv_snprintfP_OK();
		return;
	}

	// RTC SRAM
	// write rtcram pos string
	if (!strcmp_P( strupr(argv[1]), PSTR("RTCRAM\0"))  && ( tipo_usuario == USER_TECNICO) ) {
		( RTCSRAM_test_write ( argv[2], argv[3] ) > 0)?  pv_snprintfP_OK() : 	pv_snprintfP_ERR();
		return;
	}

	// SENS12V
	// write sens12V {on|off}
	if (!strcmp_P( strupr(argv[1]), PSTR("SENS12V\0")) && ( tipo_usuario == USER_TECNICO) ) {

		if (!strcmp_P( strupr(argv[2]), PSTR("ON\0")) ) {
			IO_set_SENS_12V_CTL();
			pv_snprintfP_OK();
		} else if  (!strcmp_P( strupr(argv[2]), PSTR("OFF\0")) ) {
			IO_clr_SENS_12V_CTL();
			pv_snprintfP_OK();
		} else {
			xprintf_P( PSTR("cmd ERROR: ( write sens12V on{off} )\r\n\0"));
			pv_snprintfP_ERR();
		}
		return;
	}

	// INA
	// write ina id rconfValue
	// Solo escribimos el registro 0 de configuracion.
	if (!strcmp_P( strupr(argv[1]), PSTR("INA\0")) && ( tipo_usuario == USER_TECNICO) ) {
		( INA_test_write ( argv[2], argv[3] ) > 0)?  pv_snprintfP_OK() : 	pv_snprintfP_ERR();
		return;
	}

	// OUTPUTS
	// write output {0..7} {set | clear}
	if (!strcmp_P( strupr(argv[1]), PSTR("OUTPUT\0")) && ( tipo_usuario == USER_TECNICO) ) {

		pin = atoi(argv[2]);

		if (!strcmp_P( strupr(argv[3]), PSTR("SET\0"))) {
			if ( DOUTPUTS_set_pin(pin) ) {
				systemVars.doutputs_conf.d_outputs |= ( 1 << ( 7 - pin )  );
				pv_snprintfP_OK();
			} else {
				pv_snprintfP_ERR();
			}

		} else if (!strcmp_P( strupr(argv[3]), PSTR("CLEAR\0"))) {
			if ( DOUTPUTS_clr_pin(pin) ) {
				systemVars.doutputs_conf.d_outputs &= ~( 1 << ( 7 - pin ) );
				pv_snprintfP_OK();
			} else {
				pv_snprintfP_ERR();
			}

		} else {
			pv_snprintfP_ERR();
		}
		return;
	}

	// RANGE
	// write range {run | stop}
	if (!strcmp_P( strupr(argv[1]), PSTR("RANGE\0")) && ( tipo_usuario == USER_TECNICO) ) {
		if (!strcmp_P( strupr(argv[2]), PSTR("RUN\0")) ) {
			RMETER_set_UPULSE_RUN(); pv_snprintfP_OK(); return;
		}

		if (!strcmp_P( strupr(argv[2]), PSTR("STOP\0")) ) {
			RMETER_clr_UPULSE_RUN(); pv_snprintfP_OK();	return;
		}

		xprintf_P( PSTR("cmd ERROR: ( write range {run|stop} )\r\n\0"));
		return;
	}

	// CONSIGNA
	// write consigna (diurna|nocturna)
	if (!strcmp_P( strupr(argv[1]), PSTR("CONSIGNA\0")) && ( tipo_usuario == USER_TECNICO) ) {

		if (!strcmp_P( strupr(argv[2]), PSTR("DIURNA\0")) ) {
			systemVars.doutputs_conf.consigna.c_aplicada = CONSIGNA_DIURNA;
			DRV8814_set_consigna_diurna(); pv_snprintfP_OK(); return;
		}

		if (!strcmp_P( strupr(argv[2]), PSTR("NOCTURNA\0")) ) {
			systemVars.doutputs_conf.consigna.c_aplicada = CONSIGNA_NOCTURNA;
			DRV8814_set_consigna_nocturna(); pv_snprintfP_OK(); return;
		}

		xprintf_P( PSTR("cmd ERROR: ( write consigna (diurna|nocturna) )\r\n\0"));
		return;
	}

	// VALVE
	// write valve (enable|disable),(set|reset),(sleep|awake),(ph01|ph10) } {A/B}
	//             (open|close) (A|B) (ms)
	//              power {on|off}
	if (!strcmp_P( strupr(argv[1]), PSTR("VALVE\0")) && ( tipo_usuario == USER_TECNICO) ) {
		pv_cmd_write_valve();
		return;
	}

	// GPRS
	// write gprs pwr|sw|rts {on|off}
	// write gprs cmd {atcmd}
	if (!strcmp_P( strupr(argv[1]), PSTR("GPRS\0")) && ( tipo_usuario == USER_TECNICO) ) {
		pv_cmd_rwGPRS(WR_CMD);
		return;
	}

	// CMD NOT FOUND
	xprintf_P( PSTR("ERROR\r\nCMD NOT DEFINED\r\n\0"));
	return;
}
//------------------------------------------------------------------------------------
static void cmdReadFunction(void)
{

int16_t range;

	FRTOS_CMD_makeArgv();

	// DEBUG
 	if (!strcmp_P( strupr(argv[1]), PSTR("DEBUG\0"))) {
 		pv_cmd_read_debug();
 		return;
 	}

	// FUSES
 	if (!strcmp_P( strupr(argv[1]), PSTR("FUSES\0"))) {
 		pv_cmd_read_fuses();
 		return;
 	}

	// WMK
 	if (!strcmp_P( strupr(argv[1]), PSTR("WMK\0")) && ( tipo_usuario == USER_TECNICO) ) {
 		pv_cmd_print_stack_watermarks();
 		return;
 	}

 	// WDT
 	if (!strcmp_P( strupr(argv[1]), PSTR("WDT\0")) && ( tipo_usuario == USER_TECNICO) ) {
 		ctl_print_wdg_timers();
 		return;
 	}

	// RTC
	// read rtc
	if (!strcmp_P( strupr(argv[1]), PSTR("RTC\0")) ) {
		RTC_read_time();
		return;
	}

	// EE
	// read ee address length
	if (!strcmp_P( strupr(argv[1]), PSTR("EE\0")) && ( tipo_usuario == USER_TECNICO) ) {
		 EE_test_read ( argv[2], argv[3] );
		return;
	}

	// NVMEE
	// read nvmee address length
	if (!strcmp_P( strupr(argv[1]), PSTR("NVMEE\0")) && ( tipo_usuario == USER_TECNICO) ) {
		NVMEE_test_read ( argv[2], argv[3] );
		return;
	}

	// RTC SRAM
	// read rtcram address length
	if (!strcmp_P( strupr(argv[1]), PSTR("RTCRAM\0")) && ( tipo_usuario == USER_TECNICO) ) {
		RTCSRAM_test_read ( argv[2], argv[3] );
		return;
	}

	// INA
	// read ina id regName
	if (!strcmp_P( strupr(argv[1]), PSTR("INA\0")) && ( tipo_usuario == USER_TECNICO) ) {
		 INA_test_read ( argv[2], argv[3] );
		return;
	}

	// FRAME
	// read frame
	if (!strcmp_P( strupr(argv[1]), PSTR("FRAME\0")) ) {
		data_read_frame ( true );
		return;
	}

	// BATTERY
	// read battery
	if (!strcmp_P( strupr(argv[1]), PSTR("BATTERY\0")) ) {
		pv_cmd_read_battery();
		return;
	}

	// ACH { 0..4}
	// read ach x
	if (!strcmp_P( strupr(argv[1]), PSTR("ACH\0")) && ( tipo_usuario == USER_TECNICO) ) {
		pv_cmd_read_analog_channel();
		return;
	}

	// DIN
	// read din
	if (!strcmp_P( strupr(argv[1]), PSTR("DIN\0")) && ( tipo_usuario == USER_TECNICO) ) {
		pv_cmd_read_digital_channels();
		return;
	}

	// MEMORY
	// read memory
	if (!strcmp_P( strupr(argv[1]), PSTR("MEMORY\0")) && ( tipo_usuario == USER_TECNICO) ) {
		pv_cmd_read_memory();
		return;
	}

	// RANGE
	// read range
	if ( ( spx_io_board == SPX_IO5CH ) && (!strcmp_P( strupr(argv[1]), PSTR("RANGE\0"))) && ( tipo_usuario == USER_TECNICO) ) {
		RMETER_ping( &range, false );
		xprintf_P( PSTR("RANGE=%d\r\n\0"),range);
		pv_snprintfP_OK();
		return;
	}

	// GPRS
	// read gprs (rsp,cts,dcd,ri)
	if (!strcmp_P( strupr(argv[1]), PSTR("GPRS\0")) && ( tipo_usuario == USER_TECNICO) ) {
		pv_cmd_rwGPRS(RD_CMD);
		return;
	}

	// CMD NOT FOUND
	xprintf_P( PSTR("ERROR\r\nCMD NOT DEFINED\r\n\0"));
	return;

}
//------------------------------------------------------------------------------------
static void cmdClearScreen(void)
{
	// ESC [ 2 J
	xprintf_P( PSTR("\x1B[2J\0"));
}
//------------------------------------------------------------------------------------
static void cmdConfigFunction(void)
{

bool retS = false;

	FRTOS_CMD_makeArgv();

	// COUNTERS
	// config counter {0..1} cname magPP
	if (!strcmp_P( strupr(argv[1]), PSTR("COUNTER\0")) ) {
		retS = counters_config_channel( atoi(argv[2]), argv[3], argv[4] );
		retS ? pv_snprintfP_OK() : pv_snprintfP_ERR();
		return;
	}

	// CDTIME ( counter_debounce_time )
	// config cdtime { val }
	if (!strcmp_P( strupr(argv[1]), PSTR("CDTIME\0")) ) {
		counters_config_debounce_time( argv[2] );
		pv_snprintfP_OK();
		return;
	}

	// DEBUG
	// config debug
	if (!strcmp_P( strupr(argv[1]), PSTR("DEBUG\0"))) {
		if (!strcmp_P( strupr(argv[2]), PSTR("NONE\0"))) {
			systemVars.debug = DEBUG_NONE;
			retS = true;
		} else if (!strcmp_P( strupr(argv[2]), PSTR("COUNTER\0"))) {
			systemVars.debug = DEBUG_COUNTER;
			retS = true;
		} else if (!strcmp_P( strupr(argv[2]), PSTR("DATA\0"))) {
			systemVars.debug = DEBUG_DATA;
			retS = true;
		} else if (!strcmp_P( strupr(argv[2]), PSTR("GPRS\0"))) {
			systemVars.debug = DEBUG_GPRS;
			retS = true;
		} else {
			retS = false;
		}
		retS ? pv_snprintfP_OK() : 	pv_snprintfP_ERR();
		return;
	}

	// DEFAULT
	// config default
	if (!strcmp_P( strupr(argv[1]), PSTR("DEFAULT\0"))) {
		u_load_defaults( argv[2] );
		pv_snprintfP_OK();
		return;
	}

	// SAVE
	// config save
	if (!strcmp_P( strupr(argv[1]), PSTR("SAVE\0"))) {
		u_save_params_in_NVMEE();
		pv_snprintfP_OK();
		return;
	}

	// TIMERPOLL
	// config timerpoll
	if (!strcmp_P( strupr(argv[1]), PSTR("TIMERPOLL\0")) ) {
		u_config_timerpoll( argv[2] );
		pv_snprintfP_OK();
		return;
	}

	// OFFSET
	// config offset {ch} {mag}
	if (!strcmp_P( strupr(argv[1]), PSTR("OFFSET\0")) ) {
		ainputs_config_offset( argv[2], argv[3] ) ? pv_snprintfP_OK() : pv_snprintfP_ERR();
		return;
	}

	// AUTOCAL
	// config autocal {ch} {mag}
	if (!strcmp_P( strupr(argv[1]), PSTR("AUTOCAL\0")) ) {
		ainputs_config_autocalibrar( argv[2], argv[3] ) ? pv_snprintfP_OK() : pv_snprintfP_ERR();
		return;
	}

	// SENSORTIME
	// config sensortime
	if (!strcmp_P( strupr(argv[1]), PSTR("SENSORTIME\0")) ) {
		ainputs_config_sensortime( argv[2] );
		pv_snprintfP_OK();
		return;
	}

	// INASPAN
	// config inaspan
	if (!strcmp_P( strupr(argv[1]), PSTR("INASPAN\0"))) {
		ainputs_config_span( argv[2], argv[3] );
		pv_snprintfP_OK();
		return;
	}

	// ANALOG
	// config analog {0..n} aname imin imax mmin mmax
	if (!strcmp_P( strupr(argv[1]), PSTR("ANALOG\0")) ) {
		ainputs_config_channel( atoi(argv[2]), argv[3], argv[4], argv[5], argv[6], argv[7] ) ? pv_snprintfP_OK() : pv_snprintfP_ERR();
		return;
	}

	// DIGITAL
	// config digital {0..N} dname
	if (!strcmp_P( strupr(argv[1]), PSTR("DIGITAL\0")) ) {
		dinputs_config_channel( atoi(argv[2]), argv[3]) ? pv_snprintfP_OK() : pv_snprintfP_ERR();
		return;
	}

	// DTIMERS
	// config dtimers  {on|off}
	if ( ( !strcmp_P( strupr(argv[1]), PSTR("DTIMERS\0"))) ) {

		if ( dtimers_config_timermode(argv[3]) ) {
			pv_snprintfP_OK();
			return;
		} else {
			pv_snprintfP_ERR();
			return;
		}

		pv_snprintfP_ERR();
		return;
	}

	// RANGEMETER
	// config rangemeter {on|off}
	if ( !strcmp_P( strupr(argv[1]), PSTR("RANGEMETER\0"))) {

		if ( range_config(argv[3]) ) {
			pv_snprintfP_OK();
			return;
		} else {
			pv_snprintfP_ERR();
			return;
		}

		pv_snprintfP_ERR();
		return;
	}

	// CONSIGNAS
	// config consigna {on|off) {hhmm_dia hhmm_noche}
	if ( ( spx_io_board == SPX_IO5CH ) && (!strcmp_P( strupr(argv[1]), PSTR("CONSIGNA\0"))) ) {
		doutputs_config_consignas( argv[2], argv[3], argv[4]) ? pv_snprintfP_OK() : pv_snprintfP_ERR();
		return;
	}

	// TIMERDIAL
	// config timerdial
	if ( ( spx_io_board == SPX_IO5CH ) && (!strcmp_P( strupr(argv[1]), PSTR("TIMERDIAL\0"))) ) {
		u_gprs_config_timerdial( argv[2] );
		pv_snprintfP_OK();
		return;
	}

	// PWRSAVE
	if (!strcmp_P( strupr(argv[1]), PSTR("PWRSAVE\0"))) {
		u_gprs_configPwrSave ( argv[2], argv[3], argv[4] );
		pv_snprintfP_OK();
		return;
	}

	// APN
	if (!strcmp_P( strupr(argv[1]), PSTR("APN\0"))) {
		if ( argv[2] == NULL ) {
			retS = false;
		} else {
			memset(systemVars.gprs_conf.apn, '\0', sizeof(systemVars.gprs_conf.apn));
			memcpy(systemVars.gprs_conf.apn, argv[2], sizeof(systemVars.gprs_conf.apn));
			systemVars.gprs_conf.apn[APN_LENGTH - 1] = '\0';
			retS = true;
		}
		retS ? pv_snprintfP_OK() : 	pv_snprintfP_ERR();
		return;
	}

	// PORT ( SERVER IP PORT)
	if (!strcmp_P( strupr(argv[1]), PSTR("PORT\0"))) {
		if ( argv[2] == NULL ) {
			retS = false;
		} else {
			memset(systemVars.gprs_conf.server_tcp_port, '\0', sizeof(systemVars.gprs_conf.server_tcp_port));
			memcpy(systemVars.gprs_conf.server_tcp_port, argv[2], sizeof(systemVars.gprs_conf.server_tcp_port));
			systemVars.gprs_conf.server_tcp_port[PORT_LENGTH - 1] = '\0';
			retS = true;
		}
		retS ? pv_snprintfP_OK() : 	pv_snprintfP_ERR();
		return;
	}

	// IP (SERVER IP ADDRESS)
	if (!strcmp_P( strupr(argv[1]), PSTR("IP\0"))) {
		if ( argv[2] == NULL ) {
			retS = false;
		} else {
			memset(systemVars.gprs_conf.server_ip_address, '\0', sizeof(systemVars.gprs_conf.server_ip_address));
			memcpy(systemVars.gprs_conf.server_ip_address, argv[2], sizeof(systemVars.gprs_conf.server_ip_address));
			systemVars.gprs_conf.server_ip_address[IP_LENGTH - 1] = '\0';
			retS = true;
		}
		retS ? pv_snprintfP_OK() : 	pv_snprintfP_ERR();
		return;
	}

	// SCRIPT ( SERVER SCRIPT SERVICE )
	if (!strcmp_P( strupr(argv[1]), PSTR("SCRIPT\0"))) {
		if ( argv[2] == NULL ) {
			retS = false;
		} else {
			memset(systemVars.gprs_conf.serverScript, '\0', sizeof(systemVars.gprs_conf.serverScript));
			memcpy(systemVars.gprs_conf.serverScript, argv[2], sizeof(systemVars.gprs_conf.serverScript));
			systemVars.gprs_conf.serverScript[SCRIPT_LENGTH - 1] = '\0';
			retS = true;
		}
		retS ? pv_snprintfP_OK() : 	pv_snprintfP_ERR();
		return;
	}

	// SIMPWD
	if (!strcmp_P( strupr(argv[1]), PSTR("SIMPWD\0"))) {
		if ( argv[2] == NULL ) {
			retS = false;
		} else {
			memset(systemVars.gprs_conf.simpwd, '\0', sizeof(systemVars.gprs_conf.simpwd));
			memcpy(systemVars.gprs_conf.simpwd, argv[2], sizeof(systemVars.gprs_conf.simpwd));
			systemVars.gprs_conf.simpwd[PASSWD_LENGTH - 1] = '\0';
			retS = true;
		}
		retS ? pv_snprintfP_OK() : 	pv_snprintfP_ERR();
		return;
	}

	// DLGID
	if (!strcmp_P( strupr(argv[1]), PSTR("DLGID\0"))) {
		if ( argv[2] == NULL ) {
			retS = false;
			} else {
				memset(systemVars.gprs_conf.dlgId,'\0', sizeof(systemVars.gprs_conf.dlgId) );
				memcpy(systemVars.gprs_conf.dlgId, argv[2], sizeof(systemVars.gprs_conf.dlgId));
				retS = true;
			}
		retS ? pv_snprintfP_OK() : 	pv_snprintfP_ERR();
		return;
	}

	pv_snprintfP_ERR();
	return;
}
//------------------------------------------------------------------------------------
static void cmdHelpFunction(void)
{

	FRTOS_CMD_makeArgv();

	// HELP WRITE
	if (!strcmp_P( strupr(argv[1]), PSTR("WRITE\0"))) {
		xprintf_P( PSTR("-write\r\n\0"));
		xprintf_P( PSTR("  rtc YYMMDDhhmm\r\n\0"));
		if ( tipo_usuario == USER_TECNICO ) {
			xprintf_P( PSTR("  (ee,nvmee,rtcram) {pos} {string}\r\n\0"));
			xprintf_P( PSTR("  ina {id} {rconfValue}, sens12V {on|off}\r\n\0"));

			if ( spx_io_board == SPX_IO8CH ) {
				xprintf_P( PSTR("  output {0..7} {set | clear}\r\n\0"));
			}

			if ( spx_io_board == SPX_IO5CH ) {
				xprintf_P( PSTR("  range {run | stop}\r\n\0"));
				xprintf_P( PSTR("  consigna (diurna|nocturna)\r\n\0"));
				xprintf_P( PSTR("  valve (enable|disable),(set|reset),(sleep|awake),(ph01|ph10) } {A/B}\r\n\0"));
				xprintf_P( PSTR("        (open|close) (A|B) (ms)\r\n\0"));
				xprintf_P( PSTR("        power {on|off}\r\n\0"));
			}

			xprintf_P( PSTR("  gprs (pwr|sw|cts|dtr) {on|off}\r\n\0"));
			xprintf_P( PSTR("       cmd {atcmd}, redial\r\n\0"));

		}
		return;
	}

	// HELP READ
	else if (!strcmp_P( strupr(argv[1]), PSTR("READ\0"))) {
		xprintf_P( PSTR("-read\r\n\0"));
		xprintf_P( PSTR("  rtc, frame, fuses\r\n\0"));
		if ( tipo_usuario == USER_TECNICO ) {
			xprintf_P( PSTR("  id\r\n\0"));
			xprintf_P( PSTR("  (ee,nvmee,rtcram) {pos} {lenght}\r\n\0"));
			xprintf_P( PSTR("  ina (id) {conf|chXshv|chXbusv|mfid|dieid}\r\n\0"));

			if ( spx_io_board == SPX_IO5CH ) {
				xprintf_P( PSTR("  ach {0..%d}, battery\r\n\0"), ( NRO_ANINPUTS - 1 ) );
			} else if ( spx_io_board == SPX_IO8CH ) {
				xprintf_P( PSTR("  ach {0..%d}\r\n\0"), ( NRO_ANINPUTS - 1 ) );
			}

			xprintf_P( PSTR("  din\r\n\0"));
			xprintf_P( PSTR("  memory {full}\r\n\0"));

			if ( spx_io_board == SPX_IO5CH ) {
				xprintf_P( PSTR("  range\r\n\0"));
			}

			xprintf_P( PSTR("  gprs (rsp,rts,dcd,ri)\r\n\0"));
		}
		return;

	}

	// HELP RESET
	else if (!strcmp_P( strupr(argv[1]), PSTR("RESET\0"))) {
		xprintf_P( PSTR("-reset\r\n\0"));
		xprintf_P( PSTR("  memory {soft|hard}\r\n\0"));
		return;

	}

	// HELP CONFIG
	else if (!strcmp_P( strupr(argv[1]), PSTR("CONFIG\0"))) {
		xprintf_P( PSTR("-config\r\n\0"));
		xprintf_P( PSTR("  user {normal|tecnico}\r\n\0"));
		xprintf_P( PSTR("  dlgid, apn, port, ip, script, simpasswd\r\n\0"));
		xprintf_P( PSTR("  debug {none,counter,data, gprs}\r\n\0"));
		xprintf_P( PSTR("  counter {0..%d} cname magPP\r\n\0"), ( NRO_COUNTERS - 1 ) );
		xprintf_P( PSTR("  cdtime {val}\r\n\0"));
		xprintf_P( PSTR("  analog {0..%d} aname imin imax mmin mmax\r\n\0"),( NRO_ANINPUTS - 1 ) );
		xprintf_P( PSTR("  offset {ch} {mag}, inaspan {ch} {mag}\r\n\0"));
		xprintf_P( PSTR("  autocal {ch} {mag}\r\n\0"));
		xprintf_P( PSTR("  digital {0..%d} dname\r\n\0"), ( NRO_DINPUTS - 1 ) );

		if ( spx_io_board == SPX_IO5CH ) {
			xprintf_P( PSTR("  rangemeter {on|off}\r\n\0"));
			xprintf_P( PSTR("  consigna {on|off) {hhmm_dia hhmm_noche}\r\n\0"));
			xprintf_P( PSTR("  pwrsave {on|off} {hhmm1}, {hhmm2}\r\n\0"));
			xprintf_P( PSTR("  timerpoll {val}, timerdial {val}, sensortime {val}\r\n\0"));
		}

		if ( spx_io_board == SPX_IO8CH ) {
			xprintf_P( PSTR("  dtimers {on|off}\r\n\0"));
			xprintf_P( PSTR("  timerpoll {val}, sensortime {val}\r\n\0"));
		}

		xprintf_P( PSTR("  default {SPY|OSE|UTE}\r\n\0"));
		xprintf_P( PSTR("  save\r\n\0"));
	}

	// HELP KILL
	else if (!strcmp_P( strupr(argv[1]), PSTR("KILL\0")) && ( tipo_usuario == USER_TECNICO) ) {
		xprintf_P( PSTR("-kill {counter, data, dinputs, doutputs, gprstx, gprsrx }\r\n\0"));
		return;

	} else {

		// HELP GENERAL
		xprintf_P( PSTR("\r\nSpymovil %s %s %s %s\r\n\0"), SPX_HW_MODELO, SPX_FTROS_VERSION, SPX_FW_REV, SPX_FW_DATE);
		xprintf_P( PSTR("Clock %d Mhz, Tick %d Hz\r\n\0"),SYSMAINCLK, configTICK_RATE_HZ );
		xprintf_P( PSTR("Available commands are:\r\n\0"));
		xprintf_P( PSTR("-cls\r\n\0"));
		xprintf_P( PSTR("-help\r\n\0"));
		xprintf_P( PSTR("-status\r\n\0"));
		xprintf_P( PSTR("-reset...\r\n\0"));
		xprintf_P( PSTR("-kill...\r\n\0"));
		xprintf_P( PSTR("-write...\r\n\0"));
		xprintf_P( PSTR("-read...\r\n\0"));
		xprintf_P( PSTR("-config...\r\n\0"));

	}

	xprintf_P( PSTR("\r\n\0"));

}
//------------------------------------------------------------------------------------
static void cmdKillFunction(void)
{

	FRTOS_CMD_makeArgv();

	// KILL COUNTER
	if (!strcmp_P( strupr(argv[1]), PSTR("COUNTER\0"))) {
		vTaskSuspend( xHandle_tkCounter );
		ctl_watchdog_kick(WDG_COUNT, 0x8000 );
		return;
	}

	// KILL DATA
	if (!strcmp_P( strupr(argv[1]), PSTR("DATA\0"))) {
		vTaskSuspend( xHandle_tkData );
		ctl_watchdog_kick(WDG_DAT, 0x8000 );
		return;
	}

	// KILL DTIMERS
	if (!strcmp_P( strupr(argv[1]), PSTR("DTIMERS\0"))) {
		vTaskSuspend( xHandle_tkDtimers );
		ctl_watchdog_kick(WDG_DTIM, 0x8000 );
		return;
	}

	// KILL DOUTPUTS
	if (!strcmp_P( strupr(argv[1]), PSTR("DOUTPUTS\0"))) {
		vTaskSuspend( xHandle_tkDoutputs );
		ctl_watchdog_kick(WDG_DOUT, 0x8000 );
		return;
	}

	// KILL GPRSTX
	if (!strcmp_P( strupr(argv[1]), PSTR("GPRSTX\0"))) {
		vTaskSuspend( xHandle_tkGprsTx );
		ctl_watchdog_kick(WDG_GPRSTX, 0x8000 );
		// Dejo la flag de modem prendido para poder leer comandos
		GPRS_stateVars.modem_prendido = true;
		return;
	}

	// KILL GPRSRX
	if (!strcmp_P( strupr(argv[1]), PSTR("GPRSRX\0"))) {
		vTaskSuspend( xHandle_tkGprsRx );
		ctl_watchdog_kick(WDG_GPRSRX, 0x8000 );
		return;
	}

	pv_snprintfP_OK();
	return;
}
//------------------------------------------------------------------------------------
static void pv_snprintfP_OK(void )
{
	xprintf_P( PSTR("ok\r\n\0"));
}
//------------------------------------------------------------------------------------
static void pv_snprintfP_ERR(void)
{
	xprintf_P( PSTR("error\r\n\0"));
}
//------------------------------------------------------------------------------------
static void pv_cmd_read_fuses(void)
{
	// Lee los fuses.

uint8_t fuse0,fuse1,fuse2,fuse4,fuse5;

	fuse0 = nvm_fuses_read(0x00);	// FUSE0
	xprintf_P( PSTR("FUSE0=0x%x\r\n\0"),fuse0);

	fuse1 = nvm_fuses_read(0x01);	// FUSE1
	xprintf_P( PSTR("FUSE1=0x%x\r\n\0"),fuse1);

	fuse2 = nvm_fuses_read(0x02);	// FUSE2
	xprintf_P( PSTR("FUSE2=0x%x\r\n\0"),fuse2);

	fuse4 = nvm_fuses_read(0x04);	// FUSE4
	xprintf_P( PSTR("FUSE4=0x%x\r\n\0"),fuse4);

	fuse5 = nvm_fuses_read(0x05);	// FUSE5
	xprintf_P( PSTR("FUSE5=0x%x\r\n\0"),fuse5);

	if ( (fuse0 != 0xFF) || ( fuse1 != 0xAA) || (fuse2 != 0xFD) || (fuse4 != 0xF5) || ( fuse5 != 0xD6) ) {
		xprintf_P( PSTR("FUSES ERROR !!!.\r\n\0"));
		xprintf_P( PSTR("Los valores deben ser: FUSE0=0xFF,FUSE1=0xAA,FUSE2=0xFD,FUSE4=0xF5,FUSE5=0xD6\r\n\0"));
		xprintf_P( PSTR("Deben reconfigurarse !!.\r\n\0"));
		pv_snprintfP_ERR();
		return;
	}
	pv_snprintfP_OK();
	return;
}
//------------------------------------------------------------------------------------
static void pv_cmd_print_stack_watermarks(void)
{

UBaseType_t uxHighWaterMark;

	// tkIdle
	uxHighWaterMark = uxTaskGetStackHighWaterMark( xHandle_idle );
	xprintf_P( PSTR("IDLE:%03d,%03d,[%03d]\r\n\0"),configMINIMAL_STACK_SIZE,uxHighWaterMark,(configMINIMAL_STACK_SIZE - uxHighWaterMark)) ;

	// tkControl
	uxHighWaterMark = uxTaskGetStackHighWaterMark( xHandle_tkCtl );
	xprintf_P( PSTR("CTL: %03d,%03d,[%03d]\r\n\0"),tkCtl_STACK_SIZE,uxHighWaterMark, (tkCtl_STACK_SIZE - uxHighWaterMark));

	// tkCmd
	uxHighWaterMark = uxTaskGetStackHighWaterMark( xHandle_tkCmd );
	xprintf_P( PSTR("CMD: %03d,%03d,[%03d]\r\n\0"),tkCmd_STACK_SIZE,uxHighWaterMark,(tkCmd_STACK_SIZE - uxHighWaterMark)) ;

	// tkCounters
	uxHighWaterMark = uxTaskGetStackHighWaterMark( xHandle_tkCounter );
	xprintf_P( PSTR("CNT: %03d,%03d,[%03d]\r\n\0"),tkCounter_STACK_SIZE,uxHighWaterMark, (tkCounter_STACK_SIZE - uxHighWaterMark));

	// tkData
	uxHighWaterMark = uxTaskGetStackHighWaterMark( xHandle_tkData );
	xprintf_P( PSTR("DAT: %03d,%03d,[%03d]\r\n\0"),tkData_STACK_SIZE,uxHighWaterMark, (tkData_STACK_SIZE - uxHighWaterMark));

	// tkDtimers
	uxHighWaterMark = uxTaskGetStackHighWaterMark( xHandle_tkDtimers );
	xprintf_P( PSTR("DTIM: %03d,%03d,[%03d]\r\n\0"),tkDtimers_STACK_SIZE,uxHighWaterMark, (tkDtimers_STACK_SIZE - uxHighWaterMark));

	// tkDout
	uxHighWaterMark = uxTaskGetStackHighWaterMark( xHandle_tkDoutputs );
	xprintf_P( PSTR("DOUT: %03d,%03d,[%03d]\r\n\0"), tkDoutputs_STACK_SIZE,uxHighWaterMark, ( tkDoutputs_STACK_SIZE - uxHighWaterMark));

	// tkGprsTX
	uxHighWaterMark = uxTaskGetStackHighWaterMark( xHandle_tkGprsTx );
	xprintf_P( PSTR("TX: %03d,%03d,[%03d]\r\n\0"), tkGprs_tx_STACK_SIZE ,uxHighWaterMark, ( tkGprs_tx_STACK_SIZE - uxHighWaterMark));

	// tkGprsRX
	uxHighWaterMark = uxTaskGetStackHighWaterMark( xHandle_tkGprsRx );
	xprintf_P( PSTR("RX: %03d,%03d,[%03d]\r\n\0"), tkGprs_rx_STACK_SIZE ,uxHighWaterMark, ( tkGprs_rx_STACK_SIZE - uxHighWaterMark));


}
//------------------------------------------------------------------------------------
static void pv_cmd_read_battery(void)
{

float battery;

	AINPUTS_prender_12V();
	INA_config(1, CONF_INAS_AVG128 );
	battery = 0.008 * AINPUTS_read_battery();
	INA_config(1, CONF_INAS_SLEEP );
	AINPUTS_apagar_12V();

	xprintf_P( PSTR("Battery=%.02f\r\n\0"), battery );

}
//------------------------------------------------------------------------------------
static void pv_cmd_read_analog_channel(void)
{

uint16_t raw_val;
float mag_val;


	if ( atoi(argv[2]) <  NRO_ANINPUTS ) {
		ainputs_read( atoi(argv[2]),&raw_val, &mag_val );
		xprintf_P( PSTR("anCH[%02d] raw=%d,mag=%.02f\r\n\0"),atoi(argv[2]),raw_val, mag_val );
	} else {
		pv_snprintfP_ERR();
	}
}
//------------------------------------------------------------------------------------
static void pv_cmd_read_digital_channels(void)
{

	// Leo e imprimo todos los canales digitales a la vez.

uint8_t din0, din1;
uint16_t dinputs[8];
uint8_t i;

	if ( spx_io_board == SPX_IO5CH ) {
		// Leo los canales digitales.
		din0 = DIN_read_pin(0, spx_io_board );
		din1 = DIN_read_pin(1, spx_io_board );
		xprintf_P( PSTR("Din0: %d, Din1: %d\r\n\0"), din0, din1 );
		return;

	} else 	if ( spx_io_board == SPX_IO8CH ) {

		for ( i = 0; i < 4; i++ ) {
			dinputs[i] = dinputs_read(i);
		}

		for ( i = 4; i < 8; i++ ) {
			if ( systemVars.dtimers_enabled ) {
			//	dinputs[i] = dtimers_read(i-4);
			} else {
				dinputs[i] = dinputs_read(i);
			}
		}

		xprintf_P( PSTR("DINPUTS: "));
		for ( i=0; i < 8; i++ ) {
			xprintf_P( PSTR("%d"), dinputs[i] );
		}
		xprintf_P( PSTR("\r\n"));
		return;
	}
}
//------------------------------------------------------------------------------------
static void pv_cmd_read_memory(void)
{
	// Si hay datos en memoria los lee todos y los muestra en pantalla
	// Leemos la memoria e imprimo los datos.
	// El problema es que si hay muchos datos puede excederse el tiempo de watchdog y
	// resetearse el dlg.
	// Para esto, cada 32 registros pateo el watchdog.
	// El proceso es similar a tkGprs.transmitir_dataframe


FAT_t l_fat;
size_t bRead;
uint16_t rcds = 0;
bool detail = false;

	if (!strcmp_P( strupr(argv[2]), PSTR("FULL\0"))) {
		detail = true;
	}

	FF_rewind();
	while(1) {

		bRead = FF_readRcd( &dr, sizeof(st_dataRecord_t));
		FAT_read(&l_fat);

		if ( bRead == 0) {
			xprintf_P(PSTR( "MEM Empty\r\n"));
			break;
		}

		if ( ( rcds++ % 32) == 0 ) {
			ctl_watchdog_kick(WDG_CMD, WDG_CMD_TIMEOUT);
		}

		// Inversamente a cuando en tkData guarde en memoria, convierto el datarecord a dataframe
		// Copio al dr solo los campos que correspondan
		switch ( spx_io_board ) {
		case SPX_IO5CH:
			memcpy( &df.ainputs, &dr.df.io5.ainputs, ( NRO_ANINPUTS * sizeof(float)));
			memcpy( &df.dinputsA, &dr.df.io5.dinputsA, ( NRO_DINPUTS * sizeof(uint16_t)));
			memcpy( &df.counters, &dr.df.io5.counters, ( NRO_COUNTERS * sizeof(float)));
			df.range = dr.df.io5.range;
			df.battery = dr.df.io5.battery;
			memcpy( &df.rtc, &dr.rtc, sizeof(RtcTimeType_t) );
			break;
		case SPX_IO8CH:
			memcpy( &df.ainputs, &dr.df.io8.ainputs, ( NRO_ANINPUTS * sizeof(float)));
			memcpy( &df.dinputsA, &dr.df.io8.dinputsA, ( NRO_DINPUTS * sizeof(uint8_t)));
			memcpy( &df.dinputsB, &dr.df.io8.dinputsB, ( NRO_DINPUTS * sizeof(uint16_t)));
			memcpy( &df.counters, &dr.df.io8.counters, ( NRO_COUNTERS * sizeof(float)));
			memcpy( &df.rtc, &dr.rtc, sizeof(RtcTimeType_t) );
			break;
		default:
			return;
		}

		// imprimo
		if ( detail ) {
			xprintf_P( PSTR("memory: wrPtr=%d,rdPtr=%d,delPtr=%d,r4wr=%d,r4rd=%d,r4del=%d \r\n\0"), l_fat.wrPTR,l_fat.rdPTR, l_fat.delPTR,l_fat.rcds4wr,l_fat.rcds4rd,l_fat.rcds4del );
		}

		// Imprimo el registro
		xprintf_P( PSTR("CTL=%d LINE=%04d%02d%02d,%02d%02d%02d\0"), l_fat.rdPTR, df.rtc.year, df.rtc.month, df.rtc.day, df.rtc.hour, df.rtc.min, df.rtc.sec );

		u_df_print_analogicos( &df );
	    u_df_print_digitales( &df );
		u_df_print_contadores( &df );
		u_df_print_range( &df );
		u_df_print_battery( &df );

		xprintf_P(PSTR( "\r\n"));
	}

}
//------------------------------------------------------------------------------------
static void pv_cmd_write_valve(void)
{
	// write valve (enable|disable),(set|reset),(sleep|awake),(ph01|ph10) } {A/B}
	//             (open|close) (A|B) (ms)
	//              power {on|off}

	// write valve enable (A|B)
	if (!strcmp_P( strupr(argv[2]), PSTR("ENABLE\0")) ) {
		DRV8814_enable_pin( toupper(argv[3][0]), 1); pv_snprintfP_OK();
		return;
	}

	// write valve disable (A|B)
	if (!strcmp_P( strupr(argv[2]), PSTR("DISABLE\0")) ) {
		DRV8814_enable_pin( toupper(argv[3][0]), 0); pv_snprintfP_OK();
		return;
	}

	// write valve set
	if (!strcmp_P( strupr(argv[2]), PSTR("SET\0")) ) {
		DRV8814_reset_pin(1); pv_snprintfP_OK();
		return;
	}

	// write valve reset
	if (!strcmp_P( strupr(argv[2]), PSTR("RESET\0")) ) {
		DRV8814_reset_pin(0); pv_snprintfP_OK();
		return;
	}

	// write valve sleep
	if (!strcmp_P( strupr(argv[2]), PSTR("SLEEP\0")) ) {
		DRV8814_sleep_pin(1);  pv_snprintfP_OK();
		return;
	}

	// write valve awake
	if (!strcmp_P( strupr(argv[2]), PSTR("AWAKE\0")) ) {
		DRV8814_sleep_pin(0);  pv_snprintfP_OK();
		return;
	}

	// write valve ph01 (A|B)
	if (!strcmp_P( strupr(argv[2]), PSTR("PH01\0")) ) {
		DRV8814_phase_pin( toupper(argv[3][0]), 1);  pv_snprintfP_OK();
		return;
	}

	// write valve ph10 (A|B)
	if (!strcmp_P( strupr(argv[2]), PSTR("PH10\0")) ) {
		DRV8814_phase_pin( toupper(argv[3][0]), 0);  pv_snprintfP_OK();
		return;
	}

	// write valve power on|off
	if (!strcmp_P( strupr(argv[2]), PSTR("POWER\0")) ) {

		if (!strcmp_P( strupr(argv[3]), PSTR("ON\0")) ) {
			DRV8814_power_on();
			pv_snprintfP_OK();
			return;
		}
		if (!strcmp_P( strupr(argv[3]), PSTR("OFF\0")) ) {
			DRV8814_power_off();
			pv_snprintfP_OK();
			return;
		}
		pv_snprintfP_ERR();
		return;
	}

	//  write valve (open|close) (A|B) (ms)
	if (!strcmp_P( strupr(argv[2]), PSTR("VALVE\0")) ) {

		// Proporciono corriente.
		DRV8814_power_on();
		// Espero 10s que se carguen los condensasores
		vTaskDelay( ( TickType_t)( 10000 / portTICK_RATE_MS ) );

		if (!strcmp_P( strupr(argv[3]), PSTR("OPEN\0")) ) {
			DRV8814_vopen( toupper(argv[4][0]), 100);
			return;
		}
		if (!strcmp_P( strupr(argv[3]), PSTR("CLOSE\0")) ) {
			DRV8814_vclose( toupper(argv[4][0]), 100);
			return;
		}

		DRV8814_power_off();
		return;
	}

	pv_snprintfP_ERR();
	return;

}
//------------------------------------------------------------------------------------
static void pv_cmd_read_debug(void)
{
	// Funcion privada para testing de funcionalidades particulares

	// Caso 1: Leo el nivel logico de los pines de control de comunicaciones

uint8_t ina_id;
uint8_t i2c_bus_address = 0;
char data[3];


	if (!strcmp_P( strupr(argv[2]), PSTR("INAPRES\0"))) {
		ina_id = atoi( argv[3] );
		switch ( ina_id ) {
		case 0:
			i2c_bus_address = BUSADDR_INA_A;
			break;
		case 1:
			i2c_bus_address = BUSADDR_INA_B;
			break;
		case 2:
			i2c_bus_address = BUSADDR_INA_C;
			break;

		}

		if ( I2C_test_device( i2c_bus_address ,INA3231_CONF, data, 2 ) ) {
			xprintf_P( PSTR("INA%02d present\r\n\0"), ina_id);
		} else {
			xprintf_P( PSTR("INA%02d NO present\r\n\0"), ina_id);

		}

	}


/*
  	if (!strcmp_P( strupr(argv[2]), PSTR("INAPRES\0"))) {
		ina_id = atoi( argv[3] );
		if ( INA_test_presence ( ina_id ) ) {
			xprintf_P( PSTR("INA%02d present\r\n\0"), ina_id);
		} else {
			xprintf_P( PSTR("INA%02d NO present\r\n\0"), ina_id);

		}
	}

*/

/*
uint8_t baud, term;
uint8_t bus_status;
uint8_t i;


	baud = IO_read_BAUD_PIN();
	term = IO_read_TERMCTL_PIN();

	xprintf_P( PSTR("TERM=%d\r\n\0"),term );
	xprintf_P( PSTR("BAUD=%d\r\n\0"),baud );


	bus_status = TWIE.MASTER.STATUS; //& TWI_MASTER_BUSSTATE_gm;
	xprintf_P( PSTR("I2C_STATUS: 0x%02x\r\n\0"),TWIE.MASTER.STATUS );

	if (  ( bus_status == TWI_MASTER_BUSSTATE_IDLE_gc ) || ( bus_status == TWI_MASTER_BUSSTATE_OWNER_gc ) ) {
		return;
	}

	// El status esta indicando errores. Debo limpiarlos antes de usar la interface.
	if ( (bus_status & TWI_MASTER_ARBLOST_bm) != 0 ) {
		xprintf_P( PSTR("ARBLOST\r\n\0"));
		TWIE.MASTER.STATUS = bus_status | TWI_MASTER_ARBLOST_bm;
	}

	if ( (bus_status & TWI_MASTER_BUSERR_bm) != 0 ) {
		xprintf_P( PSTR("BUSERR\r\n\0"));
		TWIE.MASTER.STATUS = bus_status | TWI_MASTER_BUSERR_bm;
	}

	if ( (bus_status & TWI_MASTER_WIF_bm) != 0 ) {
		xprintf_P( PSTR("WIF\r\n\0"));
		TWIE.MASTER.STATUS = bus_status | TWI_MASTER_WIF_bm;
	}

	if ( (bus_status & TWI_MASTER_RIF_bm) != 0 ) {
		xprintf_P( PSTR("RIF\r\n\0"));
		TWIE.MASTER.STATUS = bus_status | TWI_MASTER_RIF_bm;
	}

	if ( (bus_status & TWI_MASTER_CLKHOLD_bm) != 0 ) {
		xprintf_P( PSTR("CLKHOLD\r\n\0"));
//		TWIE.MASTER.STATUS = bus_status | TWI_MASTER_CLKHOLD_bm;
	}

	if ( (bus_status & TWI_MASTER_RXACK_bm) != 0 ) {
		xprintf_P( PSTR("RXACK\r\n\0"));
//		TWIE.MASTER.STATUS = bus_status | TWI_MASTER_RXACK_bm;

		TWIE.MASTER.CTRLA = 0x00;

		vTaskDelay( ( TickType_t)( 10 / portTICK_RATE_MS ) );


		for ( i = 0; i < 10; i++ ) {
			IO_set_SCL();
			vTaskDelay( ( TickType_t)( 1 / portTICK_RATE_MS ) );
			IO_clr_SCL();
			vTaskDelay( ( TickType_t)( 1 / portTICK_RATE_MS ) );
		}


		TWIE.MASTER.CTRLA |= ( 1<<TWI_MASTER_ENABLE_bp);	// Enable TWI


	}

//	TWIE.MASTER.STATUS = bus_status | TWI_MASTER_BUSSTATE_IDLE_gc;	// Pongo el status en 01 ( idle )
//	vTaskDelay( ( TickType_t)( 100 / portTICK_RATE_MS ) );
*/


}
//------------------------------------------------------------------------------------
static void pv_cmd_rwGPRS(uint8_t cmd_mode )
{

uint8_t pin;
//char *p;

	if ( cmd_mode == WR_CMD ) {

		// write gprs (pwr|sw|rts|dtr) {on|off}

		if (!strcmp_P( strupr(argv[2]), PSTR("PWR\0")) ) {
			if (!strcmp_P( strupr(argv[3]), PSTR("ON\0")) ) {
				IO_set_GPRS_PWR(); pv_snprintfP_OK(); return;
			}
			if (!strcmp_P( strupr(argv[3]), PSTR("OFF\0")) ) {
				IO_clr_GPRS_PWR(); pv_snprintfP_OK(); return;
			}
			pv_snprintfP_ERR();
			return;
		}

		if (!strcmp_P( strupr(argv[2]), PSTR("SW\0")) ) {
			if (!strcmp_P( strupr(argv[3]), PSTR("ON\0")) ) {
				IO_set_GPRS_SW();
				pv_snprintfP_OK(); return;
			}
			if (!strcmp_P( strupr(argv[3]), PSTR("OFF\0")) ) {
				IO_clr_GPRS_SW(); pv_snprintfP_OK(); return;
			}
			pv_snprintfP_ERR();
			return;
		}

		if (!strcmp_P( strupr(argv[2]), PSTR("CTS\0")) ) {
			if (!strcmp_P( strupr(argv[3]), PSTR("ON\0")) ) {
				IO_set_GPRS_CTS(); pv_snprintfP_OK(); return;
			}
			if (!strcmp_P( strupr(argv[3]), PSTR("OFF\0")) ) {
				IO_clr_GPRS_CTS(); pv_snprintfP_OK(); return;
			}
			pv_snprintfP_ERR();
			return;
		}

		// Por ahora cableo DTR a CTS.

		if (!strcmp_P( strupr(argv[2]), PSTR("DTR\0")) ) {
			if (!strcmp_P( strupr(argv[3]), PSTR("ON\0")) ) {
				IO_set_GPRS_CTS(); pv_snprintfP_OK(); return;
			}
			if (!strcmp_P( strupr(argv[3]), PSTR("OFF\0")) ) {
				IO_clr_GPRS_CTS(); pv_snprintfP_OK(); return;
			}
			pv_snprintfP_ERR();
			return;
		}

		// write gprs redial
		if (!strcmp_P( strupr(argv[2]), PSTR("REDIAL\0")) ) {
			u_gprs_redial();
			return;
		}
		// ATCMD
		// // write gprs cmd {atcmd}
		if (!strcmp_P(strupr(argv[2]), PSTR("CMD\0"))) {
			xprintf_P( PSTR("%s\r\0"),argv[3] );

			u_gprs_flush_RX_buffer();
			xCom_printf_P( fdGPRS,PSTR("%s\r\0"),argv[3] );

			xprintf_P( PSTR("sent->%s\r\n\0"),argv[3] );
			return;
		}

		return;
	}

	if ( cmd_mode == RD_CMD ) {
		// read gprs (rsp,cts,dcd,ri)

			// ATCMD
			// read gprs rsp
			if (!strcmp_P(strupr(argv[2]), PSTR("RSP\0"))) {
				u_gprs_print_RX_Buffer();
				//p = pub_gprs_rxbuffer_getPtr();
				//xprintf_P( PSTR("rx->%s\r\n\0"),p );
				return;
			}

			// DCD
			if (!strcmp_P( strupr(argv[2]), PSTR("DCD\0")) ) {
				pin = IO_read_DCD();
				xprintf_P( PSTR("DCD=%d\r\n\0"),pin);
				pv_snprintfP_OK();
				return;
			}

			// RI
			if (!strcmp_P( strupr(argv[2]), PSTR("RI\0")) ) {
				pin = IO_read_RI();
				xprintf_P( PSTR("RI=%d\r\n\0"),pin);
				pv_snprintfP_OK();
				return;
			}

			// RTS
			if (!strcmp_P( strupr(argv[2]), PSTR("RTS\0")) ) {
				pin = IO_read_RTS();
				xprintf_P( PSTR("RTS=%d\r\n\0"),pin);
				pv_snprintfP_OK();
				return;
			}


			pv_snprintfP_ERR();
			return;
	}

}
//------------------------------------------------------------------------------------
