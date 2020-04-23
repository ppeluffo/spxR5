/*
 * tkComms_04_prender.c
 *
 *  Created on: 5 mar. 2020
 *      Author: pablo
 */

#include <tkComms.h>

// La tarea no puede demorar mas de 180s.
#define WDG_COMMS_TO_PRENDER	WDG_TO180

//------------------------------------------------------------------------------------
t_comms_states tkComms_st_prender(void)
{
	/*
	 * Prendo el dispositivo y lo dejo listo para enviarle comandos.
	 * Salidas:
	 * 	ST_CONFIGURAR
	 * 	ST_ENTRY
	 *
	 */
	// Intento prender el modem hasta 3 veces. Si no puedo, fijo el nuevo tiempo
	// para esperar y salgo.
	// Mientras lo intento prender no atiendo mensajes ( cambio de configuracion / flooding / Redial )

uint8_t intentos = 0;
t_comms_states next_state = ST_ENTRY;

	ctl_watchdog_kick(WDG_COMMS, WDG_COMMS_TO_PRENDER);

	// Debo poner esta flag en true para que el micro no entre en sleep y pueda funcionar el puerto
	// serial y leer la respuesta del AT del modem.

	xprintf_PD( DF_COMMS, PSTR("COMMS: IN st_prender.\r\n\0"));
	xprintf_PD( DF_COMMS, PSTR("COMMS: prendo dispositivo...\r\n\0"));

// Loop:
	for ( intentos = 0; intentos < MAX_TRIES_PWRON; intentos++ ) {

		// Prendo la fuente
		if ( xCOMMS_prender_dispositivo( DF_COMMS, intentos ) == true ) {
			next_state = ST_CONFIGURAR;
			goto EXIT;
		}

		// Proceso las señales:
		if ( xCOMMS_procesar_senales( ST_PRENDER , &next_state ) )
			goto EXIT;

	}

	// Si salgo por aqui es que el modem no prendio luego de todos los reintentos

	xprintf_P( PSTR("COMMS: ERROR!! Dispositivo no prendio en HW %d intentos\r\n\0"), MAX_TRIES_PWRON );
	next_state = ST_ENTRY;

// Exit:
EXIT:

	xprintf_PD( DF_COMMS, PSTR("COMMS: OUT st_prender.\r\n\0"));
	return(next_state);

}
//------------------------------------------------------------------------------------


