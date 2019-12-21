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
bool consigna_write( char *param0, char *param1, char *param2 );
void consigna_stk(void);
uint8_t consigna_checksum(void);

#endif /* SRC_SPX_ULIBS_UL_CONSIGNA_H_ */
