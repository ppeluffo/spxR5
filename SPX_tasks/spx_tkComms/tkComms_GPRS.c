/*
 * tkComms_GPRS.c
 *
 *  Created on: 5 mar. 2020
 *      Author: pablo
 */

#include "tkComms.h"


typedef enum { prender_RXRDY = 0, prender_HW, prender_SW, prender_CPAS, prender_CFUN, prender_EXIT } t_states_fsm_prender_gprs;

#define MAX_SW_TRYES_PRENDER	3
#define MAX_HW_TRYES_PRENDER	3

#define MAX_TRYES_SOCKSETPN		3
#define MAX_TRYES_NETCLOSE		1
#define MAX_TRYES_NETOPEN		1

#define MAX_TRYES_SWITCHCMDMODE	3

#define TIMEOUT_PDP			30
#define TIMEOUT_NETCLOSE	120
#define TIMEOUT_NETOPEN		120
#define TIMEOUT_LINKCLOSE	120
#define TIMEOUT_LINKOPEN	120


#define TIMEOUT_SWITCHCMDMODE	30

bool gprs_ATCMD( bool testing_cmd, uint8_t cmd_tryes, uint8_t cmd_timeout, PGM_P *atcmdlist );
void gprs_CPOF( void );
void gprs_START_SIMULATOR(void);
bool gprs_PBDONE(void );
bool gprs_CFUN( void );
bool gprs_CPAS( void );
uint8_t gprs_CSQ( void );
bool gprs_CBC( void );
bool gprs_ATI( void );
bool gprs_IFC( void );
bool gprs_CIPMODE(void);
bool gprs_D2( void );
bool gprs_CSUART( void );
bool gprs_C1( void );
bool gprs_CPIN(  char *pin );
bool gprs_CCID( void );
bool gprs_CREG( void );
bool gprs_CPSI( void );
bool gprs_CGDCONT( char *apn);
bool gprs_CGATT( void );
bool gprs_CGATTQ( void );
bool gprs_CMGF( void );


void gprs_AT( void );


bool pv_get_token( char *p, char *buff, uint8_t size);
bool gprs_test_cmd(char *cmd);
bool gprs_test_cmd_P(PGM_P cmd);


#define MAX_TRYES_PBDONE	1
#define TIMEOUT_PBDONE		30
const char PBDONE_NAME[]  PROGMEM = "PBDONE";
const char PBDONE_TEST[]  PROGMEM = "";
const char PBDONE_CMD[]   PROGMEM = "";
const char PBDONE_RSPOK[] PROGMEM = "PB DONE";
PGM_P const AT_PBDONE[]   PROGMEM = { PBDONE_NAME, PBDONE_TEST, PBDONE_CMD, PBDONE_RSPOK };

#define MAX_TRYES_CPAS		3
#define TIMEOUT_CPAS		30
const char CPAS_NAME[]  PROGMEM = "CPAS";
const char CPAS_TEST[]  PROGMEM = "AT+CPAS=?";
const char CPAS_CMD[]   PROGMEM = "AT+CPAS";
const char CPAS_RSPOK[]	PROGMEM = "CPAS: 0";
PGM_P const AT_CPAS[]	PROGMEM = { CPAS_NAME, CPAS_TEST, CPAS_CMD, CPAS_RSPOK };

#define MAX_TRYES_CFUN		3
#define TIMEOUT_CFUN		30
const char CFUN_NAME[]  PROGMEM = "CFUN";
const char CFUN_TEST[]  PROGMEM = "AT+CFUN=?";
const char CFUN_CMD[]   PROGMEM = "AT+CFUN?";
const char CFUN_RSPOK[] PROGMEM = "CFUN: 1";
PGM_P const AT_CFUN[]   PROGMEM = { CFUN_NAME, CFUN_TEST, CFUN_CMD, CFUN_RSPOK };

#define MAX_TRYES_CSQ		1
#define TIMEOUT_CSQ			30
const char CSQ_NAME[]  PROGMEM = "CSQ";
const char CSQ_TEST[]  PROGMEM = "";
const char CSQ_CMD[]   PROGMEM = "AT+CSQ";
const char CSQ_RSPOK[] PROGMEM = "OK";
PGM_P const AT_CSQ[]   PROGMEM = { CSQ_NAME, CSQ_TEST, CSQ_CMD, CSQ_RSPOK };

#define MAX_TRYES_CBC		2
#define TIMEOUT_CBC			30
const char CBC_NAME[]  PROGMEM = "CBC";
const char CBC_TEST[]  PROGMEM = "AT+CBC=?";
const char CBC_CMD[]   PROGMEM = "AT+CBC";
const char CBC_RSPOK[] PROGMEM = "OK";
PGM_P const AT_CBC[]   PROGMEM = { CBC_NAME, CBC_TEST, CBC_CMD, CBC_RSPOK };

#define MAX_TRYES_ATI		2
#define TIMEOUT_ATI			30
const char ATI_NAME[]  PROGMEM = "ATI";
const char ATI_TEST[]  PROGMEM = "";
const char ATI_CMD[]   PROGMEM = "ATI";
const char ATI_RSPOK[] PROGMEM = "OK";
PGM_P const AT_ATI[]   PROGMEM = { ATI_NAME, ATI_TEST, ATI_CMD, ATI_RSPOK };

#define MAX_TRYES_IFC		2
#define TIMEOUT_IFC			30
const char IFC_NAME[]  PROGMEM = "IFC";
const char IFC_TEST[]  PROGMEM = "AT+IFC=?";
const char IFC_CMD[]   PROGMEM = "AT+IFC?";
const char IFC_RSPOK[] PROGMEM = "IFC: 0,0";
PGM_P const AT_IFC[]   PROGMEM = { IFC_NAME, IFC_TEST, IFC_CMD, IFC_RSPOK };

#define MAX_TRYES_CIPMODE		2
#define TIMEOUT_CIPMODE			30
const char CIPMODE_NAME[]  PROGMEM = "CIPMODE";
const char CIPMODE_TEST[]  PROGMEM = "AT+CIPMODE=?";
const char CIPMODE_CMD[]   PROGMEM = "AT+CIPMODE=1";
const char CIPMODE_RSPOK[] PROGMEM = "OK";
PGM_P const AT_CIPMODE[]  PROGMEM = { CIPMODE_NAME, CIPMODE_TEST, CIPMODE_CMD, CIPMODE_RSPOK };

#define MAX_TRYES_D2		1
#define TIMEOUT_D2			30
const char D2_NAME[]  PROGMEM = "&D2";
const char D2_TEST[]  PROGMEM = "";
const char D2_CMD[]   PROGMEM = "AT&D2";
const char D2_RSPOK[] PROGMEM = "OK";
PGM_P const AT_D2[]   PROGMEM = { D2_NAME, D2_TEST, D2_CMD, D2_RSPOK };

#define MAX_TRYES_CSUART	1
#define TIMEOUT_CSUART		30
const char CSUART_NAME[]  PROGMEM = "CSUART";
const char CSUART_TEST[]  PROGMEM = "AT+CSUART=?";
const char CSUART_CMD[]   PROGMEM = "AT+CSUART=1";
const char CSUART_RSPOK[] PROGMEM = "OK";
PGM_P const AT_CSUART[]   PROGMEM = { CSUART_NAME, CSUART_TEST, CSUART_CMD, CSUART_RSPOK };

#define MAX_TRYES_C1		1
#define TIMEOUT_C1			30
const char C1_NAME[]  PROGMEM = "&C1";
const char C1_TEST[]  PROGMEM = "";
const char C1_CMD[]   PROGMEM = "AT&C1";
const char C1_RSPOK[] PROGMEM = "OK";
PGM_P const AT_C1[]   PROGMEM = { C1_NAME, C1_TEST, C1_CMD, C1_RSPOK };

#define MAX_TRYES_CPIN		3
#define TIMEOUT_CPIN		30
const char CPIN_NAME[]  PROGMEM = "CPIN";
const char CPIN_TEST[]  PROGMEM = "AT+CPIN=?";
const char CPIN_CMD[]   PROGMEM = "AT+CPIN?";
const char CPIN_RSPOK[] PROGMEM = "READY";
PGM_P const AT_CPIN[]   PROGMEM = { CPIN_NAME, CPIN_TEST, CPIN_CMD, CPIN_RSPOK };

