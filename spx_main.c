/*
 * main.c
 *
 *  Created on: 8 dic. 2018
 *      Author: pablo
 */

/*
 * Pendiente:
 * cOUNTERS: Pasar el handle al INIT para usarlo en las ISR
 *
 * Agregar a las librerias las funciones de diagnostico de CMD
 *
 * Problemas con los printf al controlar TERMINAL_PIN en cmd task
 *
 * Como determinar el tipo de arquitectura IO ?
 *
 * autocalibrar / span / offset
 * analog spx_8CH
 *
 *
 */

#include "spx.h"

//------------------------------------------------------------------------------------
int main( void )
{
	// Leo la causa del reset para trasmitirla en el init.
	wdg_resetCause = RST.STATUS;
	RST.STATUS = wdg_resetCause;
	//RST_PORF_bm | RST_EXTRF_bm | RST_BORF_bm | RST_WDRF_bm | RST_PDIRF_bm | RST_SRF_bm | RST_SDRF_bm;

	// Clock principal del sistema
	u_configure_systemMainClock();
	u_configure_RTC32();

	// Configuramos y habilitamos el watchdog a 8s.
	WDT_EnableAndSetTimeout(  WDT_PER_8KCLK_gc );
	if ( WDT_IsWindowModeEnabled() )
		WDT_DisableWindowMode();

	set_sleep_mode(SLEEP_MODE_PWR_SAVE);

	// Inicializacion de los devices del frtos-io
	if ( BAUD_PIN_115200() ) {
		frtos_open(fdTERM, 115200 );
	} else {
		frtos_open(fdTERM, 9600 );
	}

	frtos_open(fdGPRS, 115200);
	frtos_open(fdI2C, 100 );

	// Creo los semaforos
	sem_SYSVars = xSemaphoreCreateMutexStatic( &SYSVARS_xMutexBuffer );
	xprintf_init();
	FAT_init();

	startTask = false;

	// Creamos las tareas
	xTaskCreate(tkCtl, "CTL", tkCtl_STACK_SIZE, NULL, tkCtl_TASK_PRIORITY,  &xHandle_tkCtl );
	xTaskCreate(tkCmd, "CMD", tkCmd_STACK_SIZE, NULL, tkCmd_TASK_PRIORITY,  &xHandle_tkCmd);
	xTaskCreate(tkCounter, "COUNT", tkCounter_STACK_SIZE, NULL, tkCounter_TASK_PRIORITY,  &xHandle_tkCounter);
	xTaskCreate(tkData, "DATA", tkData_STACK_SIZE, NULL, tkData_TASK_PRIORITY,  &xHandle_tkData);
	xTaskCreate(tkDinputs, "DIGI", tkDinputs_STACK_SIZE, NULL, tkDinputs_TASK_PRIORITY,  &xHandle_tkDinputs);
	xTaskCreate(tkDoutputs, "DOUT", tkDoutputs_STACK_SIZE, NULL, tkDoutputs_TASK_PRIORITY,  &xHandle_tkDoutputs);

	/* Arranco el RTOS. */
	vTaskStartScheduler();

	// En caso de panico, aqui terminamos.
	exit (1);

}
//------------------------------------------------------------------------------------
void vApplicationIdleHook( void )
{
	// Como trabajo en modo tickless no entro mas en modo sleep aqui.
//	if ( sleepFlag == true ) {
//		sleep_mode();
//	}
}
//------------------------------------------------------------------------------------
// Define the function that is called by portSUPPRESS_TICKS_AND_SLEEP().
//------------------------------------------------------------------------------------
void vApplicationStackOverflowHook( TaskHandle_t xTask, signed char *pcTaskName )
{
	// Es invocada si en algun context switch se detecta un stack corrompido !!
	// Cuando el sistema este estable la removemos.
	// En FreeRTOSConfig.h debemos habilitar
	// #define configCHECK_FOR_STACK_OVERFLOW          2

//	xprintf_P( PSTR("PANIC:%s !!\r\n\0"),pcTaskName);

}
//------------------------------------------------------------------------------------

/* configSUPPORT_STATIC_ALLOCATION is set to 1, so the application must provide an
implementation of vApplicationGetIdleTaskMemory() to provide the memory that is
used by the Idle task. */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer,
                                    StackType_t **ppxIdleTaskStackBuffer,
                                    uint32_t *pulIdleTaskStackSize )
{
/* If the buffers to be provided to the Idle task are declared inside this
function then they must be declared static - otherwise they will be allocated on
the stack and so not exists after this function exits. */
static StaticTask_t xIdleTaskTCB;
static StackType_t uxIdleTaskStack[ configMINIMAL_STACK_SIZE ];

    /* Pass out a pointer to the StaticTask_t structure in which the Idle task's
    state will be stored. */
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

    /* Pass out the array that will be used as the Idle task's stack. */
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;

    /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
    Note that, as the array is necessarily of type StackType_t,
    configMINIMAL_STACK_SIZE is specified in words, not bytes. */
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}
//------------------------------------------------------------------------------------


