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
#define SPX_FW_DATE "@ 20181211"

#define SPX_HW_MODELO "spxR4 HW:xmega256A3B R1.1"
#define SPX_FTROS_VERSION "FW:FRTOS10 TICKLESS"

//#define F_CPU (32000000UL)

//#define SYSMAINCLK 2
//#define SYSMAINCLK 8
#define SYSMAINCLK 32

#define SPX_8CH
//#define SPX_5CH
// El datalogger tiene 6 canales fisicos pero 5 disponibles
// ya que uno esta para monitorear la bateria.
//
#ifdef SPX_8CH
	#define NRO_ANALOG_CHANNELS		8
	#define NRO_DINPUTS_CHANNELS	8
	#define NRO_DOUTPUTS_CHANNELS	8
	#define NRO_COUNTER_CHANNELS	2
#endif

#ifdef SPX_5CH
	#define NRO_ANALOG_CHANNELS		5
	#define NRO_DINPUTS_CHANNELS	2
	#define NRO_DOUTPUTS_CHANNELS	2
	#define NRO_COUNTER_CHANNELS	2
#endif

#define CHAR32	32
#define CHAR64	64
#define CHAR128	128
#define CHAR256	256

#define tkCtl_STACK_SIZE		512
#define tkCmd_STACK_SIZE		512
#define tkCounter_STACK_SIZE	512

#define tkCtl_TASK_PRIORITY	 		( tskIDLE_PRIORITY + 1 )
#define tkCmd_TASK_PRIORITY	 		( tskIDLE_PRIORITY + 1 )
#define tkCounter_TASK_PRIORITY	 	( tskIDLE_PRIORITY + 1 )

typedef enum { DEBUG_NONE = 0, DEBUG_COUNTER } t_debug;
typedef enum { USER_NORMAL, USER_TECNICO } usuario_t;

TaskHandle_t xHandle_idle, xHandle_tkCtl, xHandle_tkCmd, xHandle_tkCounter;

bool startTask;

xSemaphoreHandle sem_SYSVars;
StaticSemaphore_t SYSVARS_xMutexBuffer;
#define MSTOTAKESYSVARSSEMPH ((  TickType_t ) 10 )

void tkCtl(void * pvParameters);
void tkCmd(void * pvParameters);
void tkCounter(void * pvParameters);

#define DLGID_LENGTH		10
#define PARAMNAME_LENGTH	5
#define IP_LENGTH			24
#define APN_LENGTH			32
#define PORT_LENGTH			7
#define SCRIPT_LENGTH		64
#define PASSWD_LENGTH		15
#define PARAMNAME_LENGTH	5

// Tenemos 2 contadorees IN_C0, IN_C1
// Si bien son 2 contadores de 16 bits, el tema es que
// la magnitud debe ser un float.
typedef struct {
	float counters[NRO_COUNTER_CHANNELS];
} st_counters_t;

typedef struct {
	// Variables de trabajo.

	// Configuracion de canales contadores
	char counters_name[NRO_COUNTER_CHANNELS][PARAMNAME_LENGTH];
	float counters_magpp[NRO_COUNTER_CHANNELS];
	uint8_t counter_debounce_time;

	t_debug debug;

	// El checksum DEBE ser el ultimo byte del systemVars !!!!
	uint8_t checksum;

} systemVarsType;

systemVarsType systemVars;

// UTILS
void initMCU(void);
void u_configure_systemMainClock(void);
void u_configure_RTC32(void);
void pub_control_string( char *s_name );
void pub_load_defaults( void );
uint8_t pub_save_params_in_NVMEE(void);
bool pub_load_params_from_NVMEE(void);

// TKCTL
void pub_ctl_watchdog_kick(uint8_t taskWdg, uint16_t timeout_in_secs );
void pub_ctl_print_wdg_timers(void);
void pub_ctl_print_stack_watermarks(void);

// TKCOUNTER
void pub_counter_config_debounce_time( char *s_counter_debounce_time );
bool pub_counters_config_channel( uint8_t channel,char *s_param0, char *s_param1 );
void pub_counters_load_defaults(void);
void pub_counters_read_frame( st_counters_t dst_frame[], bool reset_counters );

// WATCHDOG
uint8_t wdg_resetCause;

#define WDG_CTL			0
#define WDG_CMD			1
#define WDG_COUNT		2

#define NRO_WDGS		3



#endif /* SRC_SPX_H_ */
