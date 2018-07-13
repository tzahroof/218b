#ifndef _PWMLib_H
#define _PWMLib_H

/****************************************************************************
 Module
     PWMLib.h
 Description
     header file to support use of the 16-channel version of the PWM library
     on the Tiva
 
0 - PB6         Group 0
1 - PB7         Group 0
2 - PB4         Group 1
3 - PB5         Group 1
4 - PE4         Group 2
5 - PE5         Group 2
6 - PC4         Group 3
7 - PC5         Group 3
8 - PD0         Group 4
9 - PD1         Group 4
10 - PA6        Group 5
11 - PA7        Group 5
 
*****************************************************************************/
#include <stdint.h>

bool PWM_Init(uint8_t Which);
bool PWM_SetDuty( uint8_t duty, uint8_t which);
bool PWM_SetFreq( uint16_t Freq, uint8_t group);
#endif //_PWMLib_H

