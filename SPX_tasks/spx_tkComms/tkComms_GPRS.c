/*
 * tkComms_GPRS.c
 *
 *  Created on: 5 mar. 2020
 *      Author: pablo
 *
 * MS: Mobile Station ( modem )
 * SGSN: Nodo de soporte ( registro / autentificacion )
 * GGSN: Nodo gateway(router). Interfase de la red celular a la red de datos IP
 *
 * ATTACH: Proceso por el cual el MS se conecta al SGSN en una red GPRS
 * PDP Activation: Proceso por el cual se establece una sesion entre el MS y la red destino.
 * Primero hay que attachearse y luego activarse !!!
 *
 * 1- Verificar el CPIN
 *    Nos indica que el dispositivo puede usarse
 *
 * 2- CREG?
 *    Nos indica que el dispositivo esta registrado en la red celular, que tiene senal.
 *    Si no se registra: 1-verificar la antena, 2-verificar la banda
 *
 * 3- Solicitar informacion a la red.
 *
 * 4- Ver la calidad de senal (CSQ)
 *
 * 5- Atachearse a la red GPRS
 *    Se usa el comando AT+CGATT
 *    Atachearse no significa que pueda establecer un enlace de datos ya que la sesión queda identificada
 *    por el PDP context(APN).
 *    El PDP establece la relacion entre el dipositivo y el GGSN por lo tanto debemos establecer un
 *    contexto PDP antes de enviar/recibir datos
 *
 * 6- Definir el contexto.
 *    Si uso un dial-up uso el comando AT+CGDCONT
 *    Si uso el stack TCP/IP uso el comando AT+CGSOCKCONT
 *    Indico cual es el contexto por defecto:
 *    Indico la autentificacion requerida
 *
 * 7- Debo activar el contexto y la red IP me va a devolver una direccion IP.
 *    Como estoy usando el stack TCP/IP, esto automaticamente ( activacion, abrir un socket local y pedir una IP )
 *    lo hace el comado NETOPEN.
 *
 * 8- Para abrir un socket remoto usamos TCPCONNECT
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
bool gprs_ATE( void );
bool gprs_CSUART( void );
bool gprs_C1( void );
bool gprs_CGAUTH( void );
bool gprs_CSOCKAUTH( void );
bool gprs_CPIN(  char *pin );
bool gprs_CCID( void );
bool gprs_CREG( void );
bool gprs_CPSI( void );
bool gprs_CGDSOCKCONT( char *apn);
bool gprs_CGATT( void );
bool gprs_CGATTQ( void );
bool gprs_CMGF( void );
bool gprs_CSOCKSETPN( void );

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

// CPAS: Status de actividad del modem
#define MAX_TRYES_CPAS		3
#define TIMEOUT_CPAS		30
const char CPAS_NAME[]  PROGMEM = "CPAS";
const char CPAS_TEST[]  PROGMEM = "AT+CPAS=?";
const char CPAS_CMD[]   PROGMEM = "AT+CPAS";
const char CPAS_RSPOK[]	PROGMEM = "CPAS: 0";
PGM_P const AT_CPAS[]	PROGMEM = { CPAS_NAME, CPAS_TEST, CPAS_CMD, CPAS_RSPOK };

// CFUN: Consulta de las funcionalidades del modem.
#define MAX_TRYES_CFUN		3
#define TIMEOUT_CFUN		30
const char CFUN_NAME[]  PROGMEM = "CFUN";
const char CFUN_TEST[]  PROGMEM = "AT+CFUN=?";
const char CFUN_CMD[]   PROGMEM = "AT+CFUN?";
const char CFUN_RSPOK[] PROGMEM = "CFUN: 1";
PGM_P const AT_CFUN[]   PROGMEM = { CFUN_NAME, CFUN_TEST, CFUN_CMD, CFUN_RSPOK };

// CSQ: Calidad de senal.
#define MAX_TRYES_CSQ		1
#define TIMEOUT_CSQ			30
const char CSQ_NAME[]  PROGMEM = "CSQ";
const char CSQ_TEST[]  PROGMEM = "";
const char CSQ_CMD[]   PROGMEM = "AT+CSQ";
const char CSQ_RSPOK[] PROGMEM = "OK";
PGM_P const AT_CSQ[]   PROGMEM = { CSQ_NAME, CSQ_TEST, CSQ_CMD, CSQ_RSPOK };

// CBC: Voltaje de la bateria.
#define MAX_TRYES_CBC		2
#define TIMEOUT_CBC			30
const char CBC_NAME[]  PROGMEM = "CBC";
const char CBC_TEST[]  PROGMEM = "AT+CBC=?";
const char CBC_CMD[]   PROGMEM = "AT+CBC";
const char CBC_RSPOK[] PROGMEM = "OK";
PGM_P const AT_CBC[]   PROGMEM = { CBC_NAME, CBC_TEST, CBC_CMD, CBC_RSPOK };

// ATI: Identificaciones: fabricante,modelo, revision,imei.
#define MAX_TRYES_ATI		2
#define TIMEOUT_ATI			30
const char ATI_NAME[]  PROGMEM = "ATI";
const char ATI_TEST[]  PROGMEM = "";
const char ATI_CMD[]   PROGMEM = "ATI";
const char ATI_RSPOK[] PROGMEM = "OK";
PGM_P const AT_ATI[]   PROGMEM = { ATI_NAME, ATI_TEST, ATI_CMD, ATI_RSPOK };

// ATE: Echo
#define MAX_TRYES_ATE		1
#define TIMEOUT_ATE			30
const char ATE_NAME[]  PROGMEM = "ATE";
const char ATE_TEST[]  PROGMEM = "";
const char ATE_CMD[]   PROGMEM = "ATE0";
const char ATE_RSPOK[] PROGMEM = "OK";
PGM_P const AT_ATE[]   PROGMEM = { ATE_NAME, ATE_TEST, ATE_CMD, ATE_RSPOK };

// IFC: Seteo el control de flujo.(none)
#define MAX_TRYES_IFC		2
#define TIMEOUT_IFC			30
const char IFC_NAME[]  PROGMEM = "IFC";
const char IFC_TEST[]  PROGMEM = "AT+IFC=?";
const char IFC_CMD[]   PROGMEM = "AT+IFC?";
const char IFC_RSPOK[] PROGMEM = "IFC: 0,0";
PGM_P const AT_IFC[]   PROGMEM = { IFC_NAME, IFC_TEST, IFC_CMD, IFC_RSPOK };

// CIPMODE: Selecciono modo transparente en TCP/IP.
#define MAX_TRYES_CIPMODE		2
#define TIMEOUT_CIPMODE			30
const char CIPMODE_NAME[]  PROGMEM = "CIPMODE";
const char CIPMODE_TEST[]  PROGMEM = "AT+CIPMODE=?";
const char CIPMODE_CMD[]   PROGMEM = "AT+CIPMODE=0";
const char CIPMODE_RSPOK[] PROGMEM = "OK";
PGM_P const AT_CIPMODE[]  PROGMEM = { CIPMODE_NAME, CIPMODE_TEST, CIPMODE_CMD, CIPMODE_RSPOK };

// &D2: Como actua DTR ( pasar a modo comando )
#define MAX_TRYES_D2		1
#define TIMEOUT_D2			30
const char D2_NAME[]  PROGMEM = "&D2";
const char D2_TEST[]  PROGMEM = "";
const char D2_CMD[]   PROGMEM = "AT&D2";
const char D2_RSPOK[] PROGMEM = "OK";
PGM_P const AT_D2[]   PROGMEM = { D2_NAME, D2_TEST, D2_CMD, D2_RSPOK };

// CSUART: serial port de 7 lineas
#define MAX_TRYES_CSUART	1
#define TIMEOUT_CSUART		30
const char CSUART_NAME[]  PROGMEM = "CSUART";
const char CSUART_TEST[]  PROGMEM = "AT+CSUART=?";
const char CSUART_CMD[]   PROGMEM = "AT+CSUART=1";
const char CSUART_RSPOK[] PROGMEM = "OK";
PGM_P const AT_CSUART[]   PROGMEM = { CSUART_NAME, CSUART_TEST, CSUART_CMD, CSUART_RSPOK };

// &C1: DCD on cuando hay carrier.
#define MAX_TRYES_C1		1
#define TIMEOUT_C1			30
const char C1_NAME[]  PROGMEM = "&C1";
const char C1_TEST[]  PROGMEM = "";
const char C1_CMD[]   PROGMEM = "AT&C1";
const char C1_RSPOK[] PROGMEM = "OK";
PGM_P const AT_C1[]   PROGMEM = { C1_NAME, C1_TEST, C1_CMD, C1_RSPOK };

// CGAUTH: Autentificacion PAP
#define MAX_TRYES_CGAUTH	1
#define TIMEOUT_CGAUTH		10
const char CGAUTH_NAME[]  PROGMEM = "CGAUTH";
const char CGAUTH_TEST[]  PROGMEM = "AT+CGAUTH=?";
const char CGAUTH_CMD[]   PROGMEM = "AT+CGAUTH=1,1";
const char CGAUTH_RSPOK[] PROGMEM = "OK";
PGM_P const AT_CGAUTH[]   PROGMEM = { CGAUTH_NAME, CGAUTH_TEST, CGAUTH_CMD, CGAUTH_RSPOK };

// CSOCKAUTH: Autentificacion PAP
#define MAX_TRYES_CSOCKAUTH		1
#define TIMEOUT_CSOCKAUTH		10
const char CSOCKAUTH_NAME[]  PROGMEM = "CSOCKAUTH";
const char CSOCKAUTH_TEST[]  PROGMEM = "AT+CSOCKAUTH=?";
const char CSOCKAUTH_CMD[]   PROGMEM = "AT+CSOCKAUTH=1,1";
const char CSOCKAUTH_RSPOK[] PROGMEM = "OK";
PGM_P const AT_CSOCKAUTH[]   PROGMEM = { CSOCKAUTH_NAME, CSOCKAUTH_TEST, CSOCKAUTH_CMD, CSOCKAUTH_RSPOK };

// CPIN: SIM listo para operar.
#define MAX_TRYES_CPIN		3
#define TIMEOUT_CPIN		30
const char CPIN_NAME[]  PROGMEM = "CPIN";
const char CPIN_TEST[]  PROGMEM = "AT+CPIN=?";
const char CPIN_CMD[]   PROGMEM = "AT+CPIN?";
const char CPIN_RSPOK[] PROGMEM = "READY";
PGM_P const AT_CPIN[]   PROGMEM = { CPIN_NAME, CPIN_TEST, CPIN_CMD, CPIN_RSPOK };

// CCID: ??
#define MAX_TRYES_CCID		3
#define TIMEOUT_CCID		30
const char CCID_NAME[]  PROGMEM = "CCID";
const char CCID_TEST[]  PROGMEM = "AT+CCID=?";
const char CCID_CMD[]   PROGMEM = "AT+CCID";
const char CCID_RSPOK[] PROGMEM = "OK";
PGM_P const AT_CCID[]   PROGMEM = { CCID_NAME, CCID_TEST, CCID_CMD, CCID_RSPOK };

// CREG: Indica el estado de registro del modem en la red
#define MAX_TRYES_CREG		3
#define TIMEOUT_CREG		30
const char CREG_NAME[]  PROGMEM = "CREG";
const char CREG_TEST[]  PROGMEM = "AT+CREG=?";
const char CREG_CMD[]   PROGMEM = "AT+CREG?";
const char CREG_RSPOK[] PROGMEM = "CREG: 0,1";
PGM_P const AT_CREG[]   PROGMEM = { CREG_NAME, CREG_TEST, CREG_CMD, CREG_RSPOK };

// CPSI: Solicita informacion de la red en la que esta registrado.
#define MAX_TRYES_CPSI		3
#define TIMEOUT_CPSI		30
const char CPSI_NAME[]  PROGMEM = "CPSI";
const char CPSI_TEST[]  PROGMEM = "AT+CPSI=?";
const char CPSI_CMD[]   PROGMEM = "AT+CPSI?";
const char CPSI_RSPOK[] PROGMEM = "OK";
PGM_P const AT_CPSI[]   PROGMEM = { CPSI_NAME, CPSI_TEST, CPSI_CMD, CPSI_RSPOK };

// CGACT
// CGATT: Se atachea al servicio de paquetes.(PDP)
#define MAX_TRYES_CGATT		3
#define TIMEOUT_CGATT		60
const char CGATT_NAME[]  PROGMEM = "CGATT";
const char CGATT_TEST[]  PROGMEM = "AT+CGATT=?";
const char CGATT_CMD[]   PROGMEM = "AT+CGATT=1";
const char CGATT_RSPOK[] PROGMEM = "OK";
PGM_P const AT_CGATT[]   PROGMEM = { CGATT_NAME, CGATT_TEST, CGATT_CMD, CGATT_RSPOK };

// CGATT: Pregunta el estado de la conexion al servicio de paquetes PDP.
#define MAX_TRYES_CGATTQ	3
#define TIMEOUT_CGATTQ		60
const char CGATTQ_NAME[]  PROGMEM = "CGATTQ";
const char CGATTQ_TEST[]  PROGMEM = "AT+CGATT=?";
const char CGATTQ_CMD[]   PROGMEM = "AT+CGATT?";
const char CGATTQ_RSPOK[] PROGMEM = "CGATT: 1";
PGM_P const AT_CGATTQ[]   PROGMEM = { CGATTQ_NAME, CGATTQ_TEST, CGATTQ_CMD, CGATTQ_RSPOK };

// CSOCKSETPN
// CSOCKSETPN: Indica cual PDP usar
#define MAX_TRYES_CSOCKSETPN	1
#define TIMEOUT_CSOCKSETPN		10
const char CSOCKSETPN_NAME[]  PROGMEM = "CSOCKSETPN";
const char CSOCKSETPN_TEST[]  PROGMEM = "AT+CSOCKSETPN=?";
const char CSOCKSETPN_CMD[]   PROGMEM = "AT+CSOCKSETPN=1";
const char CSOCKSETPN_RSPOK[] PROGMEM = "OK";
PGM_P const AT_CSOCKSETPN[]   PROGMEM = { CSOCKSETPN_NAME, CSOCKSETPN_TEST, CSOCKSETPN_CMD, CSOCKSETPN_RSPOK };

// CMGF: Configura los SMS para enviarse en modo texto.
#define MAX_TRYES_CMGF		1
#define TIMEOUT_CMGF		30
const char CMGF_NAME[]  PROGMEM = "CMGF";
const char CMGF_TEST[]  PROGMEM = "AT+CGATT=?";
const char CMGF_CMD[]   PROGMEM = "AT+CMGF=1";
const char CMGF_RSPOK[] PROGMEM = "OK";
PGM_P const AT_CMGF[]   PROGMEM = { CMGF_NAME, CMGF_TEST, CMGF_CMD, CMGF_RSPOK };

// IPADDR: Pregunta la IP asignada.
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

#define GPRS_RX_LINEAL_BUFFER

#define GPRS_RXBUFFER_LEN	512
struct {
	char buffer[GPRS_RXBUFFER_LEN];
	uint16_t ptr;
	uint16_t head;
	uint16_t tail;
	uint16_t max; //of the buffer
	bool full;
} gprs_rxbuffer;

#define IMEIBUFFSIZE	24
#define CCIDBUFFSIZE	24

struct {
	char buff_gprs_imei[IMEIBUFFSIZE];
	char buff_gprs_ccid[CCIDBUFFSIZE];
} gprs_status;

char cmd_name[16];
char cmd_test[16];
char cmd_at[20];
char cmd_rspok[16];

//------------------------------------------------------------------------------------
// MANEJO DEL BUFFER DE RECEPCION DE GPRS ( Circular o Lineal )
// Lo alimenta la rx interrupt del puerto gprs.
//------------------------------------------------------------------------------------
void gprs_rxbuffer_reset(void)
{
	// Vacia el buffer y lo inicializa

#ifdef GPRS_RX_LINEAL_BUFFER
	memset( gprs_rxbuffer.buffer, '\0', GPRS_RXBUFFER_LEN);
	gprs_rxbuffer.ptr = 0;

#else
	memset( gprs_rxbuffer.buffer, '\0', GPRS_RXBUFFER_LEN);
	gprs_rxbuffer.head = 0;
	gprs_rxbuffer.tail = 0;
	gprs_rxbuffer.max = GPRS_RXBUFFER_LEN;
	gprs_rxbuffer.full = false;
#endif

}
//------------------------------------------------------------------------------------
bool gprs_rxbuffer_full(void)
{
#ifdef GPRS_RX_LINEAL_BUFFER
	return ( gprs_rxbuffer.ptr == GPRS_RXBUFFER_LEN );
#else
	return( gprs_rxbuffer.full);
#endif

}
//------------------------------------------------------------------------------------
bool gprs_rxbuffer_empty(void)
{
#ifdef GPRS_RX_LINEAL_BUFFER
	return (!gprs_rxbuffer_full());
#else
	return (!gprs_rxbuffer.full && (gprs_rxbuffer.head == gprs_rxbuffer.tail));
#endif

}
//------------------------------------------------------------------------------------
uint16_t gprs_rxbuffer_usedspace(void)
{

uint16_t freespace;

#ifdef GPRS_RX_LINEAL_BUFFER
	freespace = GPRS_RXBUFFER_LEN - gprs_rxbuffer.ptr;

#else
	freespace = gprs_rxbuffer.max;
	if(!gprs_rxbuffer.full) {

		if(gprs_rxbuffer.head >= gprs_rxbuffer.tail){
			freespace = (gprs_rxbuffer.head - gprs_rxbuffer.tail);
		} else	{
			freespace = (gprs_rxbuffer.max + gprs_rxbuffer.head - gprs_rxbuffer.tail);
		}
	}
#endif

	return (freespace);
}
//------------------------------------------------------------------------------------
void gprs_rxbuffer_advance_pointer(void)
{

#ifdef GPRS_RX_LINEAL_BUFFER
	if ( gprs_rxbuffer.ptr < GPRS_RXBUFFER_LEN )
		gprs_rxbuffer.ptr++;

#else
	if( gprs_rxbuffer.full ) {
		gprs_rxbuffer.tail = (gprs_rxbuffer.tail + 1) % gprs_rxbuffer.max;
	}

	gprs_rxbuffer.head = (gprs_rxbuffer.head + 1) % gprs_rxbuffer.max;
	gprs_rxbuffer.full = (gprs_rxbuffer.head == gprs_rxbuffer.tail);
#endif
}
//------------------------------------------------------------------------------------
void gprs_rxbuffer_retreat_pointer(void)
{
#ifdef GPRS_RX_LINEAL_BUFFER
	if ( gprs_rxbuffer.ptr > 0 )
		gprs_rxbuffer.ptr--;

#else
	gprs_rxbuffer.full = false;
	gprs_rxbuffer.tail = (gprs_rxbuffer.tail + 1) % gprs_rxbuffer.max;
#endif
}
//------------------------------------------------------------------------------------
void gprs_rxbuffer_put( char data)
{
	// Avanza sobreescribiendo el ultimo si esta lleno
#ifdef GPRS_RX_LINEAL_BUFFER
	gprs_rxbuffer.buffer[ gprs_rxbuffer.ptr ] = data;
#else
	gprs_rxbuffer.buffer[gprs_rxbuffer.head] = data;
#endif

	gprs_rxbuffer_advance_pointer();


}
//------------------------------------------------------------------------------------
bool gprs_rxbuffer_put2( char data )
{
	// Solo inserta si hay lugar

#ifdef GPRS_RX_LINEAL_BUFFER
	if ( gprs_rxbuffer.ptr < GPRS_RXBUFFER_LEN ) {
		gprs_rxbuffer.buffer[ gprs_rxbuffer.ptr++ ] = data;
		return(true);
	}

#else
    if(!gprs_rxbuffer_full()) {
    	gprs_rxbuffer.buffer[gprs_rxbuffer.head] = data;
    	gprs_rxbuffer_advance_pointer();
        return(true);
    }
#endif

    return(false);
}
//------------------------------------------------------------------------------------
bool gprs_rxbuffer_get( char * data )
{
	// Retorna el ultimo y true.
	// Si esta vacio retorna false.

#ifdef GPRS_RX_LINEAL_BUFFER


#else
    if( !gprs_rxbuffer_empty() ){
        *data = gprs_rxbuffer.buffer[gprs_rxbuffer.tail];
        gprs_rxbuffer_retreat_pointer();
        return(true);
    }
#endif

    return(false);
}
//------------------------------------------------------------------------------------
void gprs_flush_RX_buffer(void)
{
	frtos_ioctl( fdGPRS,ioctl_UART_CLEAR_RX_BUFFER, NULL);
	gprs_rxbuffer_reset();
}
//------------------------------------------------------------------------------------
void gprs_flush_TX_buffer(void)
{
	frtos_ioctl( fdGPRS,ioctl_UART_CLEAR_TX_BUFFER, NULL);
}
//------------------------------------------------------------------------------------
void gprs_print_RX_buffer( void )
{
	// NO USO SEMAFORO PARA IMPRIMIR !!!!!

	if ( ! DF_COMMS )
		return;

	xprintf_P( PSTR ("\r\nGPRS: rxbuff>\r\n\0"));
#ifdef GPRS_RX_LINEAL_BUFFER
	// Imprimo todo el buffer local de RX. Sale por \0.
	// Uso esta funcion para imprimir un buffer largo, mayor al que utiliza xprintf_P. !!!
	xnprint( gprs_rxbuffer.buffer, GPRS_RXBUFFER_LEN );
	xprintf_P( PSTR ("\r\n[%d]\r\n\0"), gprs_rxbuffer.ptr );

#else

	uint16_t ptr;
	char c;

	// for i in range(tail, head)
	//	 print(buff[i]
	//
	//xprintf_P(PSTR("DEBUG: head:%d,tail:%d, used:%d\r\n"), gprs_rxbuffer.head, gprs_rxbuffer.tail, gprs_rxbuffer_usedspace() );
	if ( gprs_rxbuffer_empty() )
		return;

	ptr = gprs_rxbuffer.tail;
	while( 1 ) {
		// imprimo
		c = gprs_rxbuffer.buffer[ptr];
		frtos_putchar(fdTERM , c );
		// Avanzo ptr.
		ptr = (ptr + 1) % gprs_rxbuffer.max;
		// Controlo la salida
		if ( ptr == gprs_rxbuffer.head )
			break;
	}

#endif

}
//------------------------------------------------------------------------------------
int gprs_check_response( uint16_t start, const char *rsp )
{
	// Modifico para solo compara en mayusculas

int i;

#ifdef GPRS_RX_LINEAL_BUFFER
	i = gprs_findstr_lineal(start, rsp);
#else
	i = gprs_findstr_circular(start, rsp);
#endif

	return(i);
}
//------------------------------------------------------------------------------------
int gprs_check_response_with_to( uint16_t start, const char *rsp, uint8_t timeout )
{
	// Espera una respuesta durante un tiempo dado.
	// Hay que tener cuidado que no expire el watchdog por eso lo kickeo aqui. !!!!

int ret = -1;

	while ( timeout > 0 ) {
		timeout--;
		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );

		// Veo si tengo la respuesta correcta.
		ret = gprs_check_response (start, rsp);
		if ( ret > 0)
			return(ret);
	}

	return(-1);
}
//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
// FUNCIONES DE USO GENERAL
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
	if ( gprs_check_response(0, "OK") > 0 ) {
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
	if ( gprs_check_response(0, "OK") > 0) {
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

	xprintf_PD( DF_COMMS, PSTR("COMMS: gprs CPOF\r\n"));
	gprs_flush_RX_buffer();
	gprs_flush_TX_buffer();
	gprs_atcmd_preamble();
	xfprintf_P( fdGPRS,PSTR("AT+CPOF\r\0"));
	vTaskDelay( (portTickType)( 5000 / portTICK_RATE_MS ) );
	xprintf_PD( DF_COMMS, PSTR("COMMS: gprs CPOF OK.\r\n\0"));
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

	//xprintf_PD( DF_COMMS, PSTR("COMMS: gprs_ATCMD TESTING 1 !!!:[%s],[%s],[%s],[%s]\r\n\0"), cmd_name,cmd_test,cmd_at,cmd_rspok );
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
			if ( gprs_check_response(0, "ERROR") > 0 ) {
				// Salgo y reintento el comando.
				gprs_print_RX_buffer();
				vTaskDelay( (portTickType)( 5000 / portTICK_RATE_MS ) );
				cmd_rsp = rsp_ERROR;
				state = ATCMD_CMD;
				break;
			}

			// RSP_OK
			if ( gprs_check_response(0, cmd_rspok) > 0 ) {
			//if ( gprs_check_response(0, "+CFUN: 1") > 0 ) {
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
		memcpy(csqBuffer, &gprs_rxbuffer.buffer[0], sizeof(csqBuffer) );
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
	gprs_ATE();
	gprs_CBC();
	gprs_ATI();
	gprs_IFC();

	gprs_CIPMODE();	// modo transparente.
	gprs_D2();
	gprs_CSUART();
	gprs_C1();

	//gprs_CGAUTH();
	gprs_CSOCKAUTH();

	// PASO 2: Vemos que halla un SIM operativo.
	// AT+CPIN?
	// Vemos que halla un pin presente.
	// Nos indica que el sim esta listo para usarse
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
	// El dispositivo debe adquirir la señal de la estacion base.(celular)
	// El otro comando similar es CGREG que nos indica si estamos registrados en la red GPRS.
	// Este segundo depende del primero.
	if ( ! gprs_CREG() ) {
		*err_code = ERR_CREG_FAIL;
		return(false);
	}

	// PASO 4: Solicito informacion a la red
	// Vemos que la red este operativa
	if ( ! gprs_CPSI() ) {
		*err_code = ERR_CPSI_FAIL;
		return(false);
	}

	// PASO 5: Comandos de conexion a la red de datos ( PDP )
	// Dependen de la red por lo que pueden demorar hasta 3 minutos
	// Debo atachearme a la red de paquetes de datos.
	//
	// Si me conectara por medio de dial-up usaria AT+CGDCONT.
	// Como voy a usar el stack TCP, debo usar AT+CGDSOCKCONT

	// 4.1: Configuro el APN ( PDP context)
	if ( ! gprs_CGDSOCKCONT( apn ) ) {
		*err_code = ERR_APN_FAIL;
		return(false);
	}

	// 4.2: Indico cual va a ser el profile que deba activar.
	gprs_CSOCKSETPN();


	// 4.3: Debo Attachearme a la red GPRS.
	// Este proceso conecta al MS al SGSN en una red GPRS.
	// Esto no significa que pueda enviar datos ya que antes debo activar el PDP.
	// Si estoy en dial-up, usaria el comando CGATT pero esto usando el stack TCP
	// por lo tanto lo hace el NETOPEN.
//	if ( ! gprs_CGATT()) {
//		*err_code = ERR_NETATTACH_FAIL;
//		return(false);
//	}

	// https://www.tutorialspoint.com/gprs/gprs_pdp_context.htm
	// When a MS is already attached to a SGSN and it is about to transfer data,
	// it must activate a PDP address.
	// Activating a PDP address establishes an association between the current SGSN of
	// mobile device and the GGSN that anchors the PDP address.

//	if ( ! gprs_CGATTQ()) {
//		*err_code = ERR_NETATTACH_FAIL;
//		return(false);
//	}

	// La activacion la hace el comando NETOPEN !!!

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
		p = strstr(gprs_rxbuffer.buffer,"IMEI:");
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
bool gprs_ATE( void )
{
bool retS;

	retS = gprs_ATCMD( false, MAX_TRYES_ATE , TIMEOUT_ATE, (PGM_P *)AT_ATE );
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
bool gprs_CGAUTH( void )
{
	// Configura para autentificar PAP

bool retS;

	retS = gprs_ATCMD( true, MAX_TRYES_CGAUTH , TIMEOUT_CGAUTH, (PGM_P *)AT_CGAUTH );
	return(retS);
}
//------------------------------------------------------------------------------------
bool gprs_CSOCKAUTH( void )
{
	// Configura para autentificar PAP

bool retS;

	retS = gprs_ATCMD( true, MAX_TRYES_CSOCKAUTH , TIMEOUT_CSOCKAUTH, (PGM_P *)AT_CSOCKAUTH );
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
		p = strstr(gprs_rxbuffer.buffer,"CCID:");
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

	 CREG testea estar registrado en la red celular
	 CGREG testea estar registrado en la red GPRS.
	 https://www.multitech.com/documents/publications/manuals/S000700.pdf

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
		if ( gprs_check_response(0, "NO SERVICE") > 0 ) {
			// Sin servicio.
			retS = false;
		}
	}

	return(retS);
}
//------------------------------------------------------------------------------------
bool gprs_CGDSOCKCONT( char *apn)
{

	// Indico cual va a ser el PDP (APN)
	// Si me conectar por medio de dial-up usaria CGDCONT
	// Cuando uso el stack TCP/IP debo usar CGDSOCKCONT
	// SIM52xx_TCP_IP_Application_note_V0.03.pdf

uint8_t tryes;
int8_t timeout;
bool retS = false;
t_responses cmd_rsp = rsp_NONE;

	xprintf_PD( DF_COMMS, PSTR("COMMS: gprs CGDSOCKCONT (define PDP)\r\n\0") );
	//Defino el PDP indicando cual es el APN.
	tryes = 0;
	while (tryes++ <  MAX_TRYES_CGDCONT ) {
		gprs_flush_RX_buffer();
		gprs_flush_TX_buffer();
		xprintf_PD( DF_COMMS, PSTR("GPRS: gprs send CGDSOCKCONT (%d)\r\n\0"),tryes);
		gprs_atcmd_preamble();
		xfprintf_P( fdGPRS, PSTR("AT+CGSOCKCONT=1,\"IP\",\"%s\"\r\0"), apn);
		//xfprintf_P( fdGPRS, PSTR("AT+CGDCONT=1,\"IP\",\"%s\"\r\0"), apn);

		timeout = 0;
		while ( timeout++ < TIMEOUT_CGDCONT ) {
			vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
			if ( gprs_check_response(0, "OK") > 0 ) {
				cmd_rsp = rsp_OK;
				goto EXIT;
			}
			if ( gprs_check_response(0, "ERROR") > 0 ) {
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
		xprintf_PD( DF_COMMS,  PSTR("COMMS: gprs CGDSOCKCONT OK en [%d] secs\r\n\0"), (( 10 * (tryes-1) ) + timeout) );
		retS = true;
		break;
	case rsp_ERROR:
		xprintf_PD( DF_COMMS,  PSTR("COMMS: ERROR gprs CGDSOCKCONT FAIL !!.\r\n\0"));
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
	/*
	 * Este proceso conecta al MS al SGSN en una red GPRS.
	   Esto no significa que pueda enviar datos ya que antes debo activar el PDP.
	   Si estoy en dial-up, usaria el comando CGATT pero esto usando el stack TCP
	   por lo tanto lo hace el NETOPEN.
	   Una vez registrado, me atacheo a la red
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
bool gprs_CSOCKSETPN( void )
{
bool retS;

	retS = gprs_ATCMD( true, MAX_TRYES_CSOCKSETPN , TIMEOUT_CSOCKSETPN, (PGM_P *)AT_CSOCKSETPN );
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
char *ts = NULL;
char c = '\0';
char *ptr = NULL;

	strcpy(ip_assigned, "0.0.0.0\0");

	retS = gprs_ATCMD( false, MAX_TRYES_IPADDR , TIMEOUT_IPADDR, (PGM_P *)AT_IPADDR );
	if ( retS ) {
		ptr = ip_assigned;
		ts = strchr( gprs_rxbuffer.buffer, ':');
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
/*
 * Los comandos son solo eso. Se manda el string AT correspondiente y se espera
 * hasta 10s la respuesta OK / ERROR
 * En ese momento se termina.
 * En particular en comandos de red que pueden demorar, no importa esto sino que
 * solo atendemos que el comando halla enviado una respuesta.
 * Luego, en la implementacion de los status podremos ocuparnos de los estados
 * de las conexiones.
 */
