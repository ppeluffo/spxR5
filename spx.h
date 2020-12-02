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
#include <avr/wdt.h>
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
#include "portable.h"

#include "frtos-io.h"
#include "FRTOS-CMD.h"

#include "l_counters.h"
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
#include "l_bytes.h"
#include "l_bps120.h"
#include "l_adt7410.h"

//------------------------------------------------------------------------------------
// DEFINES
//------------------------------------------------------------------------------------
#define SPX_FW_REV "3.0.5c"
//#define SPX_FW_REV "3.0.5BETA"
#define SPX_FW_DATE "@ 20201202"

#define SPX_HW_MODELO "spxR5 HW:xmega256A3B R1.1"
//#define SPX_FTROS_VERSION "FW:FRTOS10 TICKLESS Master(beta)"
#define SPX_FTROS_VERSION "FW:FRTOS10 TICKLESS Master Mbus."

//#define BETA_TEST
//#define MODEM_SIMULATOR
//#define MONITOR_STACK	1

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
#define IO8_DOUTPUTS_CHANNELS	8

#define MODBUS_CHANNELS			2

#define CHAR32	32
#define CHAR64	64
#define CHAR128	128
#define CHAR256	256

#define tkCtl_STACK_SIZE		384
#define tkCmd_STACK_SIZE		384
#define tkInputs_STACK_SIZE		512
#define tkComms_STACK_SIZE		640
#define tkCommsRX_STACK_SIZE	384
#define tkAplicacion_STACK_SIZE	384
#define tkAuxRX_STACK_SIZE		256

StaticTask_t xTask_Ctl_Buffer_Ptr;
StackType_t xTask_Ctl_Buffer [tkCtl_STACK_SIZE];

StaticTask_t xTask_Cmd_Buffer_Ptr;
StackType_t xTask_Cmd_Buffer [tkCmd_STACK_SIZE];

StaticTask_t xTask_Inputs_Buffer_Ptr;
StackType_t xTask_Inputs_Buffer [tkInputs_STACK_SIZE];

StaticTask_t xTask_Comms_Buffer_Ptr;
StackType_t xTask_Comms_Buffer [tkComms_STACK_SIZE];

StaticTask_t xTask_CommsRX_Buffer_Ptr;
StackType_t xTask_CommsRX_Buffer [tkCommsRX_STACK_SIZE];

StaticTask_t xTask_Aplicacion_Buffer_Ptr;
StackType_t xTask_Aplicacion_Buffer [tkAplicacion_STACK_SIZE];

StaticTask_t xTask_AuxRX_Buffer_Ptr;
StackType_t xTask_AuxRX_Buffer [tkAuxRX_STACK_SIZE];

#define tkCtl_TASK_PRIORITY	 		( tskIDLE_PRIORITY + 1 )
#define tkCmd_TASK_PRIORITY	 		( tskIDLE_PRIORITY + 1 )
#define tkInputs_TASK_PRIORITY	 	( tskIDLE_PRIORITY + 1 )
#define tkComms_TASK_PRIORITY		( tskIDLE_PRIORITY + 1 )
#define tkCommsRX_TASK_PRIORITY		( tskIDLE_PRIORITY + 1 )
#define tkAplicacion_TASK_PRIORITY	( tskIDLE_PRIORITY + 1 )
#define tkCommsRX_TASK_PRIORITY		( tskIDLE_PRIORITY + 1 )
#define tkAuxRX_TASK_PRIORITY		( tskIDLE_PRIORITY + 1 )

// Mensajes entre tareas
#define SGN_FRAME_READY			0x01
#define SGN_MON_SQE				0x02
#define SGN_REDIAL				0x03
#define SGN_SMS					0x05
#define SGN_WAKEUP				0x06
#define SGN_POLL_NOW			0x07

// Estructura que maneja las señales del sistema
struct {
	bool sgn_mon_sqe;
	bool sgn_redial;
	bool sgn_frame_ready;
	bool sgn_reset_comms_device;
	bool sgn_sms;
	bool sgn_poll_now;
} system_signals;

typedef enum { DEBUG_NONE = 0, DEBUG_COUNTER, DEBUG_DATA, DEBUG_COMMS, DEBUG_APLICACION, DEBUG_MODBUS } t_debug;
typedef enum { USER_NORMAL, USER_TECNICO } usuario_t;
typedef enum { SPX_IO5CH = 0, SPX_IO8CH } ioboard_t;
typedef enum { modoPWRSAVE_OFF = 0, modoPWRSAVE_ON } t_pwrSave;
typedef enum { DIN_NORMAL = 0, DIN_TIMER  } dinputs_modo_t;
typedef enum { CNT_LOW_SPEED = 0, CNT_HIGH_SPEED  } dcounters_modo_t;