#define MAX_TRYES_CCID		3
#define TIMEOUT_CCID		30
const char CCID_NAME[]  PROGMEM = "CCID";
const char CCID_TEST[]  PROGMEM = "AT+CCID=?";
const char CCID_CMD[]   PROGMEM = "AT+CCID?";
const char CCID_RSPOK[] PROGMEM = "OK";
PGM_P const AT_CCID[]   PROGMEM = { CCID_NAME, CCID_TEST, CCID_CMD, CCID_RSPOK };

#define MAX_TRYES_CREG		3
#define TIMEOUT_CREG		30
const char CREG_NAME[]  PROGMEM = "CREG";
const char CREG_TEST[]  PROGMEM = "AT+CREG=?";
const char CREG_CMD[]   PROGMEM = "AT+CREG?";
const char CREG_RSPOK[] PROGMEM = "CREG: 0,1";
PGM_P const AT_CREG[]   PROGMEM = { CREG_NAME, CREG_TEST, CREG_CMD, CREG_RSPOK };

#define MAX_TRYES_CPSI		3
#define TIMEOUT_CPSI		30
const char CPSI_NAME[]  PROGMEM = "CPSI";
const char CPSI_TEST[]  PROGMEM = "AT+CPSI=?";
const char CPSI_CMD[]   PROGMEM = "AT+CPSI?";
const char CPSI_RSPOK[] PROGMEM = "OK";
PGM_P const AT_CPSI[]   PROGMEM = { CPSI_NAME, CPSI_TEST, CPSI_CMD, CPSI_RSPOK };

#define MAX_TRYES_CGATT		3
#define TIMEOUT_CGATT		60
const char CGATT_NAME[]  PROGMEM = "CGATT";
const char CGATT_TEST[]  PROGMEM = "AT+CGATT=?";
const char CGATT_CMD[]   PROGMEM = "AT+CGATT=1";
const char CGATT_RSPOK[] PROGMEM = "OK";
PGM_P const AT_CGATT[]   PROGMEM = { CGATT_NAME, CGATT_TEST, CGATT_CMD, CGATT_RSPOK };

#define MAX_TRYES_CGATTQ	3
#define TIMEOUT_CGATTQ		60
const char CGATTQ_NAME[]  PROGMEM = "CGATTQ";
const char CGATTQ_TEST[]  PROGMEM = "AT+CGATT=?";
const char CGATTQ_CMD[]   PROGMEM = "AT+CGATT?";
const char CGATTQ_RSPOK[] PROGMEM = "CGATT: 1";
PGM_P const AT_CGATTQ[]   PROGMEM = { CGATTQ_NAME, CGATTQ_TEST, CGATTQ_CMD, CGATTQ_RSPOK };

#define MAX_TRYES_CMGF		1
#define TIMEOUT_CMGF		30
const char CMGF_NAME[]  PROGMEM = "CMGF";
const char CMGF_TEST[]  PROGMEM = "AT+CGATT=?";
const char CMGF_CMD[]   PROGMEM = "AT+CMGF=1";
const char CMGF_RSPOK[] PROGMEM = "OK";
PGM_P const AT_CMGF[]   PROGMEM = { CMGF_NAME, CMGF_TEST, CMGF_CMD, CMGF_RSPOK };

#define MAX_TRYES_IPADDR	1
#define TIMEOUT_IPADDR		30
const char IPADDR_NAME[]  PROGMEM = "IPADDR";
const char IPADDR_TEST[]  PROGMEM = "";
const char IPADDR_CMD[]   PROGMEM = "AT+IPADDR";
const char IPADDR_RSPOK[] PROGMEM = "OK";
PGM_P const AT_IPADDR[]   PROGMEM = { IPADDR_NAME, IPADDR_TEST, IPADDR_CMD, IPADDR_RSPOK };

#define MAX_TRYES_CGDCONT	3
#define TIMEOUT_CGDCONT		30

//------------------------------------------------------------------------------------

struct {
	char buffer[GPRS_RXBUFFER_LEN];
	uint16_t ptr;
} gprsRxBuffer;

#define IMEIBUFFSIZE	24
#define CCIDBUFFSIZE	24

struct {
	char buff_gprs_imei[IMEIBUFFSIZE];
	char buff_gprs_ccid[CCIDBUFFSIZE];
} gprs_status;

char cmd_name[10];
char cmd_test[15];
char cmd_at[15];
char cmd_rspok[15];

