/*
 * tkApp_caudalimetro.c
 *
 *  Created on: 9 abr. 2020
 *      Author: pablo
 *
 * En esta aplicacion el datalogger mide el caudal por medio del RangeMeter y
 * genera una señal de salida de pulsos que sirve para realimentar las bombas
 * dosificadoras ( Grundfoss DDC )
 * La bombas necesitan tener una señal de pulsos cuya frecuencia es proporcional al caudal.
 * El ancho minimo de pulso que pueden medir es de 5ms.
 * La salida electricamente es un open-collector.
 *
 * Esquema Fisico:
 * Usamos un transistor con una resistencia de base, conectado a una de las salidas de una
 * fase del control de las consignas.
 * Esta salida es el punto medio de un H-bridge por lo tanto comandando una fase podemos generar
 * los pulsos.
 *
 * Esquema de firmware:
 * En el datalogger, el rangemeter se mide cada timerpoll ( 1 min, 5 min ).
 * Cuando se mide, la tarea de inputs pasa a la aplicacion el valor medido.
 * Esta tarea ajusta entonces la frecuencia en forma proporcional al caudal leido.
 *
 * Pendiente:
 * - Generar los pulsos
 * - Generalizarlo a no solo usar el caudal del rangemeter sino el caudal de otro canal analogico o de pulsos
 *   Debemos poner un parámetro que nos diga de donde tomamos el caudal
 *   En el caso del caudal por rangemeter debemos introducir las funciones de los canales para tener los caudales
 *   de las alturas.
 *
 */

#include "tkApp.h"

bool pv_xAPP_caudalimetro_init(void);
void pv_xAPP_Q_generar_pulso(void);
void pv_xAPP_Q_esperar_periodo(void);

//------------------------------------------------------------------------------------
void tkApp_caudalimetro(void)
{
	// Las salidas estan configuradas para modo consigna.
	// c/25s reviso si debo aplicar una o la otra y aplico
	// Espero con lo que puedo entrar en tickless


	xprintf_PD(DF_APP,"APP: CAUDALIMETRO start\r\n\0");

	if ( ! pv_xAPP_caudalimetro_init())
		return;


	for (;;) {

		ctl_watchdog_kick( WDG_APP,  WDG_APP_TIMEOUT );

		pv_xAPP_Q_generar_pulso();
		pv_xAPP_Q_esperar_periodo();

	}
}
//------------------------------------------------------------------------------------
bool pv_xAPP_caudalimetro_init(void)
{

	if ( spx_io_board != SPX_IO5CH ) {
		xprintf_P(PSTR("APP: CAUDALIMETRO ERROR: Modo Caudalimetro only in IO_5CH.\r\n\0"));
		sVarsApp.aplicacion = APP_OFF;
		u_save_params_in_NVMEE();
		return(false);
	}

	DRV8814_init();

	// Configuro el resto de los parametros iniciales.

	return(true);
}
//------------------------------------------------------------------------------------
void pv_xAPP_Q_generar_pulso(void)
{
	// drv senal low
	vTaskDelay( ( TickType_t)( sVarsApp.caudalimetro.pulse_width / portTICK_RATE_MS ) );
	// drv senal float
}
//------------------------------------------------------------------------------------
void pv_xAPP_Q_esperar_periodo(void)
{
	/*
	 * De acuerdo al caudal medido, calcula el periodo que debe esperar entre pulsos
	 * y espera.
	 */

	sVarsApp.caudalimetro.periodo_actual = ( sVarsApp.caudalimetro.range_actual * sVarsApp.caudalimetro.factor_caudal) - sVarsApp.caudalimetro.pulse_width;
	if ( sVarsApp.caudalimetro.periodo_actual < 0)
		sVarsApp.caudalimetro.periodo_actual = 50;

	if ( sVarsApp.caudalimetro.periodo_actual > 10000)
		sVarsApp.caudalimetro.periodo_actual = 10000;

	vTaskDelay( ( TickType_t)( sVarsApp.caudalimetro.periodo_actual / portTICK_RATE_MS ) );

}
//------------------------------------------------------------------------------------
bool xAPP_caudalimetro_config ( char *pwidth, char *factorQ)
{
	sVarsApp.caudalimetro.pulse_width = atoi(pwidth);
	sVarsApp.caudalimetro.factor_caudal = atoi(factorQ);
	return(true);
}
//------------------------------------------------------------------------------------
uint8_t xAPP_caudalimetro_checksum(void)
{

uint8_t checksum = 0;
char dst[32];
char *p;
uint8_t i = 0;

	// calculate own checksum
	// Vacio el buffer temoral
	memset(dst,'\0', sizeof(dst));

	i = snprintf_P( &dst[i], sizeof(dst), PSTR("CAUDALIMETRO,"));
	i += snprintf_P(&dst[i], sizeof(dst), PSTR("%04d,%04d"), sVarsApp.caudalimetro.pulse_width, sVarsApp.caudalimetro.factor_caudal );

	//xprintf_P( PSTR("DEBUG: CONS = [%s]\r\n\0"), dst );
	// Apunto al comienzo para recorrer el buffer
	p = dst;
	// Mientras no sea NULL calculo el checksum deol buffer
	while (*p != '\0') {
		//checksum += *p++;
		checksum = u_hash(checksum, *p++);
	}
	//xprintf_P( PSTR("DEBUG: cks = [0x%02x]\r\n\0"), checksum );

	return(checksum);

}
//------------------------------------------------------------------------------------