TaskHandle_t xHandle_idle, xHandle_tkCtl, xHandle_tkCmd, xHandle_tkInputs, xHandle_tkComms, xHandle_tkCommsRX, xHandle_tkAplicacion, xHandle_tkAuxRX;

bool startTask;
uint8_t spx_io_board;
uint32_t sysTicks;
xSemaphoreHandle sem_SYSVars;
StaticSemaphore_t SYSVARS_xMutexBuffer;
#define MSTOTAKESYSVARSSEMPH ((  TickType_t ) 10 )

xSemaphoreHandle sem_WDGS;
StaticSemaphore_t WDGS_xMutexBuffer;
#define MSTOTAKEWDGSSEMPH ((  TickType_t ) 10 )

void tkCtl(void * pvParameters);
void tkCmd(void * pvParameters);
void tkInputs(void * pvParameters);
void tkComms(void * pvParameters);
void tkCommsRX(void * pvParameters);
void tkAplicacion(void * pvParameters);
void tkAuxRX(void * pvParameters);

#define DLGID_LENGTH		12
#define IP_LENGTH			24
#define APN_LENGTH			32
#define SGN_RESET_COMMS_DEV		0x04
#define PORT_LENGTH			7
#define SCRIPT_LENGTH		64
#define PASSWD_LENGTH		15
#define PARAMNAME_LENGTH	7
#define SIM_PASSWD_LENGTH	5

uint8_t NRO_COUNTERS;
uint8_t NRO_ANINPUTS;
uint8_t NRO_DINPUTS;

typedef struct {
	uint16_t idle;
	uint16_t cmd;
	uint16_t ctl;
	uint16_t in;
	uint16_t comms;
	uint16_t rx;
	uint16_t app;
} st_stack_size_t;

// Estructura de un registro de IO5CH
typedef struct {
	float ainputs[IO5_ANALOG_CHANNELS];			// 4 * 5 = 20
	uint16_t dinputs[IO5_DINPUTS_CHANNELS];		// 2 * 2 =  4
	float counters[IO5_COUNTER_CHANNELS];		// 4 * 2 =  8
	int16_t range;								// 2 * 1 =  2
	float psensor;								// 4 * 1 =  4
	float temp;									// 4 * 1 =  4
	float battery;								// 4 * 1 =  4
	uint16_t mbus_inputs[MODBUS_CHANNELS];		// 2 * 2 =  4
} st_io5_t;										// ----- = 50

// Estructura de un registro de IO8CH
typedef struct {
	float ainputs[IO8_ANALOG_CHANNELS];			// 4 * 8 = 32
	uint16_t dinputs[IO8_DINPUTS_CHANNELS];		// 2 * 8 = 16
	float counters[IO8_COUNTER_CHANNELS];		// 4 * 2 =  8
} st_io8_t; 									// ----- = 56

// Estructura de datos comun independiente de la arquitectura de IO
typedef union u_dataframe {
	st_io5_t io5;	// 50
	st_io8_t io8;	// 56
} u_dataframe_t;	// 56

typedef struct {
	u_dataframe_t df;	// 56
	RtcTimeType_t rtc;	//  7
} st_dataRecord_t;		// 63

// Estructuras para las consignas
typedef struct {
	uint8_t hour;
	uint8_t min;
} st_time_t;

typedef struct {
	bool pwrs_enabled;
	st_time_t hora_start;
	st_time_t hora_fin;
} st_pwrsave_t;

// Configuracion de canales de contadores
typedef struct {
	t_counters_hw_type hw_type;							// OPTO | NORMAL
	char name[MAX_COUNTER_CHANNELS][PARAMNAME_LENGTH];
	float magpp[MAX_COUNTER_CHANNELS];
	uint16_t pwidth[MAX_COUNTER_CHANNELS];
	uint16_t period[MAX_COUNTER_CHANNELS];
	uint8_t speed[MAX_COUNTER_CHANNELS];
	t_sensing_edge sensing_edge[MAX_COUNTER_CHANNELS];
} counters_conf_t;

// Configuracion de canales digitales
typedef struct {
	char name[MAX_DINPUTS_CHANNELS][PARAMNAME_LENGTH];
	dinputs_modo_t wrk_modo[MAX_DINPUTS_CHANNELS];
} dinputs_conf_t;

