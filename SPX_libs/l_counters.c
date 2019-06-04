/*
 * l_counters.c
 *
 *  Created on: 8 dic. 2018
 *      Author: pablo
 */


#include "l_counters.h"

BaseType_t xHigherPriorityTaskWokenDigital = false;
TaskHandle_t countersTaskHandle0, countersTaskHandle1;

bool counter1_in_HS;

//------------------------------------------------------------------------------------
void COUNTERS_init( uint8_t cnt, TaskHandle_t taskHandle )
{

	switch ( cnt ) {
	case 0:
		CNT_config_CNT0();
		//	PORTA.PIN2CTRL = PORT_OPC_PULLUP_gc | PORT_ISC_RISING_gc;	// Sensa rising edge
		//PORTA.PIN2CTRL = PORT_OPC_PULLDOWN_gc | PORT_ISC_RISING_gc;		// Sensa rising edge. Menos consumo con pulldown.
		PORTA.PIN2CTRL = PORT_OPC_PULLUP_gc | PORT_ISC_FALLING_gc;	// Sensa falling edge
		//PORTA.INT0MASK = PIN2_bm;
		//PORTA.INTCTRL = PORT_INT0LVL0_bm;
		COUNTERS_enable_interrupt(0);
		countersTaskHandle0 = ( xTaskHandle ) taskHandle;
		break;

	case 1:
		CNT_config_CNT1();
		//	PORTA.PIN2CTRL = PORT_OPC_PULLUP_gc | PORT_ISC_RISING_gc;	// Sensa rising edge
		//PORTB.PIN2CTRL = PORT_OPC_PULLDOWN_gc | PORT_ISC_RISING_gc;		// Sensa rising edge. Menos consumo con pulldown.
		PORTB.PIN2CTRL = PORT_OPC_PULLUP_gc | PORT_ISC_FALLING_gc;	// Sensa falling edge
		//PORTB.INT0MASK = PIN2_bm;
		//PORTB.INTCTRL = PORT_INT0LVL0_bm;
		COUNTERS_enable_interrupt(1);
		countersTaskHandle1 = ( xTaskHandle ) taskHandle;
		// Por defecto cuenta en modo LS.
		//counter1_in_HS = false;
		break;
	}

	CNT_config_CLRD();
	CNT_clr_CLRD();	// Borro el latch llevandolo a 0.
	CNT_set_CLRD();	// Lo dejo en reposo en 1

	// https://www.freertos.org/FreeRTOS_Support_Forum_Archive/June_2005/freertos_Get_task_handle_1311096.html
	// The task handle is just a pointer to the TCB of the task - but outside of tasks.c the type is hidden as a void*.

}
//------------------------------------------------------------------------------------
void COUNTERS_disable_interrupt( uint8_t cnt )
{
	switch ( cnt ) {
	case 0:
		PORTA.INT0MASK = 0x00;
		PORTA.INTCTRL = 0x00;
		break;

	case 1:
		PORTB.INT0MASK = 0x00;
		PORTB.INTCTRL = 0x00;
		break;
	}
}
//------------------------------------------------------------------------------------
void COUNTERS_enable_interrupt( uint8_t cnt )
{
	switch ( cnt ) {
	case 0:
		PORTA.INT0MASK = PIN2_bm;
		PORTA.INTCTRL = PORT_INT0LVL0_bm;
		PORTA.INTFLAGS = PORT_INT0IF_bm;
		break;

	case 1:
		PORTB.INT0MASK = PIN2_bm;
		PORTB.INTCTRL = PORT_INT0LVL0_bm;
		PORTB.INTFLAGS = PORT_INT0IF_bm;
		break;
	}
}
//------------------------------------------------------------------------------------
uint32_t COUNTERS_readCnt1(void)
{
	return(counter1);
}
//------------------------------------------------------------------------------------
void COUNTERS_resetCnt1(void)
{
	counter1 = 0;
}
//------------------------------------------------------------------------------------
ISR ( PORTA_INT0_vect )
{
	// Esta ISR se activa cuando el contador D2 (PA2) genera un flaco se subida.
	// Solo avisa a la tarea principal ( que esta dormida ) que se levante y cuente
	// el pulso y haga el debounced.
	// Dado que los ISR de los 2 contadores son los que despiertan a la tarea, debo
	// indicarle de donde proviene el llamado
	vTaskNotifyGiveFromISR( countersTaskHandle0 , &xHigherPriorityTaskWokenDigital );
	//PORTA.INTFLAGS = PORT_INT0IF_bm;

}
//------------------------------------------------------------------------------------
ISR( PORTB_INT0_vect )
{
	// Esta ISR se activa cuando el contador D1 (PB2) genera un flaco se subida.
	// Solo avisa a la tarea principal ( que esta dormida ) que se levante y cuente
	// el pulso y haga el debounced.
	// Dado que los ISR de los 2 contadores son los que despiertan a la tarea, debo
	// indicarle de donde proviene el llamado
	//if ( ! counter1_in_HS )
	vTaskNotifyGiveFromISR( countersTaskHandle1 , &xHigherPriorityTaskWokenDigital );
	//PORTB.INTFLAGS = PORT_INT0IF_bm;
	//counter1++;
}
//------------------------------------------------------------------------------------
void COUNTERS_set_counter1_HS(void)
{
	counter1_in_HS = true;
}
//------------------------------------------------------------------------------------
void COUNTERS_set_counter1_LS(void)
{
	counter1_in_HS = false;
}
//------------------------------------------------------------------------------------
