/****************************************************************************
 Module
     PWMLib.c
 Description
     Implementation file for the 16-channel version of the PWM Library for
     the Tiva
****************************************************************************/

// Header files
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_gpio.h"
#include "inc/hw_types.h"
#include "inc/hw_pwm.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
#include "driverlib/pwm.h"
#include "inc/hw_sysctl.h"

#include "PWMLib.h"

// Event & Services Framework
#include "ES_Framework.h"
#include "ES_DeferRecall.h"
#include "ES_ShortTimer.h"
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */
#include "ES_Events.h"    /* gets the event data type */

//Teraterm
#include "termio.h"
#include "stdio.h"

// Module Defines
#define TicksPerMS 1250 // running all clocks at system clock/32
#define DefaultPeriod 5 //ms
#define BitsPerNibble 4
#define DefaultDuty 50

//ModuleVariables


/* Init function for PWM channels
0 - PB6
1 - PB7
2 - PB4
3 - PB5
4 - PE4
5 - PE5
6 - PC4
7 - PC5
8 - PD0
9 - PD1
10 - PA6
11 - PA7

Initializes PWM channel at 50% duty cycle and 200 Hz frequency
*/
bool PWM_Init (uint8_t which)
{
    switch (which)
    {
        case 0: //PB6
              // start by enabling the clock to the PWM Module (PWM0)
              HWREG(SYSCTL_RCGCPWM) |=SYSCTL_RCGCPWM_R0;
              // enable the clock to Port B 
              HWREG(SYSCTL_RCGCGPIO) |=SYSCTL_RCGCGPIO_R1;
              // Select the PWM clock as System Clock/32
              HWREG(SYSCTL_RCC) = (HWREG(SYSCTL_RCC) & ~SYSCTL_RCC_PWMDIV_M) |
              (SYSCTL_RCC_USEPWMDIV |SYSCTL_RCC_PWMDIV_32);
              // make sure that the PWM module clock has gotten going
              while((HWREG(SYSCTL_PRPWM) & SYSCTL_PRPWM_R0) != SYSCTL_PRPWM_R0)
              ;
              // disable the PWM while initializing
              HWREG( PWM0_BASE+PWM_O_0_CTL) =0;
              // program generators to go to 1 at rising compare A, 0 on falling compare A 
              // GenA_Normal = (PWM_0_GENA_ACTCMPAU_ONE | PWM_0_GENA_ACTCMPAD_ZERO )
              HWREG( PWM0_BASE+PWM_O_0_GENA) = (PWM_0_GENA_ACTCMPAU_ONE | PWM_0_GENA_ACTCMPAD_ZERO );
              // Set the PWM period
              HWREG( PWM0_BASE+PWM_O_0_LOAD) = ((DefaultPeriod*TicksPerMS))>> 1;
              // Set the PWM duty
              PWM_SetDuty(DefaultDuty,which);
              // enable the PWM outputs
              HWREG( PWM0_BASE+PWM_O_ENABLE) |= (PWM_ENABLE_PWM0EN);
              // now configure the Port B pin to be PWM outputs
              // start by selecting the alternate function for PB6
              HWREG(GPIO_PORTB_BASE+GPIO_O_AFSEL) |= (BIT6HI);
              // now choose to map PWM tothose pins, this is a mux value of 4 that we
              // want to use for specifying the function on bits 6
              HWREG(GPIO_PORTB_BASE+GPIO_O_PCTL) |=
              (HWREG(GPIO_PORTB_BASE+GPIO_O_PCTL) &0xf0ffffff) + (4<<(6*BitsPerNibble));
              // Enable pins 6 on Port B for digital I/O
              HWREG(GPIO_PORTB_BASE+GPIO_O_DEN) |= (BIT6HI);
              // make pins 6 on Port B into outputs
              HWREG(GPIO_PORTB_BASE+GPIO_O_DIR) |= (BIT6HI);
              // set the up/down countmode, enable the PWMgenerator and make
              // both generator updates locally synchronized to zero count
              HWREG(PWM0_BASE+ PWM_O_0_CTL) = (PWM_0_CTL_MODE|PWM_0_CTL_ENABLE|
              PWM_0_CTL_GENAUPD_LS);
              
              return true;
            //break;
        
        case 1: //PB7
              // start by enabling the clock to the PWM Module (PWM0)
              HWREG(SYSCTL_RCGCPWM) |=SYSCTL_RCGCPWM_R0;
              // enable the clock to Port B 
              HWREG(SYSCTL_RCGCGPIO) |=SYSCTL_RCGCGPIO_R1;
              // Select the PWM clock as System Clock/32
              HWREG(SYSCTL_RCC) = (HWREG(SYSCTL_RCC) & ~SYSCTL_RCC_PWMDIV_M) |
              (SYSCTL_RCC_USEPWMDIV |SYSCTL_RCC_PWMDIV_32);
              // make sure that the PWM module clock has gotten going
              while((HWREG(SYSCTL_PRPWM) & SYSCTL_PRPWM_R0) != SYSCTL_PRPWM_R0)
              ;
              // disable the PWM while initializing
              HWREG( PWM0_BASE+PWM_O_0_CTL) =0;
              // program generators to go to 1 at rising compare B, 0 on falling compare B 
              HWREG( PWM0_BASE+PWM_O_0_GENB) = (PWM_0_GENB_ACTCMPBU_ONE | PWM_0_GENB_ACTCMPBD_ZERO );
              // Set the PWM period // Set only once for case 0 and 1
              //HWREG( PWM0_BASE+PWM_O_0_LOAD) = ((DefaultPeriod*TicksPerMS))>> 1;
              // Set the PWM duty
              PWM_SetDuty(DefaultDuty,which);
              // enable the PWM outputs
              HWREG( PWM0_BASE+PWM_O_ENABLE) |= (PWM_ENABLE_PWM1EN);
              // now configure the Port B pin to be PWM outputs
              // start by selecting the alternate function for PB6
              HWREG(GPIO_PORTB_BASE+GPIO_O_AFSEL) |= (BIT7HI);
              // now choose to map PWM tothose pins, this is a mux value of 4 that we
              // want to use for specifying the function on bits 6
              HWREG(GPIO_PORTB_BASE+GPIO_O_PCTL) |=
              (HWREG(GPIO_PORTB_BASE+GPIO_O_PCTL) &0x0fffffff) + (4<<(7*BitsPerNibble));
              // Enable pins 6 on Port B for digital I/O
              HWREG(GPIO_PORTB_BASE+GPIO_O_DEN) |= (BIT7HI);
              // make pins 6 on Port B into outputs
              HWREG(GPIO_PORTB_BASE+GPIO_O_DIR) |= (BIT7HI);
              // set the up/down countmode, enable the PWMgenerator and make
              // both generator updates locally synchronized to zero count
              HWREG(PWM0_BASE+ PWM_O_0_CTL) |= (PWM_0_CTL_MODE|PWM_0_CTL_ENABLE|
              PWM_0_CTL_GENBUPD_LS | PWM_0_CTL_GENAUPD_LS);
              
              return true;
            //break;
        
        case 2: //PB4
              // start by enabling the clock to the PWM Module (PWM0)
              HWREG(SYSCTL_RCGCPWM) |=SYSCTL_RCGCPWM_R0;
              // enable the clock to Port B 
              HWREG(SYSCTL_RCGCGPIO) |=SYSCTL_RCGCGPIO_R1;
              // Select the PWM clock as System Clock/32
              HWREG(SYSCTL_RCC) = (HWREG(SYSCTL_RCC) & ~SYSCTL_RCC_PWMDIV_M) |
              (SYSCTL_RCC_USEPWMDIV |SYSCTL_RCC_PWMDIV_32);
              // make sure that the PWM module clock has gotten going
              while((HWREG(SYSCTL_PRPWM) & SYSCTL_PRPWM_R0) != SYSCTL_PRPWM_R0)
              ;
              // disable the PWM while initializing
              HWREG( PWM0_BASE+PWM_O_1_CTL) =0;
              // program generators to go to 1 at rising compare A, 0 on falling compare A 
              // GenA_Normal = (PWM_0_GENA_ACTCMPAU_ONE | PWM_0_GENA_ACTCMPAD_ZERO )
              HWREG( PWM0_BASE+PWM_O_1_GENA) = (PWM_1_GENA_ACTCMPAU_ONE | PWM_1_GENA_ACTCMPAD_ZERO );
              // Set the PWM period
              HWREG( PWM0_BASE+PWM_O_1_LOAD) = ((DefaultPeriod*TicksPerMS))>> 1;
              // Set the PWM duty
              PWM_SetDuty(DefaultDuty,which);
              // enable the PWM outputs
              HWREG( PWM0_BASE+PWM_O_ENABLE) |= (PWM_ENABLE_PWM2EN);
              // now configure the Port B pin to be PWM outputs
              // start by selecting the alternate function for PB4
              HWREG(GPIO_PORTB_BASE+GPIO_O_AFSEL) |= (BIT4HI);
              // now choose to map PWM tothose pins, this is a mux value of 4 that we
              // want to use for specifying the function on bits 4
              HWREG(GPIO_PORTB_BASE+GPIO_O_PCTL) |=
              (HWREG(GPIO_PORTB_BASE+GPIO_O_PCTL) &0xfff0ffff) + (4<<(4*BitsPerNibble));
              // Enable pins 4 on Port B for digital I/O
              HWREG(GPIO_PORTB_BASE+GPIO_O_DEN) |= (BIT4HI);
              // make pins 4 on Port B into outputs
              HWREG(GPIO_PORTB_BASE+GPIO_O_DIR) |= (BIT4HI);
              // set the up/down countmode, enable the PWMgenerator and make
              // both generator updates locally synchronized to zero count
              HWREG(PWM0_BASE+ PWM_O_1_CTL) |= (PWM_1_CTL_MODE|PWM_1_CTL_ENABLE|
              PWM_1_CTL_GENAUPD_LS);
              
              return true;
            //break;
        
        case 3: //PB5
              // start by enabling the clock to the PWM Module (PWM0)
              HWREG(SYSCTL_RCGCPWM) |=SYSCTL_RCGCPWM_R0;
              // enable the clock to Port B 
              HWREG(SYSCTL_RCGCGPIO) |=SYSCTL_RCGCGPIO_R1;
              // Select the PWM clock as System Clock/32
              HWREG(SYSCTL_RCC) = (HWREG(SYSCTL_RCC) & ~SYSCTL_RCC_PWMDIV_M) |
              (SYSCTL_RCC_USEPWMDIV |SYSCTL_RCC_PWMDIV_32);
              // make sure that the PWM module clock has gotten going
              while((HWREG(SYSCTL_PRPWM) & SYSCTL_PRPWM_R0) != SYSCTL_PRPWM_R0)
              ;
              // disable the PWM while initializing
              HWREG( PWM0_BASE+PWM_O_1_CTL) =0;
              // program generators to go to 1 at rising compare A, 0 on falling compare A 
              // GenA_Normal = (PWM_0_GENA_ACTCMPAU_ONE | PWM_0_GENA_ACTCMPAD_ZERO )
              HWREG( PWM0_BASE+PWM_O_1_GENB) = (PWM_1_GENB_ACTCMPBU_ONE | PWM_1_GENB_ACTCMPBD_ZERO );
              // Set the PWM period
              //HWREG( PWM0_BASE+PWM_O_1_LOAD) = ((DefaultPeriod*TicksPerMS))>> 1;
              // Set the PWM duty
              PWM_SetDuty(DefaultDuty,which);
              // enable the PWM outputs
              HWREG( PWM0_BASE+PWM_O_ENABLE) |= (PWM_ENABLE_PWM3EN);
              // now configure the Port B pin to be PWM outputs
              // start by selecting the alternate function for PB5
              HWREG(GPIO_PORTB_BASE+GPIO_O_AFSEL) |= (BIT5HI);
              // now choose to map PWM tothose pins, this is a mux value of 4 that we
              // want to use for specifying the function on bits 6
              HWREG(GPIO_PORTB_BASE+GPIO_O_PCTL) |=
              (HWREG(GPIO_PORTB_BASE+GPIO_O_PCTL) &0xff0fffff) + (4<<(5*BitsPerNibble));
              // Enable pins 5 on Port B for digital I/O
              HWREG(GPIO_PORTB_BASE+GPIO_O_DEN) |= (BIT5HI);
              // make pins 5 on Port B into outputs
              HWREG(GPIO_PORTB_BASE+GPIO_O_DIR) |= (BIT5HI);
              // set the up/down countmode, enable the PWMgenerator and make
              // both generator updates locally synchronized to zero count
              HWREG(PWM0_BASE+ PWM_O_1_CTL) |= (PWM_1_CTL_MODE|PWM_1_CTL_ENABLE|
              PWM_1_CTL_GENBUPD_LS | PWM_1_CTL_GENAUPD_LS);
              
              return true;
            //break;
        
        case 4: //PE4
              // start by enabling the clock to the PWM Module (PWM0)
              HWREG(SYSCTL_RCGCPWM) |=SYSCTL_RCGCPWM_R0;
              // enable the clock to Port E 
              HWREG(SYSCTL_RCGCGPIO) |=SYSCTL_RCGCGPIO_R4;
              // Select the PWM clock as System Clock/32
              HWREG(SYSCTL_RCC) = (HWREG(SYSCTL_RCC) & ~SYSCTL_RCC_PWMDIV_M) |
              (SYSCTL_RCC_USEPWMDIV |SYSCTL_RCC_PWMDIV_32);
              // make sure that the PWM module clock has gotten going
              while((HWREG(SYSCTL_PRPWM) & SYSCTL_PRPWM_R0) != SYSCTL_PRPWM_R0)
              ;
              // disable the PWM while initializing
              HWREG( PWM0_BASE+PWM_O_2_CTL) =0;
              // program generators to go to 1 at rising compare A, 0 on falling compare A 
              // GenA_Normal = (PWM_0_GENA_ACTCMPAU_ONE | PWM_0_GENA_ACTCMPAD_ZERO )
              HWREG( PWM0_BASE+PWM_O_2_GENA) = (PWM_2_GENA_ACTCMPAU_ONE | PWM_2_GENA_ACTCMPAD_ZERO );
              // Set the PWM period
              HWREG( PWM0_BASE+PWM_O_2_LOAD) = ((DefaultPeriod*TicksPerMS))>> 1;
              // Set the PWM duty
              PWM_SetDuty(DefaultDuty,which);
              // enable the PWM outputs
              HWREG( PWM0_BASE+PWM_O_ENABLE) |= (PWM_ENABLE_PWM4EN);
              // now configure the Port E pin to be PWM outputs
              // start by selecting the alternate function for PE4
              HWREG(GPIO_PORTE_BASE+GPIO_O_AFSEL) |= (BIT4HI);
              // now choose to map PWM tothose pins, this is a mux value of 4 that we
              // want to use for specifying the function on bits 6
              HWREG(GPIO_PORTE_BASE+GPIO_O_PCTL) |=
              (HWREG(GPIO_PORTE_BASE+GPIO_O_PCTL) &0xfff0ffff) + (4<<(4*BitsPerNibble));
              // Enable pins 4 on Port E for digital I/O
              HWREG(GPIO_PORTE_BASE+GPIO_O_DEN) |= (BIT4HI);
              // make pins 4 on Port E into outputs
              HWREG(GPIO_PORTE_BASE+GPIO_O_DIR) |= (BIT4HI);
              // set the up/down countmode, enable the PWMgenerator and make
              // both generator updates locally synchronized to zero count
              HWREG(PWM0_BASE+ PWM_O_2_CTL) |= (PWM_2_CTL_MODE|PWM_2_CTL_ENABLE|
              PWM_2_CTL_GENAUPD_LS);
              
              return true;
            //break;
        
        case 5: //PE5
              // start by enabling the clock to the PWM Module (PWM0)
              HWREG(SYSCTL_RCGCPWM) |=SYSCTL_RCGCPWM_R0;
              // enable the clock to Port E 
              HWREG(SYSCTL_RCGCGPIO) |=SYSCTL_RCGCGPIO_R4;
              // Select the PWM clock as System Clock/32
              HWREG(SYSCTL_RCC) = (HWREG(SYSCTL_RCC) & ~SYSCTL_RCC_PWMDIV_M) |
              (SYSCTL_RCC_USEPWMDIV |SYSCTL_RCC_PWMDIV_32);
              // make sure that the PWM module clock has gotten going
              while((HWREG(SYSCTL_PRPWM) & SYSCTL_PRPWM_R0) != SYSCTL_PRPWM_R0)
              ;
              // disable the PWM while initializing
              HWREG( PWM0_BASE+PWM_O_2_CTL) =0;
              // program generators to go to 1 at rising compare A, 0 on falling compare A 
              // GenA_Normal = (PWM_0_GENA_ACTCMPAU_ONE | PWM_0_GENA_ACTCMPAD_ZERO )
              HWREG( PWM0_BASE+PWM_O_2_GENB) = (PWM_2_GENB_ACTCMPBU_ONE | PWM_2_GENB_ACTCMPBD_ZERO );
              // Set the PWM period
              //HWREG( PWM0_BASE+PWM_O_2_LOAD) = ((DefaultPeriod*TicksPerMS))>> 1;
              // Set the PWM duty
              PWM_SetDuty(DefaultDuty,which);
              // enable the PWM outputs
              HWREG( PWM0_BASE+PWM_O_ENABLE) |= (PWM_ENABLE_PWM5EN);
              // now configure the Port E pin to be PWM outputs
              // start by selecting the alternate function for PE5
              HWREG(GPIO_PORTE_BASE+GPIO_O_AFSEL) |= (BIT5HI);
              // now choose to map PWM tothose pins, this is a mux value of 4 that we
              // want to use for specifying the function on bits 6
              HWREG(GPIO_PORTE_BASE+GPIO_O_PCTL) |=
              (HWREG(GPIO_PORTE_BASE+GPIO_O_PCTL) &0xff0fffff) + (4<<(5*BitsPerNibble));
              // Enable pins 5 on Port E for digital I/O
              HWREG(GPIO_PORTE_BASE+GPIO_O_DEN) |= (BIT5HI);
              // make pins 5 on Port E into outputs
              HWREG(GPIO_PORTE_BASE+GPIO_O_DIR) |= (BIT5HI);
              // set the up/down countmode, enable the PWMgenerator and make
              // both generator updates locally synchronized to zero count
              HWREG(PWM0_BASE+ PWM_O_2_CTL) |= (PWM_2_CTL_MODE|PWM_2_CTL_ENABLE|
              PWM_2_CTL_GENBUPD_LS | PWM_2_CTL_GENAUPD_LS);
              
              return true;
            //break;
        
        case 6: //PC4
              // start by enabling the clock to the PWM Module (PWM0)
              HWREG(SYSCTL_RCGCPWM) |=SYSCTL_RCGCPWM_R0;
              // enable the clock to Port C 
              HWREG(SYSCTL_RCGCGPIO) |=SYSCTL_RCGCGPIO_R2;
              // Select the PWM clock as System Clock/32
              HWREG(SYSCTL_RCC) = (HWREG(SYSCTL_RCC) & ~SYSCTL_RCC_PWMDIV_M) |
              (SYSCTL_RCC_USEPWMDIV |SYSCTL_RCC_PWMDIV_32);
              // make sure that the PWM module clock has gotten going
              while((HWREG(SYSCTL_PRPWM) & SYSCTL_PRPWM_R0) != SYSCTL_PRPWM_R0)
              ;
              // disable the PWM while initializing
              HWREG( PWM0_BASE+PWM_O_3_CTL) =0;
              // program generators to go to 1 at rising compare A, 0 on falling compare A 
              // GenA_Normal = (PWM_0_GENA_ACTCMPAU_ONE | PWM_0_GENA_ACTCMPAD_ZERO )
              HWREG( PWM0_BASE+PWM_O_3_GENA) = (PWM_3_GENA_ACTCMPAU_ONE | PWM_3_GENA_ACTCMPAD_ZERO );
              // Set the PWM period
              HWREG( PWM0_BASE+PWM_O_3_LOAD) = ((DefaultPeriod*TicksPerMS))>> 1;
              // Set the PWM duty
              PWM_SetDuty(DefaultDuty,which);
              // enable the PWM outputs
              HWREG( PWM0_BASE+PWM_O_ENABLE) |= (PWM_ENABLE_PWM6EN);
              // now configure the Port C pin to be PWM outputs
              // start by selecting the alternate function for PE4
              HWREG(GPIO_PORTC_BASE+GPIO_O_AFSEL) |= (BIT4HI);
              // now choose to map PWM tothose pins, this is a mux value of 4 that we
              // want to use for specifying the function on bits 6
              HWREG(GPIO_PORTC_BASE+GPIO_O_PCTL) |=
              (HWREG(GPIO_PORTC_BASE+GPIO_O_PCTL) &0xfff0ffff) + (4<<(4*BitsPerNibble));
              // Enable pins 4 on Port C for digital I/O
              HWREG(GPIO_PORTC_BASE+GPIO_O_DEN) |= (BIT4HI);
              // make pins 4 on Port C into outputs
              HWREG(GPIO_PORTC_BASE+GPIO_O_DIR) |= (BIT4HI);
              // set the up/down countmode, enable the PWMgenerator and make
              // both generator updates locally synchronized to zero count
              HWREG(PWM0_BASE+ PWM_O_3_CTL) |= (PWM_3_CTL_MODE|PWM_3_CTL_ENABLE|
              PWM_3_CTL_GENAUPD_LS);
              
              return true;
            //break;
        
        case 7: //PC5
              // start by enabling the clock to the PWM Module (PWM0)
              HWREG(SYSCTL_RCGCPWM) |=SYSCTL_RCGCPWM_R0;
              // enable the clock to Port C 
              HWREG(SYSCTL_RCGCGPIO) |=SYSCTL_RCGCGPIO_R2;
              // Select the PWM clock as System Clock/32
              HWREG(SYSCTL_RCC) = (HWREG(SYSCTL_RCC) & ~SYSCTL_RCC_PWMDIV_M) |
              (SYSCTL_RCC_USEPWMDIV |SYSCTL_RCC_PWMDIV_32);
              // make sure that the PWM module clock has gotten going
              while((HWREG(SYSCTL_PRPWM) & SYSCTL_PRPWM_R0) != SYSCTL_PRPWM_R0)
              ;
              // disable the PWM while initializing
              HWREG( PWM0_BASE+PWM_O_3_CTL) =0;
              // program generators to go to 1 at rising compare A, 0 on falling compare A 
              // GenA_Normal = (PWM_0_GENA_ACTCMPAU_ONE | PWM_0_GENA_ACTCMPAD_ZERO )
              //HWREG( PWM0_BASE+PWM_O_2_GENB) = (PWM_2_GENB_ACTCMPBU_ONE | PWM_2_GENB_ACTCMPBD_ZERO );
              // Set the PWM period
              //HWREG( PWM0_BASE+PWM_O_2_LOAD) = ((DefaultPeriod*TicksPerMS))>> 1;
              // Set the PWM duty
              PWM_SetDuty(DefaultDuty,which);
              // enable the PWM outputs
              HWREG( PWM0_BASE+PWM_O_ENABLE) |= (PWM_ENABLE_PWM7EN);
              // now configure the Port C pin to be PWM outputs
              // start by selecting the alternate function for PE5
              HWREG(GPIO_PORTC_BASE+GPIO_O_AFSEL) |= (BIT5HI);
              // now choose to map PWM tothose pins, this is a mux value of 4 that we
              // want to use for specifying the function on bits 6
              HWREG(GPIO_PORTC_BASE+GPIO_O_PCTL) |=
              (HWREG(GPIO_PORTC_BASE+GPIO_O_PCTL) &0xff0fffff) + (4<<(5*BitsPerNibble));
              // Enable pins 5 on Port C for digital I/O
              HWREG(GPIO_PORTC_BASE+GPIO_O_DEN) |= (BIT5HI);
              // make pins 5 on Port C into outputs
              HWREG(GPIO_PORTC_BASE+GPIO_O_DIR) |= (BIT5HI);
              // set the up/down countmode, enable the PWMgenerator and make
              // both generator updates locally synchronized to zero count
              HWREG(PWM0_BASE+ PWM_O_3_CTL) |= (PWM_3_CTL_MODE|PWM_3_CTL_ENABLE|
              PWM_3_CTL_GENBUPD_LS | PWM_3_CTL_GENAUPD_LS); 
              
              return true;
            //break;
        
        case 8: //PD0
              // start by enabling the clock to the PWM Module (PWM1)
              HWREG(SYSCTL_RCGCPWM) |=SYSCTL_RCGCPWM_R1;
              // enable the clock to Port D 
              HWREG(SYSCTL_RCGCGPIO) |=SYSCTL_RCGCGPIO_R3;
              // Select the PWM clock as System Clock/32
              HWREG(SYSCTL_RCC) = (HWREG(SYSCTL_RCC) & ~SYSCTL_RCC_PWMDIV_M) |
              (SYSCTL_RCC_USEPWMDIV |SYSCTL_RCC_PWMDIV_32);
              printf("About to enter while loop \r\n");
              // make sure that the PWM module clock has gotten going
              while((HWREG(SYSCTL_PRPWM) & SYSCTL_PRPWM_R1) != SYSCTL_PRPWM_R1)
              ;
              // disable the PWM while initializing
              HWREG( PWM1_BASE+PWM_O_0_CTL) =0;
              // program generators to go to 1 at rising compare A, 0 on falling compare A 
              // GenA_Normal = (PWM_0_GENA_ACTCMPAU_ONE | PWM_0_GENA_ACTCMPAD_ZERO )
              HWREG( PWM1_BASE+PWM_O_0_GENA) = (PWM_0_GENA_ACTCMPAU_ONE | PWM_0_GENA_ACTCMPAD_ZERO );
              // Set the PWM period
              HWREG( PWM1_BASE+PWM_O_0_LOAD) = ((DefaultPeriod*TicksPerMS))>> 1;
              // Set the PWM duty
              PWM_SetDuty(DefaultDuty,which);
              // enable the PWM outputs
              HWREG( PWM1_BASE+PWM_O_ENABLE) |= (PWM_ENABLE_PWM0EN);
              // now configure the Port B pin to be PWM outputs
              // start by selecting the alternate function for PD0
              HWREG(GPIO_PORTD_BASE+GPIO_O_AFSEL) |= (BIT0HI);
              // now choose to map PWM tothose pins, this is a mux value of 4 that we
              // want to use for specifying the function on bits 0
              HWREG(GPIO_PORTD_BASE+GPIO_O_PCTL) |=
              (HWREG(GPIO_PORTD_BASE+GPIO_O_PCTL) &0xfffffff0) + (5<<(0*BitsPerNibble));
              // Enable pins 6 on Port B for digital I/O
              HWREG(GPIO_PORTD_BASE+GPIO_O_DEN) |= (BIT0HI);
              // make pins 6 on Port B into outputs
              HWREG(GPIO_PORTD_BASE+GPIO_O_DIR) |= (BIT0HI);
              // set the up/down countmode, enable the PWMgenerator and make
              // both generator updates locally synchronized to zero count
              HWREG(PWM1_BASE+ PWM_O_0_CTL) = (PWM_0_CTL_MODE|PWM_0_CTL_ENABLE|
              PWM_0_CTL_GENAUPD_LS);
              printf("PD0 initialization done \r\n");
              return true;
            //break;
        
        case 9: //PD1
              // start by enabling the clock to the PWM Module (PWM1)
              HWREG(SYSCTL_RCGCPWM) |=SYSCTL_RCGCPWM_R1;
              // enable the clock to Port B 
              HWREG(SYSCTL_RCGCGPIO) |=SYSCTL_RCGCGPIO_R3;
              // Select the PWM clock as System Clock/32
              HWREG(SYSCTL_RCC) = (HWREG(SYSCTL_RCC) & ~SYSCTL_RCC_PWMDIV_M) |
              (SYSCTL_RCC_USEPWMDIV |SYSCTL_RCC_PWMDIV_32);
              // make sure that the PWM module clock has gotten going
              while((HWREG(SYSCTL_PRPWM) & SYSCTL_PRPWM_R1) != SYSCTL_PRPWM_R1)
              ;
              // disable the PWM while initializing
              HWREG( PWM0_BASE+PWM_O_0_CTL) =0;
              // program generators to go to 1 at rising compare A, 0 on falling compare A 
              // GenA_Normal = (PWM_0_GENA_ACTCMPAU_ONE | PWM_0_GENA_ACTCMPAD_ZERO )
              HWREG( PWM1_BASE+PWM_O_0_GENB) = (PWM_0_GENB_ACTCMPBU_ONE | PWM_0_GENB_ACTCMPBD_ZERO );
              // Set the PWM period
              //HWREG( PWM1_BASE+PWM_O_0_LOAD) = ((DefaultPeriod*TicksPerMS))>> 1;
              // Set the PWM duty
              PWM_SetDuty(DefaultDuty,which);
              // enable the PWM outputs
              HWREG( PWM1_BASE+PWM_O_ENABLE) |= (PWM_ENABLE_PWM1EN);
              // now configure the Port B pin to be PWM outputs
              // start by selecting the alternate function for PD1
              HWREG(GPIO_PORTD_BASE+GPIO_O_AFSEL) |= (BIT1HI);
              // now choose to map PWM tothose pins, this is a mux value of 4 that we
              // want to use for specifying the function on bits 6
              HWREG(GPIO_PORTD_BASE+GPIO_O_PCTL) |=
              (HWREG(GPIO_PORTD_BASE+GPIO_O_PCTL) &0xffffff0f) + (5<<(1*BitsPerNibble));
              // Enable pins 6 on Port B for digital I/O
              HWREG(GPIO_PORTD_BASE+GPIO_O_DEN) |= (BIT1HI);
              // make pins 6 on Port B into outputs
              HWREG(GPIO_PORTD_BASE+GPIO_O_DIR) |= (BIT1HI);
              // set the up/down countmode, enable the PWMgenerator and make
              // both generator updates locally synchronized to zero count
              HWREG(PWM1_BASE+ PWM_O_0_CTL) = (PWM_0_CTL_MODE|PWM_0_CTL_ENABLE|
              PWM_0_CTL_GENAUPD_LS | PWM_0_CTL_GENBUPD_LS);
              
              return true;
            //break;
        
        case 10: //PA6
              // start by enabling the clock to the PWM Module (PWM1)
              HWREG(SYSCTL_RCGCPWM) |=SYSCTL_RCGCPWM_R1;
              // enable the clock to Port A 
              HWREG(SYSCTL_RCGCGPIO) |=SYSCTL_RCGCGPIO_R0;
              // Select the PWM clock as System Clock/32
              HWREG(SYSCTL_RCC) = (HWREG(SYSCTL_RCC) & ~SYSCTL_RCC_PWMDIV_M) |
              (SYSCTL_RCC_USEPWMDIV |SYSCTL_RCC_PWMDIV_32);
              // make sure that the PWM module clock has gotten going
              while((HWREG(SYSCTL_PRPWM) & SYSCTL_PRPWM_R1) != SYSCTL_PRPWM_R1)
              ;
              // disable the PWM while initializing
              HWREG( PWM1_BASE+PWM_O_1_CTL) =0;
              // program generators to go to 1 at rising compare A, 0 on falling compare A 
              // GenA_Normal = (PWM_0_GENA_ACTCMPAU_ONE | PWM_0_GENA_ACTCMPAD_ZERO )
              HWREG( PWM1_BASE+PWM_O_1_GENA) = (PWM_1_GENA_ACTCMPAU_ONE | PWM_1_GENA_ACTCMPAD_ZERO );
              // Set the PWM period
              HWREG( PWM1_BASE+PWM_O_1_LOAD) = ((DefaultPeriod*TicksPerMS))>> 1;
              // Set the PWM duty
              PWM_SetDuty(DefaultDuty,which);
              // enable the PWM outputs
              HWREG( PWM1_BASE+PWM_O_ENABLE) |= (PWM_ENABLE_PWM2EN);
              // now configure the Port B pin to be PWM outputs
              // start by selecting the alternate function for PA6
              HWREG(GPIO_PORTA_BASE+GPIO_O_AFSEL) |= (BIT6HI);
              // now choose to map PWM tothose pins, this is a mux value of 4 that we
              // want to use for specifying the function on bits 6
              HWREG(GPIO_PORTA_BASE+GPIO_O_PCTL) =
              (HWREG(GPIO_PORTA_BASE+GPIO_O_PCTL) &0xf0ffffff) + (5<<(6*BitsPerNibble));
              // Enable pins 6 on Port A for digital I/O
              HWREG(GPIO_PORTA_BASE+GPIO_O_DEN) |= (BIT6HI);
              // make pins 6 on Port A into outputs
              HWREG(GPIO_PORTA_BASE+GPIO_O_DIR) |= (BIT6HI);
              // set the up/down countmode, enable the PWMgenerator and make
              // both generator updates locally synchronized to zero count
              HWREG(PWM1_BASE+ PWM_O_1_CTL) = (PWM_1_CTL_MODE|PWM_1_CTL_ENABLE|
              PWM_1_CTL_GENAUPD_LS);
              
              return true;
            //break;
        
        case 11: //PA7
              // start by enabling the clock to the PWM Module (PWM1)
              HWREG(SYSCTL_RCGCPWM) |=SYSCTL_RCGCPWM_R1;
              // enable the clock to Port A 
              HWREG(SYSCTL_RCGCGPIO) |=SYSCTL_RCGCGPIO_R0;
              // Select the PWM clock as System Clock/32
              HWREG(SYSCTL_RCC) = (HWREG(SYSCTL_RCC) & ~SYSCTL_RCC_PWMDIV_M) |
              (SYSCTL_RCC_USEPWMDIV |SYSCTL_RCC_PWMDIV_32);
              // make sure that the PWM module clock has gotten going
              while((HWREG(SYSCTL_PRPWM) & SYSCTL_PRPWM_R1) != SYSCTL_PRPWM_R1)
              ;
              // disable the PWM while initializing
              HWREG( PWM1_BASE+PWM_O_1_CTL) =0;
              // program generators to go to 1 at rising compare A, 0 on falling compare A 
              // GenA_Normal = (PWM_0_GENA_ACTCMPAU_ONE | PWM_0_GENA_ACTCMPAD_ZERO )
              HWREG( PWM1_BASE+PWM_O_1_GENB) = (PWM_1_GENB_ACTCMPBU_ONE | PWM_1_GENB_ACTCMPBD_ZERO );
              // Set the PWM period
              // HWREG( PWM1_BASE+PWM_O_0_LOAD) = ((DefaultPeriod*TicksPerMS))>> 1;
              // Set the PWM duty
              PWM_SetDuty(DefaultDuty,which);
              // enable the PWM outputs
              HWREG( PWM1_BASE+PWM_O_ENABLE) |= (PWM_ENABLE_PWM3EN);
              // now configure the Port B pin to be PWM outputs
              // start by selecting the alternate function for PA7
              HWREG(GPIO_PORTA_BASE+GPIO_O_AFSEL) |= (BIT7HI);
              // now choose to map PWM tothose pins, this is a mux value of 4 that we
              // want to use for specifying the function on bits 6
              HWREG(GPIO_PORTA_BASE+GPIO_O_PCTL) |=
              (HWREG(GPIO_PORTA_BASE+GPIO_O_PCTL) &0x0fffffff) + (5<<(7*BitsPerNibble));
              // Enable pins 6 on Port B for digital I/O
              HWREG(GPIO_PORTA_BASE+GPIO_O_DEN) |= (BIT7HI);
              // make pins 6 on Port B into outputs
              HWREG(GPIO_PORTA_BASE+GPIO_O_DIR) |= (BIT7HI);
              // set the up/down countmode, enable the PWMgenerator and make
              // both generator updates locally synchronized to zero count
              HWREG(PWM1_BASE+ PWM_O_0_CTL) = (PWM_1_CTL_MODE|PWM_1_CTL_ENABLE|
              PWM_1_CTL_GENBUPD_LS | PWM_1_CTL_GENAUPD_LS);
              
              return true;
            //break;
        
    }
    return false;
}

