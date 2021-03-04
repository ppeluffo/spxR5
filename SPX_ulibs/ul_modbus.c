/*
 * ul_modbus.c
 *
 *  Created on: 2 set. 2020
 *      Author: pablo
 *
 *  Problema con los ejemplos:
 *  Los chinos al leer los holding registers (fc=0x03) consideran que la respuesta
 *  es el mismo nro.de bytes que solicitaron pero la norma dice que los registros
 *  de holding son de 16 bits por lo tanto si pedimos 2 registros, recibimos 4 bytes !!
 *
 *  En modbus el 0x00 es valido por lo tanto hay que considerarlo en el manejo tanto en
 *  la trasmision de datos como la recepcion.
 *  El problema es que con la implementacion actual de frtos_read/write, se utilizan
 *  buffers y el 0x00 es el NULL que se detecta como fin de datos.
 *  #
 *  WRITE:
 *  Defino la funcion void frtos_putchar( file_descriptor_t fd ,const char cChar )
 *  que escribe directamente el byte en la UART.
 *  READ:
 *
 *  Ejemplo de FRAME:
 *  15 03 00 6B 00 03
 *  15: The SlaveID address (21 = 0x15)
 *  03: The function code (read analog output holding registers)
 *  006B: The data address of the first register requested (40108 - 40001 offset = 107 = 0x6B).
 *  0003: The total number of registers requested. (read 3 registers 40108 to 40110)
 *
 *  El CRC calculado es:
 *  7703: The CRC (cyclic redundancy check) for error checking.
 *
 *  El frame completo a transmitir es:
 *  15 03 00 6B 00 03 77 03
 *
 */



#include "spx.h"
#include "tkComms.h"
#include "frtos-io.h"

const uint8_t auchCRCHi[] PROGMEM = {
		0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
		0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
		0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01,
		0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
		0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81,
		0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
		0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01,
		0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
		0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
		0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
		0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01,
		0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
		0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
		0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
		0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01,
		0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
		0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
		0x40
	} ;

const uint8_t auchCRCLo[] PROGMEM = {
		0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06, 0x07, 0xC7, 0x05, 0xC5, 0xC4,
		0x04, 0xCC, 0x0C, 0x0D, 0xCD, 0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09,
		0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A, 0x1E, 0xDE, 0xDF, 0x1F, 0xDD,
		0x1D, 0x1C, 0xDC, 0x14, 0xD4, 0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
		0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 0xF2, 0x32, 0x36, 0xF6, 0xF7,
		0x37, 0xF5, 0x35, 0x34, 0xF4, 0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A,
		0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29, 0xEB, 0x2B, 0x2A, 0xEA, 0xEE,
		0x2E, 0x2F, 0xEF, 0x2D, 0xED, 0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
		0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60, 0x61, 0xA1, 0x63, 0xA3, 0xA2,
		0x62, 0x66, 0xA6, 0xA7, 0x67, 0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F,
		0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68, 0x78, 0xB8, 0xB9, 0x79, 0xBB,
		0x7B, 0x7A, 0xBA, 0xBE, 0x7E, 0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
		0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71, 0x70, 0xB0, 0x50, 0x90, 0x91,
		0x51, 0x93, 0x53, 0x52, 0x92, 0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C,
		0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B, 0x99, 0x59, 0x58, 0x98, 0x88,
		0x48, 0x49, 0x89, 0x4B, 0x8B, 0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
		0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42, 0x43, 0x83, 0x41, 0x81, 0x80,
		0x40
	};


uint16_t pv_modbus_CRC16( uint8_t *msg, uint8_t msg_size );

#define DF_MBUS ( systemVars.debug == DEBUG_MODBUS )
#define MBUS_TXMSG_LENGTH	AUX_RXBUFFER_LEN
#define MBUS_RXMSG_LENGTH	AUX_RXBUFFER_LEN

uint8_t mbus_rxbuffer[MBUS_RXMSG_LENGTH];