//------------------------------------------------------------------------------------
void gprs_atcmd_preamble(void)
{
	// Espera antes de c/comando. ( ver recomendaciones de TELIT )
	vTaskDelay( (portTickType)( 50 / portTICK_RATE_MS ) );
}
//------------------------------------------------------------------------------------
bool gprs_test_cmd_P(PGM_P cmd)
{
	gprs_flush_RX_buffer();
	gprs_flush_TX_buffer();
	gprs_atcmd_preamble();
	xfprintf_P( fdGPRS , PSTR("%s\r"), cmd );
	vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
	gprs_print_RX_buffer();
	if ( gprs_check_response("OK") ) {
		return(true);
	}
	return(false);
}
//------------------------------------------------------------------------------------
bool gprs_test_cmd(char *cmd)
{
	gprs_flush_RX_buffer();
	gprs_flush_TX_buffer();
	gprs_atcmd_preamble();
	xfprintf( fdGPRS , "%s\r", cmd);
	vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
	gprs_print_RX_buffer();
	if ( gprs_check_response("OK") ) {
		return(true);
	}
	return(false);
}
//------------------------------------------------------------------------------------
void gprs_init(void)
{
	// GPRS
	IO_config_GPRS_SW();
	IO_config_GPRS_PWR();
	IO_config_GPRS_RTS();
	IO_config_GPRS_CTS();
	IO_config_GPRS_DCD();
	IO_config_GPRS_RI();
	IO_config_GPRS_RX();
	IO_config_GPRS_TX();
	IO_config_GPRS_DTR();

	IO_set_GPRS_DTR();
	IO_set_GPRS_RTS();

	memset(gprs_status.buff_gprs_ccid, '\0', IMEIBUFFSIZE );
	memset(gprs_status.buff_gprs_ccid, '\0', IMEIBUFFSIZE );
}
//------------------------------------------------------------------------------------
void gprs_hw_pwr_on(uint8_t delay_factor)
{
	/*
	 * Prendo la fuente del modem y espero que se estabilize la fuente.
	 */

	IO_clr_GPRS_SW();	// GPRS=0V, PWR_ON pullup 1.8V )
	IO_set_GPRS_PWR();	// Prendo la fuente ( alimento al modem ) HW

	vTaskDelay( (portTickType)( ( 2000 + 2000 * delay_factor) / portTICK_RATE_MS ) );

}
//------------------------------------------------------------------------------------
void gprs_sw_pwr(void)
{
	/*
	 * Genera un pulso en la linea PWR_SW. Como tiene un FET la senal se invierte.
	 * En reposo debe la linea estar en 0 para que el fet flote y por un pull-up del modem
	 * la entrada PWR_SW esta en 1.
	 * El PWR_ON se pulsa a 0 saturando el fet.
	 */
	IO_set_GPRS_SW();	// GPRS_SW = 3V, PWR_ON = 0V.
	vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
	IO_clr_GPRS_SW();	// GPRS_SW = 0V, PWR_ON = pullup, 1.8V

}
//------------------------------------------------------------------------------------
void gprs_rxBuffer_fill(char c)
{
	/*
	 * Guarda el dato en el buffer LINEAL de operacion del GPRS
	 * Si hay lugar meto el dato.
	 */

	if ( gprsRxBuffer.ptr < GPRS_RXBUFFER_LEN )
		gprsRxBuffer.buffer[ gprsRxBuffer.ptr++ ] = c;
}
//------------------------------------------------------------------------------------
void gprs_flush_RX_buffer(void)
{

	frtos_ioctl( fdGPRS,ioctl_UART_CLEAR_RX_BUFFER, NULL);
	memset( gprsRxBuffer.buffer, '\0', GPRS_RXBUFFER_LEN);
	gprsRxBuffer.ptr = 0;
}
//------------------------------------------------------------------------------------
void gprs_flush_TX_buffer(void)
{
	frtos_ioctl( fdGPRS,ioctl_UART_CLEAR_TX_BUFFER, NULL);
}
//------------------------------------------------------------------------------------
void gprs_print_RX_buffer( void )
{

	if ( DF_COMMS ) {
		// Imprimo todo el buffer local de RX. Sale por \0.
		xprintf_P( PSTR ("GPRS: rxbuff>\r\n\0"));

		// Uso esta funcion para imprimir un buffer largo, mayor al que utiliza xprintf_P. !!!
		xnprint( gprsRxBuffer.buffer, GPRS_RXBUFFER_LEN );

		xprintf_P( PSTR ("\r\n[%d]\r\n\0"), gprsRxBuffer.ptr );
	}
}
//------------------------------------------------------------------------------------
bool gprs_check_response( const char *rsp )
{
	// Modifico para solo compara en mayusculas
bool retS = false;

	if ( strcasestr( gprsRxBuffer.buffer, rsp ) != NULL ) {
		retS = true;
	}
	return(retS);
}
//------------------------------------------------------------------------------------
bool gprs_check_response_with_to( const char *rsp, uint8_t timeout )
{
	// Espera una respuesta durante un tiempo dado.
	// Hay que tener cuidado que no expire el watchdog por eso lo kickeo aqui. !!!!

bool retS = false;

	while ( timeout > 0 ) {
		timeout--;
		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );

		// Veo si tengo la respuesta correcta.
		if ( strstr( gprsRxBuffer.buffer, rsp) != NULL ) {
			retS = true;
			break;
		}
	}

	return(retS);
}
//------------------------------------------------------------------------------------
char *gprs_get_buffer_ptr( char *pattern)
{

	return( strstr( gprsRxBuffer.buffer, pattern) );
}
//------------------------------------------------------------------------------------
void gprs_apagar(void)
{
	/*
	 * Apaga el dispositivo quitando la energia del mismo
	 *
	 */

	gprs_CPOF();

	IO_clr_GPRS_SW();	// Es un FET que lo dejo cortado
	vTaskDelay( (portTickType)( 100 / portTICK_RATE_MS ) );

	IO_clr_GPRS_PWR();	// Apago la fuente.
	vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );

}
//------------------------------------------------------------------------------------
void gprs_CPOF( void )
{
	// Apaga el modem en modo 'soft'

	xprintf_P( PSTR("COMMS: gprs CPOF\r\n"));
	gprs_flush_RX_buffer();
	gprs_flush_TX_buffer();
	gprs_atcmd_preamble();
	xfprintf_P( fdGPRS,PSTR("AT+CPOF\r\0"));
	vTaskDelay( (portTickType)( 5000 / portTICK_RATE_MS ) );
	xprintf_P( PSTR("COMMS: gprs CPOF OK.\r\n\0"));
	return;
}
//------------------------------------------------------------------------------------
bool gprs_prender( void )
{
	/*
	 * Prende el modem.
	 * Implementa la FSM descripta en el archivo FSM_gprs_prender.
	 *
	 * EL stado de actividad del teléfono se obtiene con + CPAS que indica el general actual de actividad del ME.
	 * El comando + CFUN se usa para configurar el ME a diferentes potencias y estados de consumo.
	 * El comando + CPIN se usa para ingresar las contraseñas ME que se necesitan
	 */

uint8_t state;
uint8_t sw_tryes;
uint8_t hw_tryes;
bool retS = false;

	state = prender_RXRDY;

	xprintf_PD( DF_COMMS, PSTR("COMMS: gprs Modem prendiendo...\r\n\0"));

	while(1) {

		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );

		switch (state ) {
		case prender_RXRDY:
			// Avisa a la tarea de rx que se despierte.
#ifdef BETA_TEST
			xprintf_PD( DF_COMMS, PSTR("COMMS: fsm_prender pwr_on RXDDY.\r\n\0"));
#endif
			// Aviso a la tarea de RX que se despierte ( para leer las respuestas del AT ) !!!
			while ( xTaskNotify( xHandle_tkCommsRX, SGN_WAKEUP , eSetBits ) != pdPASS ) {
				vTaskDelay( ( TickType_t)( 100 / portTICK_RATE_MS ) );
			}
			hw_tryes = 0;
			state = prender_HW;
			break;

		case prender_HW:
#ifdef BETA_TEST
			xprintf_PD( DF_COMMS, PSTR("COMMS: fsm_prender pwr_on HW(%d).\r\n\0"),hw_tryes);
#endif
			if ( hw_tryes >= MAX_HW_TRYES_PRENDER ) {
				state = prender_EXIT;
				retS = false;
			} else {
				hw_tryes++;
				sw_tryes = 0;
				// Prendo la fuente del modem (HW)
				gprs_hw_pwr_on(hw_tryes);
				state = prender_SW;
			}
			break;

		case prender_SW:
#ifdef BETA_TEST
			xprintf_PD( DF_COMMS, PSTR("COMMS: fsm_prender pwr_on SW(%d).\r\n\0"),sw_tryes);
#endif
			if ( sw_tryes >= MAX_SW_TRYES_PRENDER ) {
				// Apago el HW y espero
				gprs_apagar();
				vTaskDelay( (portTickType)( 10000 / portTICK_RATE_MS ) );
				state = prender_HW;
			} else {
				// Genero un pulso en el pin SW para prenderlo logicamente
				sw_tryes++;
				gprs_sw_pwr();

#ifdef BETA_TEST
				gprs_START_SIMULATOR();
#endif

				if ( gprs_PBDONE()) {
					// Paso a CFUN solo si respondio con PB DONE
					// Si no reintento en este estado
					// Espero un poco mas para asegurarme que todo esta bien.
					vTaskDelay( (portTickType)( 5000 / portTICK_RATE_MS ) );
					state = prender_CPAS;
				}
			}
			break;

		case prender_CPAS:
#ifdef BETA_TEST
			xprintf_PD( DF_COMMS, PSTR("COMMS: fsm_prender CPAS.\r\n\0"));
#endif
			if ( gprs_CPAS() ) {
				state = prender_CFUN;
				retS = true;
			} else {
				hw_tryes++;
				gprs_apagar();
				vTaskDelay( (portTickType)( 10000 / portTICK_RATE_MS ) );
				state = prender_HW;
			}
			break;

		case prender_CFUN:
#ifdef BETA_TEST
			xprintf_PD( DF_COMMS, PSTR("COMMS: fsm_prender CFUN.\r\n\0"));
#endif
			if ( gprs_CFUN() ) {
				state = prender_EXIT;
			} else {
				hw_tryes++;
				gprs_apagar();
				vTaskDelay( (portTickType)( 10000 / portTICK_RATE_MS ) );
				state = prender_HW;
			}
			break;

		case prender_EXIT:
			if ( retS ) {
				xprintf_PD( DF_COMMS, PSTR("COMMS: gprs Modem on.\r\n\0"));
			} else {
				xprintf_PD( DF_COMMS, PSTR("COMMS: ERROR gprs Modem NO prendio !!!.\r\n\0"));
			}
			goto EXIT;
			break;
		}
	}