bool PWM_SetDuty( uint8_t duty, uint8_t which)
{
    switch (which)
    {
        case 0:
          if(duty==100)
          {
            HWREG(PWM0_BASE+PWM_O_0_GENA) =PWM_0_GENA_ACTZERO_ZERO;
          }
          else if(duty==0)
          {
            HWREG(PWM0_BASE+PWM_O_0_GENA) =PWM_0_GENA_ACTZERO_ONE;
          }
          else
          {
            HWREG( PWM0_BASE+PWM_O_0_GENA) = (PWM_0_GENA_ACTCMPAU_ONE | PWM_0_GENA_ACTCMPAD_ZERO );
            HWREG( PWM0_BASE+PWM_O_0_CMPA) = ((HWREG(PWM0_BASE+PWM_O_0_LOAD))*(100-duty))/100;
          }
          return true;
           // break;
        
        case 1:
          if(duty==100)
          {
            HWREG(PWM0_BASE+PWM_O_0_GENB) =PWM_0_GENB_ACTZERO_ZERO;
          }
          else if(duty==0)
          {
            HWREG(PWM0_BASE+PWM_O_0_GENB) =PWM_0_GENB_ACTZERO_ONE;
          }
          else
          {
            HWREG( PWM0_BASE+PWM_O_0_GENB) = (PWM_0_GENB_ACTCMPBU_ONE | PWM_0_GENB_ACTCMPBD_ZERO );
            HWREG( PWM0_BASE+PWM_O_0_CMPB) = ((HWREG(PWM0_BASE+PWM_O_0_LOAD))*(100-duty))/100;
          }
          return true;
           // break;
        
        case 2:
          if(duty==100)
          {
            HWREG(PWM0_BASE+PWM_O_1_GENA) =PWM_1_GENA_ACTZERO_ZERO;
          }
          else if(duty==0)
          {
            HWREG(PWM0_BASE+PWM_O_1_GENA) =PWM_1_GENA_ACTZERO_ONE;
          }
          else
          {
            HWREG( PWM0_BASE+PWM_O_1_GENA) = (PWM_1_GENA_ACTCMPAU_ONE | PWM_1_GENA_ACTCMPAD_ZERO );
            HWREG( PWM0_BASE+PWM_O_1_CMPA) = ((HWREG(PWM0_BASE+PWM_O_1_LOAD))*(100-duty))/100;
          }
          return true;
           // break;
        
        case 3:
          if(duty==100)
          {
            HWREG(PWM0_BASE+PWM_O_1_GENB) =PWM_1_GENB_ACTZERO_ZERO;
          }
          else if(duty==0)
          {
            HWREG(PWM0_BASE+PWM_O_1_GENB) =PWM_1_GENB_ACTZERO_ONE;
          }
          else
          {
            HWREG( PWM0_BASE+PWM_O_1_GENB) = (PWM_1_GENB_ACTCMPBU_ONE | PWM_1_GENB_ACTCMPBD_ZERO );
            HWREG( PWM0_BASE+PWM_O_1_CMPB) = ((HWREG(PWM0_BASE+PWM_O_1_LOAD))*(100-duty))/100;
          }
          return true;  
           // break;
        
        case 4:
          if(duty==100)
          {
            HWREG(PWM0_BASE+PWM_O_2_GENA) =PWM_2_GENA_ACTZERO_ZERO;
          }
          else if(duty==0)
          {
            HWREG(PWM0_BASE+PWM_O_2_GENA) =PWM_2_GENA_ACTZERO_ONE;
          }
          else
          {
            HWREG( PWM0_BASE+PWM_O_2_GENA) = (PWM_2_GENA_ACTCMPAU_ONE | PWM_2_GENA_ACTCMPAD_ZERO );
            HWREG( PWM0_BASE+PWM_O_2_CMPA) = ((HWREG(PWM0_BASE+PWM_O_2_LOAD))*(100-duty))/100;
          }
          return true;
           // break;
        
        case 5:
          if(duty==100)
          {
            HWREG(PWM0_BASE+PWM_O_2_GENB) =PWM_2_GENB_ACTZERO_ZERO;
          }
          else if(duty==0)
          {
            HWREG(PWM0_BASE+PWM_O_2_GENB) =PWM_2_GENB_ACTZERO_ONE;
          }
          else
          {
            HWREG( PWM0_BASE+PWM_O_2_GENB) = (PWM_2_GENB_ACTCMPBU_ONE | PWM_2_GENB_ACTCMPBD_ZERO );
            HWREG( PWM0_BASE+PWM_O_2_CMPB) = ((HWREG(PWM0_BASE+PWM_O_2_LOAD))*(100-duty))/100;
          }
           return true; 
           // break;
        
        case 6:
          if(duty==100)
          {
            HWREG(PWM0_BASE+PWM_O_3_GENA) =PWM_3_GENA_ACTZERO_ZERO;
          }
          else if(duty==0)
          {
            HWREG(PWM0_BASE+PWM_O_3_GENA) =PWM_3_GENA_ACTZERO_ONE;
          }
          else
          {
            HWREG( PWM0_BASE+PWM_O_3_GENA) = (PWM_3_GENA_ACTCMPAU_ONE | PWM_3_GENA_ACTCMPAD_ZERO );
            HWREG( PWM0_BASE+PWM_O_3_CMPA) = ((HWREG(PWM0_BASE+PWM_O_3_LOAD))*(100-duty))/100;
          }
          return true;
           // break;
        
        case 7:
          if(duty==100)
          {
            HWREG(PWM0_BASE+PWM_O_3_GENB) =PWM_0_GENB_ACTZERO_ZERO;
          }
          else if(duty==0)
          {
            HWREG(PWM0_BASE+PWM_O_3_GENB) =PWM_0_GENB_ACTZERO_ONE;
          }
          else
          {
            HWREG( PWM0_BASE+PWM_O_3_GENB) = (PWM_3_GENB_ACTCMPBU_ONE | PWM_3_GENB_ACTCMPBD_ZERO );
            HWREG( PWM0_BASE+PWM_O_3_CMPB) = ((HWREG(PWM0_BASE+PWM_O_3_LOAD))*(100-duty))/100;
          }
           return true; 
           // break;
        
        case 8:
          if(duty==100)
          {
            HWREG(PWM1_BASE+PWM_O_0_GENA) =PWM_0_GENA_ACTZERO_ZERO;
          }
          else if(duty==0)
          {
            HWREG(PWM1_BASE+PWM_O_0_GENA) =PWM_0_GENA_ACTZERO_ONE;
          }
          else
          {
            HWREG( PWM1_BASE+PWM_O_0_GENA) = (PWM_0_GENA_ACTCMPAU_ONE | PWM_0_GENA_ACTCMPAD_ZERO );
            HWREG( PWM1_BASE+PWM_O_0_CMPA) = ((HWREG(PWM1_BASE+PWM_O_0_LOAD))*(100-duty))/100;
          }
          return true;
           // break;
        
        case 9:
          if(duty==100)
          {
            HWREG(PWM1_BASE+PWM_O_0_GENB) =PWM_0_GENB_ACTZERO_ZERO;
          }
          else if(duty==0)
          {
            HWREG(PWM1_BASE+PWM_O_0_GENB) =PWM_0_GENB_ACTZERO_ONE;
          }
          else
          {
            HWREG( PWM1_BASE+PWM_O_0_GENB) = (PWM_0_GENB_ACTCMPBU_ONE | PWM_0_GENB_ACTCMPBD_ZERO );
            HWREG( PWM1_BASE+PWM_O_0_CMPB) = ((HWREG(PWM1_BASE+PWM_O_0_LOAD))*(100-duty))/100;
          }
          return true;
           // break;
        
        case 10:
          if(duty==100)
          {
            HWREG(PWM1_BASE+PWM_O_1_GENA) =PWM_1_GENA_ACTZERO_ZERO;
          }
          else if(duty==0)
          {
            HWREG(PWM1_BASE+PWM_O_1_GENA) =PWM_1_GENA_ACTZERO_ONE;
          }
          else
          {
            HWREG( PWM1_BASE+PWM_O_1_GENA) = (PWM_1_GENA_ACTCMPAU_ONE | PWM_1_GENA_ACTCMPAD_ZERO );
            HWREG( PWM1_BASE+PWM_O_1_CMPA) = ((HWREG(PWM1_BASE+PWM_O_1_LOAD))*(100-duty))/100;
          }
          return true;
          //  break;
        
        case 11:
          if(duty==100)
          {
            HWREG(PWM1_BASE+PWM_O_0_GENB) =PWM_1_GENB_ACTZERO_ZERO;
          }
          else if(duty==0)
          {
            HWREG(PWM1_BASE+PWM_O_0_GENB) =PWM_1_GENB_ACTZERO_ONE;
          }
          else
          {
            HWREG( PWM1_BASE+PWM_O_1_GENB) = (PWM_1_GENB_ACTCMPBU_ONE | PWM_1_GENB_ACTCMPBD_ZERO );
            HWREG( PWM1_BASE+PWM_O_1_CMPB) = ((HWREG(PWM1_BASE+PWM_O_1_LOAD))*(100-duty))/100;
          }
          return true;
           // break;
        
    }
    return false;
}

// give frequency in kHz
bool PWM_SetFreq( uint16_t Freq, uint8_t group)
{
    switch (group)
    {
        case 0:
            HWREG( PWM0_BASE+PWM_O_0_LOAD) = ((TicksPerMS*1000/Freq))>> 1;
            return true;
        //break;
        
        case 1:
            HWREG( PWM0_BASE+PWM_O_1_LOAD) = ((TicksPerMS*1000/Freq))>> 1;
            return true;
        //break;
        
        case 2:
            HWREG( PWM0_BASE+PWM_O_2_LOAD) = ((TicksPerMS*1000/Freq))>> 1;
            return true;
        //break;
        
        case 3:
            HWREG( PWM0_BASE+PWM_O_3_LOAD) = ((TicksPerMS*1000/Freq))>> 1;
            return true;
        //break;
        
        case 4:
            HWREG( PWM1_BASE+PWM_O_0_LOAD) = ((TicksPerMS*1000/Freq))>> 1;
            return true;
        //break;
        
        case 5:
            HWREG( PWM1_BASE+PWM_O_1_LOAD) = ((TicksPerMS*1000/Freq))>> 1;
            return true;
        //break;
    }
        
    return false;
}
