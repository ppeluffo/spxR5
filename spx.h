/*
 * spx.h
 *
 *  Created on: 8 dic. 2018
 *      Author: pablo
 */

#ifndef SRC_SPX_H_
#define SRC_SPX_H_

//------------------------------------------------------------------------------------
// INCLUDES
//------------------------------------------------------------------------------------
#include <avr/io.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include <compat/deprecated.h>
#include <avr/pgmspace.h>
#include <stdarg.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <avr/sleep.h>
#include <ctype.h>
#include "avr_compiler.h"
#include "clksys_driver.h"
#include <inttypes.h>

#include "TC_driver.h"
#include "pmic_driver.h"
#include "wdt_driver.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "list.h"
#include "croutine.h"
#include "semphr.h"
#include "timers.h"
#include "limits.h"

#include "frtos-io.h"
#include "FRTOS-CMD.h"

#include "l_ainputs.h"
#include "l_counters.h"
#include "l_dinputs.h"
#include "l_doutputs.h"
#include "l_drv8814.h"
#include "l_iopines.h"
#include "l_eeprom.h"
#include "l_file.h"
#include "l_i2c.h"
#include "l_ina3221.h"
#include "l_iopines.h"
#include "l_rtc79410.h"
#include "l_nvm.h"
#include "l_printf.h"
#include "l_rangeMeter.h"

//------------------------------------------------------------------------------------
// DEFINES
//------------------------------------------------------------------------------------
#define SPX_FW_REV "0.0.4.R1"
#define SPX_FW_DATE "@ 20190308"

#define SPX_HW_MODELO "spxR4 HW:xmega256A3B R1.1"
#define SPX_FTROS_VERSION "FW:FRTOS10 TICKLESS"

//#define F_CPU (32000000UL)

//#define SYSMAINCLK 2
//#define SYSMAINCLK 8
#define SYSMAINCLK 32
//
#define MAX_ANALOG_CHANNELS		8
#define MAX_DINPUTS_CHANNELS	8
#define MAX_DOUTPUTS_CHANNELS	8
#define MAX_COUNTER_CHANNELS	2

#define IO5_ANALOG_CHANNELS		5
#define IO5_DINPUTS_CHANNELS	2
#define IO5_COUNTER_CHANNELS	2

#define IO8_ANALOG_CHANNELS		8
#define IO8_DINPUTS_CHANNELS	8
#define IO8_COUNTER_CHANNELS	2

#define CHAR32	32
#define CHAR64	64
#define CHAR128	128
#define CHAR256	256

#define tkCtl_STACK_SIZE		512
#define tkCmd_STACK_SIZE		512
#define tkCounter_STACK_SIZE	512
#define tkData_STACK_SIZE		512
#define tkDinputs_STACK_SIZE	512
#define tkDoutputs_STACK_SIZE	512
#define tkGprs_rx_STACK_SIZE	1024
#define tkGprs_tx_STACK_SIZE	1024

#define tkCtl_TASK_PRIORITY	 		( tskIDLE_PRIORITY + 1 )
#define tkCmd_TASK_PRIORITY	 		( tskIDLE_PRIORITY + 1 )
#define tkCounter_TASK_PRIORITY	 	( tskIDLE_PRIORITY + 1 )
#define tkData_TASK_PRIORITY	 	( tskIDLE_PRIORITY + 1 )
#define tkDinputs_TASK_PRIORITY	 	( tskIDLE_PRIORITY + 1 )
#define tkDoutputs_TASK_PRIORITY	( tskIDLE_PRIORITY + 1 )
#define tkGprs_rx_TASK_PRIORITY		( tskIDLE_PRIORITY + 1 )
#define tkGprs_tx_TASK_PRIORITY		( tskIDLE_PRIORITY + 1 )

#define MODO_DISCRETO ( (systemVars.timerDial > 0 ) ? true : false )

// Mensajes entre tareas
#define TK_FRAME_READY			0x01	//
#define TK_REDIAL				0x04	//

