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


#define MAX_TRIES_PWRON 		3	// Intentos de prender HW el modem
#define MAX_TRYES_NET_ATTCH		6	// Intentos de atachearme a la red GPRS

#define GPRS_RXBUFFER_LEN	512
#define XBEE_RXBUFFER_LEN	512

#define SIMPIN_DEFAULT	"1234\0"

#define DF_COMMS ( systemVars.debug == DEBUG_COMMS )

t_comms_states tkComms_st_entry(void);
t_comms_states tkComms_st_espera_apagado(void);
t_comms_states tkComms_st_espera_prendido(void);
t_comms_states tkComms_st_prender(void);
t_comms_states tkComms_st_configurar(void);
t_comms_states tkComms_st_mon_sqe(void);
t_comms_states tkComms_st_scan(void);
t_comms_states tkComms_st_ip(void);
t_comms_states tkComms_st_initframe(void);

void xCOMMS_init(void);
void xCOMMS_apagar_dispositivo(void);
bool xCOMMS_prender_dispositivo(bool f_debug, uint8_t delay_factor);
bool xCOMMS_configurar_dispositivo(bool f_debug, char *pin, uint8_t *err_code );
bool xCOMMS_scan(bool f_debug, char *apn, char *ip_server, char *dlgid, uint8_t *err_code );
void xCOMMS_mon_sqe(bool f_debug,  bool modo_continuo, uint8_t *csq );
bool xCOMMS_ip(bool f_debug, char *apn, char *ip_assigned, uint8_t *err_code );

void xbee_init(void);
void xbee_rxBuffer_fill(char c);
void xbee_flush_RX_buffer(void);
void xbee_flush_TX_buffer(void);
void xbee_print_RX_buffer(void);
bool xbee_check_response( const char *rsp );
bool xbee_prender( bool debug, uint8_t delay_factor );
void xbee_apagar(void);
bool xbee_configurar_dispositivo( uint8_t *err_code );
void xbee_mon_sqe( void );
bool xbee_scan(bool f_debug,char *ip_server, char *dlgid, uint8_t *err_code );
bool xbee_ip( void );

void gprs_init(void);
void gprs_rxBuffer_fill(char c);
void gprs_flush_RX_buffer(void);
void gprs_flush_TX_buffer(void);
void gprs_print_RX_buffer(void);
bool gprs_check_response( const char *rsp );
bool gprs_prender(bool f_debug, uint8_t delay_factor );
void gprs_hw_pwr_on(uint8_t delay_factor);
void gprs_sw_pwr(void);
void gprs_apagar(void);
void gprs_readImei(bool f_debug);
void gprs_readCcid(bool f_debug);
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
bool gprs_scan(bool f_debug, char *apn, char *ip_server, char *dlgid, uint8_t *err_code );
bool gprs_ip(bool f_debug, char *apn, char *ip_assigned, uint8_t *err_code );
void gprs_set_apn(bool f_debug, char *apn);
bool gprs_netopen(bool f_debug);
void gprs_read_ip_assigned(bool f_debug, char *ip_assigned );


t_comms_states tkComms_state;

typedef struct {
	uint8_t csq;
	char ip_assigned[IP_LENGTH];

} t_xCOMMS_stateVars;

t_xCOMMS_stateVars xCOMMS_stateVars;

#endif /* SRC_SPX_TASKS_SPX_TKCOMMS_TKCOMMS_H_ */
