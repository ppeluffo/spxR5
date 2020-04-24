/*
 * tkComms_SMS.c
 *
 *  Created on: 18 mar. 2020
 *      Author: pablo
 */

#include "spx.h"
#include "tkComms.h"

struct {
	t_sms queue[SMS_MSG_QUEUE_LENGTH];
	int8_t ptr;
} qsms;

#define xSMS_queue_is_empty() ( qsms.ptr == -1 )


//------------------------------------------------------------------------------------
void xSMS_init(void)
{
	/*
	 * Inicializo la cola de mensajes sms y la señal.
	 */

	SPX_CLEAR_SIGNAL( SGN_SMS );

	memset(&qsms, '\0', sizeof(qsms));
	qsms.ptr = -1;

}
//------------------------------------------------------------------------------------
// ENVIO DE SMS
//------------------------------------------------------------------------------------
bool xSMS_enqueue(char *dst_nbr, char *msg )
{
	/*
	 * Funcion publica que la usa el resto del programa para trasmitir mensajes SMS
	 * Si hay lugar en la cola de mensajes, lo encola y prende la senal para que se
	 * transmita
	 */

	// Testeo SMS queue llena
	if ( qsms.ptr == ( SMS_MSG_QUEUE_LENGTH - 1 )) {
		// La cola de mensajes esta llena. Prendo la señal
		SPX_SEND_SIGNAL( SGN_SMS );
		return(false);
	}

	// Pongo el mensaje en la cola.
	// Avanzo el puntero y almaceno
	// El puntero apunta siempre a una posicion libre.

	qsms.ptr++;

	strncpy( qsms.queue[qsms.ptr].nro, dst_nbr, SMS_NRO_LENGTH );
	qsms.queue[qsms.ptr].nro[SMS_NRO_LENGTH - 1] = '\0';

	strncpy(qsms.queue[qsms.ptr].msg, msg, SMS_MSG_LENGTH );
	qsms.queue[qsms.ptr].msg[SMS_MSG_LENGTH - 1] = '\0';

	xprintf_PD( DF_COMMS, PSTR("SMS enq ptr = %d\r\n"), qsms.ptr );
	xprintf_PD( DF_COMMS, PSTR("SMS enq nbr = %s\r\n"), qsms.queue[qsms.ptr].nro );
	xprintf_PD( DF_COMMS, PSTR("SMS enq msg = [%s]\r\n"), qsms.queue[qsms.ptr].msg );

	// Aviso al gprs que hay mensajes para trasmitir.
	// Lo mando por txcheckpoint
	SPX_SEND_SIGNAL( SGN_SMS );
	return(true);

}
//------------------------------------------------------------------------------------
t_sms *xSMS_dequeue(void)
{
	/*
	 * Desencola un mensaje de la cola de SMS
	 *
	 */
	if ( ! xSMS_queue_is_empty() ) {
		return( &qsms.queue[qsms.ptr]);
	}

	return(NULL);

}
//------------------------------------------------------------------------------------
void xSMS_delete(void)
{
	/*
	 * Borra el ultimo SMS de la cola
	 */

	if ( ! xSMS_queue_is_empty() ) {
		memset( qsms.queue[qsms.ptr].nro, '\0', SMS_NRO_LENGTH );
		memset(qsms.queue[qsms.ptr].msg, '\0', SMS_MSG_LENGTH );
		qsms.ptr--;

		//xprintf_PD( DF_COMMS, PSTR("SMS del ptr = %d\r\n"), qsms.ptr );
		//xprintf_PD( DF_COMMS, PSTR("SMS del nbr = %s\r\n"), qsms.queue[qsms.ptr].nro );
		//xprintf_PD( DF_COMMS, PSTR("SMS del msg = [%s]\r\n"), qsms.queue[qsms.ptr].msg );

	}
}
//------------------------------------------------------------------------------------
void xSMS_txcheckpoint(void)
{
	// Funcion que la tarea de gprs utiliza en determinados puntos para ver
	// si hay SMS pendientes de envio y entonces enviarlos.
	// Envia todos los sms que hay en la cola y la va vaciando.
	// Los manda del modo quick.
	// Por las dudas debe verificar que el modem este en modo comando.


uint8_t intentos;
t_sms *sms_boundle;

	xprintf_PD( DF_COMMS, PSTR("SMS: TX checkpoint\r\n" ));

	// Si no hay sms para enviar, salgo
	//if ( ! SPX_SIGNAL( SGN_SMS ) )
	//	return;

	// Envio todos los SMS pendientes
	while ( ! xSMS_queue_is_empty() ) {

		for ( intentos = 0; intentos < 4; intentos++) {

			// Desencolo
			sms_boundle = xSMS_dequeue();

			if ( sms_boundle == NULL ) {
				xprintf_P( PSTR("SMS: ERROR dequeue\r\n" ));
				xSMS_init();
				break;
			}

			// Aqui tengo el mensaje desencolado.
			// Veo que no sean basura ( mensaje corto )
			xprintf_PD(DF_COMMS, PSTR("SMS TXCKP msg=[%s], len=[%d]\r\n\0"), sms_boundle->msg, strlen(sms_boundle->msg));
			if ( strlen(sms_boundle->msg) < 3 ) {
				xSMS_delete();
				xprintf_P( PSTR("SMS: TXCKP Mensaje corto: borro !!\r\n" ));
				break;
			}
			// Intento transmitirlo
		    if ( xSMS_send(sms_boundle->nro, sms_boundle->msg) ) {
		    	xSMS_delete();
				xprintf_P( PSTR("SMS: TXCKP Sent SMS OK !!\r\n" ));
				break;
			} else {
				xprintf_P( PSTR("SMS: TXCKP Sent SMS FAIL (%d) !!\r\n" ),intentos);
			}

		    if ( intentos == 3 ) {
		    	// No pude trasmitirlo en varios reintentos: lo borro
		    	xSMS_delete();
		    	xprintf_P( PSTR("SMS: TXCKP Sent SMS FAIL. Delete!!\r\n" ));
		    	break;
		    }
		}
	}
	// No hay mas mensajes pendientes.
	SPX_CLEAR_SIGNAL( SGN_SMS );

}
//------------------------------------------------------------------------------------
bool xSMS_send( char *dst_nbr, char *msg )
{
	// Envio un mensaje sms.
	// Si el modem esta conectado, lo paso a offline
	// Mando un comando para setear el modo de sms en texto
	// Envio el comando.
	// Puedo mandar cualquier caracter de control !!
	// Se usa para mensajes formateados


uint8_t ctlz = 0x1A;
bool retS = false;

	// CMGF: Selecciono el modo de mandar sms: 1-> texto
	gprs_flush_RX_buffer();
	xfprintf_P( fdGPRS, PSTR("AT+CMGF=1\r"));
	vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );

	// CMGS: Envio SMS.
	xfprintf_P( fdGPRS, PSTR("AT+CMGS=\"%s\"\r"), dst_nbr);

	// Espero el prompt > para enviar el mensaje.
	if ( ! gprs_check_response_with_to( ">", 10 ) ) {
		xprintf_P( PSTR("SMS: ERROR Sent Fail(Timeout 1) !!\r\n" ));
		return(false);
	}

	// Recibi el prompt
	//xprintf_P( PSTR("\r\n Prompt OK.\r\n0" ));
	gprs_print_RX_buffer(DF_COMMS);

	// Envio el mensaje:
	gprs_flush_RX_buffer();
	xfprintf_P( fdGPRS, PSTR("%s\r %c"), msg, ctlz  );
	xprintf_PD( DF_COMMS, PSTR("SMS: msgtxt=[%s]\r\n"), msg);

	// Espero el OK
	if ( ! gprs_check_response_with_to( "OK", 10 ) ) {
		xprintf_P( PSTR("SMS: ERROR Sent Fail(Timeout 2) !!\r\n" ));
		retS = false;
	} else {
		xprintf_P( PSTR("SMS: Sent OK !!\r\n" ));
		retS = true;
	}

	gprs_print_RX_buffer(DF_COMMS);

	return(retS);

}
//------------------------------------------------------------------------------------
char *xSMS_format(char *msg)
{
	/*
	 * Dado un mensaje, le agrega la fecha y hora y retorna un puntero
	 * para enviarlo por SMS
	 */


RtcTimeType_t rtc;
static char sms_msg[SMS_MSG_LENGTH];

	memset( &rtc, '\0', sizeof(RtcTimeType_t));
	RTC_read_dtime(&rtc);
	memset( &sms_msg, '\0', SMS_MSG_LENGTH );
	snprintf_P( sms_msg, SMS_MSG_LENGTH, PSTR("Fecha: %02d/%02d/%02d %02d:%02d\n%s"), rtc.year, rtc.month, rtc.day, rtc.hour, rtc.min, msg );
	xprintf_PD( DF_COMMS, PSTR("SMS: formatted [%s]\r\n" ), sms_msg );
	return((char *)&sms_msg);

}
//------------------------------------------------------------------------------------
// LECTURA DE SMS
//------------------------------------------------------------------------------------
void xSMS_rxcheckpoint(void)
{
	// Leo todos los mensaejes que hay en la memoria, los proceso y los borro
	// El comando AT+CMGL="ALL" lista todos los mensajes.
	// Filtro por el primer +CMGL: index,
	// Leo y borro con AT+CMGRD=index
	// Cuando no tengo mas mensajes, el comando AT+CMGL no me devuelve nada mas.

uint8_t msg_index;
char *sms_msg;

	xprintf_PD( DF_COMMS, PSTR("SMS: RX checkpoint\r\n" ));

	while ( xSMS_received(&msg_index) ) {
		// Veo si hay mensajes pendientes
		sms_msg = xSMS_read_and_delete_by_index(msg_index);
		xSMS_process(sms_msg);
		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
	}

}
//------------------------------------------------------------------------------------
char *xSMS_read_and_delete_by_index( uint8_t msg_index )
{
	// Leo y borro con AT+CMGRD=index
	// Filtro el mensaje y lo imprimo.
	// +CMGRD: "REC READ","+59899394959","","19/10/30,16:11:24-12"
	// Msg3
	//
	// OK

char *p = NULL;
//char localStr[32] = { 0 };
char *stringp = NULL;
char *tk_msg= NULL;
char *delim = "\r";

	gprs_flush_RX_buffer();
	xfprintf_P( fdGPRS, PSTR("AT+CMGRD=%d\r"), msg_index);
	vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );

	gprs_print_RX_buffer(DF_COMMS);

	p = gprs_get_pattern_in_buffer("+CMGRD:");
	if ( p != NULL ) {
		stringp = p;
		tk_msg = strsep(&stringp,delim);		// +CMGRD:...........\r
		tk_msg = strsep(&stringp,delim);		// "mensaje"
		tk_msg++;
		//xprintf_P( PSTR("DEBUG: SMS_MSG: %s\r\n"), tk_msg );
	}

	return(tk_msg);

}
//------------------------------------------------------------------------------------
bool xSMS_received( uint8_t *first_msg_index )
{
	// Funcion que la tarea gprsTX utiliza para saber si el modem ha
	// recibido un SMS.
	// Envio el comando AT+CMGL="ALL" y espero por una respuesta
	// del tipo +CMGL:
	// Si no la recibo no hay mensajes pendientes.
	// +CMGL: 1,"REC READ","+59899394959","","19/10/30,15:39:26-12"
	// Va 2
	// +CMGL: 0,"REC UNREAD","+59899394959","","19/10/30,15:45:38-12"
	// Otro

bool retS = false;
char *p = NULL;
char localStr[32] = { 0 };
char *stringp = NULL;
char *tk_idx= NULL;
char *delim = ",:";

	gprs_flush_RX_buffer();
	xfprintf_P( fdGPRS, PSTR("AT+CMGL=\"ALL\"\r"));
	vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );

	retS = gprs_check_response_with_to( "+CMGL:", 5 );

	gprs_print_RX_buffer(DF_COMMS);

	if ( retS ) {
		// Hay al menos un mensaje pendiente. Decodifico su indice
		p = gprs_get_pattern_in_buffer("+CMGL:");
		if ( p != NULL ) {
			memset(localStr,'\0',sizeof(localStr));
			memcpy(localStr,p,sizeof(localStr));
			stringp = localStr;
			tk_idx = strsep(&stringp,delim);		// +CMGL:
			tk_idx = strsep(&stringp,delim);		//  0
			*first_msg_index = atoi(tk_idx);
		}
	}

	return(retS);
}
//------------------------------------------------------------------------------------
void xSMS_process( char *sms_msg)
{
	// Aqui es donde parseo el mensaje del SMS y actuo en consecuencia.

	xprintf_P( PSTR("SMS:PROCESS: %s\r\n"), sms_msg );

}
//------------------------------------------------------------------------------------