typedef enum { DEBUG_NONE = 0, DEBUG_COUNTER, DEBUG_DATA, DEBUG_GPRS } t_debug;
typedef enum { USER_NORMAL, USER_TECNICO } usuario_t;
typedef enum { SPX_IO5CH = 0, SPX_IO8CH } ioboard_t;
typedef enum { CONSIGNA_OFF = 0, CONSIGNA_DIURNA, CONSIGNA_NOCTURNA } consigna_t;
typedef enum { modoPWRSAVE_OFF = 0, modoPWRSAVE_ON } t_pwrSave;

TaskHandle_t xHandle_idle, xHandle_tkCtl, xHandle_tkCmd, xHandle_tkCounter, xHandle_tkData, xHandle_tkDinputs, xHandle_tkDoutputs,  xHandle_tkGprsRx, xHandle_tkGprsTx;

bool startTask;
uint8_t spx_io_board;

xSemaphoreHandle sem_SYSVars;
StaticSemaphore_t SYSVARS_xMutexBuffer;
#define MSTOTAKESYSVARSSEMPH ((  TickType_t ) 10 )

void tkCtl(void * pvParameters);
void tkCmd(void * pvParameters);
void tkCounter(void * pvParameters);
void tkData(void * pvParameters);
void tkDinputs(void * pvParameters);
void tkDoutputs(void * pvParameters);
void tkGprsRx(void * pvParameters);
void tkGprsTx(void * pvParameters);

#define DLGID_LENGTH		10
#define PARAMNAME_LENGTH	5
#define IP_LENGTH			24
#define APN_LENGTH			32
#define PORT_LENGTH			7
#define SCRIPT_LENGTH		64
#define PASSWD_LENGTH		15
#define PARAMNAME_LENGTH	5

uint8_t NRO_COUNTERS;
uint8_t NRO_ANINPUTS;
uint8_t NRO_DINPUTS;

// Estructura de un registro de IO5CH
typedef struct {
	float ainputs[IO5_ANALOG_CHANNELS];
	uint16_t dinputs[IO5_DINPUTS_CHANNELS];
	float counters[IO5_COUNTER_CHANNELS];
	int16_t range;
	float battery;
} st_io5_t;

// Estructura de un registro de IO8CH
typedef struct {
	float ainputs[IO8_ANALOG_CHANNELS];
	uint16_t dinputs[IO8_DINPUTS_CHANNELS];
	float counters[IO8_COUNTER_CHANNELS];
} st_io8_t;

// Estructura de datos comun independiente de la arquitectura de IO
typedef union u_dataframe {
	st_io5_t io5;
	st_io8_t io8;
} u_dataframe_t;

typedef struct {
	u_dataframe_t df;
	RtcTimeType_t rtc;
} st_dataRecord_t;

// Estructuras para las consignas
typedef struct {
	uint8_t hour;
	uint8_t min;
} st_time_t;

typedef struct {
	st_time_t hhmm_c_diurna;
	st_time_t hhmm_c_nocturna;
	consigna_t c_aplicada;
	bool c_enabled;
} st_consigna_t;

typedef struct {
	uint8_t modo;
	st_time_t hora_start;
	st_time_t hora_fin;
} st_pwrsave_t;

// Configuracion de canales de contadores
typedef struct {
	char name[MAX_COUNTER_CHANNELS][PARAMNAME_LENGTH];
	float magpp[MAX_COUNTER_CHANNELS];
	uint8_t debounce_time;
} counters_conf_t;

// Configuracion de canales digitales
typedef struct {
	char name[MAX_DINPUTS_CHANNELS][PARAMNAME_LENGTH];
	bool timers_enabled;
	uint8_t checksum;
} dinputs_conf_t;

// Configuracion de canales analogicos
typedef struct {
	uint8_t imin[MAX_ANALOG_CHANNELS];	// Coeficientes de conversion de I->magnitud (presion)
	uint8_t imax[MAX_ANALOG_CHANNELS];
	float mmin[MAX_ANALOG_CHANNELS];
	float mmax[MAX_ANALOG_CHANNELS];
	char name[MAX_ANALOG_CHANNELS][PARAMNAME_LENGTH];
	float offset[MAX_ANALOG_CHANNELS];
	float inaspan[MAX_ANALOG_CHANNELS];
	uint8_t pwr_settle_time;
} ainputs_conf_t;

