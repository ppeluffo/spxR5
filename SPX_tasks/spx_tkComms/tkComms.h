/*
 * tkComms.h
 *
 *  Created on: 5 mar. 2020
 *      Author: pablo
 */

#ifndef SRC_SPX_TASKS_SPX_TKCOMMS_TKCOMMS_H_
#define SRC_SPX_TASKS_SPX_TKCOMMS_TKCOMMS_H_


#include "spx.h"

typedef enum { ST_ENTRY = 0, ST_ESPERA_APAGADO, ST_ESPERA_PRENDIDO, ST_PRENDER, ST_CONFIGURAR, ST_MON_SQE, ST_SCAN, ST_IP, ST_INITFRAME, ST_DATAFRAME } t_comms_states;
typedef enum { ERR_NONE = 0, ERR_CPIN_FAIL, ERR_NETATTACH_FAIL, ERR_APN_FAIL, ERR_IPSERVER_FAIL, ERR_DLGID_FAIL } t_comms_error_code;
typedef enum { LINK_CLOSED = 0, LINK_OPEN, LINK_FAIL, LINK_ERROR } t_link_status;
typedef enum { COMMS_CHANNEL_XBEE = 0, COMMS_CHANNEL_GPRS } t_comms_channel;

#define MAX_TRIES_PWRON 		3	// Intentos de prender HW el modem
#define MAX_TRYES_NET_ATTCH		6	// Intentos de atachearme a la red GPRS
#define MAX_TRYES_OPEN_COMMLINK	3	// Intentos de abrir un socket
#define MAX_RCDS_WINDOW_SIZE	10	// Maximos registros enviados en un bulk de datos

#define GPRS_RXBUFFER_LEN	512
#define AUX1_RXBUFFER_LEN	512

#define SIMPIN_DEFAULT	"1234\0"

#define DF_COMMS ( systemVars.debug == DEBUG_COMMS )

#define TDIAL_MIN_DISCRETO 900

#define MODO_DISCRETO ( (sVarsComms.timerDial >= TDIAL_MIN_DISCRETO ) ? true : false )

int32_t time_to_next_dial;

t_comms_states tkComms_state;

typedef struct {
	uint8_t csq;
	char ip_assigned[IP_LENGTH];
	bool dispositivo_prendido;
	bool dispositivo_inicializado;

} t_xCOMMS_stateVars;

t_xCOMMS_stateVars xCOMMS_stateVars;

typedef struct {
	bool f_debug;
	char *apn;
	char *server_ip;
	char *tcp_port;
	char *dlgid;
	char *cpin;
	char *script;
} t_scan_struct;

typedef struct {
	t_comms_channel comms_channel;
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
t_comms_states tkComms_st_ip(void);
t_comms_states tkComms_st_initframe(void);
t_comms_states tkComms_st_dataframe(void);

void xCOMMS_init(void);
file_descriptor_t xCOMMS_get_fd(void);
void xCOMMS_apagar_dispositivo(void);
bool xCOMMS_prender_dispositivo(bool f_debug, uint8_t delay_factor);
bool xCOMMS_configurar_dispositivo(bool f_debug, char *pin, uint8_t *err_code );
bool xCOMMS_scan( t_scan_struct scan_boundle );
bool xCOMMS_need_scan( t_scan_struct scan_boundle );
void xCOMMS_mon_sqe(bool f_debug,  bool modo_continuo, uint8_t *csq );
bool xCOMMS_ip(bool f_debug, char *apn, char *ip_assigned, uint8_t *err_code );
t_link_status xCOMMS_link_status(bool f_debug);
void xCOMMS_flush_RX(void);
void xCOMMS_flush_TX(void);
void xCOMMS_send_header(char *type);
void xCOMMS_send_tail(void);
t_link_status xCOMMS_open_link(bool f_debug, char *ip, char *port);
void xCOMM_send_global_params(void);
bool xCOMMS_check_response( const char *pattern );
void xCOMMS_print_RX_buffer(bool d_flag );
char *xCOMM_get_buffer_ptr( char *pattern);
void xCOMMS_send_dr(bool d_flag, st_dataRecord_t *dr);
bool xCOMMS_procesar_senales( t_comms_states state, t_comms_states *next_state );
uint16_t xCOMMS_datos_para_transmitir(void);

void aux1_init(void);
void aux1_rxBuffer_fill(char c);
void aux1_flush_RX_buffer(void);
void aux1_flush_TX_buffer(void);
void aux1_print_RX_buffer(void);
bool aux1_check_response( const char *rsp );
bool aux1_prender( bool debug, uint8_t delay_factor );
void aux1_apagar(void);
bool aux1_configurar_dispositivo( uint8_t *err_code );
void aux1_mon_sqe( void );
bool aux1_scan( t_scan_struct scan_boundle );
bool aux1_need_scan( t_scan_struct scan_boundle );
bool aux1_ip( void );
t_link_status aux1_check_socket_status(bool f_debug);
t_link_status aux1_open_socket(bool f_debug, char *ip, char *port);
char *aux1_get_pattern_in_buffer( char *pattern);
char *aux1_get_buffer_ptr(void);

void gprs_init(void);
void gprs_rxBuffer_fill(char c);
void gprs_flush_RX_buffer(void);
void gprs_flush_TX_buffer(void);
void gprs_print_RX_buffer(bool f_debug );
bool gprs_check_response( const char *rsp );
bool gprs_check_response_with_to( const char *rsp, uint8_t timeout );
bool gprs_prender(bool f_debug, uint8_t delay_factor );
void gprs_hw_pwr_on(uint8_t delay_factor);
void gprs_sw_pwr(void);
void gprs_apagar(void);
void gprs_readImei(bool f_debug);
char *gprs_get_imei(void);
void gprs_readCcid(bool f_debug);
char  *gprs_get_ccid(void);
bool gprs_configurar_dispositivo( bool f_debug, char *pin, uint8_t *err_code );
bool gprs_CPIN(  bool f_debug, char *pin );
bool gprs_CGREG( bool f_debug );
uint8_t gprs_read_sqe( bool f_debug );
bool gprs_CGATT(bool f_debug);
void gprs_CIPMODE(bool f_debug);
void gprs_DCDMODE( bool f_debug );
void gprs_CMGF( bool f_debug );
void gprs_CFGRI (bool f_debug);
void gprs_mon_sqe( bool f_debug,  bool modo_continuo, uint8_t *csq);
bool gprs_scan( t_scan_struct scan_boundle );
bool gprs_need_scan( t_scan_struct scan_boundle );
bool gprs_ip(bool f_debug, char *apn, char *ip_assigned, uint8_t *err_code );
void gprs_set_apn(bool f_debug, char *apn);
bool gprs_netopen(bool f_debug);
bool gprs_read_ip_assigned(bool f_debug, char *ip_assigned );
t_link_status gprs_check_socket_status(bool f_debug);
t_link_status gprs_open_socket(bool f_debug, char *ip, char *port);
char *gprs_get_pattern_in_buffer( char *pattern);
char *gprs_get_buffer_ptr(void);
//void gprs_test(void);
//void gprs_scan_test (PGM_P *dlist );

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


#endif /* SRC_SPX_TASKS_SPX_TKCOMMS_TKCOMMS_H_ */
