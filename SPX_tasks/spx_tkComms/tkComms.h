/*
 * tkComms.h
 *
 *  Created on: 5 mar. 2020
 *      Author: pablo
 */

#ifndef SRC_SPX_TASKS_SPX_TKCOMMS_TKCOMMS_H_
#define SRC_SPX_TASKS_SPX_TKCOMMS_TKCOMMS_H_


#include "spx.h"

typedef enum { ST_ENTRY = 0, ST_ESPERA_APAGADO, ST_ESPERA_PRENDIDO, ST_PRENDER, ST_CONFIGURAR, ST_MON_SQE, ST_SCAN, ST_INITFRAME, ST_DATAFRAME } t_comms_states;
typedef enum { ERR_NONE = 0, ERR_CPIN_FAIL, ERR_CREG_FAIL, ERR_CPSI_FAIL, ERR_NETATTACH_FAIL, ERR_APN_FAIL, ERR_IPSERVER_FAIL, ERR_DLGID_FAIL } t_comms_error_code;
typedef enum { LINK_OPEN = 0, LINK_CLOSE, LINK_UNKNOWN } t_link_status;
typedef enum { NET_OPEN = 0, NET_CLOSE, NET_UNKNOWN } t_net_status;
typedef enum { SEND_OK = 0, SEND_FAIL, SEND_NODATA } t_send_status;
typedef enum { ATCMD_ENTRY = 0, ATCMD_TEST, ATCMD_CMD, ATCMD_WAIT, ATCMD_EXIT } atcmd_state_t;

typedef enum { INIT_AUTH = 0, INIT_SRVUPDATE, INIT_GLOBAL, INIT_BASE, INIT_ANALOG, INIT_DIGITAL, INIT_COUNTERS, INIT_RANGE, INIT_PSENSOR, INIT_APP_A, INIT_APP_B, INIT_APP_C, INIT_MODBUS, DATA, SCAN } t_frame;
typedef enum { frame_ENTRY = 0, frame_RESPONSE, frame_NET } t_frame_states;
typedef enum { rsp_OK = 0, rsp_ERROR, rsp_NONE } t_responses;

#define MAX_TRIES_PWRON 		3	// Intentos de prender HW el modem
#define MAX_TRYES_NET_ATTCH		3	// Intentos de atachearme a la red GPRS
#define MAX_TRYES_OPEN_COMMLINK	3	// Intentos de abrir un socket
#define MAX_RCDS_WINDOW_SIZE	10	// Maximos registros enviados en un bulk de datos

#define AUX_RXBUFFER_LEN	 32

#define MAX_XCOMM_TO_TIMER	180

#define SIMPIN_DEFAULT	"1234\0"

#define DF_COMMS ( systemVars.debug == DEBUG_COMMS )

#define TDIAL_MIN_DISCRETO 300

#define MODO_DISCRETO ( (sVarsComms.timerDial >= TDIAL_MIN_DISCRETO ) ? true : false )

#define INTER_FRAMES_DELAY	100

int32_t time_to_next_dial;

t_comms_states tkComms_state;

typedef struct {
	uint8_t csq;
	char ip_assigned[IP_LENGTH];
	bool gprs_prendido;
	bool gprs_inicializado;
	uint8_t errores_comms;
} t_xCOMMS_stateVars;

t_xCOMMS_stateVars xCOMMS_stateVars;

#define MAX_ERRORES_COMMS 5

typedef struct {
	char dlgId[DLGID_LENGTH];
	char apn[APN_LENGTH];
	char server_tcp_port[PORT_LENGTH];
	char server_ip_address[IP_LENGTH];
	char serverScript[SCRIPT_LENGTH];
	char simpwd[SIM_PASSWD_LENGTH];
	uint32_t timerDial;
	st_pwrsave_t pwrSave;
} xComms_conf_t;

xComms_conf_t sVarsComms;

#define SMS_NRO_LENGTH			10
#define SMS_MSG_LENGTH 			70
#define SMS_MSG_QUEUE_LENGTH 	9

#define WDG_COMMS_TO_PROCESSFRAME	WDG_TO300

typedef struct {
	char nro[SMS_NRO_LENGTH];
	char msg[SMS_MSG_LENGTH];
} t_sms;

t_comms_states tkComms_st_entry(void);
t_comms_states tkComms_st_espera_apagado(void);
t_comms_states tkComms_st_espera_prendido(void);
t_comms_states tkComms_st_prender(void);
t_comms_states tkComms_st_configurar(void);
t_comms_states tkComms_st_mon_sqe(void);
t_comms_states tkComms_st_scan(void);
t_comms_states tkComms_st_initframe(void);
t_comms_states tkComms_st_dataframe(void);

void xCOMMS_init(void);
void xCOMMS_apagar_dispositivo(void);
bool xCOMMS_prender_dispositivo(void);
bool xCOMMS_configurar_dispositivo( char *pin, char *apn, uint8_t *err_code );

bool xCOMMS_need_scan( void );
bool xCOMMS_scan(void);
bool xCOMMS_scan_try ( PGM_P *dlist );

void xCOMMS_mon_sqe( bool modo_continuo, uint8_t *csq );

