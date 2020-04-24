/*
 * ul_modbus.c
 *
 *  Created on: 23 abr. 2020
 *      Author: pablo
 */


#include "spx.h"
#include "tkComms.h"

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


float pv_modbus_poll(uint8_t channel);
uint16_t pv_modbus_CRC16( uint8_t *msg, uint8_t msg_size );
void pv_modbus_tx(bool f_debug, uint8_t slave_address, uint8_t function_code, uint16_t start_address, uint8_t nro_regs );
float pv_modbus_rx(void);

#define DF_MBUS ( systemVars.debug == DEBUG_MODBUS )
#define MBUS_TXMSG_LENGTH	12
#define MBUS_RXMSG_LENGTH	12

//------------------------------------------------------------------------------------
void modbus_init(void)
{
	aux1_prender(false,1);
}
//------------------------------------------------------------------------------------
bool modbus_config_mode( char *mode)
{

	if ( strcmp_P( strupr(mode), PSTR("ON\0")) == 0 ) {
		systemVars.modbus_conf.modbus_enabled = true;
		return(true);
	}

	if ( strcmp_P( strupr(mode), PSTR("OFF\0")) == 0 ) {
		systemVars.modbus_conf.modbus_enabled = false;
		return(true);
	}

	return(false);
}
//------------------------------------------------------------------------------------
bool modbus_config_slave_address( char *address)
{

	systemVars.modbus_conf.modbus_slave_address = atoi(address);
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

	if ( u_control_string(s_name) == 0 ) {
		xprintf_P( PSTR("DEBUG MODBUS ERROR: A%d[%s]\r\n\0"), channel, s_name );
		return( false );
	}

	if (( s_name == NULL ) || ( s_addr == NULL) || (s_length == NULL) || ( s_rcode == NULL)) {
		return(false);
	}

	if ( ( channel >=  0) && ( channel < MODBUS_CHANNELS ) ) {

		snprintf_P( systemVars.modbus_conf.var_name[channel], PARAMNAME_LENGTH, PSTR("%s\0"), s_name );
		systemVars.modbus_conf.var_address[channel] = atoi(s_addr);
		systemVars.modbus_conf.var_length[channel] = atoi(s_length);
		systemVars.modbus_conf.var_function_code[channel] = atoi(s_rcode);
		return(true);
	}

	return(false);
}
//------------------------------------------------------------------------------------
void modbus_config_defaults(void)
{

uint8_t channel = 0;

	systemVars.modbus_conf.modbus_enabled = false;
	systemVars.modbus_conf.modbus_slave_address = 0x00;

	for ( channel = 0; channel < MODBUS_CHANNELS; channel++) {
		snprintf_P( systemVars.modbus_conf.var_name[channel], PARAMNAME_LENGTH, PSTR("X\0") );
		systemVars.modbus_conf.var_address[channel] = 0x00;
		systemVars.modbus_conf.var_length[channel] = 0;
		systemVars.modbus_conf.var_function_code[channel] = 0;
	}
}
//------------------------------------------------------------------------------------
uint8_t modbus_hash(void)
{
 // https://portal.u-blox.com/s/question/0D52p00008HKDMyCAP/python-code-to-generate-checksums-so-that-i-may-validate-data-coming-off-the-serial-interface

uint16_t i;
uint8_t hash = 0;
char dst[32];
char *p;
uint8_t j = 0;

	//	bool modbus_enabled;
	//	uint8_t modbus_slave_address;
	//	char var_name[MODBUS_CHANNELS][PARAMNAME_LENGTH];
	//	uint16_t var_address[MODBUS_CHANNELS];				// Direccion en el slave de la variable a leer
	//	uint8_t var_length[MODBUS_CHANNELS];				// Cantidad de bytes a leer
	//	uint8_t var_function_code[MODBUS_CHANNELS];			// Funcion de lectura (3-Holding reg, 4-Normal reg)

	memset(dst,'\0', sizeof(dst));
	if ( systemVars.modbus_conf.modbus_enabled ) {
		snprintf_P( dst, sizeof(dst), PSTR("ON,%d;"), systemVars.modbus_conf.modbus_slave_address );
	} else {
		snprintf_P( dst, sizeof(dst), PSTR("OFF,%d;"), systemVars.modbus_conf.modbus_slave_address );
	}
	p = dst;
	while (*p != '\0') {
		hash = u_hash(hash, *p++);
	}

	for(i=0; i<MODBUS_CHANNELS; i++) {
		// Vacio el buffer temoral
		memset(dst,'\0', sizeof(dst));
		// Copio sobe el buffer una vista ascii ( imprimible ) de c/registro.
		j = 0;
		j += snprintf_P( &dst[j], sizeof(dst), PSTR("MB%d:%s,"), i, systemVars.modbus_conf.var_name[i] );
		j += snprintf_P(&dst[j], sizeof(dst), PSTR("%d,%d,%d;"), systemVars.modbus_conf.var_address[i],systemVars.modbus_conf.var_length[i],systemVars.modbus_conf.var_function_code[i] );

		// Apunto al comienzo para recorrer el buffer
		p = dst;
		while (*p != '\0') {
			hash = u_hash(hash, *p++);
		}
	}

	return(hash);

}
//------------------------------------------------------------------------------------
bool modbus_read( float mbus_in[] )
{

uint8_t channel;

	// Si lo tengo habilitado, solo poleo las variables que sean !X.
	for ( channel = 0; channel < MODBUS_CHANNELS; channel++) {
		mbus_in[channel] = pv_modbus_poll(channel);
	}

	return(true);
}
//------------------------------------------------------------------------------------
void modbus_print(file_descriptor_t fd, float src[] )
{
	// Imprime los canales configurados ( no X ) en un fd ( tty_gprs,tty_xbee,tty_term) en
	// forma formateada.
	// Los lee de una estructura array pasada como src

uint8_t channel = 0;

	for ( channel = 0; channel < MODBUS_CHANNELS; channel++) {
		if ( ! strcmp ( systemVars.modbus_conf.var_name[channel], "X" ) )
			continue;
		xfprintf_P(fd, PSTR("%s:%.01f;"), systemVars.modbus_conf.var_name[channel], src[channel] );
	}

}
//------------------------------------------------------------------------------------
float pv_modbus_poll (uint8_t channel )
{
	// Genera el poleo de una variable:
	// Transmite el frame, espera la respuesta y la procesa

float mbus_rsp;

	// Si no tengo habilitado el modbus, salgo
	if ( systemVars.modbus_conf.modbus_enabled == false ) {
		xprintf_PD( DF_MBUS, PSTR("MBUS: Modbus no habilitado.\r\n\0"));
		return(-9999);
	}

	if ( strcmp ( systemVars.modbus_conf.var_name[channel], "X" ) == 0 ) {
		//xprintf_PD( DF_MBUS, PSTR("MBUS: canal %d no habilitado.\r\n\0"), channel );
		return(-9999);
	}

	pv_modbus_tx( DF_MBUS,
			systemVars.modbus_conf.modbus_slave_address,
			systemVars.modbus_conf.var_function_code[channel],
			systemVars.modbus_conf.var_address[channel],
			systemVars.modbus_conf.var_length[channel] );


	mbus_rsp = pv_modbus_rx();

	return(mbus_rsp);
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
void pv_modbus_tx(bool f_debug, uint8_t slave_address, uint8_t function_code, uint16_t start_address, uint8_t nro_regs )
{
uint8_t mbus_msg[MBUS_TXMSG_LENGTH];
uint16_t crc;
uint8_t msg_size = 0;
uint8_t i;

	// Transmite un mensaje MODBUS

	/*
	xprintf_P(PSTR("DEBUG2: slave_addr=0x%02x\r\n\0"), slave_address);
	xprintf_P(PSTR("DEBUG2: rcode=0x%02x\r\n\0"), function_code);
	xprintf_P(PSTR("DEBUG2: addr=0x%02x\r\n\0"), start_address);
	xprintf_P(PSTR("DEBUG2: length=0x%02x\r\n\0"), nro_regs);
	*/

	// Preparo el mensaje
	memset( mbus_msg,'\0', MBUS_TXMSG_LENGTH);
	msg_size = 0;
	mbus_msg[0] = slave_address;										// slave address
	msg_size++;
	mbus_msg[1] = function_code;										// Function code
	msg_size++;
	mbus_msg[2] = (uint8_t)( ( (start_address - 1) & 0xFF00 ) >> 8 );	// start addreess HIGH
	msg_size++;
	mbus_msg[3] = (uint8_t)( (start_address - 1 ) & 0x00FF );			// start addreess LOW
	msg_size++;
	mbus_msg[4] = 0x00;											// Nro.reg. HIGH
	msg_size++;
	mbus_msg[5] = nro_regs;										// Nro.reg. LOW
	msg_size++;

	crc = pv_modbus_CRC16(mbus_msg, 6);

	//xprintf_P(PSTR("DEBUG2: crc=0x%04x\r\n\0"), crc);

	mbus_msg[6] = (uint8_t)( crc & 0x00FF );			// CRC Low
	msg_size++;
	mbus_msg[7] = (uint8_t)( (crc & 0xFF00) >> 8 );		// CRC High
	msg_size++;

	// Transmito
	// borro buffers y espero 3.5T (9600) = 3.5ms
	aux1_flush_RX_buffer();
	aux1_flush_TX_buffer();
	vTaskDelay( (portTickType)( 100 / portTICK_RATE_MS ) );

	// Transmito
	xfprintf_P( fdAUX1, PSTR("%s"), mbus_msg );

	// Log
	if ( f_debug ) {
		xprintf_P(PSTR("MBUS_TX:"));
		for ( i = 0 ; i < msg_size; i++ )
			xprintf_P(PSTR("[0x%02X]"), mbus_msg[i]);
		xprintf_P(PSTR("\r\n\0"));
	}

}
//------------------------------------------------------------------------------------
float pv_modbus_rx(void)
{
	// https://stackoverflow.com/questions/13254432/modbus-is-there-a-maximum-time-a-device-can-take-to-respondhttps://stackoverflow.com/questions/13254432/modbus-is-there-a-maximum-time-a-device-can-take-to-respond
	// Esperamos la respuesta 1s y si no llego damos timeout

uint8_t mbus_msg[MBUS_RXMSG_LENGTH];
uint16_t crc_msg;
uint16_t crc_calc;
uint8_t *p;
uint8_t length;

	vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ));

	memset( mbus_msg, '\0', MBUS_RXMSG_LENGTH );
	strncpy( aux1_get_buffer_ptr(), (char *)&mbus_msg, MBUS_RXMSG_LENGTH );

	// Paso 1: Log

	// Paso2: Calculo el largo.
	length = 0;
	p = mbus_msg;
	while (*p++ != '\0')
		length++;

	if ( length < 3) {
		// Timeout error:
		xprintf_P(PSTR("MBUS_RX: TIMEOUT ERROR\r\n\0"));
		return(-9999);
	}

	// Pass3: Calculo y verifico el CRC
	crc_calc = pv_modbus_CRC16(mbus_msg, (length - 2));
	crc_msg = mbus_msg[length-1] + ( mbus_msg[length]<<8 );

	if ( crc_calc != crc_msg) {
		xprintf_P(PSTR("MBUS_RX: CRC ERROR: rx[0x%02x], calc[0x%02x]\r\n\0"), crc_msg, crc_calc);
		// De acuerdo al protocolo se ignora el frame recibido con errores CRC
		return(-9999);
	}

	// Pass4:
	return(0);

}
//------------------------------------------------------------------------------------
void modbus_test( char* c_slave_address, char *c_function_code, char * c_start_address, char * c_nro_regs)
{
uint8_t slave_address = atoi(c_slave_address);
uint8_t function_code = atoi(c_function_code);
uint16_t start_address = atoi(c_start_address);
uint8_t nro_regs = atoi(c_nro_regs);

	/*
	xprintf_P(PSTR("DEBUG1: slave_addr=%s\r\n\0"), c_slave_address);
	xprintf_P(PSTR("DEBUG1: rcode=%s\r\n\0"), c_function_code);
	xprintf_P(PSTR("DEBUG1: addr=%s\r\n\0"), c_start_address);
	xprintf_P(PSTR("DEBUG1: length=%s\r\n\0"), c_nro_regs);
	*/

	pv_modbus_tx(true, slave_address, function_code, start_address, nro_regs);

}
//------------------------------------------------------------------------------------