EXIT:

	return(retS);
}
//------------------------------------------------------------------------------------
void gprs_START_SIMULATOR(void)
{

	gprs_flush_RX_buffer();
	gprs_flush_TX_buffer();
	xprintf_P( PSTR("GPRS: gprs START PWRSIM\r\n\0"));
	xfprintf_P( fdGPRS , PSTR("START_PWRSIM\r\n\0"));
}
//------------------------------------------------------------------------------------
void gprs_AT( void )
{
	// En caso que algun comando no responda, envio un solo AT y veo que pasa.
	// Esto me sirve para ver si el modem esta respondiendo o no.

	xprintf_P( PSTR("GPRS: gprs AT TESTING START >>>>>>>>>>\r\n\0"));
	gprs_atcmd_preamble();
	xfprintf_P( fdGPRS , PSTR("AT\r\0"));
	vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
	gprs_print_RX_buffer();
	xprintf_P( PSTR("GPRS: gprs AT TESTING END <<<<<<<<<<<\r\n\0"));

}
//------------------------------------------------------------------------------------
bool gprs_ATCMD( bool testing_cmd, uint8_t cmd_tryes, uint8_t cmd_timeout, PGM_P *atcmdlist )
{
	/*
	 * Doy el comando AT+CFUN? hasta 3 veces con timout de 10s esperando
	 * una respuesta OK/ERROR/Timeout
	 * La respuesta 1 indica: 1 – full functionality, online mode
	 * De acuerdo a QUECTEL Maximum Response Time 15s, determined by the network.
	 */

int8_t tryes;
int8_t timeout;
t_responses cmd_rsp = rsp_NONE;
atcmd_state_t state = ATCMD_ENTRY;

	strcpy_P( cmd_name, (PGM_P)pgm_read_word( &atcmdlist[0]));
	strcpy_P( cmd_test, (PGM_P)pgm_read_word( &atcmdlist[1]));
	strcpy_P( cmd_at, (PGM_P)pgm_read_word( &atcmdlist[2]));
	strcpy_P( cmd_rspok, (PGM_P)pgm_read_word( &atcmdlist[3]));

	//xprintf_PD( DF_COMMS, PSTR("COMMS: gprs_ATCMD TESTING 1 !!!:[%s], [%s]\r\n\0"), cmd_name, cmd_test );
	//return(false);

	while(1) {

		switch ( state ) {
		case ATCMD_ENTRY:
			xprintf_PD( DF_COMMS,  PSTR("COMMS: gprs %s.\r\n"), cmd_name);
			if ( testing_cmd ) {
				state = ATCMD_TEST;
			} else {
				tryes = 0;
				state = ATCMD_CMD;
			}
			break;

		case ATCMD_TEST:
			if ( gprs_test_cmd(cmd_test) == true) {
			//if ( gprs_test_cmd_P(PSTR("AT+CFUN=?\r\0")) == true) {
				tryes = 0;
				state = ATCMD_CMD;
			} else {
				xprintf_PD( DF_COMMS,  PSTR("COMMS: ERROR(TEST) gprs %s FAIL !!.\r\n\0"), cmd_name);
				return(false);
			}
			break;

		case ATCMD_CMD:
			if ( tryes == cmd_tryes ) {
				state = ATCMD_EXIT;
			} else {
				timeout = 0;
				tryes++;
				// Envio el comando
				gprs_flush_RX_buffer();
				gprs_flush_TX_buffer();
				xprintf_PD( DF_COMMS, PSTR("GPRS: gprs send %s (%d)\r\n\0"),cmd_name, tryes );
				gprs_atcmd_preamble();
				xfprintf_P( fdGPRS , PSTR("%s\r"), cmd_at );
				//xfprintf_P( fdGPRS , PSTR("%s\r"), at_cmd);
				state = ATCMD_WAIT;
			}
			break;

		case ATCMD_WAIT:
			vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
			++timeout;

			// TIMEOUT
			if ( timeout == cmd_timeout ) {
				gprs_print_RX_buffer();
				state = ATCMD_CMD;
				break;
			}

			// RSP_ERROR
			if ( gprs_check_response("ERROR") ) {
				// Salgo y reintento el comando.
				gprs_print_RX_buffer();
				vTaskDelay( (portTickType)( 5000 / portTICK_RATE_MS ) );
				cmd_rsp = rsp_ERROR;
				state = ATCMD_CMD;
				break;
			}

			// RSP_OK
			if ( gprs_check_response(cmd_rspok) ) {
			//if ( gprs_check_response("+CFUN: 1") ) {
				// Respuesta correcta. Salgo.
				cmd_rsp = rsp_OK;
				gprs_print_RX_buffer();
				xprintf_PD( DF_COMMS,  PSTR("COMMS: gprs %s OK en [%d] secs\r\n\0"), cmd_name, (( 10 * (tryes-1) ) + timeout) );
				return(true);
				break;
			}

			// RSP_NONE: Sigo en este estado esperando
			break;

		case ATCMD_EXIT:
			if ( cmd_rsp == rsp_ERROR ) {
				xprintf_PD( DF_COMMS,  PSTR("COMMS: ERROR gprs %s FAIL !!.\r\n\0"), cmd_name);
				return(false);
			}
			if ( cmd_rsp == rsp_NONE ) {
				xprintf_PD( DF_COMMS,  PSTR("COMMS: ERROR(TO) gprs %s !!.\r\n\0"), cmd_name);

			}
			return(false);

		default:
			return(false);
		}
	}
	return(false);
}
//------------------------------------------------------------------------------------
bool gprs_PBDONE(void)
{
	/*
	 * Espero por la linea PB DONE que indica que el modem se
	 * inicializo y termino el proceso de booteo.
	 */

bool retS;

	retS = gprs_ATCMD( false, MAX_TRYES_PBDONE , TIMEOUT_PBDONE, (PGM_P *)AT_PBDONE );
	return(retS);
}
 //------------------------------------------------------------------------------------