//------------------------------------------------------------------------------------
// NET COMMANDS R2
//------------------------------------------------------------------------------------
bool gprs_cmd_netopen( void )
{
	// Inicia el servicio de sockets abriendo un socket. ( no es una conexion sino un
	// socket local por el cual se va a comunicar. )
	// Activa el contexto y crea el socket local
	// La red asigna una IP.
	// Puede demorar unos segundos por lo que espero para chequear el resultado
	// y reintento varias veces.
	// OK si el comando responde ( no importa la respuesta )

int8_t timeout;

	xprintf_PD(  DF_COMMS,  PSTR("COMMS: gprs_cmd_netopen\r\n\0"));
	gprs_flush_RX_buffer();
	gprs_flush_TX_buffer();
	gprs_atcmd_preamble();
	xfprintf_P( fdGPRS,PSTR("AT+NETOPEN\r\0"));
	// Espero la respuesta del comando.
	timeout=10;
	while(timeout-->0) {
		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
		if ( ( gprs_check_response(0, "OK") > 0) ||( gprs_check_response(0, "ERROR") > 0) )
			return(true);
	}
	// Timeout de la respuesta
	return(false);
}
//------------------------------------------------------------------------------------
bool gprs_cmd_netclose( void )
{

int8_t timeout;

	xprintf_PD( DF_COMMS ,  PSTR("COMMS: gprs_cmd_netclose\r\n\0"));
	gprs_flush_RX_buffer();
	gprs_flush_TX_buffer();
	gprs_atcmd_preamble();
	xfprintf_P( fdGPRS,PSTR("AT+NETCLOSE\r\0"));
	// Espero la respuesta del comando.
	timeout=10;
	while(timeout-->0) {
		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
		if ( (gprs_check_response(0, "OK") > 0) || (gprs_check_response(0, "ERROR") > 0) )
			return(true);
	}
	// Timeout de la respuesta
	return(false);
}
//------------------------------------------------------------------------------------
bool gprs_cmd_netstatus( t_net_status *net_status )
{
	// Consulto el estado del servicio. Solo espero 10s a que el comando responda
	// Solo espero la respuesta del comando, no que la net quede open !!!
	// No borro el rxbuffer !!!

uint8_t timeout;
bool retS = false;

	xprintf_PD( DF_COMMS, PSTR("COMMS: gprs_cmd_netstatus.\r\n\0"));
	//gprs_flush_RX_buffer();
	gprs_flush_TX_buffer();
	gprs_atcmd_preamble();
	xfprintf_P( fdGPRS, PSTR("AT+NETOPEN?\r\0"));
	*net_status = NET_UNKNOWN;
	// Espero respuesta
	timeout = 10;
	while(timeout-->0) {
		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );

		if ( gprs_check_response(0, "OK") > 0 ) {
			if ( gprs_check_response(0, "+NETOPEN: 0") > 0 ) {
				*net_status = NET_CLOSE;
				xprintf_PD( DF_COMMS, PSTR("COMMS: gprs_cmd_netstatus->close.\r\n\0") );
				retS = true;
				goto EXIT;
			}
			if ( gprs_check_response(0, "+NETOPEN: 1") > 0 ) {
				*net_status = NET_OPEN;
				xprintf_PD( DF_COMMS, PSTR("COMMS: gprs gprs_cmd_netstatus->open.\r\n\0"));
				retS = true;
				goto EXIT;
			}

		} else if ( gprs_check_response(0, "ERROR") > 0 ) {
			*net_status = NET_UNKNOWN;
			xprintf_PD( DF_COMMS, PSTR("COMMS: gprs_cmd_netstatus->unknown.\r\n\0") );
			retS = true;
			goto EXIT;
		}
		// Espero
	}

	// Sali por timeout:
	xprintf_PD( DF_COMMS,  PSTR("COMMS: gprs_cmd_netstatus->ERROR(TO) !!.\r\n\0"));