// Configuracion de canales analogicos
typedef struct {
	uint8_t imin[MAX_ANALOG_CHANNELS];	// Coeficientes de conversion de I->magnitud (presion)
	uint8_t imax[MAX_ANALOG_CHANNELS];
	float mmin[MAX_ANALOG_CHANNELS];
	float mmax[MAX_ANALOG_CHANNELS];
	char name[MAX_ANALOG_CHANNELS][PARAMNAME_LENGTH];
	float offset[MAX_ANALOG_CHANNELS];
	uint8_t pwr_settle_time;
	float ieq_min[MAX_ANALOG_CHANNELS];
	float ieq_max[MAX_ANALOG_CHANNELS];
} ainputs_conf_t;


// Configuracion del sensor i2c de presion
typedef struct {
	char name[PARAMNAME_LENGTH];
	uint16_t count_min;
	uint16_t count_max;
	float pmin;
	float pmax;
	float offset;
} psensor_conf_t;

// Configuracion de modbus
typedef struct {
	uint8_t modbus_slave_address;
	char var_name[MODBUS_CHANNELS][PARAMNAME_LENGTH];
	uint16_t var_address[MODBUS_CHANNELS];				// Direccion en el slave de la variable a leer
	uint8_t var_length[MODBUS_CHANNELS];				// Cantidad de bytes a leer
	uint8_t var_function_code[MODBUS_CHANNELS];			// Funcion de lectura (3-Holding reg, 4-Normal reg)
} modbus_conf_t;


typedef struct {

	// Variables de trabajo.

	t_debug debug;
	uint16_t timerPoll;

	char range_name[PARAMNAME_LENGTH];

	counters_conf_t counters_conf;	// Estructura con la configuracion de los contadores
	dinputs_conf_t dinputs_conf;	// Estructura con la configuracion de las entradas digitales
	ainputs_conf_t ainputs_conf;	// Estructura con la configuracion de las entradas analogicas
	psensor_conf_t psensor_conf;

	uint8_t an_calibrados;

	bool mide_bateria;

	modbus_conf_t modbus_conf;

	// El checksum DEBE ser el ultimo byte del systemVars !!!!
	uint8_t checksum;

} systemVarsType;

systemVarsType systemVars;

char hash_buffer[64];

// UTILS
void xCOMMS_config_defaults( char *opt );
void xCOMMS_status(void);

void debug_print_wdg_timers(void);
void debug_full_print_stack_watermarks(void);
void debug_print_stack_watermarks(char *id);
void debug_read_stack_watermarks(st_stack_size_t *stack_wmk );
void debug_monitor_stack_watermarks(char *id);
int debug_freeRam(void);

void initMCU(void);
void u_configure_systemMainClock(void);
void u_configure_RTC32(void);
uint8_t u_control_string( char *s_name );
void u_convert_str_to_time_t ( char *time_str, st_time_t *time_struct );
void u_convert_int_to_time_t ( int int16time, st_time_t *time_struct );
void u_load_defaults( char *opt );
void u_save_params_in_NVMEE(void);
bool u_load_params_from_NVMEE(void);
void u_config_timerpoll ( char *s_timerpoll );
bool u_check_more_Rcds4Del ( FAT_t *fat );
bool u_check_more_Rcds4Tx(void);
uint8_t u_base_hash(void);
uint8_t u_aplicacion_hash( void );
bool u_config_aplicacion( char *modo );
bool u_write_output_pins( uint8_t pin, int8_t val );
bool u_set_douts( uint8_t dout );
void u_config_timerdial ( char *s_timerdial );
void u_configPwrSave( char *s_modo, char *s_startTime, char *s_endTime);
uint8_t u_checksum( uint8_t *s, uint16_t size );
uint8_t u_hash(uint8_t checksum, char ch );
void u_hash_test(void);
bool u_config_bateria( char *s_mide_bateria );


// TKCTL
void ctl_watchdog_kick(uint8_t taskWdg, uint16_t timeout_in_secs );
uint16_t ctl_readTimeToNextPoll(void);
void ctl_reload_timerPoll( uint16_t new_time );
bool ctl_terminal_connected(void);
uint32_t ctl_read_timeToNextDial(void);
void ctl_set_timeToNextDial( uint32_t new_time );