bool gprs_CPAS(void)
{

	 //  Nos devuelve el status de actividad del modem

bool retS;

	retS = gprs_ATCMD( true, MAX_TRYES_CPAS , TIMEOUT_CPAS, (PGM_P *)AT_CPAS );
	return(retS);

}
//------------------------------------------------------------------------------------
bool gprs_CFUN(void )
{
bool retS;

	retS = gprs_ATCMD( true, MAX_TRYES_CFUN , TIMEOUT_CFUN, (PGM_P *)AT_CFUN );
	return(retS);
}
//------------------------------------------------------------------------------------
void gprs_mon_sqe( bool forever, uint8_t *csq)
{

uint8_t timer = 10;

	// Recien despues de estar registrado puedo leer la calidad de señal.
	*csq = gprs_CSQ();

	if ( forever == false ) {
		return;
	}

	// Salgo por watchdog (900s) o reset
	while ( true ) {

		vTaskDelay( ( TickType_t)( 1000 / portTICK_RATE_MS ) );

		if ( --timer == 0) {
			// Expiro: monitoreo el SQE y recargo el timer a 10 segundos
			gprs_CSQ();
			timer = 10;
		}
	}
}
//------------------------------------------------------------------------------------
uint8_t gprs_CSQ( void )
{
	// Veo la calidad de senal que estoy recibiendo

char csqBuffer[32] = { 0 };
char *ts = NULL;
uint8_t csq = 0;
uint8_t dbm = 0;
bool retS;

	retS = gprs_ATCMD( false, MAX_TRYES_CSQ , TIMEOUT_CSQ, (PGM_P *)AT_CSQ );
	if ( retS ) {
		memcpy(csqBuffer, &gprsRxBuffer.buffer[0], sizeof(csqBuffer) );
		if ( (ts = strchr(csqBuffer, ':')) ) {
			ts++;
			csq = atoi(ts);
			dbm = 113 - 2 * csq;
			xprintf_PD( DF_COMMS,  PSTR("COMMS: csq=%d, DBM=%d\r\n\0"), csq, dbm );
		}
	}
	return(csq);
}
//------------------------------------------------------------------------------------
bool gprs_configurar_dispositivo( char *pin, char *apn, uint8_t *err_code )
{
	/*
	 * Consiste en enviar los comandos AT de modo que el modem GPRS
	 * quede disponible para trabajar
	 * Doy primero los comandos que no tienen que ver con el sim ni con la network
	 * Estos son lo que demoran mas y que si no responden debo abortar.
	 */

	// PASO 1: Configuro el modem. ( respuestas locales inmediatas )
	gprs_CBC();
	gprs_ATI();
	gprs_IFC();

	gprs_CIPMODE();	// modo transparente.
	gprs_D2();
	gprs_CSUART();
	gprs_C1();

	// PASO 2: Vemos que halla un SIM operativo.
	// AT+CPIN?
	// Vemos que halla un pin presente.
	if ( ! gprs_CPIN(pin) ) {
		*err_code = ERR_CPIN_FAIL;
		return(false);
	}

	//
	// AT+CCID
	if ( gprs_CCID() == false ) {
		xprintf_PD( DF_COMMS, PSTR("COMMS: gprs ERROR: CCID not available!!\r\n\0"));
		//gprs_apagar();
		//return (false );
	}

	// PASO 3: Comandos que dependen del estado de la red ( demoran )
	// NETWORK GSM/GPRS (base de trasmision de datos)
	// Vemos que el modem este registrado en la red. Pude demorar hasta 1 minuto ( CLARO )
	// Con esto estoy conectado a la red movil.
	if ( ! gprs_CREG() ) {
		*err_code = ERR_CREG_FAIL;
		return(false);
	}

	// Vemos que la red este operativa
	if ( ! gprs_CPSI() ) {
		*err_code = ERR_CPSI_FAIL;
		return(false);
	}

	// PASO 4: Comandos de conexion a la red de datos ( PDP )
	// Dependen de la red por lo que pueden demorar hasta 3 minutos
	// Debo atachearme a la red de paquetes de datos.
	// Configuro el APN ( PDP context)
	if ( ! gprs_CGDCONT( apn ) ) {
		*err_code = ERR_APN_FAIL;
		return(false);
	}

	// Activo el APN
	if ( ! gprs_CGATT()) {
		*err_code = ERR_NETATTACH_FAIL;
		return(false);
	}

	if ( ! gprs_CGATTQ()) {
		*err_code = ERR_NETATTACH_FAIL;
		return(false);
	}

	gprs_CMGF();		// Configuro para mandar SMS en modo TEXTO

	*err_code = ERR_NONE;
	return(true);
}
//------------------------------------------------------------------------------------
bool gprs_CBC( void )
{
	// Lee el voltaje del modem
	// Solo espero un OK.
	// Es de respuesta inmediata ( 100 ms )
bool retS;

	retS = gprs_ATCMD( true, MAX_TRYES_CBC , TIMEOUT_CBC, (PGM_P *)AT_CBC );
	return(retS);
}
//------------------------------------------------------------------------------------
bool gprs_ATI( void )
{
	// Mando el comando ATI que me sustituye al CGMM,CGMR, CGMI, IMEI
	// El IMEI se consigue con AT+CGSN y telit le da un timeout de 20s !!!
	// Si da error no tranca nada asi que intento una sola vez

bool retS;
char *p;

	retS = gprs_ATCMD( false, MAX_TRYES_ATI , TIMEOUT_ATI, (PGM_P *)AT_ATI );

	if ( retS ) {
		p = strstr(gprsRxBuffer.buffer,"IMEI:");
		retS = pv_get_token(p, gprs_status.buff_gprs_imei, IMEIBUFFSIZE );
		xprintf_PD( DF_COMMS,  PSTR("COMMS: IMEI [%s]\r\n\0"), gprs_status.buff_gprs_imei);
	}

	return(retS);
}
//------------------------------------------------------------------------------------
bool gprs_IFC( void )
{
	// Lee si tiene configurado control de flujo.
bool retS;

	retS = gprs_ATCMD( true, MAX_TRYES_IFC , TIMEOUT_IFC, (PGM_P *)AT_IFC );
	return(retS);
}
//------------------------------------------------------------------------------------
bool gprs_CIPMODE( void )
{
	// Funcion que configura el TCP/IP en modo transparente.

bool retS;

	retS = gprs_ATCMD( true, MAX_TRYES_CIPMODE , TIMEOUT_CIPMODE, (PGM_P *)AT_CIPMODE );
	return(retS);
}
//------------------------------------------------------------------------------------
bool gprs_D2( void )
{
bool retS;

	retS = gprs_ATCMD( false, MAX_TRYES_D2 , TIMEOUT_D2, (PGM_P *)AT_D2 );
	return(retS);

}
//------------------------------------------------------------------------------------
bool gprs_CSUART( void )
{
	// Funcion que configura la UART con 7 lineas ( DCD/RTS/CTS/DTR)

bool retS;

	retS = gprs_ATCMD( false, MAX_TRYES_CSUART , TIMEOUT_CSUART, (PGM_P *)AT_CSUART );
	return(retS);
}
//------------------------------------------------------------------------------------
bool gprs_C1( void )
{
bool retS;

	retS = gprs_ATCMD( false, MAX_TRYES_C1 , TIMEOUT_C1, (PGM_P *)AT_C1 );
	return(retS);
}
//------------------------------------------------------------------------------------
bool gprs_CPIN( char *pin )
{
	// Chequeo que el SIM este en condiciones de funcionar.
	// AT+CPIN?
	// No configuro el PIN !!!
	// TELIT le da un timeout de 20s.

bool retS;

	retS = gprs_ATCMD( true, MAX_TRYES_CPIN , TIMEOUT_CPIN, (PGM_P *)AT_CPIN );
	return(retS);

}
//------------------------------------------------------------------------------------
bool gprs_CCID( void )
{
	// Leo el ccid del sim para poder trasmitirlo al server y asi
	// llevar un control de donde esta c/sim
	// AT+CCID
	// +CCID: "8959801611637152574F"
	//
	// OK

bool retS;
char *p;

	retS = gprs_ATCMD( true, MAX_TRYES_CCID , TIMEOUT_CCID, (PGM_P *)AT_CCID );
	if ( retS ) {
		p = strstr(gprsRxBuffer.buffer,"CCID:");
		retS = pv_get_token(p, gprs_status.buff_gprs_ccid, CCIDBUFFSIZE );
		xprintf_PD( DF_COMMS,  PSTR("COMMS: CCID [%s]\r\n\0"), gprs_status.buff_gprs_ccid);
	}

	return(retS);
}
//------------------------------------------------------------------------------------
bool gprs_CREG( void )
{
	/* Chequeo que el TE este registrado en la red.
	 Esto deberia ser automatico.
	 Normalmente el SIM esta para que el dispositivo se registre automaticamente
	 Esto se puede ver con el comando AT+COPS? el cual tiene la red preferida y el modo 0
	 indica que esta para registrarse automaticamente.
	 Este comando se usa para de-registrar y registrar en la red.
	 Conviene dejarlo automatico de modo que si el TE se puede registrar lo va a hacer.
	 Solo chequeamos que este registrado con CGREG.
	 AT+CGREG?
	 +CGREG: 0,1
	 HAy casos como con los sims CLARO que puede llegar hasta 1 minuto a demorar conectarse
	 a la red, por eso esperamos mas.
	 En realidad el comando retorna en no mas de 5s, pero el registro puede demorar hasta 1 minuto
	 o mas, dependiendo de la calidad de señal ( antena ) y la red.
	 Para esto, el mon_sqe lo ponemos antes.
	*/

bool retS;

	retS = gprs_ATCMD( true, MAX_TRYES_CREG , TIMEOUT_CREG, (PGM_P *)AT_CREG );
	return(retS);

}
//------------------------------------------------------------------------------------
bool gprs_CPSI( void )
{
	// Chequeo que la red este operativa

bool retS;

	retS = gprs_ATCMD( true, MAX_TRYES_CPSI , TIMEOUT_CPSI, (PGM_P *)AT_CPSI );
	if (retS ) {
		if ( gprs_check_response("NO SERVICE") ) {
			// Sin servicio.
			retS = false;
		}
	}

	return(retS);
}
//------------------------------------------------------------------------------------
bool gprs_CGDCONT( char *apn)
{

uint8_t tryes;
int8_t timeout;
bool retS = false;
t_responses cmd_rsp = rsp_NONE;

	xprintf_PD( DF_COMMS, PSTR("COMMS: gprs CGDCONT (define PDP)\r\n\0") );
	//Defino el PDP indicando cual es el APN.
	tryes = 0;
	while (tryes++ <  MAX_TRYES_CGDCONT ) {
		gprs_flush_RX_buffer();
		gprs_flush_TX_buffer();
		xprintf_P(PSTR("GPRS: gprs send CGDCONT (%d)\r\n\0"),tryes);
		gprs_atcmd_preamble();
		// xfprintf_P( fdGPRS, PSTR("AT+CGSOCKCONT=1,\"IP\",\"%s\"\r\0"), apn);
		xfprintf_P( fdGPRS, PSTR("AT+CGDCONT=1,\"IP\",\"%s\"\r\0"), apn);

		timeout = 0;
		while ( timeout++ < TIMEOUT_CGDCONT ) {
			vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
			if ( gprs_check_response("OK") ) {
				cmd_rsp = rsp_OK;
				goto EXIT;
			}
			if ( gprs_check_response("ERROR") ) {
				cmd_rsp = rsp_ERROR;
				goto EXIT;
			}

			// Espero
		}

		gprs_print_RX_buffer();
		// Sali por timeout:
		if ( cmd_rsp == rsp_NONE) {
			gprs_AT();
		}
	}

EXIT:

	gprs_print_RX_buffer();

	switch(cmd_rsp) {
	case rsp_OK:
		xprintf_PD( DF_COMMS,  PSTR("COMMS: gprs CGDCONT OK en [%d] secs\r\n\0"), (( 10 * (tryes-1) ) + timeout) );
		retS = true;
		break;
	case rsp_ERROR:
		xprintf_PD( DF_COMMS,  PSTR("COMMS: ERROR gprs CGDCONT FAIL !!.\r\n\0"));
		retS = false;
		break;
	default:
		// Timeout
		gprs_AT();
		xprintf_PD( DF_COMMS,  PSTR("COMMS: ERROR(TO) gprs CGDCONT FAIL !!.\r\n\0"));
		retS = false;
	}

	return(retS);
}
//------------------------------------------------------------------------------------
bool gprs_CGATT( void )
{
	/* Una vez registrado, me atacheo a la red
	 AT+CGATT=1
	 AT+CGATT?
	 +CGATT: 1
	 Puede demorar mucho, hasta 75s. ( 1 min en CLARO )
	*/
bool retS;

	retS = gprs_ATCMD( true, MAX_TRYES_CGATT , TIMEOUT_CGATT, (PGM_P *)AT_CGATT );
	return(retS);
}
//------------------------------------------------------------------------------------
bool gprs_CGATTQ(void)
{
	// Consulto esperando que me diga que estoy atacheado a la red

bool retS;

	retS = gprs_ATCMD( true, MAX_TRYES_CGATTQ , TIMEOUT_CGATTQ, (PGM_P *)AT_CGATTQ );
	return(retS);
}
//------------------------------------------------------------------------------------
bool gprs_CMGF(void )
{
	// Configura para mandar SMS en modo texto
	// Telit le da un timeout de 5s
bool retS;

	retS = gprs_ATCMD( false, MAX_TRYES_CMGF , TIMEOUT_CMGF, (PGM_P *)AT_CMGF );
	return(retS);
}
//------------------------------------------------------------------------------------
char *gprs_get_imei(void)
{
	return( gprs_status.buff_gprs_imei );
}
//------------------------------------------------------------------------------------
char *gprs_get_ccid(void)
{
	return( gprs_status.buff_gprs_ccid );

}
//------------------------------------------------------------------------------------
bool pv_get_token( char *p, char *buff, uint8_t size)
{

uint8_t i = 0;
bool retS = false;
char c;

	if ( p == NULL ) {
		// No lo pude leer.
		retS = false;
		goto EXIT;
	}

	memset( buff, '\0', size);

	// Start. Busco el primer caracter en rxbuffer
	for (i=0; i<64; i++) {
		c = *p;
		if ( isdigit(c) ) {
			break;
		}
		if (i==63) {
			retS = false;
			goto EXIT;
		}
		p++;
	}

	// Copio hasta un \r
	for (i=0;i<size;i++) {
		c = *p;
		if (( c =='\r' ) || ( c == '"')) {
			retS = true;
			break;
		}
		buff[i] = c;
		p++;
	}


EXIT:

	return(retS);

}
//------------------------------------------------------------------------------------