EXIT:

	if ( DF_COMMS)
		gprs_print_RX_buffer();
	return(retS);

}
//------------------------------------------------------------------------------------
// LINK COMMANDS R2
//------------------------------------------------------------------------------------
 bool gprs_cmd_linkopen( char *ip, char *port)
{

int8_t timeout;

	xprintf_PD( DF_COMMS, PSTR("COMMS: gprs_cmd_linkopen.\r\n\0"));
	gprs_flush_RX_buffer();
	gprs_flush_TX_buffer();
	gprs_atcmd_preamble();
	xfprintf_P( fdGPRS, PSTR("AT+CIPOPEN=0,\"TCP\",\"%s\",%s\r\0"), ip, port);
	// Espero la respuesta del comando.
	timeout=10;
	while(timeout-->0) {
		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
		if ( (gprs_check_response(0, "OK") > 0) || (gprs_check_response(0, "ERROR") > 0) )
			return(true);
	}
	// Timeout de la respuesta
	return(false);
}
//------------------------------------------------------------------------------------
bool gprs_cmd_linkclose( void )
{

int8_t timeout;

	xprintf_PD( DF_COMMS, PSTR("COMMS: gprs_cmd_linkclose.\r\n\0"));
	gprs_flush_RX_buffer();
	gprs_flush_TX_buffer();
	gprs_atcmd_preamble();
	xfprintf_P( fdGPRS, PSTR("AT+CIPCLOSE=0\r"));
	// Espero la respuesta del comando.
	timeout=10;
	while(timeout-->0) {
		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
		if ( (gprs_check_response(0, "OK") > 0) || (gprs_check_response(0, "ERROR") > 0) )
			return(true);
	}
	// Timeout de la respuesta
	return(false);
}
//------------------------------------------------------------------------------------
bool gprs_cmd_linkstatus( t_link_status *link_status )
{
	// Consulto el estado del servicio. Solo espero 10s a que el comando responda
	// Solo espero la respuesta del comando, no que la net quede open !!!
	// No borro el rxbuffer !!!

uint8_t timeout;
bool retS = false;

	xprintf_PD( DF_COMMS, PSTR("COMMS: gprs_cmd_linkstatus.\r\n"));

	//gprs_flush_RX_buffer();
	gprs_flush_TX_buffer();
	gprs_atcmd_preamble();
	xfprintf_P( fdGPRS, PSTR("AT+CIPOPEN?\r"));
	*link_status = LINK_UNKNOWN;
	// Espero respuesta
	timeout = 10;
	while(timeout-->0) {
		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );

		if ( gprs_check_response(0, "OK") > 0 ) {

			if ( gprs_check_response(0, "CIPOPEN: 0,\"TCP\"") > 0 ) {
				// link connected
				*link_status = LINK_OPEN;
				xprintf_PD( DF_COMMS, PSTR("COMMS: gprs_cmd_linkstatus->open.\r\n"));
				retS = true;
				goto EXIT;
			}

			if ( gprs_check_response(0, "CIPOPEN: 0") > 0 ) {
				// link not connected
				*link_status = LINK_CLOSE;
				xprintf_PD( DF_COMMS, PSTR("COMMS: gprs_cmd_linkstatus->close.\r\n"));
				retS = true;
				goto EXIT;
			}

		} else if ( gprs_check_response(0, "ERROR") > 0 ) {
			// +IP ERROR: Network not opened
			// +IP ERROR: Operation not supported
			*link_status = LINK_UNKNOWN;
			xprintf_PD( DF_COMMS, PSTR("COMMS: gprs_cmd_linkstatus->unknown.\r\n\0") );
			retS = true;
			goto EXIT;
		}
		// Espero
	}

	// Sali por timeout:
	xprintf_PD( DF_COMMS,  PSTR("COMMS: gprs_cmd_linkstatus->ERROR(TO) !!.\r\n\0"));

