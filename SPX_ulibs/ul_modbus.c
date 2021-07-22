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
 *  MODBUS R1:
 *  No usamos el factor de divisor_p10. Por defecto queda en 1.
 *  No se configura ni se muestra en status.
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


#define DF_MBUS ( systemVars.debug == DEBUG_MODBUS )
#define MBUS_TXMSG_LENGTH	AUX_RXBUFFER_LEN
#define MBUS_RXMSG_LENGTH	AUX_RXBUFFER_LEN

// Estructura para el manejo de frames (TX/RX)
struct {
	uint8_t tx_buffer[MBUS_TXMSG_LENGTH];
	uint8_t rx_buffer[MBUS_RXMSG_LENGTH];
	uint8_t tx_size;
	uint8_t rx_size;
	uint8_t rx_payload;
} mbus_data_s;

typedef struct {
	uint8_t sla_address;
	uint16_t address;
	uint8_t length;
	uint8_t function_code;
	char type;
	uint8_t divisor_p10;
} mbus_control_t;

uint16_t pv_modbus_CRC16( uint8_t *msg, uint8_t msg_size );
void pv_modbus_io ( bool f_debug, mbus_control_t *mbus_ctl, hold_reg_t *hreg );
void pv_modbus_make_tx_frame( mbus_control_t *mbus_ctl );
void pv_modbus_txmit_frame( bool f_debug );
bool pv_modbus_rcvd_frame( bool f_debug  );
void pv_modbus_decode_frame ( bool f_debug, mbus_control_t *mbus_ctl, hold_reg_t *hreg, bool read_status );
void pv_modbus_decode_function3( hold_reg_t *hreg, char type);
void pv_modbus_decode_function6( hold_reg_t *hreg, char type);
void pv_modbus_print_value(  bool f_debug, mbus_control_t *mbus_ctl, hold_reg_t *hreg );

