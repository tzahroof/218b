//Detecter Service
//uses an interrupt on PD2


/* include header files for hardware access
*/
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"

/* include header files for the framework
*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "ES_DeferRecall.h"
#include "ES_ShortTimer.h"
#include <stdio.h>
#include "inc/hw_timer.h"
#include "Detecter_Service.h"
#include "PWM16Tiva.h"
#include "ADMulti.h"
#include "inc/hw_pwm.h"
#include "inc/tm4c123gh6pm.h"
#include "inc/hw_nvic.h"
#include "PWMLib.h"

//500 Hz is .002s or 2 milliseconds
//1000 Hz is .001s or 1 millisecond

//Added function to Keil (check)
//TODO: Add this module to ES_config
//Update the header file (check)


#define BroadcastTIMEOUT 200
#define IRPWMLine 4

#define WTIMERBASE WTIMER3_BASE
#define TimerTicksPerMS 40000
#define upperBoundPeriod (TimerTicksPerMS * 2) //500 Hz
#define lowerBoundPeriod (TimerTicksPerMS * 1)//1000 Hz
#define AccuracyThreshold 5

static uint32_t ActualPeriod = 2.5*TimerTicksPerMS; //initialize with a period of some garbage value
static uint32_t PotentialPeriod; //Period that isn't noise and might be the ActualPeriod it enough times
static uint32_t TempPeriod; //Period that is read every interrupt response
static uint16_t counter = 0;
static uint32_t LastCapture;
static void InitDetecterInterrupt(void);

/****************************************************************************
 Function
     InitDetecterService

 Parameters
     void

 Returns
     void
     
 Purpose
     Initializes Interrupt, PWM Line, 
          used to initializeCheckForBroadcastTimer (timer that doesn't do anything atm)
 
****************************************************************************/

//TODO: Phase out as a Service
//Remove from ES_Configure
//Do PWM Lines conflict?

void InitDetecterService(void){
    printf("Entered init of InitDetecterService\r\n");
    InitDetecterInterrupt();
    PWM_Init(IRPWMLine);
//    ES_Timer_InitTimer(CheckForBroadcastTimer, BroadcastTIMEOUT); //No use for timer. Legacy code
    printf("Finished init of InitDetecterService\r\n");
}

/****************************************************************************
 Function
     InitDetecterInterrupt

 Parameters
     void

 Returns
     void
     
 Purpose
     Initializes the interrupt to let the IR Beacon read the Loading Station's Signal to transmit 2x the signal
     Initializes Port D, Bit 2
     Wide Timer 3, Subtimer A
****************************************************************************/

static void InitDetecterInterrupt(void) {
    //wide-timer 3, subtimer A
    //Port D, Bit 2
    
    //start by enabling the clock to the timer
    HWREG(SYSCTL_RCGCWTIMER) |= SYSCTL_RCGCWTIMER_R3;
    
    //enable the clock to Port D
    HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R3;
    
    //make sure that timer (Timer A) is disabled before configuring
    HWREG(WTIMERBASE+TIMER_O_CTL) &= ~TIMER_CTL_TAEN;
    
    //set it up in 32bit wide (individual, not concatenated) mode
    HWREG(WTIMERBASE+TIMER_O_TAILR) = TIMER_CFG_16_BIT;
    
    //we want to use the full 32bit count, so initialize the Interval Load
    //Register to 0xffff.ffff (its default value)
    HWREG(WTIMERBASE + TIMER_O_TAILR) = 0xffffffff;
    
    //set up timer A in capture mode (TAMR=3, TAAMS=0),
    //for edge time (TACMR=1) and up-counting (TACDIR=1)
    HWREG(WTIMERBASE+TIMER_O_TAMR) = (HWREG(WTIMERBASE+TIMER_O_TAMR) & ~TIMER_TAMR_TAAMS) |
        (TIMER_TAMR_TACDIR | TIMER_TAMR_TACMR | TIMER_TAMR_TAMR_CAP);
        
    //To set the event to rising edge, we need to modify the TAEVENT bits in GPTMCTL.
    //Rising edge = 00, so we clear the TAEVENT bits
    HWREG(WTIMERBASE + TIMER_O_CTL) &= ~TIMER_CTL_TAEVENT_M;
    
    //Now set up the port to do the capture (clock was enabled earlier)
    //start by setting the alternate function for Port D Bit 2 (WT3CCP0)
    HWREG(GPIO_PORTD_BASE + GPIO_O_AFSEL) |= BIT2HI;
    
   //Then map Bit 2's alternate function to WT3CCP0
   //7 is the mux value (page 651) to select WT3CCP0, 2*4 to reach the
   //right nibble (4 bits per nibble * 2 bits)
   HWREG(GPIO_PORTD_BASE+GPIO_O_PCTL) = (HWREG(GPIO_PORTD_BASE+GPIO_O_PCTL) & 0xfffff0ff) + (7 << (2 * 4));
   
   //Enable pin on Port D for digital I/O
   HWREG(GPIO_PORTD_BASE+GPIO_O_DEN) |= BIT2HI;
   
   //make pin 2 on Port D into an input
   HWREG(GPIO_PORTD_BASE + GPIO_O_DIR) &= BIT2LO;
   
   //back to the timer to enable a local capture interrupt
   HWREG(WTIMERBASE + TIMER_O_IMR) |= TIMER_IMR_CAEIM;
   
   //enable the Timer A in Wide Timer 3 interrupt in the NVIC
   //it is interrupt EN3 BIT4HI
   HWREG(NVIC_EN3) |= BIT4HI;
   
   // it is interrupt number 100
   // TODO: Ensure that this is correct?
   HWREG(NVIC_PRI25) = (HWREG(NVIC_PRI25)& ~NVIC_PRI25_INTA_M)| (7 << NVIC_PRI25_INTA_S);
   
   //make sure interrupts are enabled globally
   __enable_irq();
   
   //now kick the timer off by enabling it and enabling the timer to
   //stall while stopped by the debugger
   HWREG(WTIMERBASE + TIMER_O_CTL) |= (TIMER_CTL_TAEN | TIMER_CTL_TASTALL);
   
}

