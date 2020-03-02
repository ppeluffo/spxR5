/*
 * spx_tkGprs.h
 *
 *  Created on: 25 de oct. de 2017
 *      Author: pablo
 */

#ifndef SRC_SPXR3_TKGPRS_SPXR3_TKGPRS_H_
#define SRC_SPXR3_TKGPRS_SPXR3_TKGPRS_H_

#include "frtos-io.h"
#include "spx.h"

#define MAX_HW_TRIES_PWRON 		3	// Intentos de prender HW el modem
#define MAX_SW_TRIES_PWRON 		3	// Intentos de prender SW el modem
#define MAX_TRYES_NET_ATTCH		6	// Intentos de atachearme a la red GPRS
#define MAX_IP_QUERIES 			4	// Intentos de conseguir una IP
#define MAX_INIT_TRYES			4	// Intentos de procesar un frame de INIT
#define MAX_IPSCAN_TRYES		4	// Intentos de abrir un socket para descrubrir la IP en un SCAN
#define MAX_TRYES_OPEN_SOCKET	4 	// Intentos de abrir un socket
#define MAX_RCDS_WINDOW_SIZE	10	// Maximos registros enviados en un bulk de datos
#define MAX_TX_WINDOW_TRYES		4	// Intentos de enviar el mismo paquete de datos

#define SMS_NRO_LENGTH			10
#define SMS_MSG_LENGTH			70
#define SMS_MSG_QUEUE_LENGTH	9

#define CTRL_Z 26

#define SIMPIN_DEFAULT	"1234\0"

// Datos del buffer local de recepcion de datos del GPRS.
// Es del mismo tamanio que el ringBuffer asociado a la uart RX.
// Es lineal, no ringBuffer !!! ( para poder usar las funciones de busqueda de strings )
#define COMMS_RXBUFFER_LEN GPRS_RXSTORAGE_SIZE

//-------------------------------------------------------------------------------------
char xcomms_rx_buffer[COMMS_RXBUFFER_LEN];

typedef struct {
	char *buff;
	uint16_t ptr;
	uint16_t size;
} t_comms_rx_buffer;

t_comms_rx_buffer xCOMMS_RX_buffer;

//-------------------------------------------------------------------------------------


struct {
	char buffer[COMMS_RXBUFFER_LEN];
	uint16_t ptr;
} commsRxBuffer;

#define IMEIBUFFSIZE	24
char buff_gprs_imei[IMEIBUFFSIZE];
char buff_gprs_ccid[IMEIBUFFSIZE];

typedef enum { G_ESPERA_APAGADO = 0, G_PRENDER, G_CONFIGURAR, G_MON_SQE, G_SCAN_APN, G_GET_IP, G_SCAN_FRAME, G_INITS, G_DATA, G_DATA_AWAITING } t_gprs_states;
typedef enum { SOCK_CLOSED = 0, SOCK_OPEN, SOCK_ERROR, SOCK_FAIL } t_socket_status;
typedef enum { FRAME_ERROR = 0, FRAME_SOCK_CLOSE, FRAME_OK, FRAME_NOT_ALLOWED, FRAME_ERR404, FRAME_RETRY, FRAME_SRV_ERR } t_frame_responses;
typedef enum { INIT_FRAME_A = 0, INIT_FRAME_B, SCAN_FRAME } t_frames;

typedef enum { ST_STANDBY = 0, ST_INIT, ST_DATA } t_comms_states;

typedef enum { LINK_CLOSED = 0, LINK_OPEN, LINK_FAIL, LINK_ERROR } t_link_status;

typedef enum { SGN_RESET_DEVICE = 0 } t_comms_signals;

#define MAX_TRYES_OPEN_COMMLINK	4

void tkComms(void * pvParameters);
t_comms_states tkComms_st_standby(void);
t_comms_states tkComms_st_init(void);
t_comms_states tkComms_st_data(void);

t_link_status linklayer_check_commlink_status(void);

void u_comms_signal (t_comms_signals signal);

struct {
	bool modem_prendido;
	bool signal_redial;
	bool signal_frameReady;
	bool monitor_sqe;
	uint8_t state;
	uint8_t	dcd;
	//uint8_t inits_counter;
	uint8_t csq;
	uint8_t dbm;
	int32_t waiting_time;
	char dlg_ip_address[IP_LENGTH];
	char server_ip_address[IP_LENGTH];
	bool sms_for_tx;

} GPRS_stateVars;