t_net_status gprs_NETCLOSE(bool f_debug)
{
	// Paso a modo comando por las dudas

//t_responses cmd_rsp = rsp_NONE;
t_net_status net_status = NET_UNKNOWN;
uint8_t tryes;
int8_t timeout;

	xprintf_PD( f_debug,  PSTR("COMMS: gprs NETCLOSE\r\n\0"));
	tryes = 0;
	while (tryes++ <  MAX_TRYES_NETCLOSE ) {
		gprs_flush_RX_buffer();
		gprs_flush_TX_buffer();
		xprintf_P(PSTR("GPRS: gprs send NETCLOSE (%d)\r\n\0"),tryes);
		gprs_atcmd_preamble();
		xfprintf_P( fdGPRS,PSTR("AT+NETCLOSE\r\0"));

		timeout = 0;
		while ( timeout++ < TIMEOUT_NETCLOSE ) {

			vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );

			if ( gprs_check_response("Network closed")) {
				//cmd_rsp = rsp_OK;
				net_status = NET_CLOSE;
				goto EXIT;
			}

			if ( gprs_check_response("OK") ) {
				//cmd_rsp = rsp_OK;
				net_status = NET_CLOSE;
				goto EXIT;
			}

			if ( gprs_check_response("Network is already closed")) {
				//cmd_rsp = rsp_ERROR;
				net_status = NET_CLOSE;
				goto EXIT;
			}

			// +IP ERROR / +CME ERROR
			if ( gprs_check_response("ERROR") ) {
				//cmd_rsp = rsp_ERROR;
				net_status = NET_UNKNOWN;
				goto EXIT;
			}

			// Sin respuesta aun ( hasta 2 minutos.
		}

		// Sali por timeout
		gprs_print_RX_buffer();
	}

	xprintf_PD( f_debug,  PSTR("COMMS: ERROR(TO) gprs NETCLOSE !!.\r\n\0"));

EXIT:

	gprs_print_RX_buffer();
	if ( net_status == NET_CLOSE ) {
		xprintf_PD( f_debug,  PSTR("COMMS: gprs NETCLOSE OK en [%d] secs\r\n\0"), (( 10 * (tryes-1) ) + timeout) );
	} else {
		xprintf_PD( f_debug,  PSTR("COMMS: ERROR gprs NETCLOSE FAIL !!.\r\n\0"));
	}

	return(net_status);
}
//------------------------------------------------------------------------------------
t_net_status gprs_NETOPEN(bool f_debug)
{
	// Inicia el servicio de sockets abriendo un socket. ( no es una conexion sino un
	// socket local por el cual se va a comunicar. )
	// Activa el contexto y crea el socket local
	// La red asigna una IP.
	// Puede demorar unos segundos por lo que espero para chequear el resultado
	// y reintento varias veces.

//t_responses cmd_rsp = rsp_NONE;
t_net_status net_status = NET_UNKNOWN;
uint8_t tryes;
int8_t timeout;

	xprintf_PD( f_debug,  PSTR("COMMS: gprs NETOPEN\r\n\0"));
	tryes = 0;
	while (tryes++ <  MAX_TRYES_NETOPEN ) {

		gprs_flush_RX_buffer();
		gprs_flush_TX_buffer();
		xprintf_P(PSTR("GPRS: gprs send NETOPEN (%d)\r\n\0"),tryes);
		gprs_atcmd_preamble();
		xfprintf_P( fdGPRS,PSTR("AT+NETOPEN=\"TCP\"\r\0"));

		timeout = 0;
		while ( timeout++ < TIMEOUT_NETOPEN ) {
			vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );

			// Evaluo las respuestas del modem.
			if ( gprs_check_response("Connect ok")) {
				//cmd_rsp = rsp_OK;
				net_status = NET_OPEN;
				goto EXIT;
			}

			if ( gprs_check_response("Network opened")) {
				//cmd_rsp = rsp_ERROR;
				net_status = NET_OPEN;
				goto EXIT;
			}

			// +IP ERROR: Network is already opened
			if ( gprs_check_response("Network is already opened")) {
				//cmd_rsp = rsp_OK;
				net_status = NET_OPEN;
				goto EXIT;
			}

			// +IP ERROR: Network not opened
			if ( gprs_check_response("Network not opened")) {
				//cmd_rsp = rsp_ERROR;
				net_status = NET_CLOSE;
				goto EXIT;
			}

			// +IP ERROR / +CME ERROR
			if ( gprs_check_response("ERROR") ) {
				//cmd_rsp = rsp_ERROR;
				net_status = NET_UNKNOWN;
				goto EXIT;
			}

			// Espero
		}

		// Sali por timeout:
		gprs_print_RX_buffer();
	}

	xprintf_PD( f_debug,  PSTR("COMMS: ERROR(TO) gprs NETOPEN !!.\r\n\0"));

