/*
 * sp5K_tkCmd.c
 *
 *  Created on: 27/12/2013
 *      Author: root
 */

#include "spx.h"
#include "tkComms.h"
#include "SPX_tasks/spx_tkApp/tkApp.h"

//----------------------------------------------------------------------------------------
// FUNCIONES DE USO PRIVADO
//----------------------------------------------------------------------------------------
static void pv_snprintfP_OK(void );
static void pv_snprintfP_ERR(void);
static void pv_cmd_read_fuses(void);
static void pv_cmd_read_memory(void);
static void pv_cmd_rwGPRS(uint8_t cmd_mode );
static void pv_cmd_rwMCP(uint8_t cmd_mode );
static void pv_cmd_I2Cscan(bool busscan);
static bool pv_cmd_configGPRS(void);

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
static void cmdPeekFunction(void);
static void cmdPokeFunction(void);

#define WR_CMD 0
#define RD_CMD 1

#define WDG_CMD_TIMEOUT	WDG_TO120

static usuario_t tipo_usuario;
RtcTimeType_t rtc;

//------------------------------------------------------------------------------------
void tkCmd(void * pvParameters)
{

uint8_t c = 0;
uint8_t ticks = 0;

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
	FRTOS_CMD_register( "peek\0", cmdPeekFunction );
	FRTOS_CMD_register( "poke\0", cmdPokeFunction );

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

		// Si no tengo terminal conectada, duermo 25s lo que me permite entrar en tickless.
		while ( ! ctl_terminal_connected() ) {
			ctl_watchdog_kick(WDG_CMD, WDG_CMD_TIMEOUT);
			vTaskDelay( ( TickType_t)( 25000 / portTICK_RATE_MS ) );
		}

		c = '\0';	// Lo borro para que luego del un CR no resetee siempre el timer.
		// el read se bloquea 50ms. lo que genera la espera.
		//while ( CMD_read( (char *)&c, 1 ) == 1 ) {
		while ( frtos_read( fdTERM, (char *)&c, 1 ) == 1 ) {
			FRTOS_CMD_process(c);
		}

	}
}
//------------------------------------------------------------------------------------
static void cmdStatusFunction(void)
{

FAT_t l_fat;
uint8_t channel = 0;
st_dataRecord_t dr;

	FRTOS_CMD_makeArgv();

	memset( &l_fat, '\0', sizeof(FAT_t));

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
	xprintf_P( PSTR("dlgid: %s\r\n\0"), sVarsComms.dlgId );

	// Memoria
	FAT_read(&l_fat);
	xprintf_P( PSTR("memory: rcdSize=%d, wrPtr=%d,rdPtr=%d,delPtr=%d,r4wr=%d,r4rd=%d,r4del=%d \r\n\0"), sizeof(st_dataRecord_t), l_fat.wrPTR,l_fat.rdPTR, l_fat.delPTR,l_fat.rcds4wr,l_fat.rcds4rd,l_fat.rcds4del );

	// SERVER
	//	xprintf_P( PSTR(">Server:\r\n\0"));

	// COMMS Status
	xCOMMS_status();

	// MODO DE OPERACION:
	xprintf_P( PSTR(">Aplicacion:\r\n\0"));
	switch (sVarsApp.aplicacion ) {
	case APP_OFF:
		xprintf_P( PSTR("  modo: OFF\r\n\0"));
		break;
	case APP_CONSIGNA:
		if ( sVarsApp.consigna.c_aplicada == CONSIGNA_DIURNA ) {
			xprintf_P( PSTR("  modo: Consignas: (c_dia=%02d:%02d, c_noche=%02d:%02d) (DIURNA)\r\n"), sVarsApp.consigna.hhmm_c_diurna.hour, sVarsApp.consigna.hhmm_c_diurna.min, sVarsApp.consigna.hhmm_c_nocturna.hour, sVarsApp.consigna.hhmm_c_nocturna.min );
		} else {
			xprintf_P( PSTR("  modo: Consignas: (c_dia=%02d:%02d, c_noche=%02d:%02d) (NOCTURNA)\r\n"), sVarsApp.consigna.hhmm_c_diurna.hour, sVarsApp.consigna.hhmm_c_diurna.min, sVarsApp.consigna.hhmm_c_nocturna.hour, sVarsApp.consigna.hhmm_c_nocturna.min );
		}
		break;
	case APP_PERFORACION:
		xAPP_perforacion_print_status();
		break;
	case APP_PLANTAPOT:
		xAPP_plantapot_print_status(false);
		break;
	case APP_CAUDALIMETRO:
		xprintf_P( PSTR("  modo: Caudalimetro: (Qinst:%0.2f, pWidth:%02d, period=%02d:%02d)\r\n"), sVarsApp.caudalimetro.range_actual, sVarsApp.consigna.hhmm_c_diurna.min, sVarsApp.caudalimetro.pulse_width, sVarsApp.caudalimetro.periodo_actual );

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
	case DEBUG_COMMS:
		xprintf_P( PSTR("  debug: comms\r\n\0") );
		break;
	case DEBUG_APLICACION:
		xprintf_P( PSTR("  debug: aplicacion\r\n\0") );
		break;
	default:
		xprintf_P( PSTR("  debug: ???\r\n\0") );
		break;
	}

	// Timerpoll
	xprintf_P( PSTR("  timerPoll: [%d s]/ %d\r\n\0"), systemVars.timerPoll, ctl_readTimeToNextPoll() );

	// Timerdial
	xprintf_P( PSTR("  timerDial: [%d s]/\0"), sVarsComms.timerDial );
	xprintf_P( PSTR(" %d \r\n\0"), xcomms_time_to_next_dial() );

	// Sensor Pwr Time
	xprintf_P( PSTR("  timerPwrSensor: [%d s]\r\n\0"), systemVars.ainputs_conf.pwr_settle_time );

//	if ( ( spx_io_board == SPX_IO5CH ) || (strcmp_P( strupr(argv[1]), PSTR("ALL\0")) == 0 ) ) {
	if ( spx_io_board == SPX_IO5CH ) {

		// PWR SAVE:
		if ( sVarsComms.pwrSave.pwrs_enabled ==  false ) {
			xprintf_P(  PSTR("  pwrsave: OFF\r\n\0"));
		} else {
			xprintf_P(  PSTR("  pwrsave: ON, start[%02d:%02d], end[%02d:%02d]\r\n\0"), sVarsComms.pwrSave.hora_start.hour, sVarsComms.pwrSave.hora_start.min, sVarsComms.pwrSave.hora_fin.hour, sVarsComms.pwrSave.hora_fin.min);
		}

		// RangeMeter: PULSE WIDTH
		xprintf_P( PSTR("  range: %s\r\n\0"), systemVars.range_name );

		// Psensor
		if ( strcmp ( systemVars.psensor_conf.name, "X" ) == 0 ) {
			xprintf_P( PSTR("  psensor: X\r\n\0"));
		} else {
			xprintf_P( PSTR("  psensor: %s (%d-%d / %.01f-%.01f)[offset=%0.02f]\r\n\0"), systemVars.psensor_conf.name, systemVars.psensor_conf.count_min, systemVars.psensor_conf.count_max, systemVars.psensor_conf.pmin, systemVars.psensor_conf.pmax, systemVars.psensor_conf.offset );
		}
	}

	// contadores( Solo hay 2 )
	switch ( systemVars.counters_conf.hw_type ) {
	case COUNTERS_HW_SIMPLE:
		xprintf_P( PSTR("  counters hw: simple\r\n\0"));
		break;
	case COUNTERS_HW_OPTO:
		xprintf_P( PSTR("  counters hw: opto\r\n\0"));
		break;
	}

	// aninputs
	for ( channel = 0; channel < NRO_ANINPUTS; channel++) {
		xprintf_P( PSTR("  a%d [%d-%d mA/ %.02f,%.02f | %.02f | %.03f | %.03f | %s]"),
				channel,
				systemVars.ainputs_conf.imin[channel],
				systemVars.ainputs_conf.imax[channel],
				systemVars.ainputs_conf.mmin[channel],
				systemVars.ainputs_conf.mmax[channel],
				systemVars.ainputs_conf.offset[channel] ,
				systemVars.ainputs_conf.ieq_min[channel] ,
				systemVars.ainputs_conf.ieq_max[channel],
				systemVars.ainputs_conf.name[channel] );

		if ( ( systemVars.an_calibrados & ( 1<<channel)) == 0 ) {
			xprintf_P(PSTR("\r\n"));
		} else {
			xprintf_P(PSTR("(*)\r\n"));
		}

	}

	// dinputs
	for ( channel = 0; channel <  NRO_DINPUTS; channel++) {
		if ( systemVars.dinputs_conf.wrk_modo[channel] == DIN_NORMAL ) {
			xprintf_P( PSTR("  d%d (N),%s \r\n\0"),channel, systemVars.dinputs_conf.name[channel]);
		} else {
			xprintf_P( PSTR("  d%d (T),%s \r\n\0"),channel, systemVars.dinputs_conf.name[channel]);
		}
	}

	for ( channel = 0; channel <  NRO_COUNTERS; channel++) {
		xprintf_P( PSTR("  c%d [%s,magpp=%.03f,pw=%d,T=%d\0"),channel,systemVars.counters_conf.name[channel], systemVars.counters_conf.magpp[channel], systemVars.counters_conf.pwidth[channel], systemVars.counters_conf.period[channel] );
		if ( systemVars.counters_conf.speed[channel] == CNT_LOW_SPEED ) {
			xprintf_P(PSTR(" (LS)\0"));
		} else {
			xprintf_P(PSTR(" (HS)\0"));
		}

		if ( systemVars.counters_conf.sensing_edge[channel] == RISING_EDGE ) {
			xprintf_P(PSTR("(RISE)\r\n\0"));
		} else {
			xprintf_P(PSTR("(FALL)\r\n\0"));
		}
	}

	// Muestro los datos
	// CONFIG
	xprintf_P( PSTR(">Frame:\r\n\0"));
	data_read_inputs(&dr, true );
	data_print_inputs(fdTERM, &dr);
}
//-----------------------------------------------------------------------------------
static void cmdResetFunction(void)
{
	// Resetea al micro prendiendo el watchdog

	FRTOS_CMD_makeArgv();

	// Reset memory ??
	if ( strcmp_P( strupr(argv[1]), PSTR("MEMORY\0"))  == 0) {

		// Nadie debe usar la memoria !!!
		ctl_watchdog_kick(WDG_CMD, 0x8000 );

		vTaskSuspend( xHandle_tkAplicacion );
		ctl_watchdog_kick(WDG_APP, 0x8000 );

		vTaskSuspend( xHandle_tkInputs );
		ctl_watchdog_kick(WDG_DINPUTS, 0x8000 );

		vTaskSuspend( xHandle_tkComms );
		ctl_watchdog_kick(WDG_COMMS, 0x8000 );

		if (strcmp_P( strupr(argv[2]), PSTR("SOFT\0")) == 0) {
			FF_format(false );
		} else if (strcmp_P( strupr(argv[2]), PSTR("HARD\0")) == 0) {
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

uint8_t pin = 0;
int8_t state = -1;
char l_data[10] = { '\0' };

//t_sms *sms_boundle;

	FRTOS_CMD_makeArgv();


	// TEST
	/*
	if ( strcmp_P( strupr(argv[1]), PSTR("TEST\0")) == 0)  {
		if ( strcmp_P( strupr(argv[2]), PSTR("ENQUEUE\0")) == 0)  {
			xSMS_enqueue( argv[3], argv[4] );
			pv_snprintfP_OK();
			return;
		}
		if ( strcmp_P( strupr(argv[2]), PSTR("DEQUEUE\0")) == 0)  {
			sms_boundle = xSMS_dequeue();
			if ( sms_boundle != NULL ) {
				xprintf_P(PSTR("deq nro=[%s]\r\n\0"), sms_boundle->nro);
				xprintf_P(PSTR("deq msg=[%s]\r\n\0"),sms_boundle->msg);
				pv_snprintfP_OK();
				return;
			} else {
				pv_snprintfP_ERR();
				return;
			}
		}
		if ( strcmp_P( strupr(argv[2]), PSTR("DELETE\0")) == 0)  {
			xSMS_delete();
			return;
		}
	}
*/


	// GPRS
	// write gprs pwr|sw|rts {on|off}
	// write gprs cmd {atcmd}
	// write gprs sms nbr msg
	// write gprs qsms nbr msg
	if ( ( strcmp_P( strupr(argv[1]), PSTR("GPRS\0")) == 0) && ( tipo_usuario == USER_TECNICO) ) {
		pv_cmd_rwGPRS(WR_CMD);
		return;
	}

	// APPALARMA
	// write appalarma (prender/apagar) (lroja,lverde,lamarilla,lnaranja, sirena)
	if ( ( strcmp_P( strupr(argv[1]), PSTR("APPALARMA\0")) == 0) && ( tipo_usuario == USER_TECNICO) ) {
		xAPP_plantapot_servicio_tecnico( argv[2], argv[3]);
		return;
	}

	// OUTPIN
	// write outpin {0..7} {set | clear}
	if (( strcmp_P( strupr(argv[1]), PSTR("OUTPIN\0")) == 0) && ( tipo_usuario == USER_TECNICO) ) {
		memcpy(l_data, argv[3], sizeof(l_data));
		strupr(l_data);
		pin = atoi(argv[2]);

		if (strcmp_P( l_data, PSTR("SET")) == 0 ) {
			state = 1;
		}
		if (strcmp_P( l_data, PSTR("CLEAR")) == 0) {
			state = 0;
		}

		u_write_output_pins( pin,state ) ?  pv_snprintfP_OK() : pv_snprintfP_ERR();
		return;
	}

	// OUTPUTS
	// outputs (val dec.)
	if ( (strcmp_P( strupr(argv[1]), PSTR("DOUT\0")) == 0) && ( tipo_usuario == USER_TECNICO) ) {
		xAPP_set_douts( atoi(argv[2]), MASK_NORMAL );
		pv_snprintfP_OK();
		return;
	}

	// CONSIGNA
	// write consigna (diurna|nocturna)
	//		          (enable|disable),(set|reset),(sleep|awake),(ph01|ph10) } {A/B}
	//		          (open|close) (A|B)
	//		          pulse (A|B) (secs)
	//		          power {on|off}
	if ((strcmp_P( strupr(argv[1]), PSTR("CONSIGNA\0")) == 0 ) && ( tipo_usuario == USER_TECNICO) ) {
		xAPP_consigna_write( argv[2],argv[3],argv[4] ) ?  pv_snprintfP_OK() : 	pv_snprintfP_ERR();
		return;
	}

	// ANALOG
	// write analog wakeup/sleep
	if ((strcmp_P( strupr(argv[1]), PSTR("ANALOG\0")) == 0) && ( tipo_usuario == USER_TECNICO) ) {
		if ( strcmp_P( strupr(argv[2]), PSTR("WAKEUP\0")) == 0 ) {
			ainputs_awake();
			return;
		}
		if (strcmp_P( strupr(argv[2]), PSTR(" SLEEP\0")) == 0 ) {
			ainputs_sleep();
			return;
		}
		return;
	}

	// MCP
	// write mcp regAddr data
	if ( (strcmp_P( strupr(argv[1]), PSTR("MCP\0")) == 0) && ( tipo_usuario == USER_TECNICO) ) {
		pv_cmd_rwMCP(WR_CMD);
		return;
	}

	// RTC
	// write rtc YYMMDDhhmm
	if ( strcmp_P( strupr(argv[1]), PSTR("RTC\0")) == 0 ) {
		( RTC_write_time( argv[2]) > 0)?  pv_snprintfP_OK() : 	pv_snprintfP_ERR();
		return;
	}

	// EE
	// write ee pos string
	if ((strcmp_P( strupr(argv[1]), PSTR("EE\0")) == 0) && ( tipo_usuario == USER_TECNICO) ) {
		( EE_test_write ( argv[2], argv[3] ) > 0)?  pv_snprintfP_OK() : 	pv_snprintfP_ERR();
		return;
	}

	// NVMEE
	// write nvmee pos string
	if ( (strcmp_P( strupr(argv[1]), PSTR("NVMEE\0")) == 0) && ( tipo_usuario == USER_TECNICO) ) {
		NVMEE_test_write ( argv[2], argv[3] );
		pv_snprintfP_OK();
		return;
	}

	// RTC SRAM
	// write rtcram pos string
	if ( (strcmp_P( strupr(argv[1]), PSTR("RTCRAM\0")) == 0)  && ( tipo_usuario == USER_TECNICO) ) {
		( RTCSRAM_test_write ( argv[2], argv[3] ) > 0)?  pv_snprintfP_OK() : 	pv_snprintfP_ERR();
		return;
	}

	// SENS12V
	// write sens12V {on|off}
	if ( (strcmp_P( strupr(argv[1]), PSTR("SENS12V\0")) == 0) && ( tipo_usuario == USER_TECNICO) ) {

		if ( strcmp_P( strupr(argv[2]), PSTR("ON\0")) == 0 ) {
			IO_set_SENS_12V_CTL();
			pv_snprintfP_OK();
		} else if  ( strcmp_P( strupr(argv[2]), PSTR("OFF\0")) == 0 ) {
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
	if ((strcmp_P( strupr(argv[1]), PSTR("INA\0")) == 0) && ( tipo_usuario == USER_TECNICO) ) {
		( INA_test_write ( argv[2], argv[3] ) > 0)?  pv_snprintfP_OK() : 	pv_snprintfP_ERR();
		return;
	}

	// RANGE
	// write range {run | stop}
	if ( (strcmp_P( strupr(argv[1]), PSTR("RANGE\0")) == 0) && ( tipo_usuario == USER_TECNICO) ) {
		if ( strcmp_P( strupr(argv[2]), PSTR("RUN\0")) == 0 ) {
			RMETER_start(); pv_snprintfP_OK(); return;
		}

		if ( strcmp_P( strupr(argv[2]), PSTR("STOP\0")) == 0) {
			RMETER_stop(); pv_snprintfP_OK();	return;
		}

		xprintf_P( PSTR("cmd ERROR: ( write range {run|stop} )\r\n\0"));
		return;
	}

	// CMD NOT FOUND
	xprintf_P( PSTR("ERROR\r\nCMD NOT DEFINED\r\n\0"));
	return;
}
//------------------------------------------------------------------------------------
static void cmdReadFunction(void)
{

st_dataRecord_t dr;
uint8_t cks;

	FRTOS_CMD_makeArgv();

	// ALMTEST
	// read almtest
	if (!strcmp_P( strupr(argv[1]), PSTR("ALMTEST\0")) && ( tipo_usuario == USER_TECNICO) ) {
		xAPP_plantapot_test();
		 return;
	}

	// READ HASH
/*	if (!strcmp_P( strupr(argv[1]), PSTR("HASH\0"))) {
		u_hash_test();
		return;
	}
*/

	// HASHES
	// read hashes
	if (!strcmp_P( strupr(argv[1]), PSTR("HASHES\0")) && ( tipo_usuario == USER_TECNICO) ) {
		cks = u_base_hash();
		xprintf_P( PSTR("Base hash = [0x%02x]\r\n\0"), cks );
		cks = ainputs_hash();
		xprintf_P( PSTR("Analog hash = [0x%02x]\r\n\0"), cks );
		cks = dinputs_hash();
		xprintf_P( PSTR("Digital hash = [0x%02x]\r\n\0"), cks );
		cks = counters_hash();
		xprintf_P( PSTR("Counters hash = [0x%02x]\r\n\0"), cks );
		cks = psensor_hash();
		xprintf_P( PSTR("Pensor hash = [0x%02x]\r\n\0"), cks );
		cks = range_hash();
		xprintf_P( PSTR("Range hash = [0x%02x]\r\n\0"), cks );
		cks = u_aplicacion_hash();
		xprintf_P( PSTR("App hash = [0x%02x]\r\n\0"), cks );
		return;
	}


	if (!strcmp_P( strupr(argv[1]), PSTR("TEST\0")) && ( tipo_usuario == USER_TECNICO) ) {
		//xprintf_P( PSTR("PLOAD=CLASS:BASE;TDIAL:%d;TPOLL:%d;PWRS_MODO:ON;PWRS_START:0630;PWRS_END:1230\r\r\0"), sVarsComms.timerDial,systemVars.timerPoll );
		/*
		xprintf_P( PSTR("st_io5_t = %d\r\n\0"), sizeof(st_io5_t) );
		xprintf_P( PSTR("st_io8_t = %d\r\n\0"), sizeof(st_io8_t) );
		xprintf_P( PSTR("u_dataframe_t = %d\r\n\0"), sizeof(u_dataframe_t) );
		xprintf_P( PSTR("st_dataRecord_t = %d\r\n\0"), sizeof(st_dataRecord_t) );
		xprintf_P( PSTR("dataframe_s = %d\r\n\0"), sizeof(dataframe_s) );
		xprintf_P( PSTR("RtcTimeType_t = %d\r\n\0"), sizeof(RtcTimeType_t) );
		*/

		//gprs_init_test();

		psensor_test_read();
		tempsensor_test_read();
		return;
	}


	if (!strcmp_P( strupr(argv[1]), PSTR("TEMP\0")) && ( tipo_usuario == USER_TECNICO) ) {
		tempsensor_test_read();
		return;
	}

	if (!strcmp_P( strupr(argv[1]), PSTR("PSENS\0")) && ( tipo_usuario == USER_TECNICO) ) {
		psensor_test_read();
		return;
	}

	// I2Cscan
	// read i2cscan busaddr
	if (!strcmp_P( strupr(argv[1]), PSTR("I2CSCAN\0")) && ( tipo_usuario == USER_TECNICO) ) {
		pv_cmd_I2Cscan(false);
		return;
	}

	// I2Cscanbus
	// read i2cscanbus
	if (!strcmp_P( strupr(argv[1]), PSTR("I2CSCANBUS\0")) && ( tipo_usuario == USER_TECNICO) ) {
		pv_cmd_I2Cscan(true);
		return;
	}

	// MCP
	// read mcp regAddr
	if (!strcmp_P( strupr(argv[1]), PSTR("MCP\0")) && ( tipo_usuario == USER_TECNICO) ) {
		pv_cmd_rwMCP(RD_CMD);
		return;
	}

	// FUSES
 	if (!strcmp_P( strupr(argv[1]), PSTR("FUSES\0"))) {
 		pv_cmd_read_fuses();
 		return;
 	}

 	// FULLSTACK
 	if (!strcmp_P( strupr(argv[1]), PSTR("FULLSTACK\0")) && ( tipo_usuario == USER_TECNICO) ) {
 		debug_full_print_stack_watermarks();
 		return;
 	}

	// STACK
 	if (!strcmp_P( strupr(argv[1]), PSTR("STACK\0")) && ( tipo_usuario == USER_TECNICO) ) {
 		debug_print_stack_watermarks(" ");
 		return;
 	}

	// FREERAM
 	if (!strcmp_P( strupr(argv[1]), PSTR("FREERAM\0")) && ( tipo_usuario == USER_TECNICO) ) {
 		xprintf_P(PSTR("Free Ram = %d\r\n\0"), debug_freeRam() );
 		return;
 	}

 	// WDT
 	if (!strcmp_P( strupr(argv[1]), PSTR("WDT\0")) && ( tipo_usuario == USER_TECNICO) ) {
 		debug_print_wdg_timers();
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
		ainputs_awake();
		INA_test_read ( argv[2], argv[3] );
		ainputs_sleep();
		return;
	}

	// FRAME
	// read frame
	if (!strcmp_P( strupr(argv[1]), PSTR("FRAME\0")) ) {
		data_read_inputs(&dr, false );
		data_print_inputs(fdTERM, &dr);
		return;
	}

	// MEMORY
	// read memory
	if (!strcmp_P( strupr(argv[1]), PSTR("MEMORY\0")) && ( tipo_usuario == USER_TECNICO) ) {
		pv_cmd_read_memory();
		return;
	}

	// DINPUTS
	// read dinputs
	if (!strcmp_P( strupr(argv[1]), PSTR("DINPUTS\0")) && ( tipo_usuario == USER_TECNICO) ) {
		dinputs_service_read();
		return;
	}

	// GPRS
	// read gprs (rsp,cts,dcd,ri, sms)
	if (!strcmp_P( strupr(argv[1]), PSTR("GPRS\0")) && ( tipo_usuario == USER_TECNICO) ) {
		pv_cmd_rwGPRS(RD_CMD);
		return;
	}

	// ACH {n}
	// read ach n
	if (!strcmp_P( strupr(argv[1]), PSTR("ACH\0")) && ( tipo_usuario == USER_TECNICO) ) {
		ainputs_test_channel( atoi(argv[2]));
		return;
	}

	// battery
	// read battery
	if (!strcmp_P( strupr(argv[1]), PSTR("BATTERY\0")) && ( tipo_usuario == USER_TECNICO) ) {
		ainputs_test_channel(99);
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

	// GPRS SAT
	// config gprs SAT {enable|disable}
	if (!strcmp_P( strupr(argv[1]), PSTR("GPRS\0")) ) {
		retS =  pv_cmd_configGPRS();
		retS ? pv_snprintfP_OK() : pv_snprintfP_ERR();
		return;
	}

	// APPALARM
	// config appalarma
	//                  sms {id} {nro} {almlevel}\r\n\0"));
	//                  nivel {chid} {alerta} {inf|sup} val\r\n\0"));
	if (!strcmp_P( strupr(argv[1]), PSTR("APPALARMA\0")) ) {
		retS = xAPP_plantapot_config( argv[2], argv[3], argv[4], argv[5],argv[6] );
		retS ? pv_snprintfP_OK() : pv_snprintfP_ERR();
		return;
	}

	// APLICACION
	// aplicacion { off,consigna,perforacion,piloto, tanque, alarmas }
	if (!strcmp_P( strupr(argv[1]), PSTR("APLICACION\0")) ) {
		retS = u_config_aplicacion( argv[2] );
		retS ? pv_snprintfP_OK() : pv_snprintfP_ERR();
		return;
	}

	// CONSIGNA
	// config consigna {hhmm1} {hhmm2}
	if (!strcmp_P( strupr(argv[1]), PSTR("CONSIGNA\0")) ) {
		retS = xAPP_consigna_config( argv[2], argv[3]);
		retS ? pv_snprintfP_OK() : pv_snprintfP_ERR();
		return;
	}

	// CAUDALIMETRO
	// config caudal {pwidth} {factorQ}
	if (!strcmp_P( strupr(argv[1]), PSTR("CAUDAL\0")) ) {
		retS = xAPP_caudalimetro_config( argv[2], argv[3]);
		retS ? pv_snprintfP_OK() : pv_snprintfP_ERR();
		return;
	}

	// COUNTERS
	// config counter {0..1} cname magPP pulseWidth period speed sensing
	// counter hw {SIMPLE/OPTO)
	if ( strcmp_P( strupr(argv[1]), PSTR("COUNTER\0")) == 0 ) {

		if (strcmp_P( strupr(argv[2]), PSTR("HW\0")) == 0 ) {
			retS = counters_config_hw( argv[3]);
		} else {
			retS = counters_config_channel( atoi(argv[2]), argv[3], argv[4], argv[5], argv[6], argv[7], argv[8] );
		}

		retS ? pv_snprintfP_OK() : pv_snprintfP_ERR();
		return;
	}


	// DEFAULT
	// config default
	if (!strcmp_P( strupr(argv[1]), PSTR("DEFAULT\0"))) {
		u_load_defaults( strupr(argv[2]) );
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

	// Parametros ENTRADAS DIGITALES------------------------------------------------------------------------
	// DIGITAL
	// config digital {0..N} dname {timer}
	if (!strcmp_P( strupr(argv[1]), PSTR("DIGITAL\0")) ) {
		dinputs_config_channel( atoi(argv[2]), argv[3], argv[4]) ? pv_snprintfP_OK() : pv_snprintfP_ERR();
		return;
	}

	// Parametros ENTRADAS ANALOGICAS------------------------------------------------------------------------
	// AUTOCAL
	// config autocal {ch,PSENSOR} {mag}
	if (!strcmp_P( strupr(argv[1]), PSTR("AUTOCAL\0")) ) {
		if (!strcmp_P( strupr(argv[2]), PSTR("PSENSOR\0")) ) {
			psensor_config_autocalibrar( argv[3] ) ? pv_snprintfP_OK() : pv_snprintfP_ERR();
			return;
		}
		ainputs_config_autocalibrar( argv[2], argv[3] ) ? pv_snprintfP_OK() : pv_snprintfP_ERR();
		return;
	}

	// ICAL
	// config ical {ch} {4|20}
	if (!strcmp_P( strupr(argv[1]), PSTR("ICAL\0")) ) {
		ainputs_config_ical( argv[2], argv[3] ) ? pv_snprintfP_OK() : pv_snprintfP_ERR();
		return;
	}

	// MCAL
	// config mcal {ch} {p1|p2} {mag}
	if (!strcmp_P( strupr(argv[1]), PSTR("MCAL\0")) ) {
		ainputs_config_mcal( argv[2], argv[3], argv[4] ) ? pv_snprintfP_OK() : pv_snprintfP_ERR();
		return;
	}

	// ANALOG
	// config analog {0..n} aname imin imax mmin mmax offset
	if (!strcmp_P( strupr(argv[1]), PSTR("ANALOG\0")) ) {
		ainputs_config_channel( atoi(argv[2]), argv[3], argv[4], argv[5], argv[6], argv[7], argv[8] ) ? pv_snprintfP_OK() : pv_snprintfP_ERR();
		return;
	}

	// Parametros COMUNICACIONES ---------------------------------------------------------------------------------
	// APN
	if (!strcmp_P( strupr(argv[1]), PSTR("APN\0"))) {
		if ( argv[2] == NULL ) {
			retS = false;
		} else {
			memset(sVarsComms.apn, '\0', sizeof(sVarsComms.apn));
			memcpy(sVarsComms.apn, argv[2], sizeof(sVarsComms.apn));
			sVarsComms.apn[APN_LENGTH - 1] = '\0';
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
			memset(sVarsComms.server_tcp_port, '\0', sizeof(sVarsComms.server_tcp_port));
			memcpy(sVarsComms.server_tcp_port, argv[2], sizeof(sVarsComms.server_tcp_port));
			sVarsComms.server_tcp_port[PORT_LENGTH - 1] = '\0';
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
			memset(sVarsComms.server_ip_address, '\0', sizeof(sVarsComms.server_ip_address));
			memcpy(sVarsComms.server_ip_address, argv[2], sizeof(sVarsComms.server_ip_address));
			sVarsComms.server_ip_address[IP_LENGTH - 1] = '\0';
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
			memset(sVarsComms.serverScript, '\0', sizeof(sVarsComms.serverScript));
			memcpy(sVarsComms.serverScript, argv[2], sizeof(sVarsComms.serverScript));
			sVarsComms.serverScript[SCRIPT_LENGTH - 1] = '\0';
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
			memset(sVarsComms.simpwd, '\0', sizeof(sVarsComms.simpwd));
			memcpy(sVarsComms.simpwd, argv[2], sizeof(sVarsComms.simpwd));
			sVarsComms.simpwd[SIM_PASSWD_LENGTH - 1] = '\0';
			retS = true;
		}
		retS ? pv_snprintfP_OK() : 	pv_snprintfP_ERR();
		return;
	}

	// Parametros BASE --------------------------------------------------------------------------------------------
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
		} else if (!strcmp_P( strupr(argv[2]), PSTR("COMMS\0"))) {
			systemVars.debug = DEBUG_COMMS;
			retS = true;
		} else if (!strcmp_P( strupr(argv[2]), PSTR("APLICACION\0"))) {
			systemVars.debug = DEBUG_APLICACION;
			retS = true;
		} else {
			retS = false;
		}
		retS ? pv_snprintfP_OK() : 	pv_snprintfP_ERR();
		return;
	}

	// PSENSOR
	// config psensor name countMin countMax pmin pmax offset
	if (!strcmp_P( strupr(argv[1]), PSTR("PSENSOR\0")) ) {
		psensor_config( argv[2], argv[3], argv[4], argv[5], argv[6], argv[7] ) ? pv_snprintfP_OK() : pv_snprintfP_ERR();
		return;
	}

	// RANGEMETER
	// config rangemeter {name}
	if ( !strcmp_P( strupr(argv[1]), PSTR("RANGEMETER\0"))) {

		if ( range_config(argv[2]) ) {
			pv_snprintfP_OK();
			return;
		} else {
			pv_snprintfP_ERR();
			return;
		}

		pv_snprintfP_ERR();
		return;
	}

	// TIMEPWRSENSOR
	// config timepwrsensor
	if (!strcmp_P( strupr(argv[1]), PSTR("TIMEPWRSENSOR\0")) ) {
		ainputs_config_timepwrsensor( argv[2] );
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

	// TIMERDIAL
	// config timerdial
	if ( !strcmp_P( strupr(argv[1]), PSTR("TIMERDIAL\0"))) {
		u_config_timerdial( argv[2] );
		pv_snprintfP_OK();
		return;
	}

	// PWRSAVE
	if (!strcmp_P( strupr(argv[1]), PSTR("PWRSAVE\0"))) {
		u_configPwrSave ( argv[2], argv[3], argv[4] );
		pv_snprintfP_OK();
		return;
	}

	// DLGID
	if (!strcmp_P( strupr(argv[1]), PSTR("DLGID\0"))) {
		if ( argv[2] == NULL ) {
			retS = false;
			} else {
				memset(sVarsComms.dlgId,'\0', sizeof(sVarsComms.dlgId) );
				memcpy(sVarsComms.dlgId, argv[2], sizeof(sVarsComms.dlgId));
				sVarsComms.dlgId[DLGID_LENGTH - 1] = '\0';
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
			xprintf_P( PSTR("  analog (wakeup | sleep )\r\n\0"));

			if ( spx_io_board == SPX_IO8CH ) {
				xprintf_P( PSTR("  mcp {regAddr} {data}, mcpinit\r\n\0"));
				xprintf_P( PSTR("  outputs (val dec.)\r\n\0"));
				xprintf_P( PSTR("  outpin {0..7} {set | clear}\r\n\0"));
				xprintf_P( PSTR("  appalarma (prender/apagar/flash) (lroja,lverde,lamarilla,lnaranja,lazul,sirena) \r\n\0"));
			}

			if ( spx_io_board == SPX_IO5CH ) {

				if ( strcmp_P( systemVars.range_name, PSTR("X\0")) != 0 ) {
					xprintf_P( PSTR("  range {run | stop}\r\n\0"));
				}

				xprintf_P( PSTR("  consigna (diurna|nocturna)\r\n\0"));
				xprintf_P( PSTR("           (enable|disable),(set|reset),(sleep|awake),(ph01|ph10) } {A/B}\r\n\0"));
				xprintf_P( PSTR("           (open|close) (A|B)\r\n\0"));
				xprintf_P( PSTR("           pulse (A|B) (secs) \r\n\0"));
				xprintf_P( PSTR("           power {on|off}\r\n\0"));
			}

			xprintf_P( PSTR("  gprs (pwr|sw|rts|dtr) {on|off}\r\n\0"));
			xprintf_P( PSTR("       cmd {atcmd}, redial, monsqe\r\n\0"));
			xprintf_P( PSTR("       sms,qsms,fsms {nbr,msg}\r\n\0"));

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
			xprintf_P( PSTR("  i2cscan {busaddr}, i2cscanbus\r\n\0"));
			xprintf_P( PSTR("  temp,psens\r\n\0"));
			xprintf_P( PSTR("  ach {n}, battery\r\n\0"));
			if ( spx_io_board == SPX_IO8CH ) {
				xprintf_P( PSTR("  mcp {regAddr}\r\n\0"));
			}
			xprintf_P( PSTR("  memory {full}\r\n\0"));
			xprintf_P( PSTR("  dinputs\r\n\0"));
			xprintf_P( PSTR("  gprs (rsp,cts,dcd,ri,sms)\r\n\0"));
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
		xprintf_P( PSTR("  comms {GPRS}\r\n\0"));
		xprintf_P( PSTR("  gprs SAT {check|enable|disable}\r\n\0"));
		xprintf_P( PSTR("  pwrsave {on|off} {hhmm1}, {hhmm2}\r\n\0"));
		xprintf_P( PSTR("  timerpoll {val}, timerdial {val}, timepwrsensor {val}\r\n\0"));
		xprintf_P( PSTR("  rangemeter {name}\r\n\0"));
		xprintf_P( PSTR("  psensor name countMin countMax pmin pmax offset\r\n\0"));

		xprintf_P( PSTR("  debug {none,counter,data,comms,aplicacion }\r\n\0"));

		xprintf_P( PSTR("  digital {0..%d} dname {normal,timer}\r\n\0"), ( NRO_DINPUTS - 1 ) );

		xprintf_P( PSTR("  counter {0..%d} cname magPP pw(ms) period(ms) speed(LS/HS) edge(RISE/FALL)\r\n\0"), ( NRO_COUNTERS - 1 ) );
		xprintf_P( PSTR("  counter hw {SIMPLE/OPTO)\r\n\0") );

		xprintf_P( PSTR("  analog {0..%d} aname imin imax mmin mmax offset\r\n\0"),( NRO_ANINPUTS - 1 ) );
		xprintf_P( PSTR("  autocal {ch,PSENSOR} {mag}\r\n\0"));
		xprintf_P( PSTR("  ical {ch} {imin | imax}\r\n\0"));
		xprintf_P( PSTR("  mcal {ch} {p1|p2} {mag}\r\n\0"));

		xprintf_P( PSTR("  aplicacion {off,consigna,perforacion,tanque}\r\n\0"));
		xprintf_P( PSTR("  appalarma sms {id} {nro} {almlevel}\r\n\0"));
		xprintf_P( PSTR("            nivel {chid} {alerta} {inf|sup} val\r\n\0"));
		xprintf_P( PSTR("  piloto reg {CHICA|MEDIA|GRANDE}\r\n\0"));
		xprintf_P( PSTR("         pband {pband}\r\n\0"));
		xprintf_P( PSTR("         steps {steps}\r\n\0"));
		xprintf_P( PSTR("         slot {idx} {hhmm} {pout}\r\n\0"));
		xprintf_P( PSTR("  tanque sms {id} nro\r\n\0"));
		xprintf_P( PSTR("         {nivelB,nivelA} valor\r\n\0"));
		xprintf_P( PSTR("  consigna {hhmm1} {hhmm2}\r\n\0"));
		xprintf_P( PSTR("  caudal {pwidth} {factorQ}\r\n\0"));

		xprintf_P( PSTR("  default {SPY|OSE|UTE|CLARO}\r\n\0"));
		xprintf_P( PSTR("  save\r\n\0"));
	}

	// HELP KILL
	else if (!strcmp_P( strupr(argv[1]), PSTR("KILL\0")) && ( tipo_usuario == USER_TECNICO) ) {
		xprintf_P( PSTR("-kill {data, commstx }\r\n\0"));
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

	// KILL DATA
	if (!strcmp_P( strupr(argv[1]), PSTR("DATA\0"))) {
		vTaskSuspend( xHandle_tkInputs );
		ctl_watchdog_kick(WDG_DINPUTS, 0x8000 );
		pv_snprintfP_OK();
		return;
	}

	// KILL COMMSTX
	if (!strcmp_P( strupr(argv[1]), PSTR("COMMSTX\0"))) {
		vTaskSuspend( xHandle_tkComms );
		ctl_watchdog_kick(WDG_COMMS, 0x8000 );
		// Dejo la flag de modem prendido para poder leer comandos
		xCOMMS_stateVars.gprs_prendido = true;
		pv_snprintfP_OK();
		return;
	}

	// KILL APPlicacion
	if (!strcmp_P( strupr(argv[1]), PSTR("APP\0"))) {
		vTaskSuspend( xHandle_tkAplicacion );
		ctl_watchdog_kick(WDG_APP, 0x8000 );
		pv_snprintfP_OK();
		return;
	}

	pv_snprintfP_ERR();
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

uint8_t fuse0 = 0;
uint8_t fuse1 = 0;
uint8_t fuse2 = 0;
uint8_t fuse4 = 0;
uint8_t fuse5 = 0;

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
static void pv_cmd_read_memory(void)
{
	// Si hay datos en memoria los lee todos y los muestra en pantalla
	// Leemos la memoria e imprimo los datos.
	// El problema es que si hay muchos datos puede excederse el tiempo de watchdog y
	// resetearse el dlg.
	// Para esto, cada 32 registros pateo el watchdog.
	// El proceso es similar a tkGprs.transmitir_dataframe


FAT_t l_fat;
size_t bRead = 0;
uint16_t rcds = 0;
bool detail = false;
st_dataRecord_t dr;

	memset( &l_fat, '\0', sizeof(FAT_t));

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

		// imprimo
		if ( detail ) {
			xprintf_P( PSTR("memory: wrPtr=%d,rdPtr=%d,delPtr=%d,r4wr=%d,r4rd=%d,r4del=%d \r\n\0"), l_fat.wrPTR,l_fat.rdPTR, l_fat.delPTR,l_fat.rcds4wr,l_fat.rcds4rd,l_fat.rcds4del );
		}

		// Imprimo el registro
		xprintf_P( PSTR("CTL:%d;\0"), l_fat.rdPTR);
		data_print_inputs(fdTERM, &dr);

		xprintf_P(PSTR( "\r\n"));
	}

}
//------------------------------------------------------------------------------------
static void pv_cmd_rwGPRS(uint8_t cmd_mode )
{

uint8_t pin = 0;
//char *p;

	if ( cmd_mode == WR_CMD ) {

		// write gprs test at
/*		if ( strcmp_P( strupr(argv[2]), PSTR("TEST\0")) == 0 ) {
			gprs_CSOCKAUTH();
			pv_snprintfP_OK();
			return;
		}
*/
		// write gprs sms nbr msg
		if ( strcmp_P( strupr(argv[2]), PSTR("SMS\0")) == 0 ) {
			xSMS_enqueue( argv[3], (char *) xSMS_format(argv[4]) );
			pv_snprintfP_OK();
			return;
		}

		// write gprs qsms nbr msg
		if ( strcmp_P( strupr(argv[2]), PSTR("QSMS\0")) == 0 ) {
//C			u_gprs_quick_send_sms( argv[3], argv[4] );
			pv_snprintfP_OK();
			return;
		}

		// write gprs fsms nbr msg
		if ( strcmp_P( strupr(argv[2]), PSTR("FSMS\0")) == 0) {
			xSMS_send( argv[3], xSMS_format(argv[4]) );
			pv_snprintfP_OK();
			return;
		}

		// write gprs (pwr|sw|rts|dtr) {on|off}
		if ( strcmp_P( strupr(argv[2]), PSTR("PWR\0")) == 0 ) {
			if ( strcmp_P( strupr(argv[3]), PSTR("ON\0")) == 0 ) {
				// Para que pueda constestar!!!
				xCOMMS_stateVars.gprs_prendido = true;
				IO_set_GPRS_PWR(); pv_snprintfP_OK(); return;
			}
			if ( strcmp_P( strupr(argv[3]), PSTR("OFF\0")) == 0 ) {
				IO_clr_GPRS_PWR(); pv_snprintfP_OK(); return;
			}
			pv_snprintfP_ERR();
			return;
		}

		if ( strcmp_P( strupr(argv[2]), PSTR("SW\0"))  == 0 ) {
			if ( strcmp_P( strupr(argv[3]), PSTR("ON\0")) == 0 ) {
				IO_set_GPRS_SW();
				pv_snprintfP_OK(); return;
			}
			if ( strcmp_P( strupr(argv[3]), PSTR("OFF\0")) == 0 ) {
				IO_clr_GPRS_SW(); pv_snprintfP_OK(); return;
			}
			pv_snprintfP_ERR();
			return;
		}

		if ( strcmp_P( strupr(argv[2]), PSTR("RTS\0")) == 0 ) {
			if ( strcmp_P( strupr(argv[3]), PSTR("ON\0")) == 0 ) {
				IO_set_GPRS_RTS(); pv_snprintfP_OK(); return;
			}
			if ( strcmp_P( strupr(argv[3]), PSTR("OFF\0")) == 0 ) {
				IO_clr_GPRS_RTS(); pv_snprintfP_OK(); return;
			}
			pv_snprintfP_ERR();
			return;
		}

		// Por ahora cableo DTR a CTS.

		if ( strcmp_P( strupr(argv[2]), PSTR("DTR\0")) == 0 ) {
			if ( strcmp_P( strupr(argv[3]), PSTR("ON\0")) == 0 ) {
				IO_set_GPRS_DTR(); pv_snprintfP_OK(); return;
			}
			if ( strcmp_P( strupr(argv[3]), PSTR("OFF\0")) == 0 ) {
				IO_clr_GPRS_DTR(); pv_snprintfP_OK(); return;
			}
			pv_snprintfP_ERR();
			return;
		}

		// write gprs monsqe
		if ( strcmp_P( strupr(argv[2]), PSTR("MONSQE\0")) == 0 ) {
			SPX_SEND_SIGNAL( SGN_MON_SQE );
			return;
		}
		// write gprs redial
		if ( strcmp_P( strupr(argv[2]), PSTR("REDIAL\0")) == 0 ) {
			SPX_SEND_SIGNAL( SGN_REDIAL );
			return;
		}

		// ATCMD
		// write gprs cmd {atcmd}
		if ( strcmp_P(strupr(argv[2]), PSTR("CMD\0")) == 0 ) {
			//xprintf_P( PSTR("%s\r\0"),argv[3] );

			gprs_flush_RX_buffer();
			if (strcmp_P( argv[3], PSTR("+++")) == 0 ) {
				xfprintf_P( fdGPRS,PSTR("%s"),argv[3] );
			} else {
				xfprintf_P( fdGPRS,PSTR("%s\r\0"),argv[3] );
			}

			xprintf_P( PSTR("sent->%s\r\n\0"),argv[3] );
			return;
		}

		return;
	}


	if ( cmd_mode == RD_CMD ) {
		// read gprs (rsp,cts,dcd,ri)

		// SMS
		// read gprs sms
		if ( strcmp_P(strupr(argv[2]), PSTR("SMS\0")) == 0 ) {
//C			u_gprs_sms_rxcheckpoint();
			return;
		}

		// ATCMD RSP
		// read gprs rsp
		if ( strcmp_P(strupr(argv[2]), PSTR("RSP\0")) == 0 ) {
			gprs_print_RX_buffer();
			//p = pub_gprs_rxbuffer_getPtr();
			//xprintf_P( PSTR("rx->%s\r\n\0"),p );
			return;
		}

		// DCD
		if ( strcmp_P( strupr(argv[2]), PSTR("DCD\0")) == 0 ) {
			pin = IO_read_DCD();
			xprintf_P( PSTR("DCD=%d\r\n\0"),pin);
			pv_snprintfP_OK();
			return;
		}

		// RI
		if ( strcmp_P( strupr(argv[2]), PSTR("RI\0")) == 0 ) {
			pin = IO_read_RI();
			xprintf_P( PSTR("RI=%d\r\n\0"),pin);
			pv_snprintfP_OK();
			return;
		}

		// CTS
		if ( strcmp_P( strupr(argv[2]), PSTR("CTS\0")) == 0 ) {
			pin = IO_read_CTS();
			xprintf_P( PSTR("CTS=%d\r\n\0"),pin);
			pv_snprintfP_OK();
			return;
		}

		pv_snprintfP_ERR();
		return;
	}

}
//------------------------------------------------------------------------------------
static void pv_cmd_rwMCP(uint8_t cmd_mode )
{

int xBytes = 0;
uint8_t data = 0;

	if ( spx_io_board != SPX_IO8CH ) {
		xprintf_P(PSTR("ERROR: IOboard NOT spx8CH !!\r\n\0"));
		return;
	}

	// read mcp {regAddr}
	if ( cmd_mode == RD_CMD ) {
		xBytes = MCP_read( (uint32_t)(atoi(argv[2])), (char *)&data, 1 );
		if ( xBytes == -1 )
			xprintf_P(PSTR("ERROR: I2C:MCP:pv_cmd_rwMCP\r\n\0"));

		if ( xBytes > 0 ) {
			xprintf_P( PSTR( "MCP ADDR=0x%x, VALUE=0x%x\r\n\0"),atoi(argv[2]), data);
		}
		( xBytes > 0 ) ? pv_snprintfP_OK() : 	pv_snprintfP_ERR();
		return;
	}

	// write mcp regAddr value
	if ( cmd_mode == WR_CMD ) {
		data = atoi(argv[3]);

		xBytes = MCP_write( (uint32_t)(atoi(argv[2])), (char *)&data , 1 );
		if ( xBytes == -1 )
			xprintf_P(PSTR("ERROR: I2C:MCP:pv_cmd_rwMCP\r\n\0"));

		// Si estoy escribiendo el registro OLATB que refleja las salidas
		// debo dejar actualizado el valor que puse
//		if ( atoi( argv[2]) == MCP_OLATB ) {
//			perforaciones_set_douts(data);
//		}

		( xBytes > 0 ) ? pv_snprintfP_OK() : 	pv_snprintfP_ERR();
		return;
	}


}
//------------------------------------------------------------------------------------
static void cmdPeekFunction(void)
{
	// Comando utilizado para recuperar variables.
	// peek varName
	// Return: datos de configuracion de la variable.

//uint8_t ch = 0;

	FRTOS_CMD_makeArgv();

	if (!strcmp_P( strupr(argv[1]), PSTR("VERSION\0")) ) {
		xprintf_P( PSTR("<VERSION=%s,%s,%s,%s,%d,%d>\r\n\0"), SPX_HW_MODELO, SPX_FTROS_VERSION, SPX_FW_REV, SPX_FW_DATE, SYSMAINCLK, configTICK_RATE_HZ);

	} else if (!strcmp_P( strupr(argv[1]), PSTR("IOBOARD\0")) ) {
		xprintf_P( PSTR("<IOBOARD=%d>\r\n\0"), spx_io_board );

	} else if (!strcmp_P( strupr(argv[1]), PSTR("UID\0")) ) {
		xprintf_P( PSTR("<UID=%s>\r\n\0"), NVMEE_readID() );

	} else if (!strcmp_P( strupr(argv[1]), PSTR("DLGID\0")) ) {
		xprintf_P( PSTR("<DLGID=%s>\r\n\0"), sVarsComms.dlgId );

	} else if  (!strcmp_P( strupr(argv[1]), PSTR("APN\0")) ) {
		xprintf_P( PSTR("<APN=%s>\r\n\0"), sVarsComms.apn );

	} else if  (!strcmp_P( strupr(argv[1]), PSTR("PORT\0")) ) {
		xprintf_P( PSTR("<PORT=%s>\r\n\0"), sVarsComms.server_tcp_port );

	} else if  (!strcmp_P( strupr(argv[1]), PSTR("IP\0")) ) {
		xprintf_P( PSTR("<IP=%s>\r\n\0"), sVarsComms.server_ip_address );

	} else if  (!strcmp_P( strupr(argv[1]), PSTR("SCRIPT\0")) ) {
		xprintf_P( PSTR("<SCRIPT=%s>\r\n\0"), sVarsComms.serverScript );

	} else if  (!strcmp_P( strupr(argv[1]), PSTR("SIMPWD\0")) ) {
		xprintf_P( PSTR("<SIMPWD=%s>\r\n\0"), sVarsComms.simpwd );

	} else if  (!strcmp_P( strupr(argv[1]), PSTR("TIMERPOLL\0")) ) {
		xprintf_P( PSTR("<TIMERPOLL=%d>\r\n\0"), systemVars.timerPoll );

	} else if  (!strcmp_P( strupr(argv[1]), PSTR("TIMERDIAL\0")) ) {
		xprintf_P( PSTR("<TIMERDIAL=%d>\r\n\0"), sVarsComms.timerDial );

	} else if  (!strcmp_P( strupr(argv[1]), PSTR("timepwrsensor\0")) ) {
		xprintf_P( PSTR("<TIMEPWRSENSOR=%d>\r\n\0"), systemVars.ainputs_conf.pwr_settle_time );

	} else if  (!strcmp_P( strupr(argv[1]), PSTR("DEBUG\0")) ) {
		xprintf_P( PSTR("<DEBUG=%d>\r\n\0"), systemVars.debug );

	/*
	} else if  (!strcmp_P( strupr(argv[1]), PSTR("RANGEMETER\0")) ) {
	xprintf_P( PSTR("RANGEMETER=%d\r\n\0"), systemVars.rangeMeter_enabled );

	} else if  (!strcmp_P( strupr(argv[1]), PSTR("PWRSAVE\0")) ) {
		xprintf_P( PSTR("PWRSAVE=%d,%d,%d\r\n\0"), sVarsComms.pwrSave.pwrs_enabled, sVarsComms.pwrSave.hora_start, sVarsComms.pwrSave.hora_fin  );

	} else if  (!strcmp_P( strupr(argv[1]), PSTR("ANALOG\0")) && ( argv[2] != NULL )) {
		ch = atoi( argv[2]);
		xprintf_P( PSTR("ANALOG=%d,%s,%d,%d,%.02f,%.02f\r\n\0"), ch, systemVars.ainputs_conf.name[ch], systemVars.ainputs_conf.imin[ch], systemVars.ainputs_conf.imax[ch], systemVars.ainputs_conf.mmin[ch], systemVars.ainputs_conf.mmax[ch] );

	} else if  (!strcmp_P( strupr(argv[1]), PSTR("OFFSET\0")) && ( argv[2] != NULL )) {
		ch = atoi( argv[2]);
		xprintf_P( PSTR("OFFSET=%d,%.02f\r\n\0"), ch,systemVars.ainputs_conf.offset[ch] );

	} else if  (!strcmp_P( strupr(argv[1]), PSTR("DIGITAL\0")) && ( argv[2] != NULL )) {
		ch = atoi( argv[2]);
		xprintf_P( PSTR("DIGITAL=%d,%s\r\n\0"), ch,systemVars.dinputs_conf.name[ch] );

	} else if  (!strcmp_P( strupr(argv[1]), PSTR("COUNTER\0")) && ( argv[2] != NULL )) {
		ch = atoi( argv[2]);
		xprintf_P( PSTR("COUNTER=%d,%s,%.03f,%d,%d,%s\r\n\0"), ch,systemVars.counters_conf.name[ch],systemVars.counters_conf.magpp[ch],systemVars.counters_conf.pwidth[ch],systemVars.counters_conf.period[ch],systemVars.counters_conf.speed[ch] );

	*/

	} else {
		xprintf_P( PSTR("ERROR\r\n\0"));
	}
	return;

}
//------------------------------------------------------------------------------------
static void cmdPokeFunction(void)
{
	// Comando para configurar variables.

	FRTOS_CMD_makeArgv();


	if (!strcmp_P( strupr(argv[1]), PSTR("DLGID\0")) ) {

		memset(sVarsComms.dlgId,'\0', sizeof(sVarsComms.dlgId) );
		memcpy(sVarsComms.dlgId, argv[2], sizeof(sVarsComms.dlgId));
		sVarsComms.dlgId[DLGID_LENGTH - 1] = '\0';
		xprintf_P(PSTR("good1\r\n\0"));

	} else if  (!strcmp_P( strupr(argv[1]), PSTR("APN\0")) ) {
		memset(sVarsComms.apn, '\0', sizeof(sVarsComms.apn));
		memcpy(sVarsComms.apn, argv[2], sizeof(sVarsComms.apn));
		sVarsComms.apn[APN_LENGTH - 1] = '\0';
		xprintf_P(PSTR("good2\r\n\0"));

	} else if  (!strcmp_P( strupr(argv[1]), PSTR("PORT\0")) ) {
		memset(sVarsComms.server_tcp_port, '\0', sizeof(sVarsComms.server_tcp_port));
		memcpy(sVarsComms.server_tcp_port, argv[2], sizeof(sVarsComms.server_tcp_port));
		sVarsComms.server_tcp_port[PORT_LENGTH - 1] = '\0';
		xprintf_P(PSTR("good3\r\n\0"));

	} else if  (!strcmp_P( strupr(argv[1]), PSTR("IP\0")) ) {
		memset(sVarsComms.server_ip_address, '\0', sizeof(sVarsComms.server_ip_address));
		memcpy(sVarsComms.server_ip_address, argv[2], sizeof(sVarsComms.server_ip_address));
		sVarsComms.server_ip_address[IP_LENGTH - 1] = '\0';
		xprintf_P(PSTR("good4\r\n\0"));

	} else if  (!strcmp_P( strupr(argv[1]), PSTR("SCRIPT\0")) ) {
		memset(sVarsComms.serverScript, '\0', sizeof(sVarsComms.serverScript));
		memcpy(sVarsComms.serverScript, argv[2], sizeof(sVarsComms.serverScript));
		sVarsComms.serverScript[SCRIPT_LENGTH - 1] = '\0';
		xprintf_P(PSTR("good5\r\n\0"));

	} else if  (!strcmp_P( strupr(argv[1]), PSTR("SIMPWD\0")) ) {
		memset(sVarsComms.simpwd, '\0', sizeof(sVarsComms.simpwd));
		memcpy(sVarsComms.simpwd, argv[2], sizeof(sVarsComms.simpwd));
		sVarsComms.simpwd[SIM_PASSWD_LENGTH - 1] = '\0';
		xprintf_P(PSTR("good6\r\n\0"));

	} else if  (!strcmp_P( strupr(argv[1]), PSTR("TIMERPOLL\0")) ) {
		u_config_timerpoll( argv[2] );

	} else if  (!strcmp_P( strupr(argv[1]), PSTR("TIMERDIAL\0")) ) {
		u_config_timerdial( argv[2] );

	} else if  (!strcmp_P( strupr(argv[1]), PSTR("timepwrsensor\0")) ) {
		ainputs_config_timepwrsensor( argv[2] );

	} else if  (!strcmp_P( strupr(argv[1]), PSTR("DEBUG\0")) ) {
		systemVars.debug = atoi(argv[2]);

	/*
	} else if  (!strcmp_P( strupr(argv[1]), PSTR("RANGEMETER\0")) ) {
		range_config(argv[2]);

	} else if  (!strcmp_P( strupr(argv[1]), PSTR("PWRSAVE\0")) ) {
		u_configPwrSave ( argv[2], argv[3], argv[4] );

	} else if  (!strcmp_P( strupr(argv[1]), PSTR("COUNTER\0")) ) {
		counters_config_channel( atoi(argv[2]), argv[3], argv[4], argv[5], argv[6], argv[7] );

	} else if  (!strcmp_P( strupr(argv[1]), PSTR("DIGITAL\0")) ) {
		dinputs_config_channel( atoi(argv[2]), argv[3], argv[4] );

	} else if  (!strcmp_P( strupr(argv[1]), PSTR("ANALOG\0")) ) {
		ainputs_config_channel( atoi(argv[2]), argv[3], argv[4], argv[5], argv[6], argv[7], argv[8] );
	*/
	}

}
//------------------------------------------------------------------------------------
static void pv_cmd_I2Cscan(bool busscan)
{

bool retS = false;
uint8_t i2c_address;


	// Scan de una direccion
	if ( busscan == false ) {
		i2c_address = atoi(argv[2]);
		retS = I2C_scan_device(i2c_address);
		if (retS) {
			xprintf_P( PSTR("I2C device found at 0x%02x\r\n\0"), i2c_address );
		} else {
			xprintf_P( PSTR("I2C device NOT found at 0x%02x\r\n\0"), i2c_address );
		}
		return;
	}

	// Scan de todo el bus.00..FF.
	// Solo muestro las direcciones donde encuentro un device.
	for ( i2c_address = 0x00; i2c_address < 0xFF; i2c_address++ ) {
		retS = I2C_scan_device(i2c_address);
		if (retS) {
			xprintf_P( PSTR("I2C device found at 0x%02x\r\n\0"), i2c_address );
		};
	}

}
//------------------------------------------------------------------------------------
static bool pv_cmd_configGPRS(void)
{

	// config gprs SAT {check|enable|disable}
	if ( strcmp_P( strupr(argv[2]), PSTR("SAT\0")) == 0 ) {

		if ( strcmp_P( strupr(argv[3]), PSTR("DISABLE\0")) == 0 ) {
			return( gprs_SAT_set(0));
		}

		if ( strcmp_P( strupr(argv[3]), PSTR("ENABLE\0")) == 0 ) {
			return( gprs_SAT_set(1));
		}

		if ( strcmp_P( strupr(argv[3]), PSTR("CHECK\0")) == 0 ) {
			return( gprs_SAT_set(2));
		}
	}

	return(false);
}
//------------------------------------------------------------------------------------

