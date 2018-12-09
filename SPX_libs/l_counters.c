/*
 * l_counters.c
 *
 *  Created on: 8 dic. 2018
 *      Author: pablo
 */


#include "l_counters.h"

BaseType_t xHigherPriorityTaskWokenDigital = false;
static bool wakeup_for_C0, wakeup_for_C1;
TaskHandle_t countersTaskHandle;

//------------------------------------------------------------------------------------
void COUNTERS_init( void )
{
	CNT_config_CLRD();
	CNT_config_CNT0();
	CNT_config_CNT1();

	PORTA.PIN2CTRL = PORT_OPC_PULLUP_gc | PORT_ISC_RISING_gc;	// Sensa rising edge
	PORTA.INT0MASK = PIN2_bm;
	PORTA.INTCTRL = PORT_INT0LVL0_bm;

	PORTB.PIN2CTRL = PORT_OPC_PULLUP_gc | PORT_ISC_RISING_gc;	// Sensa rising edge
	PORTB.INT0MASK = PIN2_bm;
	PORTB.INTCTRL = PORT_INT0LVL0_bm;

	CNT_clr_CLRD();	// Borro el latch llevandolo a 0.
	CNT_set_CLRD();	// Lo dejo en reposo en 1

}
//------------------------------------------------------------------------------------
void COUNTERS_init_handle( TaskHandle_t taskHandle, TaskHandle_t libHandle )
{


}
//------------------------------------------------------------------------------------
ISR ( PORTA_INT0_vect )
{
	// Esta ISR se activa cuando el contador D2 (PA2) genera un flaco se subida.
	// Solo avisa a la tarea principal ( que esta dormida ) que se levante y cuente
	// el pulso y haga el debounced.
	// Dado que los ISR de los 2 contadores son los que despiertan a la tarea, debo
	// indicarle de donde proviene el llamado
	wakeup_for_C0 = true;
	vTaskNotifyGiveFromISR( countersTaskHandle , &xHigherPriorityTaskWokenDigital );
	PORTA.INTFLAGS = PORT_INT0IF_bm;

}
//------------------------------------------------------------------------------------
ISR( PORTB_INT0_vect )
{
	// Esta ISR se activa cuando el contador D1 (PB2) genera un flaco se subida.
	// Solo avisa a la tarea principal ( que esta dormida ) que se levante y cuente
	// el pulso y haga el debounced.
	// Dado que los ISR de los 2 contadores son los que despiertan a la tarea, debo
	// indicarle de donde proviene el llamado
	wakeup_for_C1 = true;
	vTaskNotifyGiveFromISR( countersTaskHandle , &xHigherPriorityTaskWokenDigital );
	PORTB.INTFLAGS = PORT_INT0IF_bm;

}
//------------------------------------------------------------------------------------
