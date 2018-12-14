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
#define SPX_FW_REV "0.0.1.R4"
#define SPX_FW_DATE "@ 20181212"

#define SPX_HW_MODELO "spxR4 HW:xmega256A3B R1.1"
#define SPX_FTROS_VERSION "FW:FRTOS10 TICKLESS"

//#define F_CPU (32000000UL)

//#define SYSMAINCLK 2
//#define SYSMAINCLK 8
#define SYSMAINCLK 32

//#define SPX_8CH
#define SPX_5CH
//
#define MAX_ANALOG_CHANNELS		8
#define MAX_DINPUTS_CHANNELS	8
#define MAX_DOUTPUTS_CHANNELS	8
#define MAX_COUNTER_CHANNELS	2

#define CHAR32	32
#define CHAR64	64
#define CHAR128	128
#define CHAR256	256

#define tkCtl_STACK_SIZE		512
#define tkCmd_STACK_SIZE		512
#define tkCounter_STACK_SIZE	512
#define tkData_STACK_SIZE		512

#define tkCtl_TASK_PRIORITY	 		( tskIDLE_PRIORITY + 1 )
#define tkCmd_TASK_PRIORITY	 		( tskIDLE_PRIORITY + 1 )
#define tkCounter_TASK_PRIORITY	 	( tskIDLE_PRIORITY + 1 )
#define tkData_TASK_PRIORITY	 	( tskIDLE_PRIORITY + 1 )

typedef enum { DEBUG_NONE = 0, DEBUG_COUNTER, DEBUG_DATA } t_debug;
typedef enum { USER_NORMAL, USER_TECNICO } usuario_t;
typedef enum { SPX_IO5CH = 0, SPX_IO8CH } ioboard_t;

TaskHandle_t xHandle_idle, xHandle_tkCtl, xHandle_tkCmd, xHandle_tkCounter, xHandle_tkData;

bool startTask;
uint8_t spx_io_board;

xSemaphoreHandle sem_SYSVars;
StaticSemaphore_t SYSVARS_xMutexBuffer;
#define MSTOTAKESYSVARSSEMPH ((  TickType_t ) 10 )

void tkCtl(void * pvParameters);
void tkCmd(void * pvParameters);
void tkCounter(void * pvParameters);
void tkData(void * pvParameters);

#define DLGID_LENGTH		10
#define PARAMNAME_LENGTH	5
#define IP_LENGTH			24
#define APN_LENGTH			32
#define PORT_LENGTH			7
#define SCRIPT_LENGTH		64
#define PASSWD_LENGTH		15
#define PARAMNAME_LENGTH	5


typedef struct {
	float counters[MAX_COUNTER_CHANNELS];
} st_counters_t;

typedef struct {
	float ainputs[MAX_ANALOG_CHANNELS];
} st_analog_inputs_t;


uint8_t NRO_COUNTERS;
uint8_t NRO_ANINPUTS;

typedef struct {
	// Variables de trabajo.

	// Configuracion de canales contadores
	char counters_name[MAX_COUNTER_CHANNELS][PARAMNAME_LENGTH];
	float counters_magpp[MAX_COUNTER_CHANNELS];
	uint8_t counter_debounce_time;

	// Configuracion de Canales analogicos
	uint8_t ain_imin[MAX_ANALOG_CHANNELS];	// Coeficientes de conversion de I->magnitud (presion)
	uint8_t ain_imax[MAX_ANALOG_CHANNELS];
	float ain_mmin[MAX_ANALOG_CHANNELS];
	float ain_mmax[MAX_ANALOG_CHANNELS];
	char ain_name[MAX_ANALOG_CHANNELS][PARAMNAME_LENGTH];
	float ain_mag_offset[MAX_ANALOG_CHANNELS];
	float ain_mag_span[MAX_ANALOG_CHANNELS];

	t_debug debug;
	bool rangeMeter_enabled;
	uint16_t timerPoll;
	uint8_t pwr_settle_time;


	// El checksum DEBE ser el ultimo byte del systemVars !!!!
	uint8_t checksum;

} systemVarsType;

systemVarsType systemVars;

// UTILS
void initMCU(void);
void u_configure_systemMainClock(void);
void u_configure_RTC32(void);
void u_control_string( char *s_name );
void u_load_defaults( void );
uint8_t u_save_params_in_NVMEE(void);
bool u_load_params_from_NVMEE(void);
void u_config_timerpoll ( char *s_timerpoll );
bool u_config_counter_channel( uint8_t channel,char *s_param0, char *s_param1 );
bool u_config_analog_channel( uint8_t channel,char *s_aname,char *s_imin,char *s_imax,char *s_mmin,char *s_mmax );
void u_read_analog_channel ( uint8_t io_board, uint8_t io_channel, uint16_t *raw_val, float *mag_val );

// TKCTL
void ctl_watchdog_kick(uint8_t taskWdg, uint16_t timeout_in_secs );
void ctl_print_wdg_timers(void);
uint16_t ctl_readTimeToNextPoll(void);
void ctl_reload_timerPoll(void);

// TKCOUNTER
void counters_read_frame( st_counters_t dst_frame[], bool reset_counters );

// TKDATA
void data_read_frame(void);
void data_print_frame(void);

// WATCHDOG
uint8_t wdg_resetCause;

#define WDG_CTL			0
#define WDG_CMD			1
#define WDG_COUNT		2
#define WDG_DAT			3

#define NRO_WDGS		4


#endif /* SRC_SPX_H_ */
