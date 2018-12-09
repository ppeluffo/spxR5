/*
 * l_drv8814.c
 *
 *  Created on: 8 dic. 2018
 *      Author: pablo
 */

#include "l_drv8814.h"

//------------------------------------------------------------------------------------
void DRV8814_init(void)
{
	// Configura los pines del micro que son interface del driver DRV8814.

	IO_config_ENA();
	IO_config_PHA();
	IO_config_ENB();
	IO_config_PHB();
	IO_config_V12_OUTS_CTL();
	IO_config_RES();
	IO_config_SLP();

}
//------------------------------------------------------------------------------------
void DRV8814_power_on(void)
{
	// Prende la fuente de 12V que alimenta el DRV8814

	IO_set_V12_OUTS_CTL();
}
//------------------------------------------------------------------------------------
void DRV8814_power_off(void)
{
	IO_clr_V12_OUTS_CTL();
}
//------------------------------------------------------------------------------------
// Valvulas
// open,close, pulse
// Los pulsos son de abre-cierra !!!
// Al operar sobre las valvulas se asume que hay fisicamente valvulas conectadas
// por lo tanto se debe propocionar corriente, sacar al driver del estado de reposo, generar
// la apertura/cierre, dejar al driver en reposo y quitar la corriente.
// No se aplica cuando queremos una salida FIJA !!!!

void DRV8814_vopen( char valve_id, uint8_t duracion )
{

	// Saco al driver 8814 de reposo.
	IO_set_RES();
	IO_set_SLP();

	switch (valve_id ) {
	case 'A':
		IO_set_PHA();
		IO_set_ENA();
		vTaskDelay( ( TickType_t)( duracion / portTICK_RATE_MS ) );
		IO_clr_ENA();
		IO_clr_PHA();
		break;
	case 'B':
		IO_set_PHB();
		IO_set_ENB();
		vTaskDelay( ( TickType_t)( duracion / portTICK_RATE_MS ) );
		IO_clr_ENB();
		IO_clr_PHB();
		break;
	}

	IO_clr_RES();
	IO_clr_SLP();

}
//------------------------------------------------------------------------------------
void DRV8814_vclose( char valve_id, uint8_t duracion )
{

	// Saco al driver 8814 de reposo.
	IO_set_RES();
	IO_set_SLP();

	switch (valve_id ) {
	case 'A':
		IO_clr_PHA();
		IO_set_ENA();
		vTaskDelay( ( TickType_t)( duracion / portTICK_RATE_MS ) );
		IO_clr_ENA();
		break;
	case 'B':
		IO_clr_PHB();
		IO_set_ENB();
		vTaskDelay( ( TickType_t)( duracion / portTICK_RATE_MS ) );
		IO_clr_ENB();
		break;
	}

	IO_clr_RES();
	IO_clr_SLP();

}
//------------------------------------------------------------------------------------
void DRV8814_set_consigna_diurna(void)
{
	// En consigna diurna la valvula A (JP28) queda abierta y la valvula B (JP2) cerrada.
	//

	// Proporciono corriente.
	DRV8814_power_on();
	// Espero 10s que se carguen los condensasores
	vTaskDelay( ( TickType_t)( 10000 / portTICK_RATE_MS ) );

	DRV8814_vopen( 'A', 100 );
	vTaskDelay( ( TickType_t)( 2000 / portTICK_RATE_MS ) );
	DRV8814_vclose( 'B', 100 );

	DRV8814_power_off();

}
//----------------------------------------------------------------------------------------
void DRV8814_set_consigna_nocturna(void)
{

	// Proporciono corriente.
	DRV8814_power_on();
	// Espero 10s que se carguen los condensasores
	vTaskDelay( ( TickType_t)( 10000 / portTICK_RATE_MS ) );

	DRV8814_vclose( 'A', 100 );
	vTaskDelay( ( TickType_t)( 2000 / portTICK_RATE_MS ) );
	DRV8814_vopen( 'B', 100 );

	DRV8814_power_off();

}
//----------------------------------------------------------------------------------------
