/*
 * tkComms_04_prender.c
 *
 *  Created on: 5 mar. 2020
 *      Author: pablo
 */

#include <tkComms.h>

// La tarea no puede demorar mas de 180s.
#define WDG_COMMS_TO_PRENDER	WDG_TO300

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

t_comms_states next_state = ST_ENTRY;

#ifdef BETA_TEST
	xprintf_PD( DF_COMMS, PSTR("COMMS: IN st_prender.[%d,%d,%d]\r\n"),xCOMMS_stateVars.gprs_prendido, xCOMMS_stateVars.gprs_inicializado,xCOMMS_stateVars.errores_comms);
#else
	xprintf_PD( DF_COMMS, PSTR("COMMS: IN st_prender.\r\n"));
#endif

#ifdef MONITOR_STACK
	debug_print_stack_watermarks("4");
#endif

	ctl_watchdog_kick(WDG_COMMS, WDG_COMMS_TO_PRENDER);

	// Debo poner esta flag en true para que el micro no entre en sleep y pueda funcionar el puerto
	// serial y leer la respuesta del AT del modem.

	xprintf_PD( DF_COMMS, PSTR("COMMS: prendo dispositivo...\r\n\0"));

	// Me aseguro que este apagado
	xCOMMS_apagar_dispositivo();
	// Prendo la fuente
	if ( xCOMMS_prender_dispositivo() == true ) {
		next_state = ST_MON_SQE;
		goto EXIT;
	}

	// No tengo que preocesar señales

	// Si salgo por aqui es que el modem no prendio luego de todos los reintentos en gprs.
	xCOMMS_stateVars.errores_comms++;
	xprintf_P( PSTR("COMMS: ERROR!! Dispositivo no prendio HW \r\n\0"));
	next_state = ST_ENTRY;

// Exit:
EXIT:

#ifdef BETA_TEST
	xprintf_PD( DF_COMMS, PSTR("COMMS: OUT st_prender.[%d,%d,%d](%d)\r\n\0"),xCOMMS_stateVars.gprs_prendido, xCOMMS_stateVars.gprs_inicializado,xCOMMS_stateVars.errores_comms, next_state);
#else
	xprintf_PD( DF_COMMS, PSTR("COMMS: OUT st_prender.\r\n"));
#endif


	return(next_state);

}
//------------------------------------------------------------------------------------