typedef struct {
	// Variables de trabajo.
	char dlgId[DLGID_LENGTH];
	char apn[APN_LENGTH];
	char server_tcp_port[PORT_LENGTH];
	char server_ip_address[IP_LENGTH];
	char serverScript[SCRIPT_LENGTH];
	char simpwd[PASSWD_LENGTH];
	uint32_t timerDial;
	st_pwrsave_t pwrSave;

	t_debug debug;
	uint16_t timerPoll;
	bool rangeMeter_enabled;

	uint8_t d_outputs;
	st_consigna_t consigna;

	counters_conf_t counters_conf;	// Estructura con la configuracion de los contadores
	dinputs_conf_t dinputs_conf;	// Estructura con la configuracion de las entradas digitales
	ainputs_conf_t ainputs_conf;	// Estructura con la configuracion de las entradas analogicas

	// El checksum DEBE ser el ultimo byte del systemVars !!!!
	uint8_t checksum;

} systemVarsType;

systemVarsType systemVars;

uint32_t u_getSystemTicks(void);

//extern struct counters_s  counters;

// UTILS
void initMCU(void);
void u_configure_systemMainClock(void);
void u_configure_RTC32(void);
void u_control_string( char *s_name );
void u_convert_str_to_time_t ( char *time_str, st_time_t *time_struct );
void u_convert_int_to_time_t ( int int16time, st_time_t *time_struct );
void u_load_defaults( void );
uint8_t u_save_params_in_NVMEE(void);
bool u_load_params_from_NVMEE(void);
void u_config_timerpoll ( char *s_timerpoll );

// TKCTL
void ctl_watchdog_kick(uint8_t taskWdg, uint16_t timeout_in_secs );
void ctl_print_wdg_timers(void);
uint16_t ctl_readTimeToNextPoll(void);
void ctl_reload_timerPoll( uint16_t new_time );
bool ctl_terminal_connected(void);
uint32_t ctl_readTimeToNextDial(void);
void ctl_reload_timerDial( uint32_t new_time );

// TKCOUNTER
float counters_read( uint8_t cnt, bool reset_counter );
void counters_config_defaults(void);
bool counters_config_channel( uint8_t channel,char *s_param0, char *s_param1 );
void counters_config_debounce_time( char *s_counter_debounce_time );

// TKDINPUTS
int16_t dinputs_read ( uint8_t din );
void dinputs_config_defaults(void);
bool dinputs_config_channel( uint8_t channel,char *s_aname );
bool dinputs_config_timermode ( char *s_mode );

// RANGE
int16_t range_read(void);
bool range_config ( char *s_mode );
void range_config_defaults(void);

// AINPUTS
void ainputs_read ( uint8_t io_channel, uint16_t *raw_val, float *mag_val );
bool ainputs_config_channel( uint8_t channel,char *s_aname,char *s_imin,char *s_imax,char *s_mmin,char *s_mmax );
void ainputs_config_defaults(void);
bool ainputs_config_offset( char *s_channel, char *s_offset );
void ainputs_config_sensortime ( char *s_sensortime );
void ainputs_config_span ( char *s_channel, char *s_span );
bool ainputs_config_autocalibrar( char *s_channel, char *s_mag_val );

// TKDATA
void data_read_frame( bool polling );

// TKDOUTPUTS
void doutputs_config_defaults(void);
bool doutputs_config_consignas( char *_cmodo, char *hhmm_dia, char *hhmm_noche);

// WATCHDOG
uint8_t wdg_resetCause;

#define WDG_CTL			0
#define WDG_CMD			1
#define WDG_COUNT		2
#define WDG_DAT			3
#define WDG_DIN			4
#define WDG_DOUT		5
#define WDG_GPRSRX		6
#define WDG_GPRSTX		7

#define NRO_WDGS		8


#endif /* SRC_SPX_H_ */
