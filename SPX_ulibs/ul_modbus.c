/*
 * ul_modbus.c
 *
 *  Created on: 23 abr. 2020
 *      Author: pablo
 */


#include "spx.h"

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
		snprintf_P( &dst, sizeof(dst), PSTR("ON,%d;"), systemVars.modbus_conf.modbus_slave_address );
	} else {
		snprintf_P( &dst, sizeof(dst), PSTR("OFF,%d;"), systemVars.modbus_conf.modbus_slave_address );
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