//------------------------------------------------------------------------------------
void modbus_init(void)
{
	aux_prender();
}
//------------------------------------------------------------------------------------
// FUNCIONES DE CONFIGURACION Y STATUS
//------------------------------------------------------------------------------------
bool modbus_config_slave_address( char *address)
{
	// Configura la direccion del slave.
	// Se da la direccion en hexadecimal ( 0x??)

char *ptr;

	systemVars.modbus_conf.modbus_slave_address = strtol(address, &ptr, 16);
	return(true);
}
//------------------------------------------------------------------------------------
bool modbus_config_channel(uint8_t channel,char *s_name,char *s_addr,char *s_length,char *s_rcode)
{

//	xprintf_P(PSTR("DEBUG: channel=%d\r\n\0"), channel);
//	xprintf_P(PSTR("DEBUG: name=%s\r\n\0"), s_name);
//	xprintf_P(PSTR("DEBUG: addr=%s\r\n\0"), s_addr);
//	xprintf_P(PSTR("DEBUG: length=%s\r\n\0"), s_length);
//	xprintf_P(PSTR("DEBUG: rcode=%s\r\n\0"), s_rcode);

char *ptr;

	if ( u_control_string(s_name) == 0 ) {
		xprintf_P( PSTR("DEBUG MODBUS ERROR: A%d[%s]\r\n\0"), channel, s_name );
		return( false );
	}

	if (( s_name == NULL ) || ( s_addr == NULL) || (s_length == NULL) || ( s_rcode == NULL)) {
		return(false);
	}

	if ( ( channel >=  0) && ( channel < MODBUS_CHANNELS ) ) {

		snprintf_P( systemVars.modbus_conf.var_name[channel], PARAMNAME_LENGTH, PSTR("%s\0"), s_name );
		systemVars.modbus_conf.var_address[channel] = strtol(s_addr, &ptr, 16);
		systemVars.modbus_conf.var_length[channel] = strtol(s_length, &ptr, 16);
		systemVars.modbus_conf.var_function_code[channel] = strtol(s_rcode, &ptr, 16);
		return(true);
	}

	return(false);
}
//------------------------------------------------------------------------------------
void modbus_config_defaults(void)
{

uint8_t channel = 0;

	systemVars.modbus_conf.modbus_slave_address = 0x00;

	for ( channel = 0; channel < MODBUS_CHANNELS; channel++) {
		snprintf_P( systemVars.modbus_conf.var_name[channel], PARAMNAME_LENGTH, PSTR("X\0") );
		systemVars.modbus_conf.var_address[channel] = 0x00;
		systemVars.modbus_conf.var_length[channel] = 0;
		systemVars.modbus_conf.var_function_code[channel] = 0;
	}
}
//------------------------------------------------------------------------------------
void modbus_status(void)
{
	xprintf_P( PSTR(">Modbus:\r\n\0"));
	xprintf_P( PSTR("  mbus sladdr: 0x%02x\r\n\0"), systemVars.modbus_conf.modbus_slave_address );
	xprintf_P( PSTR("  m0 [ addr:0x%04x, size:0x%02x, fc:0x%02x, %s]\r\n\0"), systemVars.modbus_conf.var_address[0], systemVars.modbus_conf.var_length[0], systemVars.modbus_conf.var_function_code[0], systemVars.modbus_conf.var_name[0] );
	xprintf_P( PSTR("  m1 [ addr:0x%04x, size:0x%02x, fc:0x%02x, %s]\r\n\0"), systemVars.modbus_conf.var_address[1], systemVars.modbus_conf.var_length[1], systemVars.modbus_conf.var_function_code[1], systemVars.modbus_conf.var_name[1] );

}
//------------------------------------------------------------------------------------
// FUNCIONES AUXILIARES
//------------------------------------------------------------------------------------
uint8_t modbus_hash(void)
{
	// Calcula el hash de la configuracion para mandar en los frames de INIT.

 // https://portal.u-blox.com/s/question/0D52p00008HKDMyCAP/python-code-to-generate-checksums-so-that-i-may-validate-data-coming-off-the-serial-interface

uint16_t i;
uint8_t hash = 0;
//char dst[48];
char *p;
int16_t free_size = sizeof(hash_buffer);

	//	uint8_t modbus_slave_address;
	//	char var_name[MODBUS_CHANNELS][PARAMNAME_LENGTH];
	//	uint16_t var_address[MODBUS_CHANNELS];				// Direccion en el slave de la variable a leer
	//	uint8_t var_length[MODBUS_CHANNELS];				// Cantidad de bytes a leer
	//	uint8_t var_function_code[MODBUS_CHANNELS];			// Funcion de lectura (3-Holding reg, 4-Normal reg)
	//  SLA:0x%02x;M0:MBU1,0x0004,0x02,0x02;M1:MBU2,0x0004,0x02,0x02;

	memset(hash_buffer,'\0', sizeof(hash_buffer));
	i = 0;
	memset(hash_buffer,'\0', sizeof(hash_buffer));
	i = snprintf_P( &hash_buffer[i], free_size, PSTR("SLA:0x%02x;"), systemVars.modbus_conf.modbus_slave_address );
	free_size = (  sizeof(hash_buffer) - i );
	if ( free_size < 0 ) goto exit_error;
	p = hash_buffer;
	while (*p != '\0') {
		hash = u_hash(hash, *p++);
	}
	i=0;
	memset(hash_buffer,'\0', sizeof(hash_buffer));
	i += snprintf_P( &hash_buffer[i], sizeof(hash_buffer), PSTR("M0:%s,"), systemVars.modbus_conf.var_name[0] );
	i += snprintf_P(&hash_buffer[i], sizeof(hash_buffer), PSTR("0x%04x,0x%02x,0x%02x;"), systemVars.modbus_conf.var_address[0],systemVars.modbus_conf.var_length[0],systemVars.modbus_conf.var_function_code[0] );
	free_size = (  sizeof(hash_buffer) - i );
		if ( free_size < 0 ) goto exit_error;
	p = hash_buffer;
	while (*p != '\0') {
		hash = u_hash(hash, *p++);
	}
	i = 0;
	memset(hash_buffer,'\0', sizeof(hash_buffer));
	i += snprintf_P( &hash_buffer[i], sizeof(hash_buffer), PSTR("M1:%s,"), systemVars.modbus_conf.var_name[1] );
	i += snprintf_P(&hash_buffer[i], sizeof(hash_buffer), PSTR("0x%04x,0x%02x,0x%02x;"), systemVars.modbus_conf.var_address[1],systemVars.modbus_conf.var_length[1],systemVars.modbus_conf.var_function_code[1] );
	free_size = (  sizeof(hash_buffer) - i );
		if ( free_size < 0 ) goto exit_error;
	p = hash_buffer;
	while (*p != '\0') {
		hash = u_hash(hash, *p++);
	}

	return(hash);

exit_error:
	xprintf_P( PSTR("COMMS: modbus_hash ERROR !!!\r\n\0"));
	return(0x00);

}
//------------------------------------------------------------------------------------
void modbus_print(file_descriptor_t fd,  uint16_t mbus[] )
{
	// Imprime los canales configurados ( no X ) en un fd ( tty_gprs,tty_xbee,tty_term) en
	// forma formateada.
	// Los lee de una estructura array pasada como src

uint8_t channel = 0;

	for ( channel = 0; channel < MODBUS_CHANNELS; channel++) {
		if ( ! strcmp ( systemVars.modbus_conf.var_name[channel], "X" ) )
			continue;
		xfprintf_P(fd, PSTR("%s:%u;"), systemVars.modbus_conf.var_name[channel], mbus[channel] );
	}

}
//------------------------------------------------------------------------------------
uint16_t pv_modbus_CRC16( uint8_t *msg, uint8_t msg_size )
{

uint8_t CRC_Hi = 0xFF;
uint8_t CRC_Lo = 0xFF;
uint8_t tmp;

uint8_t uindex;

	while (msg_size--) {
		uindex = CRC_Lo ^ *msg++;
		tmp = (PGM_P) pgm_read_byte_far(&(auchCRCHi[uindex]));
		CRC_Lo = CRC_Hi ^ tmp;
		tmp = (PGM_P)pgm_read_byte_far(&(auchCRCLo[uindex]));
		CRC_Hi = tmp;
	}

	return( CRC_Hi << 8 | CRC_Lo);

}
//------------------------------------------------------------------------------------
// FUNCIONES DE TRASMISION / RECEPCION DE FRAMES
//------------------------------------------------------------------------------------
void modbus_txFrame(bool f_debug, uint8_t *data, uint8_t data_size )
{
	// Arma el frame: SLA | FC | DATA_0....DATA_N | CRCL | CRCH y lo transmite
	// OJO: En MODBUS, los bytes pueden ser 0x00 y no deben ser interpretados como NULL
	// por lo tanto hay que usar el data_size


uint16_t crc;
uint8_t crc_lo, crc_hi;
uint8_t i;

	// Transmite un mensaje MODBUS
	// Habilita el RTS para indicar que transmite
	// Apaga la recepcion. ( El IC 485 copia la tx y rx )

	xprintf_P(PSTR("MODBUS TX:\r\n"));

	// Calculamos el CRC del buffer y lo agrego al final
	crc = pv_modbus_CRC16( data, data_size );
	crc_lo = (uint8_t)( crc & 0x00FF );			// CRC Low
	crc_hi = (uint8_t)( (crc & 0xFF00) >> 8 );	// CRC High

	xprintf_P(PSTR("MODBUS TX: crc_lo=%02x\r\n"), crc_lo);
	xprintf_P(PSTR("MODBUS TX: crc_hi=%02x\r\n"), crc_hi);

	data[data_size++] = crc_lo;
	data[data_size++] = crc_hi;

	// Transmito
	// borro buffers y espero 3.5T (9600) = 3.5ms ( START )
	aux_flush_TX_buffer();
	aux_flush_RX_buffer();
	vTaskDelay( (portTickType)( 10 / portTICK_RATE_MS ) );

	// Log
	if ( f_debug ) {
		xprintf_P(PSTR("MODBUS TX(%d):"), data_size);
		//for ( i = 0 ; i < ( data_size + 2) ; i++ ) {
		for ( i = 0 ; i < data_size ; i++ ) {
			xprintf_P(PSTR("[0x%02X]"), data[i]);
		}
		xprintf_P(PSTR("\r\n\0"));
	}

	// Transmito
	//i = xnprintf_MBUS( (const char *)data, (data_size + 2) );
	i = xnprintf_MBUS( (const char *)data, data_size );

	xprintf_P(PSTR("MODBUS TX END(%d):\r\n"),i);
}
//------------------------------------------------------------------------------------
bool modbus_rxFrame( bool f_debug, uint8_t *data, uint8_t max_data_size )
{
	// https://stackoverflow.com/questions/13254432/modbus-is-there-a-maximum-time-a-device-can-take-to-respond
	// Esperamos la respuesta 1s y si no llego damos timeout

	// Lee el buffer de recepcion del puerto AUX (MODBUS )
	// Es por poleo de modo que una vez que transmiti un frame, debo esperar un tiempo que llegue la respuesta
	// y luego usar esta funcion para decodificar la lectura.

uint16_t crc_msg;
uint16_t crc_calc;
char *p;
uint8_t length;
uint8_t i;

	// Leo el rx. No uso strcpy por el tema de los NULL.
	memset( data, '\0', max_data_size );
	length = (uint8_t)aux_get_buffer_ptr();
	if ( length > max_data_size )
		length = max_data_size;

	// Copio todos los bytes recibidos.
	// strncpy( (char *)&mbus_msg, aux_get_buffer(),  length );
	p = aux_get_buffer();
	for ( i=0; i < length; i++ ) {
		data[i] = *p++;
	}


	// Paso 1: Log
	if (f_debug) {
		xprintf_P(PSTR("MBUS RX SIZE:[%d]\r\n"), length);
		xprintf_P(PSTR("MBUS RX BUFF: "), length);
		for ( i = 0 ; i < length; i++ ) {
			xprintf_P(PSTR("[0x%02X]"), data[i]);
		}
		xprintf_P(PSTR("\r\n"));
	}

	// Paso 2: Largo.
	if ( length < 3) {
		// Timeout error:
		xprintf_P(PSTR("MBUS_RX: TIMEOUT ERROR\r\n\0"));
		return(false);
	}

	// Pass3: Calculo y verifico el CRC
	crc_calc = pv_modbus_CRC16( data, (length - 2));
	crc_msg = data[length-2] + ( data[length-1] << 8 );

	if (f_debug) {
		xprintf_P(PSTR("MBUS_RX CRC: rx[0x%02x], calc[0x%02x]\r\n\0"), crc_msg, crc_calc);
	}

	if ( crc_calc != crc_msg) {
		xprintf_P(PSTR("MBUS_RX: CRC ERROR: rx[0x%02x], calc[0x%02x]\r\n\0"), crc_msg, crc_calc);
		// De acuerdo al protocolo se ignora el frame recibido con errores CRC
		return(false);
	}

	return(true);

}
//------------------------------------------------------------------------------------
void modbus_decodeRxFrame ( bool f_debug, uint8_t *data, uint8_t data_size )
{
	// Decodifica el frame recibido ( pasado en *data )
	// Lo que hacemos es escribir y leer registros del PLC.
	// (0x03) Read Holding Registers
	// (0x04) Read Input Registers
	// (0x06) Write Single Register

	// Por ahora solo imprimo el resultado.

uint8_t function_code;
uint8_t byte_count;
uint8_t i;

	function_code = data[1];
	switch(function_code) {
	case 0x3:
		xprintf_P(PSTR("MODBUS: FC=0x03: Read Holding Register\r\n"));
		byte_count = data[2];
		xprintf_P(PSTR("MODBUS: byte_count=%d\r\n"), byte_count);
		break;
	case 0x4:
		xprintf_P(PSTR("MODBUS: FC=0x04: Read Input Register\r\n"));
		byte_count = data[2];
		xprintf_P(PSTR("MODBUS: byte_count=%d\r\n"), byte_count);
		break;
	case 0x6:
		xprintf_P(PSTR("MODBUS: FC=0x06: Write Single Register\r\n"));
		break;
	default:
		xprintf_P(PSTR("MODBUS: Codigo de funcion no soportado.\r\n"));
		break;
	}

	// Imprimo el payload de la respuesta
	xprintf_P(PSTR("MODBUS: data:"));
	for (i=0; i < data_size; i++) {
		xprintf_P(PSTR("[0x%02x]"), data[i]);
	}
	xprintf_P(PSTR("\r\n"));

}
//------------------------------------------------------------------------------------
// FUNCIONES DE TESTING
//------------------------------------------------------------------------------------
void modbus_poll_test( char *arg_ptr[16] )
{
	// Recibe el argv con los datos en char hex para trasmitir el frame.
	// El formato es: bytes_counts {bytes in hex}

char *ptr;
uint8_t byte_count;
uint8_t data[MBUS_TXMSG_LENGTH];
uint8_t i;

	xprintf_P(PSTR("MODBUS TEST:\r\n"));

	// Vemos cuantos bytes vienen
	byte_count = strtol(arg_ptr[3], &ptr,  16);
	xprintf_P(PSTR("MODBUS TEST: byte count = %d\r\n"), byte_count);

	// Armo el frame.
	memset( data,'\0', MBUS_TXMSG_LENGTH);
	for (i=0; i < byte_count; i++ ) {
		data[i] = strtol(arg_ptr[4+i], &ptr,  16);
		xprintf_P(PSTR("MODBUS TEST: byte_count=[%d], data=[%02x]\r\n"),i,data[i]);
	}

	// Proceso el frame
	modbus_txFrame(true, data, byte_count );
	// Espero la respuesta
	vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );
	modbus_rxFrame(true, data, MBUS_TXMSG_LENGTH );
//	modbus_decodeRxFrame(true, data, MBUS_TXMSG_LENGTH );

	xprintf_P(PSTR("MODBUS TEST END:\r\n"));

}
//------------------------------------------------------------------------------------
void modbus_link_test( void )
{
	// Transmite un string ASCII por el bus.


uint8_t byte_count;
uint8_t data[MBUS_TXMSG_LENGTH];

	xprintf_P(PSTR("MODBUS LINK TEST:\r\n"));

	byte_count = sizeof("LINK TEST MODBUS\r\n");
	strncpy(data, "LINK TEST MODBUS\r\n", byte_count );

	xnprintf_MBUS( &data, byte_count );

	xprintf_P(PSTR("MODBUS LINK TEST END (%d):\r\n"), byte_count);

}
//------------------------------------------------------------------------------------
