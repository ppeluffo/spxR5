/*
 * l_nvm.c
 *
 *  Created on: 29 de set. de 2017
 *      Author: pablo
 */

#include "l_nvm.h"

#include <stdio.h>

static void pv_NVMEE_FlushBuffer( void );
static void pv_NVMEE_WaitForNVM( void );

static uint8_t pv_ReadSignatureByte(uint16_t Address);
bool signature_ok = false;
char nvmid_str[32];

void nvm_eeprom_write_byte(eeprom_addr_t address, uint8_t value);
void nvm_eeprom_flush_buffer(void);
void nvm_eeprom_load_byte_to_buffer(uint8_t byte_addr, uint8_t value);
void nvm_eeprom_load_page_to_buffer(const uint8_t *values);
void nvm_eeprom_atomic_write_page(uint8_t page_addr);

//------------------------------------------------------------------------------------
// NVM EEPROM
//------------------------------------------------------------------------------------
int NVMEE_write_buffer(eeprom_addr_t address, const void *buf, uint16_t len)
{
	// Escribe un buffer en la internal EEprom de a 1 byte. No considero paginacion.
	// Utiliza una funcion que hace erase&write.
	// El problema es que la funcion nvm_eeprom_write_byte escribe una pagina y entonces
	// demora 1 pagina por c/byte.
	// La nueva solucion es paginar.

	// Esta funcion considera el parametro eeprom_addr_t como la direccion de la primera pagina !!!!!!

int bytes2wr = 0;
uint8_t eebuff_addr;
uint8_t page_addr;

	if ( address >= NVMEEPROM_SIZE) return(bytes2wr);

	page_addr = 0;
	while( len > 0 ) {
		// Borro el EEPROM buffer
		nvm_eeprom_flush_buffer();
		// Lo lleno ( hasta completar o terminar los datos )
		eebuff_addr = 0;
		while( ( eebuff_addr < NVMEE_PAGE_SIZE ) && ( len > 0 ) ) {
			nvm_eeprom_load_byte_to_buffer( eebuff_addr++, *(uint8_t*)buf );
			buf = (uint8_t*)buf + 1;
			len--;
			bytes2wr++;
		}
		// Grabo una pagina y avanzo a la siguiente
		nvm_eeprom_atomic_write_page( page_addr++ );
	}


/*
	while (len > 0) {
	//	NVMEE_write_byte(address++, *(uint8_t*)buf);
		nvm_eeprom_write_byte(address++, *(uint8_t*)buf);
        buf = (uint8_t*)buf + 1;
        len--;
        bytes2wr++;
       if (( bytes2wr % 32) == 0 )
//      	IO_set_LED_KA();
    	   vTaskDelay( ( TickType_t)( 10 ) );
//        	IO_clr_LED_KA();
    }

//	taskEXIT_CRITICAL();
*/
	return(bytes2wr);
}
//------------------------------------------------------------------------------------
void NVMEE_write_byte(eeprom_addr_t address, uint8_t value)
{
	// Escribe de a 1 byte en la internal EEprom

	if ( address >= NVMEEPROM_SIZE ) return;

    /*  Flush buffer to make sure no unintentional data is written and load
     *  the "Page Load" command into the command register.
     */
    pv_NVMEE_FlushBuffer();
    NVM.CMD = NVM_CMD_LOAD_EEPROM_BUFFER_gc;

    // Set address to write to
    NVM.ADDR2 = 0x00;
    NVM.ADDR1 = (address >> 8) & 0xFF;
    NVM.ADDR0 = address & 0xFF;

	/* Load data to write, which triggers the loading of EEPROM page buffer. */
	NVM.DATA0 = value;

	/*  Issue EEPROM Atomic Write (Erase&Write) command. Load command, write
	 *  the protection signature and execute command.
	 */
	NVM.CMD = NVM_CMD_ERASE_WRITE_EEPROM_PAGE_gc;
	NVM_EXEC();

}
//------------------------------------------------------------------------------------
uint8_t NVMEE_ReadByte( eeprom_addr_t address )
{
	/* Wait until NVM is not busy. */
//	pv_NVMEE_WaitForNVM();

	/* Set address to read from. */
//	NVM.ADDR0 = address & 0xFF;
//	NVM.ADDR1 = (address >> 8) & 0xFF;
//	NVM.ADDR2 = 0x00;

	/* Issue EEPROM Read command. */
//	NVM.CMD = NVM_CMD_READ_EEPROM_gc;
//	NVM_EXEC();

//	return NVM.DATA0;

uint8_t data;

	/* Wait until NVM is ready */
	nvm_wait_until_ready();
	eeprom_enable_mapping();
	data = *(uint8_t*)(address + MAPPED_EEPROM_START),
	eeprom_disable_mapping();
	return data;

}
//------------------------------------------------------------------------------------
int NVMEE_read_buffer(eeprom_addr_t address, char *buf, uint16_t len)
{

uint8_t rb;
int bytes2rd = 0;

	if ( address >= NVMEEPROM_SIZE) return(bytes2rd);

	taskENTER_CRITICAL();

	while (len--) {
		rb = NVMEE_ReadByte(address++);
		*buf++ = rb;
		bytes2rd++;
    }

	taskEXIT_CRITICAL();

	return(bytes2rd);
}
//------------------------------------------------------------------------------------
void NVMEE_EraseAll( void )
{
	/* Wait until NVM is not busy. */
	pv_NVMEE_WaitForNVM();

	/* Issue EEPROM Erase All command. */
	NVM.CMD = NVM_CMD_ERASE_EEPROM_gc;
	NVM_EXEC();

}
//------------------------------------------------------------------------------------
static void pv_NVMEE_FlushBuffer( void )
{
	// Flushea el eeprom page buffer.

	/* Wait until NVM is not busy. */
	pv_NVMEE_WaitForNVM();

	/* Flush EEPROM page buffer if necessary. */
	if ((NVM.STATUS & NVM_EELOAD_bm) != 0) {
		NVM.CMD = NVM_CMD_ERASE_EEPROM_BUFFER_gc;
		NVM_EXEC();
	}
}
//------------------------------------------------------------------------------------
static void pv_NVMEE_WaitForNVM( void )
{
	/* Wait for any NVM access to finish, including EEPROM.
 	 *
 	 *  This function is blcoking and waits for any NVM access to finish,
 	 *  including EEPROM. Use this function before any EEPROM accesses,
 	 *  if you are not certain that any previous operations are finished yet,
 	 *  like an EEPROM write.
 	 */
	do {
		/* Block execution while waiting for the NVM to be ready. */
	} while ((NVM.STATUS & NVM_NVMBUSY_bm) == NVM_NVMBUSY_bm);
}
//------------------------------------------------------------------------------------
// NVM ID
//------------------------------------------------------------------------------------
char *NVMEE_readID( void )
{

	if ( ! signature_ok ) {
		memset(nvmid_str, '\0', sizeof(nvmid_str));

		// Paso los bytes de identificacion a un string para su display.
		signature[ 0]=pv_ReadSignatureByte(LOTNUM0);
		signature[ 1]=pv_ReadSignatureByte(LOTNUM1);
		signature[ 2]=pv_ReadSignatureByte(LOTNUM2);
		signature[ 3]=pv_ReadSignatureByte(LOTNUM3);
		signature[ 4]=pv_ReadSignatureByte(LOTNUM4);
		signature[ 5]=pv_ReadSignatureByte(LOTNUM5);
		signature[ 6]=pv_ReadSignatureByte(WAFNUM );
		signature[ 7]=pv_ReadSignatureByte(COORDX0);
		signature[ 8]=pv_ReadSignatureByte(COORDX1);
		signature[ 9]=pv_ReadSignatureByte(COORDY0);
		signature[10]=pv_ReadSignatureByte(COORDY1);
/*
		signature[ 0] = nvm_read_production_signature_row( nvm_get_production_signature_row_offset(LOTNUM0));
		signature[ 1] = nvm_read_production_signature_row(nvm_get_production_signature_row_offset(LOTNUM1));
		signature[ 2] = nvm_read_production_signature_row(nvm_get_production_signature_row_offset(LOTNUM2));
		signature[ 3] = nvm_read_production_signature_row(nvm_get_production_signature_row_offset(LOTNUM3));
		signature[ 4] = nvm_read_production_signature_row(nvm_get_production_signature_row_offset(LOTNUM4));
		signature[ 5] = nvm_read_production_signature_row(nvm_get_production_signature_row_offset(LOTNUM5));
		signature[ 6] = nvm_read_production_signature_row(nvm_get_production_signature_row_offset(WAFNUM));
		signature[ 7] = nvm_read_production_signature_row(nvm_get_production_signature_row_offset(COORDX0));
		signature[ 8] = nvm_read_production_signature_row(nvm_get_production_signature_row_offset(COORDX1));
		signature[ 9] = nvm_read_production_signature_row(nvm_get_production_signature_row_offset(COORDY0));
		signature[10] = nvm_read_production_signature_row(nvm_get_production_signature_row_offset(COORDY1));
*/
		snprintf( nvmid_str, 32 ,"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",signature[0],signature[1],signature[2],signature[3],signature[4],signature[5],signature[6],signature[7],signature[8],signature[9],signature[10]  );
		signature_ok = true;

	}

	return(nvmid_str);
}
//------------------------------------------------------------------------------------
static uint8_t pv_ReadSignatureByte(uint16_t Address) {

	// Funcion que lee la memoria NVR, la calibration ROW de a una posicion ( de 16 bits )

	NVM_CMD = NVM_CMD_READ_CALIB_ROW_gc;
	uint8_t Result;
	__asm__ ("lpm %0, Z\n" : "=r" (Result) : "z" (Address));
	//  __asm__ ("lpm \n  mov %0, r0 \n" : "=r" (Result) : "z" (Address) : "r0");
	NVM_CMD = NVM_CMD_NO_OPERATION_gc;
	return Result;
}
//----------------------------------------------------------------------------------------
uint8_t NVMEE_fuses_read(uint8_t fuse)
{
	// Wait until NVM is ready
	nvm_wait_until_ready();

	// Set address
	NVM.ADDR0 = fuse;

	// Issue READ_FUSES command
	nvm_issue_command(NVM_CMD_READ_FUSES_gc);

	return NVM.DATA0;
}
//----------------------------------------------------------------------------------------
// TEST
//------------------------------------------------------------------------------------
int8_t NVMEE_test_write( char *addr, char *str )
{
	// Funcion de testing de la EEPROM interna del micro xmega
	// Escribe en una direccion de memoria un string
	// parametros: *addr > puntero char a la posicion de inicio de escritura
	//             *str >  puntero char al texto a escribir
	// retorna: -1 error
	//			nro.de bytes escritos

	// Calculamos el largo del texto a escribir en la eeprom.

int8_t xBytes = 0;
uint8_t length = 0;
char *p;


	p = str;
	while (*p != 0) {
		p++;
		length++;
	}

	xBytes = NVMEE_write( (uint16_t)(atoi(addr)), str, length );
	if ( xBytes == -1 )
		xprintf_P(PSTR("ERROR: NVMEE_test_write\r\n\0"));

	return(xBytes);
}
//------------------------------------------------------------------------------------
int8_t NVMEE_test_read( char *addr, char *size )
{
	// Funcion de testing de la EEPROM interna del micro xmega
	// Lee de una direccion de la memoria una cantiad de bytes y los imprime
	// parametros: *addr > puntero char a la posicion de inicio de lectura
	//             *size >  puntero char al largo de bytes a leer
	// retorna: -1 error
	//			nro.de bytes escritos

int8_t xBytes = 0;
char buffer[32];

	xBytes = NVMEE_read( (uint16_t)(atoi(addr)), buffer, (uint8_t)(atoi( size) ) );
	if ( xBytes == -1 )
		xprintf_P(PSTR("ERROR: NVMEE_test_read\r\n\0"));

	if ( xBytes > 0 )
		xprintf_P( PSTR( "%s\r\n\0"),buffer);

	return (xBytes );

}
//------------------------------------------------------------------------------------
// FUNCIONES PRIVADAS
//------------------------------------------------------------------------------------
static inline void nvm_exec(void)
{
	CCPWrite((uint8_t *)&NVM.CTRLA, NVM_CMDEX_bm);
}
//------------------------------------------------------------------------------------
void nvm_eeprom_write_byte(eeprom_addr_t address, uint8_t value)
{
	uint8_t old_cmd;

	/*  Flush buffer to make sure no unintentional data is written and load
	 *  the "Page Load" command into the command register.
	 */

	IO_set_LED_KA();
	old_cmd = NVM.CMD;
	nvm_eeprom_flush_buffer();
	// Wait until NVM is ready
	nvm_wait_until_ready();
	nvm_eeprom_load_byte_to_buffer(address, value);

	// Set address to write to
	NVM.ADDR2 = 0x00;
	NVM.ADDR1 = (address >> 8) & 0xFF;
	NVM.ADDR0 = address & 0xFF;

	/*  Issue EEPROM Atomic Write (Erase&Write) command. Load command, write
	 *  the protection signature and execute command.
	 */
	NVM.CMD = NVM_CMD_ERASE_WRITE_EEPROM_PAGE_gc;
	nvm_exec();
	NVM.CMD = old_cmd;

	IO_clr_LED_KA();
}
//------------------------------------------------------------------------------------
void nvm_eeprom_flush_buffer(void)
{
	// Wait until NVM is ready
	nvm_wait_until_ready();

	// Flush EEPROM page buffer if necessary
	if ((NVM.STATUS & NVM_EELOAD_bm) != 0) {
		NVM.CMD = NVM_CMD_ERASE_EEPROM_BUFFER_gc;
		nvm_exec();
	}
}
//------------------------------------------------------------------------------------
void nvm_eeprom_load_byte_to_buffer(uint8_t byte_addr, uint8_t value)
{
	// Wait until NVM is ready
	nvm_wait_until_ready();

	eeprom_enable_mapping();
	*(uint8_t*)(byte_addr + MAPPED_EEPROM_START) = value;
	eeprom_disable_mapping();
}
//------------------------------------------------------------------------------------
void nvm_eeprom_load_page_to_buffer(const uint8_t *values)
{
	// Wait until NVM is ready
	nvm_wait_until_ready();

	// Load multiple bytes into page buffer
	uint8_t i;
	for (i = 0; i < EEPROM_PAGE_SIZE; ++i) {
		nvm_eeprom_load_byte_to_buffer(i, *values);
		++values;
	}
}
//------------------------------------------------------------------------------------
void nvm_eeprom_atomic_write_page(uint8_t page_addr)
{
	// Wait until NVM is ready
	nvm_wait_until_ready();

	// Calculate page address
	uint16_t address = (uint16_t)(page_addr * EEPROM_PAGE_SIZE);

	// Set address
	NVM.ADDR2 = 0x00;
	NVM.ADDR1 = (address >> 8) & 0xFF;
	NVM.ADDR0 = address & 0xFF;

	// Issue EEPROM Atomic Write (Erase&Write) command
	nvm_issue_command(NVM_CMD_ERASE_WRITE_EEPROM_PAGE_gc);
}
//------------------------------------------------------------------------------------