#define bool_CONTINUAR	true
#define bool_RESTART	false

bool st_gprs_esperar_apagado(void);
bool st_gprs_prender(void);
bool st_gprs_configurar(void);
bool st_gprs_monitor_sqe(void);
bool st_gprs_scan_apn(void);
bool st_gprs_get_ip(void);
bool st_gprs_scan_frame(void);
bool st_gprs_inits(void);
bool st_gprs_data(void);

void u_gprs_rxbuffer_flush(void);
bool u_gprs_check_response( const char *rsp );
bool u_gprs_check_response_with_to( const char *rsp, uint8_t timeout );
void u_gprs_modem_pwr_off(void);
void u_gprs_modem_pwr_on(uint8_t pwr_time );
void u_gprs_modem_pwr_sw(void);
void u_gprs_flush_RX_buffer(void);
void u_gprs_flush_TX_buffer(void);
t_socket_status u_gprs_open_socket(void);
bool u_gprs_close_socket(void);
t_socket_status u_gprs_check_socket_status(void);
void u_gprs_print_RX_response(void);
void u_gprs_print_RX_Buffer(void);
void u_gprs_ask_sqe(void);
void u_gprs_redial(void);
void u_gprs_config_timerdial ( char *s_timerdial );
void u_gprs_modem_pwr_off(void);
void u_gprs_modem_pwr_on(uint8_t pwr_time );
void u_gprs_modem_pwr_sw(void);
void u_gprs_configPwrSave( char *s_modo, char *s_startTime, char *s_endTime);
bool u_gprs_modem_prendido(void);
void u_gprs_load_defaults( char *opt );
void u_gprs_init_pines(void);

void u_gprs_tx_header(char *type);
void u_gprs_tx_tail(void);

void gprs_set_apn(char *apn);
bool gprs_netopen(void);
void gprs_read_ip_assigned(void);


uint32_t u_gprs_read_timeToNextDial(void);
void u_gprs_set_timeToNextDial( uint32_t time_to_dial );

void u_sms_init(void);
bool u_sms_send(char *dst_nbr, char *msg );
void u_gprs_sms_txcheckpoint(void);
bool u_gprs_send_sms( char *dst_nbr, char *msg );
bool u_gprs_quick_send_sms( char *dst_nbr, char *msg );
char *u_format_date_sms(char *msg);

void u_gprs_sms_rxcheckpoint(void);
bool u_gprs_sms_received( uint8_t *first_msg_index );
char *u_gprs_read_and_delete_sms_by_index( uint8_t msg_index );
void u_gprs_process_sms( char *sms_msg);

bool u_gprs_link_up(void);


void xCOMMS_init( t_comms_rx_buffer *comms_buff, char *data_storage, uint16_t max_size );
void xCOMMS_send_header(char *type);
void xCOMMS_send_tail(void);
int xCOMMS_send_P( PGM_P fmt, ...);
void xCOMMS_send_dr(st_dataRecord_t *dr);
int xCOMMS_read(void);
t_link_status xCOMMS_open_link(void);
t_link_status xCOMMS_link_status(void);
bool xCOMMS_check_response( const char *rsp );

void xCOMMS_print_RX_buffer(void);
void xCOMMS_flush_RX(void);
void xCOMMS_flush_TX(void);

void xCOMMS_flush_commsRxBuffer(void);
char *xCOMMS_strstr( const char *pattern );

void gprs_flush_RX_buffer(void);
void gprs_flush_TX_buffer(void);
t_link_status gprs_check_socket_status(void);
t_link_status gprs_open_socket(void);
bool gprs_rxbuffer_poke(char c, t_comms_rx_buffer *comms_buff );
bool gprs_check_response( const char *rsp, t_comms_rx_buffer *comms_buff  );
void gprs_print_RX_Buffer(t_comms_rx_buffer *comms_buff);

void xbee_flush_RX_buffer(void);
void xbee_flush_TX_buffer(void);
t_link_status xbee_check_socket_status(void);
t_link_status xbee_open_socket(void);
bool xbee_rxbuffer_poke(char c, t_comms_rx_buffer *comms_buff );
bool xbee_check_response( const char *rsp, t_comms_rx_buffer *comms_buff  );
void xbee_print_RX_Buffer(t_comms_rx_buffer *comms_buff);

//------------------------------------------------------------------------------------


#endif /* SRC_SPXR3_TKGPRS_SPXR3_TKGPRS_H_ */
