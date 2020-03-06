/*
 * borrar.c
 *
 *  Created on: 5 mar. 2020
 *      Author: pablo
 */

#include "spx.h"

//------------------------------------------------------------------------------------
void u_sms_init(void)
{

}
//------------------------------------------------------------------------------------
bool u_sms_send(char *dst_nbr, char *msg )
{
	return(true);

}
//------------------------------------------------------------------------------------
char *u_format_date_sms(char *msg)
{
	// Formatea el mensaje a enviar por SMS

RtcTimeType_t rtc;
static char sms_msg[SMS_MSG_LENGTH];

	memset( &rtc, '\0', sizeof(RtcTimeType_t));
	RTC_read_dtime(&rtc);
	memset( &sms_msg, '\0', SMS_MSG_LENGTH );
	snprintf_P( sms_msg, SMS_MSG_LENGTH, PSTR("Fecha: %02d/%02d/%02d %02d:%02d\n%s"), rtc.year, rtc.month, rtc.day, rtc.hour, rtc.min, msg );
	xprintf_P( PSTR("DEBUG SMS1: [%s]\r\n" ), sms_msg );
	return((char *)&sms_msg);

}
//------------------------------------------------------------------------------------
