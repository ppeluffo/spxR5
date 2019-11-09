/*
 * gprs_utils_sms.c
 *
 *  Created on: 2 nov. 2019
 *      Author: pablo
 */

#include <spx_tkComms/gprs.h>

StaticTimer_t sms_xTimerBuffers;
TimerHandle_t sms_xTimer;
static void pv_sms_TimerCallback( TimerHandle_t xTimer );
//------------------------------------------------------------------------------------
// ENVIO DE SMS
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
void u_gprs_read_and_process_all_sms(void)
{
	// Leo todos los mensaejes que hay en la memoria, los proceso y los borro
	// El comando AT+CMGL="ALL" lista todos los mensajes.
	// Filtro por el primer +CMGL: index,
	// Leo y borro con AT+CMGRD=index
	// Cuando no tengo mas mensajes, el comando AT+CMGL no me devuelve nada mas.

uint8_t msg_index;
char *sms_msg;

	while ( u_gprs_sms_pendiente(&msg_index) ) {
		// Veo si hay mensajes pendientes
		sms_msg = u_gprs_read_and_delete_sms_by_index(msg_index);
		u_gprs_process_sms(sms_msg);

		vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
	}

	GPRS_stateVars.sms_awaiting = 0;

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
bool u_gprs_sms_pendiente( uint8_t *first_msg_index )
{
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

	xprintf_P( PSTR("DEBUG: SMS_PROCESS: %s\r\n"), sms_msg );
}
//------------------------------------------------------------------------------------
// DETECCION DE ENTRADA DE SMS
//------------------------------------------------------------------------------------
void u_gprs_config_sms(void)
{
	/*
	Esta funcion configura lo relativo a SMS.
	Cuando recibo un SMS, el RI se va a 0.
	Para resetear el RI debo dar el comando AT+CRIRS.
	El problema es que en este momento sube con lo que queda armado para detectar otro
	sms, pero por unos 10s oscila mucho.
	Lo que hacemos es que una vez que detecte el flanco que llego un SMS, genero la indicacion
	y arranco un timer que va a ser el que luego de 15s resetee el pin RI.
	*/

	// Configuro el timer para 10s y one-shot
	sms_xTimer = xTimerCreateStatic ("SMS",
			pdMS_TO_TICKS( 10000 ),
			pdFALSE,
			( void * ) 0,
			pv_sms_TimerCallback,
			&sms_xTimerBuffers
			);

}
//------------------------------------------------------------------------------------
static void pv_sms_TimerCallback( TimerHandle_t xTimer )
{
	// Funcion de callback del timer del SMS.
	// Se ejecuta 10s luego de haber arrancado el timer.
	// Resetea el pin RI

	 GPRS_stateVars.signal_resetRI = true;

}
//------------------------------------------------------------------------------------
void u_gprs_config_ri_pin(void)
{
	PORTD.PIN7CTRL = PORT_OPC_PULLUP_gc | PORT_ISC_FALLING_gc;	// Sensa falling edge
	PORTD.INT0MASK = PIN7_bm;									// El pin 7 ( RI) interrumpe INT0
	PORTD.INTCTRL = PORT_INT0LVL0_bm;							// Enable interrupts
	PORTD.INTFLAGS = PORT_INT0IF_bm;							// Clear interrupts flag.
	GPRS_stateVars.sms_awaiting = 0;
}
//------------------------------------------------------------------------------------
ISR ( PORTD_INT0_vect )
{
	// Esta ISR se activa cuando el pin RI (PD7) genera un flaco de bajada.
	GPRS_stateVars.sms_awaiting++;
	PORTD.INTFLAGS = PORT_INT0IF_bm;			// Clear interrupts flag.

}
//------------------------------------------------------------------------------------
uint8_t u_gprs_read_sms_awaiting(void)
{
	return(GPRS_stateVars.sms_awaiting);
}
//------------------------------------------------------------------------------------
void u_gprs_resetRI()
{
	/*
	 * Manda el comando de reseteo y rearme del RI.
	 * Debo estar en modo comando y sino debo esperar
	 *
	 */
}


