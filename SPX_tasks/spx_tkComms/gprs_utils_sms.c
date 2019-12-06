/*
 * gprs_utils_sms.c
 *
 *  Created on: 2 nov. 2019
 *      Author: pablo
 */

#include <spx_tkComms/gprs.h>

#define SMS_NRO_LENGTH		10
#define SMS_MSG_LENGTH		20
#define SMS_QUEUE_LENGTH	10

typedef struct {
	char nro[SMS_NRO_LENGTH];
	char msg[SMS_MSG_LENGTH];
} t_sms;

struct {
	t_sms queue[SMS_QUEUE_LENGTH];
	int8_t ptr;
} s_sms;


//------------------------------------------------------------------------------------
void u_sms_init(void)
{
	//
	GPRS_stateVars.sms_for_tx = false;
	memset(&s_sms, '\0', sizeof(s_sms));
	s_sms.ptr = -1;

}
//------------------------------------------------------------------------------------
// ENVIO DE SMS
//------------------------------------------------------------------------------------
bool u_sms_send(char *dst_nbr, char *msg )
{
	// Funcion publica que la usa el resto del programa para trasmitir mensajes SMS

	if ( s_sms.ptr == SMS_QUEUE_LENGTH ) {
		// La cola de mensajes esta llena.
		GPRS_stateVars.sms_for_tx = true;
		return(false);
	}

	// Pongo el mensaje en la cola.
	// Avanzo el puntero y almaceno
	// El puntero apunta siempre a una posicion libre.

	s_sms.ptr++;
	memcpy( s_sms.queue[s_sms.ptr].nro, dst_nbr, SMS_NRO_LENGTH );
	s_sms.queue[s_sms.ptr].nro[SMS_NRO_LENGTH - 1] = '\0';
	memcpy(s_sms.queue[s_sms.ptr].msg, msg, SMS_MSG_LENGTH );
	s_sms.queue[s_sms.ptr].msg[SMS_MSG_LENGTH - 1] = '\0';

	if ( systemVars.debug == DEBUG_GPRS ) {
		xprintf_P( PSTR("SMS ptr = %d\r\n"), s_sms.ptr );
	}
	// Aviso al gprs que hay mensajes para trasmitir
	GPRS_stateVars.sms_for_tx = true;
	return(true);

}
//------------------------------------------------------------------------------------
void u_gprs_sms_txcheckpoint(void)
{
	// Funcion que la tarea de gprsTx utiliza en determinados puntos para ver
	// si hay SMS pendientes de envio y entonces enviarlos.
	// Envia todos los sms que hay en la cola y la va vaciando.
	// Los manda del modo quick.
	// Por las dudas debe verificar que el modem este en modo comando.

//	xprintf_P( PSTR("DEBUG SMS txcheckpoint\r\n" ));

	if ( ! GPRS_stateVars.sms_for_tx )
		return;

	// CMGF: Selecciono el modo de mandar sms: 1-> texto
	u_gprs_flush_RX_buffer();
	xCom_printf_P( fdGPRS,PSTR("AT+CMGF=1\r"));
	vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );

	while ( s_sms.ptr > 0 ) {
		// Hay mensajes pendientes.

		// Mando el mensaje
		u_gprs_flush_RX_buffer();
		xCom_printf_P( fdGPRS,PSTR("AT+CMGSO=\"%s\",\"%s\"\r"), s_sms.queue[s_sms.ptr].nro, s_sms.queue[s_sms.ptr].msg);
		xprintf_P( PSTR("AT+CMGSO=\"%s\",\"%s\"\r\n"), s_sms.queue[s_sms.ptr].nro, s_sms.queue[s_sms.ptr].msg );

		// Espero el OK
		if ( ! u_gprs_check_response_with_to( "OK", 10 ) ) {
			xprintf_P( PSTR("ERROR: Sent SMS Fail !!\r\n" ));
			return;
		}

		// Mensajes trasmitido OK.
		xprintf_P( PSTR("Sent SMS OK !!\r\n" ));
		if ( systemVars.debug == DEBUG_GPRS ) {
			u_gprs_print_RX_Buffer();
		}
		// Borro el mensaje de la lista
		memset( s_sms.queue[s_sms.ptr].nro, '\0', SMS_NRO_LENGTH );
		memset(s_sms.queue[s_sms.ptr].msg, '\0', SMS_MSG_LENGTH );
		s_sms.ptr--;
	}
	// No hay mas mensajes pendientes.
	GPRS_stateVars.sms_for_tx = false;
}
//------------------------------------------------------------------------------------
 void u_gprs_send_sms( char *dst_nbr, char *msg )
{
	// Envio un mensaje sms.
	// Si el modem esta conectado, lo paso a offline
	// Mando un comando para setear el modo de sms en texto
	// Envio el comando.


uint8_t ctlz = 0x1A;

	// CMGF: Selecciono el modo de mandar sms: 1-> texto
	u_gprs_flush_RX_buffer();
	xCom_printf_P( fdGPRS,PSTR("AT+CMGF=1\r"));
	vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );

	// CMGS: Envio SMS.
	xCom_printf_P( fdGPRS,PSTR("AT+CMGS=\"%s\"\r"), dst_nbr);

	// Espero el prompt > para enviar el mensaje.
	if ( ! u_gprs_check_response_with_to( ">", 10 ) ) {
		xprintf_P( PSTR("ERROR: Sent SMS Fail !!\r\n" ));
		return;
	}

	// Recibi el prompt
	//xprintf_P( PSTR("\r\n Prompt OK.\r\n0" ));
	if ( systemVars.debug == DEBUG_GPRS ) {
		u_gprs_print_RX_Buffer();
	}

	// Envio el mensaje:
	u_gprs_flush_RX_buffer();
	xCom_printf_P( fdGPRS,PSTR("%s\r %c"), msg, ctlz  );
	if ( systemVars.debug == DEBUG_GPRS ) {
		xprintf_P( PSTR("GPRS: sms_txt:%s\r\n"), msg);
	}

	// Espero el OK
	if ( ! u_gprs_check_response_with_to( "OK", 10 ) ) {
		xprintf_P( PSTR("ERROR: Sent SMS Fail !!\r\n" ));
	} else {
		xprintf_P( PSTR("Sent SMS OK !!\r\n" ));
	}

	if ( systemVars.debug == DEBUG_GPRS ) {
		u_gprs_print_RX_Buffer();
	}

}
//------------------------------------------------------------------------------------
void u_gprs_quick_send_sms( char *dst_nbr, char *msg )
{

	// Si el modem esta apagado debo mandarlo prender y esperar que prenda y luego
	// mandarlo apagar.
	// En este caso demora mucho.
	// Si esta prendido debo esperar que este en modo DATA_AWAITING para sacarlo del online
	// y mandar el sms.

	if ( GPRS_stateVars.state == G_ESPERA_APAGADO ) {
		// Mando la señal para prenderlo
		if ( systemVars.debug == DEBUG_GPRS ) {
			xprintf_P( PSTR("GPRS: SMS modem off. Mando prenderlo\r\n" ));
		}
		u_gprs_redial();
	}

	if ( systemVars.debug == DEBUG_GPRS ) {
		xprintf_P( PSTR("GPRS: SMS awaiting...\r\n" ));
	}
	// Espero que este en estado DATA_AWAITING
	while ( GPRS_stateVars.state != G_DATA_AWAITING ) {
		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
	}


	if ( u_gprs_check_socket_status() == SOCK_OPEN ) {
		if ( systemVars.debug == DEBUG_GPRS ) {
			xprintf_P( PSTR("GPRS: SMS socket open\r\n" ));
		}

		GPRS_stateVars.signal_redial = false;		// Si estoy conectado lo saco con +++
		u_gprs_close_socket();

	}

	// CMGF: Selecciono el modo de mandar sms: 1-> texto
	u_gprs_flush_RX_buffer();
	xCom_printf_P( fdGPRS,PSTR("AT+CMGF=1\r"));
	vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );

	// Mando el mensaje
	u_gprs_flush_RX_buffer();
	xCom_printf_P( fdGPRS,PSTR("AT+CMGSO=\"%s\",\"%s\"\r"), dst_nbr, msg);
	xprintf_P( PSTR("AT+CMGSO=\"%s\",\"%s\"\r\n"), dst_nbr, msg);

	// Espero el OK
	if ( ! u_gprs_check_response_with_to( "OK", 10 ) ) {
		xprintf_P( PSTR("ERROR: Sent SMS Fail !!\r\n" ));
	} else {
		xprintf_P( PSTR("Sent SMS OK !!\r\n" ));
	}

	if ( systemVars.debug == DEBUG_GPRS ) {
		u_gprs_print_RX_Buffer();
	}

}
//------------------------------------------------------------------------------------
// LECTURA DE SMS
//------------------------------------------------------------------------------------
void u_gprs_sms_rxcheckpoint(void)
{
	// Leo todos los mensaejes que hay en la memoria, los proceso y los borro
	// El comando AT+CMGL="ALL" lista todos los mensajes.
	// Filtro por el primer +CMGL: index,
	// Leo y borro con AT+CMGRD=index
	// Cuando no tengo mas mensajes, el comando AT+CMGL no me devuelve nada mas.

uint8_t msg_index;
char *sms_msg;

//	xprintf_P( PSTR("DEBUG SMS rxcheckpoint\r\n" ));

	while ( u_gprs_sms_received(&msg_index) ) {
		// Veo si hay mensajes pendientes
		sms_msg = u_gprs_read_and_delete_sms_by_index(msg_index);
		u_gprs_process_sms(sms_msg);
		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
	}

}
//------------------------------------------------------------------------------------
char *u_gprs_read_and_delete_sms_by_index( uint8_t msg_index )
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

	u_gprs_flush_RX_buffer();
	xCom_printf_P( fdGPRS,PSTR("AT+CMGRD=%d\r"), msg_index);
	vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );

	if ( systemVars.debug == DEBUG_GPRS ) {
		u_gprs_print_RX_Buffer();
	}

	p = strstr( (const char *)&pv_gprsRxCbuffer.buffer, "+CMGRD:");
	if ( p != NULL ) {
		//memset(localStr,'\0',sizeof(localStr));
		//memcpy(localStr,p,sizeof(localStr));
		//stringp = localStr;
		stringp = p;
		tk_msg = strsep(&stringp,delim);		// +CMGRD:...........\r

		tk_msg = strsep(&stringp,delim);		// "mensaje"
		tk_msg++;
		//xprintf_P( PSTR("DEBUG: SMS_MSG: %s\r\n"), tk_msg );

	}

	return(tk_msg);

}
//------------------------------------------------------------------------------------
bool u_gprs_sms_received( uint8_t *first_msg_index )
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


	u_gprs_flush_RX_buffer();
	xCom_printf_P( fdGPRS,PSTR("AT+CMGL=\"ALL\"\r"));
	vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );

	retS = u_gprs_check_response_with_to( "+CMGL:", 5 );

	if ( systemVars.debug == DEBUG_GPRS ) {
		u_gprs_print_RX_Buffer();
	}

	if ( retS ) {
		// Hay al menos un mensaje pendiente. Decodifico su indice
		p = strstr( (const char *)&pv_gprsRxCbuffer.buffer, "+CMGL:");
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
void u_gprs_process_sms( char *sms_msg)
{
	// Aqui es donde parseo el mensaje del SMS y actuo en consecuencia.

char localStr[16] = { 0 };
char *stringp = NULL;
char *tk_douts = NULL;
char *delim = ",=:><";

	xprintf_P( PSTR("DEBUG: SMS_PROCESS: %s\r\n"), sms_msg );

	// SMS enviado desde un tanque a una perforacion.
	if ( ( systemVars.aplicacion == APP_PERFORACION ) && (!strcmp_P( strupr(sms_msg), PSTR("PERF_OUTS\0"))) )  {

		memset(localStr,'\0',16);
		memcpy(localStr,sms_msg ,sizeof(localStr));

		stringp = localStr;
		tk_douts = strsep(&stringp,delim);	// PERF_OUTS
		tk_douts = strsep(&stringp,delim);	// Str. con el valor de las salidas. 0..128

		// Actualizo el status a travez de una funcion propia del modulo de outputs
		perforacion_set_douts_from_gprs( atoi( tk_douts ));

	}
}
//------------------------------------------------------------------------------------