bool xCOMMS_ipaddr( char *ip_assigned );

void xCOMMS_flush_RX(void);
void xCOMMS_flush_TX(void);
int xCOMMS_check_response( uint16_t start, const char *pattern );
void xCOMMS_print_RX_buffer(void);

void xCOMMS_rxbuffer_copy_to(char *dst, uint16_t start, uint16_t size );

bool xCOMMS_SGN_FRAME_READY(void);
bool xCOMMS_SGN_REDIAL(void);

bool xCOMMS_netstatus(t_net_status *net_status);
bool xCOMMS_netopen(void);
bool xCOMMS_netclose(void);
bool xCOMMS_linkopen(char *ip, char *port);
bool xCOMMS_linkclose(void);
bool xCOMMS_linkstatus(t_link_status *link_status);

uint16_t xCOMMS_datos_para_transmitir(void);
bool xCOMMS_process_frame (t_frame tipo_frame, char *dst_ip, char *dst_port );
t_net_status fsm_NET(void);
t_link_status fsm_LINK(char *dst_ip, char *dst_port);
t_send_status fsm_SEND( t_frame tipo_frame );
bool fsm_RECEIVE( t_frame tipo_frame );


t_send_status xINIT_FRAME_send(t_frame tipo_frame );
t_responses xINIT_FRAME_process_response(void);

t_send_status xDATA_FRAME_send(void);
t_responses xDATA_FRAME_process_response(void);

t_send_status xSCAN_FRAME_send(void);
t_responses xSCAN_FRAME_process_response(void);

#define XCOMMS_BUFFER_SIZE        300U

struct xcomms_buff_st {
	char txbuff[XCOMMS_BUFFER_SIZE];
	uint16_t ptr;
};

struct xcomms_buff_st xcomms_buff;
int xCOMMS_xbuffer_send( bool dflag );
//int xCOMMS_xbuffer_load_P( PGM_P fmt, ...);
void xCOMMS_xbuffer_init(void);
void xCOMMS_xbuffer_load_dataRecord(void);

//------------------------------------------------------------------------------------

void gprs_rxbuffer_reset(void);
bool gprs_rxbuffer_full(void);
bool gprs_rxbuffer_empty(void);
uint16_t gprs_rxbuffer_usedspace(void);
void gprs_rxbuffer_advance_pointer(void);
void gprs_rxbuffer_retreat_pointer(void);
void gprs_rxbuffer_put( char data);
bool gprs_rxbuffer_put2( char data );
bool gprs_rxbuffer_get( char * data );
void gprs_flush_RX_buffer(void);
void gprs_flush_TX_buffer(void);

void gprs_print_RX_buffer(void);
int gprs_check_response( uint16_t start, const char *rsp );
int gprs_check_response_with_to( uint16_t start, const char *rsp, uint8_t timeout );
char *gprs_get_buffer_ptr( char *pattern);
void gprs_rxbuffer_copy_to( char *dst, uint16_t start, uint16_t size );

//------------------------------------------------------------------------------------

void gprs_atcmd_preamble(void);
void gprs_init(void);
bool gprs_prender( void );
void gprs_hw_pwr_on(uint8_t delay_factor);
void gprs_sw_pwr(void);
void gprs_apagar(void);
char *gprs_get_imei(void);
char  *gprs_get_ccid(void);
bool gprs_configurar_dispositivo( char *pin, char *apn, uint8_t *err_code );
void gprs_mon_sqe( bool forever, uint8_t *csq);
bool gprs_IPADDR( char *ip_assigned );
bool gprs_SAT_set(uint8_t modo);
bool gprs_cmd_netstatus(t_net_status *net_status);
bool gprs_cmd_netopen(void);
bool gprs_cmd_netclose(void);
bool gprs_cmd_linkopen(char *ip, char *port);
bool gprs_cmd_linkclose(void);
bool gprs_cmd_linkstatus(t_link_status *link_status);
int gprs_findstr_circular( uint16_t start, const char *rsp );
int gprs_findstr_lineal( uint16_t start, const char *rsp );

void xSMS_init(void);
bool xSMS_enqueue(char *dst_nbr, char *msg );
t_sms *xSMS_dequeue(void);
void xSMS_txcheckpoint(void);
bool xSMS_send( char *dst_nbr, char *msg );
char *xSMS_format(char *msg);
void xSMS_delete(void);
void xSMS_rxcheckpoint(void);
char *xSMS_read_and_delete_by_index( uint8_t msg_index );
bool xSMS_received( uint8_t *first_msg_index );
void xSMS_process( char *sms_msg);

void aux_init(void);
void aux_prender(void);
void aux_apagar(void);
void aux_rts_on(void);
void aux_rts_off(void);
void aux_rxBuffer_fill(char c);
void aux_flush_RX_buffer(void);
void aux_flush_TX_buffer(void);
void aux_print_RX_buffer( bool ascii_mode );
char *aux_get_buffer( void );
uint16_t aux_get_buffer_ptr( void );

#endif /* SRC_SPX_TASKS_SPX_TKCOMMS_TKCOMMS_H_ */
