#ifndef _PWM16_TIVA_H
#define _PWM16_TIVA_H

/****************************************************************************
 Module
     PWM16Tiva.h
 Description
     header file to support use of the 16-channel version of the PWM library
     on the Tiva
 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 06/08/17       jec     re-named to match 16 channel version at Rev2
 11/11/15       jec     converted from PWM8Tiva.h
*****************************************************************************/
#include <stdint.h>

bool PWM_TIVA_Init(uint8_t HowMany);
bool PWM_TIVA_SetDuty( uint8_t dutyCycle, uint8_t channel);
bool PWM_TIVA_SetPeriod( uint16_t reqPeriod, uint8_t group);
bool PWM_TIVA_SetFreq( uint16_t reqFreq, uint8_t group);
bool PWM_TIVA_SetPulseWidth( uint16_t NewPW, uint8_t channel);

#endif //_PWM16_TIVA_H