EXIT:

	if ( DF_COMMS)
		gprs_print_RX_buffer();
	return(retS);

}
//------------------------------------------------------------------------------------
// FUNCIONES AUXILIARES SOBRE EL rxbuffer
//------------------------------------------------------------------------------------
int gprs_findstr_circular( uint16_t start, const char *rsp )
{
	// Busca el string apundado por *rsp en gprs_rxbuffer.
	// Si no lo encuentra devuelve -1
	// Si lo encuentra devuelve la primer posicion dentro de gprs_rxbuffer.

uint16_t i, j, k;
char c1, c2;

	if (start == 0) {
		i = gprs_rxbuffer.tail;
	} else {
		i = start;
	}

	while( 1 ) {
		if ( gprs_rxbuffer.buffer[i] == '\0')
		return(-1);
		//
		j = i;
		k = 0;
		while (1) {
			c1 = rsp[k];
			if ( c1 == '\0')
				return (i);

			c2 =  gprs_rxbuffer.buffer[j];
			if ( toupper(c2) != toupper(c1) )
				break;

			j = (j + 1) % gprs_rxbuffer.max;
			k++;
		}
		// Avanzo ptr.
		i = (i + 1) % gprs_rxbuffer.max;
		// Controlo la salida
		if ( i == gprs_rxbuffer.head )
		break;
	}

	return(-1);

}
//------------------------------------------------------------------------------------
int gprs_findstr_lineal( uint16_t start, const char *rsp )
{
uint16_t i, j, k;
char c1, c2;

	i = start;
	while( 1 ) {
		if ( gprs_rxbuffer.buffer[i] == '\0')
		return(-1);
		//
		j = i;
		k = 0;
		while (1) {
			c1 = rsp[k];
			if ( c1 == '\0')
				return (i);

			c2 =  gprs_rxbuffer.buffer[j];
			if ( toupper(c2) != toupper(c1) )
				break;

			j++;
			k++;
		}
		// Avanzo ptr.
		i++;
		// Controlo la salida
		if ( i == GPRS_RXBUFFER_LEN )
			break;
	}

		return(-1);
}
//------------------------------------------------------------------------------------
void gprs_rxbuffer_copy_to( char *dst, uint16_t start, uint16_t size )
{

uint16_t i;

#ifdef GPRS_RX_LINEAL_BUFFER
	for (i = 0; i < size; i++)
		dst[i] = gprs_rxbuffer.buffer[i+start];
#else

#endif


}
//------------------------------------------------------------------------------------

