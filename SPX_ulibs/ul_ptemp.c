/*
 * ul_ptemp.c
 *
 *  Created on: 1 nov. 2019
 *      Author: pablo
 */

#include "spx.h"

//------------------------------------------------------------------------------------
void tempsensor_init(void)
{
}
//------------------------------------------------------------------------------------
bool tempsensor_read( float *temp )
{

bool retS = false;

	*temp = 0.0;
	retS = true;

	return(retS);

}
//------------------------------------------------------------------------------------
void tempsensor_test_read (void)
{

}
//------------------------------------------------------------------------------------
void tempsensor_print(file_descriptor_t fd, float temp )
{

//	if ( ! strcmp ( systemVars.psensor_conf.name, "X" ) )
//		return;

	xCom_printf_P(fd, PSTR("TEMP:%.02f;"), temp );
}
//------------------------------------------------------------------------------------