/****************************************************************************
 Function
     DetecterInterruptResponse

 Parameters
     void

 Returns
     void
     
 Purpose
     Input Capture Response to read the IR Beacon Signal
     Reads the period and stores said period into a variable called TempPeriod
     If TempPeriod is reasonable (within 500-1000 Hz), then store it stores TempPeriod in PotentialPeriod
     If we get enough of the same PotentialPeriod in a row (determined by Accuracy Threshold),
          then ActualPeriod = PotentialPeriod,
              where ActualPeriod is the period we use to broadcast the signal
****************************************************************************/


void DetecterInterruptResponse(void) {
    uint32_t ThisCapture;
    
    //start by clearing the source of the interrupt, the input capture event
    HWREG(WTIMERBASE +TIMER_O_ICR) = TIMER_ICR_CAECINT;
    
    //now grab the captured value and calculate the period
    ThisCapture = HWREG(WTIMERBASE+TIMER_O_TAR);
    TempPeriod = ThisCapture - LastCapture;
    
    if(TempPeriod >= lowerBoundPeriod && TempPeriod <= upperBoundPeriod) { //ensure that the signal is within 500-1000Hz
        if(PotentialPeriod == TempPeriod) { //if our PotentialPeriod is the same as the previous one, then we are getting consistent readings. Good
            counter++;
        } else { //our PotentialPeroid reading was inconsistent with our currently read Period. We'll reset counter and set the new reading as our PotentialPeriod
            PotentialPeriod = TempPeriod;
            counter = 0;
        }
        
        if(counter >= AccuracyThreshold) { //if we have received <AccuracyThreshold> number of the same PotentialReadings, then this is most probably the signal that should be broadcast
            ActualPeriod = PotentialPeriod;
//            printf("ActualPeriod Set\r\n");
        }
    }
    LastCapture = ThisCapture;
    
    //update LastCapture to prepare for the next edge
}

/****************************************************************************
 Function
     getDetecterFreq()

 Parameters
     void

 Returns
     uint32_t
     
 Purpose
     Returns the exact frequency needed to broadcast (twice the size), in Hz.
****************************************************************************/
uint32_t getDetecterFreq(void) { 
   return 2000.0*TimerTicksPerMS/ActualPeriod;
}

/****************************************************************************
 Function
     blastBeaconFreq()

 Parameters
     void

 Returns
     void
     
 Purpose
     Blasts the appropriate Beacon Frequency
****************************************************************************/
void blastBeaconFreq(void) {
      uint32_t freqtoBroadcast = getDetecterFreq();
      printf("Frequency to broadcast :: %d\r\n",freqtoBroadcast);
      PWM_SetFreq((uint16_t)(freqtoBroadcast), 2);
      PWM_SetDuty(50,IRPWMLine);
}