//------------------------------------------------------------------------------------
void modbus_init(void)
{
	aux_prender();
}
//------------------------------------------------------------------------------------
// FUNCIONES DE CONFIGURACION
//------------------------------------------------------------------------------------
bool modbus_config_slave_address( char *address)
{
	// Configura la direccion del slave.
	// Se da la direccion en hexadecimal ( 0x??)

	systemVars.modbus_conf.modbus_slave_address = atoi(address);
	return(true);
}
//------------------------------------------------------------------------------------
bool modbus_config_waiting_poll_time( char *s_waiting_poll_time)
{
	// Configura el tiempo que espera una respuesta

	systemVars.modbus_conf.waiting_poll_time = atoi(s_waiting_poll_time);
	return(true);
}
//------------------------------------------------------------------------------------
bool modbus_config_channel(uint8_t channel,char *s_name,char *s_addr,char *s_length,char *s_rcode, char *s_type, char *s_divisor_p10 )
{

//	xprintf_P(PSTR("DEBUG: channel=%d\r\n\0"), channel);
//	xprintf_P(PSTR("DEBUG: name=%s\r\n\0"), s_name);
//	xprintf_P(PSTR("DEBUG: addr=%s\r\n\0"), s_addr);
//	xprintf_P(PSTR("DEBUG: length=%s\r\n\0"), s_length);
//	xprintf_P(PSTR("DEBUG: rcode=%s\r\n\0"), s_rcode);

char type;

	if ( u_control_string(s_name) == 0 ) {
		xprintf_P( PSTR("MODBUS CONFIG ERROR: ch:%d, [%s]\r\n\0"), channel, s_name );
		return( false );
	}

	if (( s_name == NULL ) || ( s_addr == NULL) || (s_length == NULL) || ( s_rcode == NULL)) {
		return(false);
	}

	type = toupper(s_type[0]);

	if ( ( type != 'F' ) && ( type != 'I') ) {
		xprintf_P(PSTR("MODBUS CONFIG ERROR: El tipo de canal modbus debe ser 'F' o 'I'\r\n"));
		return(false);
	}

	if ( ( channel >=  0) && ( channel < MODBUS_CHANNELS ) ) {

		snprintf_P( systemVars.modbus_conf.mbchannel[channel].name, PARAMNAME_LENGTH, PSTR("%s\0"), s_name );
		systemVars.modbus_conf.mbchannel[channel].address = atoi(s_addr);
		systemVars.modbus_conf.mbchannel[channel].length = atoi(s_length);
		systemVars.modbus_conf.mbchannel[channel].function_code = atoi(s_rcode);
		systemVars.modbus_conf.mbchannel[channel].type = type;
		systemVars.modbus_conf.mbchannel[channel].divisor_p10 = atoi(s_divisor_p10);
		return(true);
	}

	return(false);
}
//------------------------------------------------------------------------------------
void modbus_config_defaults(void)
{

uint8_t channel = 0;

	systemVars.modbus_conf.modbus_slave_address = 0x00;
	systemVars.modbus_conf.waiting_poll_time = 500;

	for ( channel = 0; channel < MODBUS_CHANNELS; channel++) {
		snprintf_P( systemVars.modbus_conf.mbchannel[channel].name, PARAMNAME_LENGTH, PSTR("X\0") );
		systemVars.modbus_conf.mbchannel[channel].address = 0;
		systemVars.modbus_conf.mbchannel[channel].length = 0;
		systemVars.modbus_conf.mbchannel[channel].function_code = 0;
		systemVars.modbus_conf.mbchannel[channel].type = 'F';
		systemVars.modbus_conf.mbchannel[channel].divisor_p10 = 1;
	}
}
//------------------------------------------------------------------------------------
// FUNCIONES AUXILIARES
//------------------------------------------------------------------------------------
void modbus_print(file_descriptor_t fd,  uint16_t mbus[] )
{
	// Imprime los canales configurados ( no X ) en un fd ( tty_gprs,tty_xbee,tty_term) en
	// forma formateada.
	// Los lee de una estructura array pasada como src

uint8_t channel = 0;

	for ( channel = 0; channel < MODBUS_CHANNELS; channel++) {
		if ( ! strcmp ( systemVars.modbus_conf.mbchannel[channel].name, "X" ) )
			continue;
		xfprintf_P(fd, PSTR("%s:%u;"), systemVars.modbus_conf.mbchannel[channel].name, mbus[channel] );
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
void modbus_poll_channel( bool f_debug, uint8_t channel, hold_reg_t *hreg )
{
	// Polea (tx, rx, decode ) un canal modbus.
	// Lleno la estructura mbus_ctl y la paso a mbus_io que es quien se encarga del resto.

mbus_control_t mbus_ctl;

	memset( hreg->rep_str, '\0', 4 );
	memset ( &mbus_ctl, '\0', sizeof(mbus_ctl));

	mbus_ctl.sla_address = systemVars.modbus_conf.modbus_slave_address;
	mbus_ctl.function_code = systemVars.modbus_conf.mbchannel[channel].function_code;
	mbus_ctl.address = systemVars.modbus_conf.mbchannel[channel].address;
	mbus_ctl.length = systemVars.modbus_conf.mbchannel[channel].length;
	mbus_ctl.type = systemVars.modbus_conf.mbchannel[channel].type;

	pv_modbus_io( f_debug, &mbus_ctl, hreg);

}
//------------------------------------------------------------------------------------
void modbus_write_output_register ( bool f_debug, uint16_t address, char type, hold_reg_t *hreg )
{

	// Recibo un reg_address y un valor ( en hreg ) y un tipo.
	// Transmito, espero la respuesta y decodifico
	// Como lo uso para escribir un registro ( fc=0x6), no retorno la respuesta
	//

mbus_control_t mbus_ctl;
uint8_t function_code;
uint8_t length;				// Cantidad de bytes a leer.

	// FCODE
	if ( toupper(type) == 'F') {
		function_code = 0x10;
		length = 4;
	} else if ( toupper(type) == 'I') {
		function_code = 0x06;
		length = 2;
	} else {
		return;
	}

	memset( hreg->rep_str, '\0', 4 );
	memset ( &mbus_ctl, '\0', sizeof(mbus_ctl));

	mbus_ctl.sla_address = systemVars.modbus_conf.modbus_slave_address;
	mbus_ctl.function_code = function_code;
	mbus_ctl.address = address;
	mbus_ctl.length = length;
	mbus_ctl.type = type;

	pv_modbus_io( f_debug, &mbus_ctl, hreg);

}
//------------------------------------------------------------------------------------
// FUNCIONES DE TESTING
//------------------------------------------------------------------------------------
void modbus_test_generic_poll( char *arg_ptr[16] )
{
	// Recibe el argv con los datos en char hex para trasmitir el frame.
	// El formato es: { type(F|I} sla fcode addr length }
	// write modbus genpoll F 9 3 4118 2
	// TX: (len=8):[0x09][0x03][0x10][0x16][0x00][0x02][0x20][0x47]

hold_reg_t hreg;
mbus_control_t mbus_ctl;

	xprintf_P(PSTR("MODBUS GENPOLL START:\r\n"));

	memset( hreg.rep_str, '\0', 4 );
	memset ( &mbus_ctl, '\0', sizeof(mbus_ctl));

	mbus_ctl.type = toupper(arg_ptr[3][0]);
	mbus_ctl.sla_address = atoi(arg_ptr[4]);
	mbus_ctl.function_code = atoi(arg_ptr[5]);
	mbus_ctl.address = atoi(arg_ptr[6]);
	mbus_ctl.length = atoi(arg_ptr[7]);
	mbus_ctl.divisor_p10 = 1;

	pv_modbus_io( true, &mbus_ctl, &hreg);

	xprintf_P(PSTR("MODBUS GENPOLL END:\r\n"));

}
//------------------------------------------------------------------------------------
void modbus_test_link( void )
{
	// Transmite un string ASCII por el bus.


uint8_t byte_count;
uint8_t data[MBUS_TXMSG_LENGTH];

	xprintf_P(PSTR("MODBUS LINK TEST:\r\n"));

	byte_count = sizeof("LINK TEST MODBUS\r\n");
	strncpy( (char *)data, "LINK TEST MODBUS\r\n", byte_count );

	xnprintf_MBUS( (char *)data, byte_count );

	xprintf_P(PSTR("MODBUS LINK TEST END (%d):\r\n"), byte_count);

}
//------------------------------------------------------------------------------------
void modbus_test_float( char *s_nbr )
{
	// Funcion de testing que lee un string representando a un float
	// y lo imprime como float y como los 4 bytes

union {
	float fvalue;
	uint8_t bytes_raw[4];
} rec;

	xprintf_P(PSTR("MODBUS FLOAT:\r\n"));

	rec.fvalue = atof(s_nbr);
	xprintf_P(PSTR("Str: %s\r\n"), s_nbr);
	xprintf_P(PSTR("Float=%f\r\n"), rec.fvalue);
	xprintf_P(PSTR("Bytes=b0[0x%02x] b1[0x%02x] b2[0x%02x] b3[0x%02x]\r\n"), rec.bytes_raw[0], rec.bytes_raw[1], rec.bytes_raw[2], rec.bytes_raw[3]);

}
//------------------------------------------------------------------------------------
void modbus_test_int( char *s_nbr )
{
	// Funcion de testing que lee un string representando a un integer
	// y lo imprime como int y como los 4 bytes

union {
	uint32_t ivalue;
	uint8_t bytes_raw[4];
} rec;

	xprintf_P(PSTR("MODBUS INT:\r\n"));

	rec.ivalue = atol(s_nbr);
	xprintf_P(PSTR("Str: %s\r\n"), s_nbr);
	xprintf_P(PSTR("Int=%lu\r\n"), rec.ivalue);
	xprintf_P(PSTR("Bytes=b0[0x%02x] b1[0x%02x] b2[0x%02x] b3[0x%02x]\r\n"), rec.bytes_raw[0], rec.bytes_raw[1], rec.bytes_raw[2], rec.bytes_raw[3]);

}
//-----------------------------------------------------------------------------------
void modbus_test_channel_poll ( char *s_channel)
{

uint8_t channel;
hold_reg_t hreg;

	xprintf_P(PSTR("MODBUS CHPOLL START:\r\n"));

	channel = atoi(s_channel);

	if ( channel >= MODBUS_CHANNELS ) {
		xprintf_P(PSTR("ERROR: Nro.canal < %d\r\n"), MODBUS_CHANNELS);
		return;
	}

	if ( ! strcmp ( systemVars.modbus_conf.mbchannel[channel].name, "X" ) ) {
		xprintf_P(PSTR("ERROR: Canal no definido (X)\r\n"));
		return;
	}

	modbus_poll_channel(true, channel, &hreg );

	xprintf_P(PSTR("MODBUS CHPOLL END.\r\n"));
}
//------------------------------------------------------------------------------------
void modbus_test_write_output (char *s_address, char *s_type, char *s_value )
{
	// fcode address type value

uint16_t address;
char type;
hold_reg_t hreg;

	address = atoi(s_address);
	type = toupper(s_type[0]);
	if  ( type == 'F') {
		hreg.fvalue = atof(s_value);
	} else {
		hreg.ivalue = atoi(s_value);
	}

	modbus_write_output_register ( DF_MBUS , address, type, &hreg );
}
//------------------------------------------------------------------------------------
// FUNCIONES DE TRASMISION / RECEPCION DE FRAMES
//------------------------------------------------------------------------------------
void pv_modbus_io ( bool f_debug, mbus_control_t *mbus_ctl, hold_reg_t *hreg )
{
	// Arma el buffer de transmision. Trasmite. Espera la respuesta y la decodifica.
	// Usa un mutex para acceder al bus MBUS.

bool read_status = false;

	// Para acceder al bus debo tomar el semaforo.
	while ( xSemaphoreTake( sem_MBUS, ( TickType_t ) 10 ) != pdTRUE )
		taskYIELD();

	pv_modbus_make_tx_frame( mbus_ctl );
	pv_modbus_txmit_frame( f_debug );

	// Espero la respuesta
	vTaskDelay( (portTickType)( systemVars.modbus_conf.waiting_poll_time / portTICK_RATE_MS ) );

	// Traspaso los datos recibidos al buffer mbus para su decodificacion
	read_status = pv_modbus_rcvd_frame( f_debug );
	pv_modbus_decode_frame( f_debug, mbus_ctl, hreg, read_status );
	pv_modbus_print_value( f_debug, mbus_ctl, hreg );

	xSemaphoreGive( sem_MBUS );

}
//------------------------------------------------------------------------------------
void pv_modbus_make_tx_frame( mbus_control_t *mbus_ctl )
{
	// Prepara el raw frame con los datos.
	// Calcula el CRC de modo que queda pronto a transmitirse por el serial
	// Usamos la estructura globla mbus_data_s

uint8_t size = 0;
uint16_t crc;

	memset( mbus_data_s.tx_buffer, '\0', MBUS_TXMSG_LENGTH );
	mbus_data_s.tx_buffer[0] = mbus_ctl->sla_address;		// SLA
	mbus_data_s.tx_buffer[1] = mbus_ctl->function_code;		// FCODE
	mbus_data_s.tx_buffer[2] = (uint8_t) (( mbus_ctl->address & 0xFF00  ) >> 8);		// DST_ADDR_H
	mbus_data_s.tx_buffer[3] = (uint8_t) ( mbus_ctl->address & 0x00FF );				// DST_ADDR_L
	mbus_data_s.tx_buffer[4] = (uint8_t) ( (mbus_ctl->length & 0xFF00 ) >> 8);		// NRO_REG_HI
	mbus_data_s.tx_buffer[5] = (uint8_t) ( mbus_ctl->length & 0x00FF );				// NRO_REG_LO

	// CRC
	size = 6;
	crc = pv_modbus_CRC16( mbus_data_s.tx_buffer, size );

	mbus_data_s.tx_buffer[6] = (uint8_t)( crc & 0x00FF );			// CRC Low
	mbus_data_s.tx_buffer[7] = (uint8_t)( (crc & 0xFF00) >> 8 );	// CRC High
	mbus_data_s.tx_size = 8;

}
//------------------------------------------------------------------------------------
void pv_modbus_txmit_frame( bool f_debug )
{
	// Transmite el frame modbus almcenado en mbus_data_s.tx_buffer
	//
	// OJO: En MODBUS, los bytes pueden ser 0x00 y no deben ser interpretados como NULL
	// al trasmitir por lo tanto hay que usar el data_size
	// Debemos cumplir extrictos limites de tiempo por lo que primero logueo y luego
	// muestro el debug.

uint8_t i;

	// Transmite un mensaje MODBUS
	// Habilita el RTS para indicar que transmite
	// Apaga la recepcion. ( El IC 485 copia la tx y rx )

	xprintf_PD( f_debug, PSTR("MODBUS TX: start\r\n"));

	// Log
	if ( f_debug ) {
		xprintf_P( PSTR("MODBUS TX: (len=%d):"), mbus_data_s.tx_size);
		for ( i = 0 ; i < mbus_data_s.tx_size ; i++ ) {
			xprintf_P( PSTR("[0x%02X]"), mbus_data_s.tx_buffer[i]);
		}
		xprintf_P( PSTR("\r\n\0"));
	}

	// Transmito
	// borro buffers y espero 3.5T (9600) = 3.5ms ( START )
	aux_flush_TX_buffer();
	aux_flush_RX_buffer();
	vTaskDelay( (portTickType)( 10 / portTICK_RATE_MS ) );
	i = xnprintf_MBUS( (const char *)mbus_data_s.tx_buffer, mbus_data_s.tx_size );

	xprintf_PD( f_debug, PSTR("MODBUS TX end(%d):\r\n"),i);
}
//------------------------------------------------------------------------------------
bool pv_modbus_rcvd_frame( bool f_debug   )
{
	// Recibe del auxBuffer un frame modbus y lo almacena en mbus_data_s.rx_buffer
	//
	// https://stackoverflow.com/questions/13254432/modbus-is-there-a-maximum-time-a-device-can-take-to-respond
	// Esperamos la respuesta 1s y si no llego damos timeout

	// Lee el buffer de recepcion del puerto AUX (MODBUS )
	// Es por poleo de modo que una vez que transmiti un frame, debo esperar un tiempo que llegue la respuesta
	// y luego usar esta funcion para decodificar la lectura.

	// Para copiar buffers no puedo usar strcpy porque pueden haber datos en 0.
	// Copio byte a byte con limite

	// El punto clave esta en la recepcion por medio de las funciones de aux !!!

uint16_t crc_rcvd;
uint16_t crc_calc;
char *p;
uint8_t i;
bool retS = false;

	xprintf_PD( f_debug, PSTR("MODBUS RX: start\r\n"));

	// Leo el rx. No uso strcpy por el tema de los NULL.
	memset( mbus_data_s.rx_buffer, '\0', MBUS_RXMSG_LENGTH );
	// Cantidad de bytes en el buffer aux. ( aux.ptr)
	mbus_data_s.rx_size = (uint8_t)aux_get_buffer_ptr();
	xprintf_PD( f_debug, PSTR("MODBUS RX: rx_size=%d\r\n"), mbus_data_s.rx_size );

	if ( mbus_data_s.rx_size > MBUS_RXMSG_LENGTH )
		mbus_data_s.rx_size = MBUS_RXMSG_LENGTH;

	// Copio todos los bytes recibidos.
	// strncpy( (char *)&mbus_msg, aux_get_buffer(),  length );
	p = aux_get_buffer();
	for ( i=0; i < mbus_data_s.rx_size; i++ ) {
		mbus_data_s.rx_buffer[i] = *p++;
	}

	// Paso 1: Log
	if (f_debug) {
		xprintf_P( PSTR("MODBUS RX: (len=%d):"), mbus_data_s.rx_size);
		for ( i = 0 ; i < mbus_data_s.rx_size; i++ ) {
			xprintf_P( PSTR("[0x%02X]"), mbus_data_s.rx_buffer[i]);
		}
		xprintf_P( PSTR("\r\n"));
	}

	// Paso 2: Controlo el largo.
	if ( mbus_data_s.rx_size < 3) {
		// Timeout error:
		xprintf_PD( f_debug, PSTR("MODBUS RX: TIMEOUT ERROR\r\n\0"));
		retS = false;
		goto quit;
	}

	// Pass3: Calculo y verifico el CRC
	crc_calc = pv_modbus_CRC16( mbus_data_s.rx_buffer, (mbus_data_s.rx_size - 2) );
	crc_rcvd = mbus_data_s.rx_buffer[mbus_data_s.rx_size - 2] + ( mbus_data_s.rx_buffer[mbus_data_s.rx_size - 1] << 8 );

	if ( crc_calc != crc_rcvd) {
		xprintf_PD( f_debug, PSTR("MODBUS RX: CRC ERROR: rx[0x%02x], calc[0x%02x]\r\n\0"), crc_rcvd, crc_calc);
		// De acuerdo al protocolo se ignora el frame recibido con errores CRC
		retS = false;
		goto quit;
	}

	retS = true;

quit:

	xprintf_PD( f_debug, PSTR("MODBUS RX end:\r\n") );
	return(retS);

}
//------------------------------------------------------------------------------------
void pv_modbus_decode_frame ( bool f_debug, mbus_control_t *mbus_ctl, hold_reg_t *hreg, bool read_status )
{
	// En la decodificacion de los enteros no hay problema porque los flowmeters y los PLC
	// mandan el MSB primero.
	// En el caso de los float, solo los mandan los PLC y ahi hay un swap de los bytes por
	// lo que para decodificarlo necesito saber que los 4 bytes corresponden a un float!!
	//
	// MODBUS es BIG_ENDIAN: Cuando se manda un nro de varios bytes, el MSB se manda primero
	// MSB ..... LSB
	// Big Endian: El MSB se pone en el byte de la direccion mas baja:
	// Little Endian: El LSB se pone en el byte de la direccion mas baja.

	// Decodifica el frame recibido que se encuentra en mbus_data.rx_buffer
	// Debo copiar los bytes a hreg->bytes_raw para luego interpretarlo como int o float.
	// La interpretacion la da el codigo F o I y se hace con la funcion pv_modbus_print_value

	// Lo que hacemos es escribir y leer registros del PLC.
	// (0x03) Read Holding Registers
	// (0x04) Read Input Registers
	// (0x06) Write Single Register
	// MBUS_RX_FRAME: SLA FCODE ..............

uint8_t function_code;

	xprintf_PD( f_debug, PSTR("MODBUS DECODE: start\r\n"));

	memset( hreg->rep_str, '\0', 4 );

	// Si alguna operacion (tx,rx) dio error ya salimos con NaN.
	if ( read_status == false ) {
		strcpy( hreg->rep_str, "NaN" );
		xprintf_PD( f_debug, PSTR("MODBUS DECODE: io_error\r\n"));
		goto quit;
	}

	function_code = mbus_data_s.rx_buffer[1];	// Solo 3 o 6.
	switch(function_code) {
	case 0x3:
		xprintf_PD(f_debug, PSTR("MODBUS DECODE: Read Holding Register (0x03)\r\n"));
		pv_modbus_decode_function3( hreg, mbus_ctl->type );
		break;

	case 0x6:
		xprintf_PD(f_debug, PSTR("MODBUS DECODE: Write Holding Register (0x06)\r\n"));
		pv_modbus_decode_function6( hreg, mbus_ctl->type );
		break;

	default:
		xprintf_PD(f_debug, PSTR("MODBUS DECODE: Codigo de funcion no soportado.\r\n"));
		// Indicamos el error con NaN
		strcpy( hreg->rep_str, "NaN" );
		break;
	}

	xprintf_PD(f_debug, PSTR("MODBUS DECODE: b0[0x%02X] b1[0x%02X] b2[0x%02X] b3[0x%02X]\r\n"), hreg->bytes_raw[0], hreg->bytes_raw[1], hreg->bytes_raw[2], hreg->bytes_raw[3]);
	xprintf_PD(f_debug, PSTR("MODBUS DECODE: Fcode=%d\r\n"), function_code);

quit:

	xprintf_PD( f_debug, PSTR("MODBUS DECODE: end\r\n") );

}
//------------------------------------------------------------------------------------
void pv_modbus_decode_function3( hold_reg_t *hreg, char type )
{
	// Decodifica una respuesta modbus de codigo 3: READ HOLDING REGISTER.
	// RX: READ HOLDING REGISTER
	// Los holding register pueden tener 2(int) o 4(float) bytes.
	// No considero mas casos

	// ADDR + FC + 4 + BC1 + BC2 + BC3 + BC4 + CRCH + CRCL
	//   0     1   2     3    4      5    6      7      8

	mbus_data_s.rx_payload = mbus_data_s.rx_buffer[2]; // bytes recibidos

	if ( mbus_data_s.rx_payload == 2 ) {
		// Leo 2 bytes ( Siempre 2 bytes representan un entero )
		// TX: [0x01][0x03][0x08][0x2B][0x00][0x01][0xF6][0x62]
		// RX: [0x01][0x03][0x02][0x04][0xD2][0x3A][0xD9]
		//       0      1     2     3     4    5     6
		//                         MSB  LSB
		// [0x04][0xD2]
		//  MSB   LSB
		// 0x04D2 -> 1234
		// En el DLG:
		// Str: 1234
		// Int=1234
		// Bytes=b0[0xd2] b1[0x04] b2[0x00] b3[0x00]
		//           0         1        2       3
		//          LSB       MSB
		hreg->bytes_raw[0] = mbus_data_s.rx_buffer[4];	// MSB
		hreg->bytes_raw[1] = mbus_data_s.rx_buffer[3];	// LSB
		hreg->bytes_raw[2] = 0x00;
		hreg->bytes_raw[3] = 0x00;

	} else if ( ( mbus_data_s.rx_payload == 4 ) && ( type == 'I') ){
		// Leo 4 bytes que representa a un entero (I)
		// TX: [0x09][0x03][0x10][0x10][0x09][0x02][0xC6][0x16]
		// RX: [0x09][0x03][0x04][0x00][0x00][0x13][0xEC][0x7F][0x4E]
		//       0      1     2     3     4    5     6      7     8
		//                         MSB              LSB
		// [0x00][0x00][0x13][0xEC]
		//  MSB                LSB
		// 0x000013EC -> 5100
		// En el DLG:
		// Str: 5100
		// Int=5100
		// Bytes=b0[0xec] b1[0x13] b2[0x00] b3[0x00]
		//           0         1        2       3
		//          LSB                        MSB
		hreg->bytes_raw[0] = mbus_data_s.rx_buffer[6];
		hreg->bytes_raw[1] = mbus_data_s.rx_buffer[5];
		hreg->bytes_raw[2] = mbus_data_s.rx_buffer[4];
		hreg->bytes_raw[3] = mbus_data_s.rx_buffer[3];

	} else if ( ( mbus_data_s.rx_payload == 4 ) && ( type == 'F') ){
		// Leo 4 bytes que representa a un float (F)
		// TX: [0x01][0x03][0x08][0x13][0x00][0x02][0x37][0xAE]
		// RX: [0x01][0x03][0x04][0x85][0x1F][0x42][0x26][0x53][0x83]
		//       0      1     2     3     4    5     6      7     8
		//                         MSB              LSB
		// [0x85][0x1F][0x42][0x26]
		//  MSB                LSB
		// En el DLG:
		// Str: 41.63
		// Float=41.629997
		// Bytes=b0[0x1e] b1[0x85] b2[0x26] b3[0x42]
		//           0         1        2       3
		//          LSB                        MSB
		// HAY QUE HACER UN SWAP DE LOS WORDS Y DE LOS BYTES en c/word. !!!
		hreg->bytes_raw[0] = mbus_data_s.rx_buffer[4];
		hreg->bytes_raw[1] = mbus_data_s.rx_buffer[3];
		hreg->bytes_raw[2] = mbus_data_s.rx_buffer[6];
		hreg->bytes_raw[3] = mbus_data_s.rx_buffer[5];

	} else {
		mbus_data_s.rx_payload = 0;
		strcpy( hreg->rep_str, "NaN" );
	}

}
//------------------------------------------------------------------------------------
void pv_modbus_decode_function6( hold_reg_t *hreg, char type)
{
	// Decodifica una respuesta modbus de codigo 6: WRITE HOLDING REGISTER.
	// RX: WRITE HOLDING REGISTER
	// ADDR + FC + RA_H + RA_L + RV_H + RV_L + CRCH + CRCL
	//  0      1     2      3       4     5      6     7
	// Solo esta implementado recibir un ENTERO de 2 bytes !!.
	hreg->bytes_raw[0] = mbus_data_s.rx_buffer[5];
	hreg->bytes_raw[1] = mbus_data_s.rx_buffer[4];
	hreg->bytes_raw[2] = 0x00;
	hreg->bytes_raw[3] = 0x00;
	mbus_data_s.rx_payload = 2;
}
//------------------------------------------------------------------------------------
void pv_modbus_print_value(  bool f_debug, mbus_control_t *mbus_ctl, hold_reg_t *hreg )
{
	// Imprime el valor del registro modbus de acuerdo a su tipo.

float pvalue;

	xprintf_PD(f_debug, PSTR("MODBUS VALUE: Length=%d\r\n"), mbus_data_s.rx_payload);
	xprintf_PD(f_debug, PSTR("MODBUS VALUE: Type=%c\r\n"), mbus_ctl->type );

	if ( strcmp ( hreg->rep_str, "NaN") == 0 ) {
		xprintf_PD( f_debug, PSTR("MODBUS VALUE: NaN\r\n"));
		return;
	}

	if ( mbus_ctl->type == 'F') {
		xprintf_PD( f_debug, PSTR("MODBUS VALUE: %.03f (F)\r\n"), hreg->fvalue);
		return;
	}

	if ( ( mbus_ctl->type == 'I') && ( mbus_data_s.rx_payload == 2 ) ) {
		// En este caso importa el parametro divisor_p10 ya que convierte al entero en float
		pvalue = 1.0 * (uint16_t)( hreg->ivalue ) / pow(10, mbus_ctl->divisor_p10 );
		xprintf_PD( f_debug, PSTR("MODBUS VALUE: %d (I), %.03f\r\n"), (uint16_t)( hreg->ivalue ), pvalue );
		return;
	}

	if ( ( mbus_ctl->type == 'I') && ( mbus_data_s.rx_payload == 4 ) ) {
		// En este caso importa el parametro divisor_p10 ya que convierte al entero en float
		pvalue = 1.0 * ( hreg->ivalue ) / pow(10, mbus_ctl->divisor_p10 );
		xprintf_PD( f_debug, PSTR("MODBUS VALUE: %lu (lu), %.03f\r\n"), hreg->ivalue, pvalue );
		return;
	}

}
//------------------------------------------------------------------------------------