// DINPUTS
void dinputs_setup(void);
void dinputs_init( void );
void dinputs_config_defaults(void);
bool dinputs_config_channel( uint8_t channel,char *s_aname ,char *s_tmodo );
void dinputs_clear(void);
bool dinputs_read(uint16_t dst[]);
void dinputs_print(file_descriptor_t fd, uint16_t src[] );
uint8_t dinputs_hash(void);
bool dinputs_service_read(void);

// COUNTERS
void counters_setup(void);
void counters_init(void);
void counters_config_defaults(void);
bool counters_config_channel( uint8_t channel,char *s_name, char *s_magpp, char *s_pw, char *s_period, char *s_speed, char *s_sensing );
bool counters_config_hw( char *s_type );
void counters_clear(void);
void counters_read(float cnt[]);
void counters_print(file_descriptor_t fd, float cnt[] );
uint8_t counters_hash(void);
bool counters_running;

// RANGE
void range_init(void);
bool range_config ( char *s_name );
void range_config_defaults(void);
bool range_read( int16_t *range );
void range_print(file_descriptor_t fd, uint16_t src );
uint8_t range_hash(void);

// PSENSOR
void psensor_init(void);
bool psensor_config ( char *s_pname, char *s_countMin, char *s_countMax, char *s_pmin, char *s_pmax , char *s_offset );
void psensor_config_defaults(void);
bool psensor_read( float *presion );
bool psensor_test_read (void);
void psensor_print(file_descriptor_t fd, float presion );
uint8_t psensor_hash(void);
bool psensor_config_autocalibrar( char *s_mag );

// TEMPSENSOR
void tempsensor_init(void);
bool tempsensor_read( float *tempC );
void tempsensor_test_read (void);
void tempsensor_print(file_descriptor_t fd, float temp );

// AINPUTS
void ainputs_init(void);
void ainputs_awake(void);
void ainputs_sleep(void);
bool ainputs_config_channel( uint8_t channel,char *s_aname,char *s_imin,char *s_imax,char *s_mmin,char *s_mmax,char *s_offset );
void ainputs_config_defaults(void);
void ainputs_config_timepwrsensor ( char *s_timepwrsensor );
bool ainputs_config_autocalibrar( char *s_channel, char *s_mag_val );
bool ainputs_config_ical( char *s_channel, char *s_ieqv );
bool ainputs_config_mcal( char *s_channel, char *s_point , char *s_mag );
bool ainputs_read( float ain[], float *battery );
void ainputs_print(file_descriptor_t fd, float src[] );
void ainputs_battery_print( file_descriptor_t fd, float battery );
bool ainputs_autocal_running(void);
uint8_t ainputs_hash(void);
void ainputs_test_channel( uint8_t io_channel);

// TKDATA
void data_read_inputs(st_dataRecord_t *dst, bool f_copy );
void data_print_inputs(file_descriptor_t fd, st_dataRecord_t *dr);

// MODBUS
void modbus_init(void);
bool modbus_config_slave_address( char *address);
bool modbus_config_channel(uint8_t channel,char *s_name,char *s_addr,char *s_length,char *s_rcode);
void modbus_config_defaults(void);
uint8_t modbus_hash(void);
bool modbus_poll( uint16_t mbus_in[] );
void modbus_print(file_descriptor_t fd, uint16_t mbus[] );
void modbus_status(void);
void modbus_wr_test( uint8_t modo, char* c_slave_address, char *c_function_code, char * c_start_address, char * c_nro_regs);
void modbus_rd_test(void);
void modbus_set_hr( bool f_debug, char* c_slave_address, char *c_function_code, char * c_start_address, char * c_values);

bool SPX_SIGNAL( uint8_t signal );
bool SPX_SEND_SIGNAL( uint8_t signal );
bool SPX_CLEAR_SIGNAL( uint8_t signal );

int32_t xcomms_time_to_next_dial(void);

// WATCHDOG
uint8_t wdg_resetCause;

#define WDG_CTL			0
#define WDG_CMD			1
#define WDG_DINPUTS		2
#define WDG_COMMS		3
#define WDG_COMMSRX		4
#define WDG_APP			5
#define WDG_AUXRX		6

#define NRO_WDGS		7
//#define NRO_WDGS		2

#define WDG_TO30		30
#define WDG_TO60		60
#define WDG_TO120		120
#define WDG_TO180	 	180
#define WDG_TO300		300
#define WDG_TO600		600
#define WDG_TO900		900

//------------------------------------------------------------------------


#endif /* SRC_SPX_H_ */