EXIT:

	// Muestro el resultado del comando (OK/ERR/TO)
	gprs_print_RX_buffer();

	if ( net_status == NET_OPEN ) {
		xprintf_PD( f_debug,  PSTR("COMMS: gprs NETOPEN OK en [%d] secs\r\n\0"), (( 10 * (tryes-1) ) + timeout) );
	} else {
		xprintf_PD( f_debug,  PSTR("COMMS: ERROR gprs NETOPEN FAIL !!.\r\n\0"));
	}

	// Retorno el resultado de la operacion
	return(net_status);

}
//------------------------------------------------------------------------------------
t_net_status gprs_NET_status( bool f_debug)
{
	// Consulto el estado del servicio. Solo espero 30s
	// Si me da timeout asumo que el servicio esta apagado


//t_responses cmd_rsp = rsp_NONE;
t_net_status net_status = NET_UNKNOWN;
uint8_t timeout;

	xprintf_PD( f_debug, PSTR("COMMS: gprs NETstatus.\r\n\0"));
	gprs_flush_RX_buffer();
	gprs_flush_TX_buffer();
	gprs_atcmd_preamble();
	xfprintf_P( fdGPRS, PSTR("AT+NETOPEN?\r\0"));
	timeout = 0;
	while ( timeout++ < TIMEOUT_NETOPEN ) {
		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );

		if ( gprs_check_response("OK")) {
			if ( gprs_check_response("NETOPEN: 0,0")) {
				//cmd_rsp = rsp_OK;
				net_status = NET_CLOSE;
				goto EXIT;
			}
			if ( gprs_check_response("NETOPEN: 1,0")) {
				//cmd_rsp = rsp_OK;
				net_status = NET_OPEN;
				goto EXIT;
			}

		} else if ( gprs_check_response("ERROR")) {
			//cmd_rsp = rsp_ERROR;
			net_status = NET_UNKNOWN;
			goto EXIT;
		}
		// Espero
	}

	// Sali por timeout:
	gprs_print_RX_buffer();
	xprintf_PD( f_debug,  PSTR("COMMS: ERROR(TO) gprs NETstatus !!.\r\n\0"));

EXIT:

	gprs_print_RX_buffer();

	switch(net_status) {
	case NET_OPEN:
		xprintf_PD( f_debug, PSTR("COMMS: gprs NETstatus is open.\r\n\0"));
		break;
	case NET_CLOSE:
		xprintf_PD( f_debug, PSTR("COMMS: gprs NETstatus is close.\r\n\0") );
		break;
	default:
		// UNKNOWN
		xprintf_PD( f_debug, PSTR("COMMS: gprs NETstatus is unknown.\r\n\0") );
		break;
	}

	return(net_status);

}
//------------------------------------------------------------------------------------
bool gprs_IPADDR( char *ip_assigned )
{

	/*
	 * Tengo la IP asignada: la leo para actualizar systemVars.ipaddress
	 * La respuesta normal seria del tipo:
	 * 		AT+IPADDR
	 * 		+IPADDR: 10.204.2.115
	 * Puede llegar a responder
	 * 		AT+IPADDR
	 * 		+IP ERROR: Network not opened
	 * lo que sirve para reintentar.
	 */

bool retS;
char *p;
char *ts = NULL;
char c = '\0';
char *ptr = NULL;

	strcpy(ip_assigned, "0.0.0.0\0");

	retS = gprs_ATCMD( false, MAX_TRYES_IPADDR , TIMEOUT_IPADDR, (PGM_P *)AT_IPADDR );
	if ( retS ) {
		ptr = ip_assigned;
		ts = strchr( gprsRxBuffer.buffer, ':');
		ts++;
		while ( (c= *ts) != '\r') {
			*ptr++ = c;
			ts++;
		}
		*ptr = '\0';
		xprintf_PD( DF_COMMS,  PSTR("COMMS: IPADDR [%s]\r\n\0"), ip_assigned );

	}
	return(retS);

}
//------------------------------------------------------------------------------------
t_link_status gprs_LINK_open(bool f_debug, char *ip, char *port)
{

	// only <link_num>=0 is allowed to operate with transparent mode.
	// Cuando el link queda abierto en modo transparente, lo que escribo en el puerto serial
	// se va por el link.
	// En caso de problemas da un timeout de 120s.

uint8_t timeout = 0;
t_link_status link_status = LINK_UNKNOWN;

	xprintf_PD( f_debug, PSTR("COMMS: gprs LINK open.\r\n\0"));

#ifdef MODEM_SIMULATOR
		break;
#endif

	// Intento abrir el socket una sola vez
	gprs_flush_RX_buffer();
	gprs_flush_TX_buffer();
	gprs_atcmd_preamble();
	xfprintf_P( fdGPRS, PSTR("AT+TCPCONNECT=\"%s\",%s\r\n\0"), ip, port);
	//xfprintf_P( fdGPRS, PSTR("AT+CIPOPEN=0,\"TCP\",\"%s\",%s\r\0"), ip, port);

	// Espero hasta 120s la respuesta
	timeout = 0;
	while ( timeout++ < TIMEOUT_LINKOPEN ) {

		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );

		if ( gprs_check_response("CONNECT 115200")) {
			link_status = LINK_OPEN;
			goto EXIT;

		} else if ( gprs_check_response("Connect ok")) {
			link_status = LINK_OPEN;
			goto EXIT;

		} else if ( gprs_check_response("OK")) {
			link_status = LINK_OPEN;
			goto EXIT;

		} else if ( gprs_check_response("Connection is already created")) {
			// Me respondio que la conexion esta abierta pero estoy en modo comando
			link_status = LINK_UNKNOWN;
			goto EXIT;

		} else if ( gprs_check_response("ERROR")) {
			link_status = LINK_UNKNOWN;
			goto EXIT;
		}
		// Espero
	}

	// Sali por timeout:
	gprs_print_RX_buffer();
	xprintf_PD( f_debug,  PSTR("COMMS: ERROR(TO) gprs LINK open !!.\r\n\0"));

EXIT:

	gprs_print_RX_buffer();
	if (link_status == LINK_OPEN ) {
		xprintf_PD( f_debug,  PSTR("COMMS: gprs LINK open OK en [%d] secs\r\n\0"), timeout );
	} else {
		xprintf_PD( f_debug,  PSTR("COMMS: ERROR gprs LINK open FAIL !!.\r\n\0"));
	}

	return(link_status);

}
//------------------------------------------------------------------------------------
t_link_status gprs_LINK_close(bool f_debug )
{

uint8_t timeout;
t_link_status link_status = LINK_UNKNOWN;

	xprintf_PD( f_debug, PSTR("COMMS: gprs LINK close.\r\n\0"));
	gprs_flush_RX_buffer();
	gprs_flush_TX_buffer();
	gprs_atcmd_preamble();
	//xfprintf_P( fdGPRS, PSTR("AT+TCPCLOSE\n\0"));
	xfprintf_P( fdGPRS, PSTR("AT+CIPCLOSE=0\r\0"));
	timeout = 0;
	while ( timeout++ < TIMEOUT_LINKCLOSE ) {
		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );

		if ( gprs_check_response("OK")) {
			link_status = LINK_CLOSE;
			goto EXIT;

		}

		if ( gprs_check_response("Operation not supported")) {
			// Puede dar +IP ERROR: Operation not supported lo que indica que esta cerrado.
			// Es cuando no se abrio nunca y por eso el cerrarlo no esta soportado
			link_status = LINK_UNKNOWN;
			goto EXIT;
		}

		if ( gprs_check_response("ERROR")) {
			link_status = LINK_UNKNOWN;
			goto EXIT;
		}
		// Espero
	}

	// Sali por timeout:
	gprs_print_RX_buffer();
	xprintf_PD( f_debug,  PSTR("COMMS: ERROR(TO) gprs LINK close !!.\r\n\0"));

