/*
 * ul_consignas.h
 *
 *  Created on: 23 oct. 2019
 *      Author: pablo
 */

#ifndef SRC_SPX_ULIBS_UL_CONSIGNA_H_
#define SRC_SPX_ULIBS_UL_CONSIGNA_H_

#include "spx.h"

bool consigna_init(void);
bool consigna_config ( char *hhmm1, char *hhmm2 );
void consigna_config_defaults(void);
bool consigna_write( char *tipo_consigna_str);
void consigna_stk(void);


#endif /* SRC_SPX_ULIBS_UL_CONSIGNA_H_ */
