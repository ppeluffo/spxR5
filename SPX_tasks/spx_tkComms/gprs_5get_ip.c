/*
 * sp5KV5_tkGprs_ask_ip.c
 *
 *  Created on: 27 de abr. de 2017
 *      Author: pablo
 */

#include <spx_tkComms/gprs.h>

/*
static bool sst_apn_configurada(void);
static bool sst_apn_en_default(void);

static bool pv_gprs_netopen(void);
static void pv_gprs_read_ip_assigned(void);
*/

// La tarea no puede demorar mas de 180s.
#define WDG_GPRS_TO_IP	180

//------------------------------------------------------------------------------------
bool st_gprs_get_ip(void)
{
	// El modem esta prendido y configurado.
	// Intento hasta 3 veces pedir la IP.
	// WATCHDOG: En el peor caso demoro 2 mins.
	// La asignacion de la direccion IP es al activar el contexto con el comando AT+CGACT

bool exit_flag = bool_RESTART;

	GPRS_stateVars.state = G_GET_IP;
	ctl_watchdog_kick(WDG_GPRSTX, WDG_GPRS_TO_IP);

	gprs_set_apn( systemVars.gprs_conf.apn );
	// Intento pedir una IP.
	if ( gprs_netopen() ) {
		gprs_read_ip_assigned();
		exit_flag= bool_CONTINUAR;
	} else {
		// Aqui es que luego de tantos reintentos no consegui la IP.
		xprintf_P( PSTR("GPRS: ERROR: ip no asignada !!.\r\n\0") );
		exit_flag = bool_RESTART;
	}

	return(exit_flag);

}
//------------------------------------------------------------------------------------