EXIT:

	if (link_status == LINK_CLOSE ) {
		xprintf_PD( f_debug, PSTR("COMMS: gprs LINK close OK en [%d] secs\r\n\0"), timeout );
	} else {
		xprintf_PD( f_debug, PSTR("COMMS: WARN gprs LINK close FAIL !!\r\n\0") );
	}

	return(link_status);

}
//------------------------------------------------------------------------------------
t_link_status gprs_LINK_status(bool f_debug, bool dcd_mode )
{
	/* En el caso de un link GPRS, el socket puede estar abierto
	 * o no.
	 * El socket esta abierto si el modem esta prendido y
	 * el DCD esta en 0.
	 * Cuando el modem esta apagado pin_dcd = 0
	 * Cuando el modem esta prendido y el socket cerrado pin_dcd = 1
	 * Cuando el modem esta prendido y el socket abierto pin_dcd = 0.
	 */


uint8_t pin_dcd = 0;
t_link_status link_status = LINK_UNKNOWN;
uint8_t timeout = 0;


#ifdef MODEM_SIMULATOR
	pin_dcd = 0;
#endif

	if ( dcd_mode == true ) {
		// Hardware mode
		pin_dcd = IO_read_DCD();
		if (  pin_dcd == 0 ) {
			link_status = LINK_OPEN;
			xprintf_PD( f_debug, PSTR("COMMS: gprs LINK open. (dcd=%d)\r\n\0"),pin_dcd);
		} else {
			link_status = LINK_CLOSE;
			xprintf_PD( f_debug, PSTR("COMMS: gprs LINK close. (dcd=%d)\r\n\0"),pin_dcd);
		}
		return(link_status);

	} else {
		// Software mode
		gprs_atcmd_preamble();
		xfprintf_P( fdGPRS, PSTR("AT+CIPCLOSE?\r\0"));
		timeout = 0;
		while ( timeout++ < TIMEOUT_LINKCLOSE ) {
			vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );

			if ( gprs_check_response("OK")) {
				// link cerrado
				link_status = LINK_CLOSE;
				goto EXIT;
			}

			if ( gprs_check_response("CIPCLOSE: 0")) {
				// link cerrado
				link_status = LINK_CLOSE;
				goto EXIT;
			}

			if ( gprs_check_response("ERROR")) {
				link_status = LINK_UNKNOWN;
				goto EXIT;
			}
			// Await
		}
		// Sali por timeout:
		gprs_print_RX_buffer();
		xprintf_PD( f_debug,  PSTR("COMMS: ERROR(TO) gprs LINK status !!.\r\n\0"));
EXIT:

		if (link_status == LINK_CLOSE ) {
			xprintf_PD( f_debug, PSTR("COMMS: gprs LINK status OK (CLOSE) en [%d] secs\r\n\0"), timeout );
		} else {
			xprintf_PD( f_debug, PSTR("COMMS: WARN gprs LINK status FAIL (UNKNOWN) !!\r\n\0") );
		}

		return(link_status);
	}

	return(link_status);
}
//------------------------------------------------------------------------------------
bool gprs_SAT_set(uint8_t modo)
{
/*
 * Seguimos viendo que luego de algún CPIN se cuelga el modem y ya aunque lo apague, luego al encenderlo
 * no responde al PIN.
 * En https://www.libelium.com/forum/viewtopic.php?t=21623 reportan algo parecido.
 * https://en.wikipedia.org/wiki/SIM_Application_Toolkit
 * Parece que el problema es que al enviar algun comando al SIM, este interactua con el STK (algun menu ) y lo bloquea.
 * Hasta no conocer bien como se hace lo dejamos sin usar.
 * " la tarjeta SIM es un ordenador diminuto con sistema operativo y programa propios.
 *   STK responde a comandos externos, por ejemplo, al presionar un botón del menú del operador,
 *   y hace que el teléfono ejecute ciertas acciones
 * "
 * https://www.techopedia.com/definition/30501/sim-toolkit-stk
 *
 * El mensaje +STIN: 25 es un mensaje no solicitado que emite el PIN STK.
 *
 * Esta rutina lo que hace es interrogar al SIM para ver si tiene la funcion SAT habilitada
 * y dar el comando de deshabilitarla
 *
 */


	xprintf_P(PSTR("GPRS: gprs SAT.(modo=%d)\r\n\0"),modo);

	switch(modo) {
	case 0:
		// Disable
		gprs_flush_RX_buffer();
		gprs_flush_TX_buffer();
		gprs_atcmd_preamble();
		xfprintf_P( fdGPRS , PSTR("AT+STK=0\r\0"));
		vTaskDelay( (portTickType)( 5000 / portTICK_RATE_MS ) );
		break;
	case 1:
		// Enable
		gprs_flush_RX_buffer();
		gprs_flush_TX_buffer();
		gprs_atcmd_preamble();
		xfprintf_P( fdGPRS , PSTR("AT+STK=1\r\0"));
		vTaskDelay( (portTickType)( 5000 / portTICK_RATE_MS ) );
		break;
	case 2:
		// Check. Query STK status ?
		xprintf_P(PSTR("GPRS: query STK status ?\r\n\0"));
		gprs_flush_RX_buffer();
		gprs_flush_TX_buffer();
		gprs_atcmd_preamble();
		xfprintf_P( fdGPRS , PSTR("AT+STK?\r\0"));
		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
		gprs_print_RX_buffer();
		break;
	default:
		return(false);
	}

	return (true);

}
//------------------------------------------------------------------------------------
bool gprs_switch_to_command_mode(bool f_debug, bool verbose )
{
	/*
	 * Para switchear de data mode a command mode, DTR=0 al menos 1s.
	 */

//uint8_t tryes;
int8_t timeout;

	gprs_flush_RX_buffer();
	gprs_flush_TX_buffer();

	if ( verbose )
		xprintf_PD( f_debug,  PSTR("COMMS: back to CMD.\r\n\0"));

	gprs_atcmd_preamble();
	xfprintf_P( fdGPRS, PSTR("AT\r"));
	vTaskDelay( (portTickType)( 500 / portTICK_RATE_MS ) );
	if ( gprs_check_response("OK")) {
		if ( verbose )
			xprintf_PD( f_debug,  PSTR("COMMS: back to CMD OK.\r\n\0"));
		return(true);
	}

	// No esto en modo comando. Lo fuerzo.
	timeout = 0;
	while ( timeout++ < TIMEOUT_SWITCHCMDMODE ) {

		vTaskDelay( (portTickType)( 1500 / portTICK_RATE_MS ) );
		xfprintf_P( fdGPRS , PSTR("+++"));
		IO_clr_GPRS_DTR();
		vTaskDelay( (portTickType)( 1500 / portTICK_RATE_MS ) );
		IO_set_GPRS_DTR();
		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
		xfprintf_P( fdGPRS, PSTR("AT\r"));
		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
		if ( gprs_check_response("OK")) {
			if ( verbose )
				xprintf_PD( f_debug,  PSTR("COMMS: back to CMD OK en [%d] secs.\r\n\0"), timeout * 4);
			return(true);
		}
	}

	if ( verbose )
		xprintf_PD( f_debug,  PSTR("COMMS: back to CMD FAIL !!.\r\n\0"));

	return(false);

	/*
	tryes = 0;
	while (tryes++ <  MAX_TRYES_SWITCHCMDMODE ) {

		gprs_flush_RX_buffer();
		gprs_flush_TX_buffer();

		vTaskDelay( (portTickType)( 1500 / portTICK_RATE_MS ) );
		xfprintf_P( fdGPRS , PSTR("+++"));
		IO_clr_GPRS_DTR();
		vTaskDelay( (portTickType)( 1500 / portTICK_RATE_MS ) );
		IO_set_GPRS_DTR();

		timeout = 0;
		while ( timeout++ < TIMEOUT_NETOPEN ) {
			if ( gprs_check_response("OK")) {
				xprintf_PD( f_debug,  PSTR("COMMS: back to command mode OK.\r\n\0"));
				return;
			}
			vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
		}
	}
	xprintf_PD( f_debug,  PSTR("COMMS: back to command mode FAIL !!.\r\n\0"));
	return;
	*/

}
//------------------------------------------------------------------------------------
