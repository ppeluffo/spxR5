/*
 * spx_tkDinputs.c
 *
 *  Created on: 17 dic. 2018
 *      Author: pablo
 */

#include "spx.h"

//------------------------------------------------------------------------------------
// FUNCIONES PUBLICAS
//------------------------------------------------------------------------------------
void dinputs_read_frame( st_dataRecord_t *drcd )
{

	// Esta funcion la invoca tkData al completar un frame para agregar los datos
	// digitales.
	// Leo los niveles de las entradas digitales y copio a dframe.

	switch (spx_io_board ) {
	case SPX_IO5CH:
		// Leo los canales digitales.
		drcd->df.io5.dinputs[0] = DIN_read_DIN0();
		drcd->df.io5.dinputs[1] = DIN_read_DIN1();
		break;
	case SPX_IO8CH:
		break;
	}

}
//------------------------------------------------------------------------------------
