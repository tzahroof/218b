
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

#include "ADService.h"
#include "StepService.h"
#include "PWM16Tiva.h"
#include "ADMulti.h"
#include "inc/hw_pwm.h"
#include "inc/tm4c123gh6pm.h"
#include "inc/hw_nvic.h"
#include "PWMLib.h"

// macro definitions
#define PeriodInMS 5
#define PWMTicksPerMS 250
#define GenA_Normal (PWM_0_GENA_ACTCMPAU_ONE | PWM_0_GENA_ACTCMPAD_ZERO )
#define BitsPerNibble 4
#define MaxDuty 4050
#define MinDuty 50
#define TimerTicksPerMS 40000
#define MinSpeed 1
#define MaxSpeed 59
#define ALL_LED_ON 255
#define MAX32VALUE 0xffffffff
#define iGain 0.1
#define pGain 1.92
#define dGain 2.5
#define MAXD 100  
#define MIND 0    
#define MAX_AD 4095
#define MIN_AD 10
#define MAX_SPEED 70


// Module Level variables Prototypes
static uint8_t MyPriority;
// add a deferral queue for up to 3 pending deferrals +1 to allow for ovehead

// define static variable ADValue
static uint32_t ADValue;

static uint32_t Period;
static uint32_t LastCapture;
static uint32_t speed;
static float NowSpeed;
static uint32_t SpeedConstant = 600000/(23);
static float RequestedDuty;
static float Rpm;
static float TargetRPM;
static uint8_t STEP_TIMER_RATE =100;

static float clamp(float , uint8_t , uint8_t );
static void InitControlLoop(void);
static float GetSpeed(void);
static void InitPeriodicInt( void );


bool initializeStepService(uint8_t Priority)
{ 
    ES_Event_t ThisEvent;
    MyPriority = Priority;
    
    //Enable port E 
    HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R4;
    while ((HWREG(SYSCTL_PRGPIO) & SYSCTL_PRGPIO_R4) != SYSCTL_PRGPIO_R4)
    {
        ;
    }
    
    //Make E1,E2,E4,E5 digital outputs
    HWREG(GPIO_PORTE_BASE+GPIO_O_DEN) |=  (BIT1HI) | (BIT2HI) | (BIT4HI)| (BIT5HI);
    HWREG(GPIO_PORTE_BASE+GPIO_O_DIR) |=  (BIT1HI) | (BIT2HI) | (BIT4HI)| (BIT5HI);

    
    HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R1;
    // make sure that the Port B module clock has gotten going
    while 
    ((HWREG(SYSCTL_PRGPIO) & SYSCTL_PRGPIO_R1) != SYSCTL_PRGPIO_R1);
    // Enable pin 4 on Port B for digital I/O and make it an output
    HWREG(GPIO_PORTB_BASE+GPIO_O_DEN) |= BIT4HI;HWREG(GPIO_PORTB_BASE+GPIO_O_DIR) |= BIT4HI;
    
    
    
    //initialize PWM
    PWM_Init(0);
    
    
    //Initialize control  output
    InitPeriodicInt();

    /* now set up to measure the period of the encoder signal */
    InitInputCapture();
    // post the initial transition event
    ThisEvent.EventType = ES_INIT;                                                                                                     
    if (ES_PostToService(MyPriority, ThisEvent) == true)
    {
        return true;
    }
    else
    {
        return false;
    }
}

// Post to service function
bool PostStepService( ES_Event_t ThisEvent )
{
  return ES_PostToService( MyPriority, ThisEvent);
}

ES_Event_t RunStepService( ES_Event_t ThisEvent )
{
    //local var ReturnValue initialized to ES_NO_EVENT
    ES_Event_t ReturnValue;
    ReturnValue.EventType = ES_NO_EVENT;
    // If this event is ES_TIMEOUT with parameter SetTimer
    
    if (ThisEvent.EventType == ES_TIMEOUT)
     
    {
      ES_Timer_InitTimer(PIDTimer, STEP_TIMER_RATE);
      PWM_SetDuty(70,0);
      PWM_SetDuty(0,1);
      PWM_SetDuty(70,2);
      PWM_SetDuty(0,3);
      speed = (TimerTicksPerMS*SpeedConstant)/Period;
      
      TargetRPM = 0.7*MAX_SPEED;
      
      printf("requested speed %f \r\n",TargetRPM);
      printf(" speed %f \r\n",NowSpeed);
    }     
    HWREG(GPIO_PORTE_BASE+(GPIO_O_DATA+ALL_BITS)) &= BIT4LO;
    //endif
    // return ReturnValue
    return ReturnValue;
    
} // End of RunADService




void InitInputCapture( void )
{
    // Enable the clock to WideTimer0
    HWREG(SYSCTL_RCGCWTIMER) |= SYSCTL_RCGCWTIMER_R0;
    
    // Enable the clock to port C
    HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R2;
    
    // Disable TimerA before configuring it 
    HWREG(WTIMER0_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TAEN;
    
    // Set up timer in indivudual mode (32 bits)
    // dont be confused by the syntax
    HWREG(WTIMER0_BASE+TIMER_O_CFG) = TIMER_CFG_16_BIT;
    
    // Initialize ILR to max 32 bit num
    HWREG(WTIMER0_BASE+TIMER_O_TAILR) = MAX32VALUE; 
    
    // Set up in CAPTURE mode (TAMR=3, TAAMS = 0)
    // for edge time (TACMR = 1), and up count (TACDIR = 1)
    HWREG(WTIMER0_BASE+TIMER_O_TAMR) =
        (HWREG(WTIMER0_BASE+TIMER_O_TAMR) & ~TIMER_TAMR_TAAMS) |
        (TIMER_TAMR_TACDIR | TIMER_TAMR_TACMR | TIMER_TAMR_TAMR_CAP);
        
    // Want rising edge, clear the TAAEVENT bits 
    HWREG(WTIMER0_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TAEVENT_M;
    
    // Set up port to do capture, use alternate function to set to PC4
    HWREG(GPIO_PORTC_BASE+GPIO_O_AFSEL) |= BIT4HI;
    
    // need to map alternate function for bit4 to WT0CP0
    HWREG(GPIO_PORTC_BASE+GPIO_O_PCTL) =
        (HWREG(GPIO_PORTC_BASE+GPIO_O_PCTL) & 0xfff0ffff) + (7<<16);
        
    // Digitial enable and then set at input
    HWREG(GPIO_PORTC_BASE+GPIO_O_DEN) |= BIT4HI;
    HWREG(GPIO_PORTC_BASE+GPIO_O_DIR) &= ~BIT4HI;
    
    // enable local capture interupt on timer 
    HWREG(WTIMER0_BASE+TIMER_O_IMR) |= TIMER_IMR_CAEIM;
    
    // Enable it in the NVIC
    HWREG(NVIC_EN2) |= BIT30HI; 
    
    // globally enable interupts 
    __enable_irq(); 
    
    // start timer set it to stall when using debugger
    HWREG(WTIMER0_BASE+TIMER_O_CTL) |= (TIMER_CTL_TAEN | TIMER_CTL_TASTALL);
}


void InputCaptureResponse( void )
{
  uint32_t ThisCapture;
  // start by clearing the source of the interrupt, the input capture event
  HWREG(WTIMER0_BASE+TIMER_O_ICR) = TIMER_ICR_CAECINT;
  // now grab the captured value and calculate the period
  ThisCapture = HWREG(WTIMER0_BASE+TIMER_O_TAR);
  Period = ThisCapture - LastCapture;
  // update LastCapture to prepare for the next edge
  LastCapture = ThisCapture;
  NowSpeed = (TimerTicksPerMS*SpeedConstant)/Period;
  ES_Event_t ThisEvent;
  ThisEvent.EventType = EdgeDetected;
  PostStepService(ThisEvent);
}

static void InitPeriodicInt( void ){
    // enable clock to wide timer 0 
    HWREG(SYSCTL_RCGCWTIMER) |= SYSCTL_RCGCWTIMER_R0;
    
    // wait for clock to get going 
    while((HWREG(SYSCTL_PRWTIMER) & SYSCTL_PRWTIMER_R0) != SYSCTL_PRWTIMER_R0)
        ;
    // Disbale timer B before configuring
    HWREG(WTIMER0_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TBEN;
    
    // set in individual (not concatenated) -- make sure this doesn't mess 
    // with other timer
    HWREG(WTIMER0_BASE+TIMER_O_CFG) = TIMER_CFG_16_BIT;
    
    // set up timer B in periodic mode 
    HWREG(WTIMER0_BASE+TIMER_O_TBMR) = (HWREG(WTIMER0_BASE+TIMER_O_TBMR) 
            & ~TIMER_TBMR_TBMR_M) | TIMER_TBMR_TBMR_PERIOD;

    // set timeout to 2 ms 
    HWREG(WTIMER0_BASE+TIMER_O_TBILR) = TimerTicksPerMS*2;
    
    // enable local timeout interrupt
    HWREG(WTIMER0_BASE+TIMER_O_IMR) |= TIMER_IMR_TBTOIM;
    
    // enable in the NVIC, Int number 95 so 95-64 = BIT31HI, NVIC_EN2
    HWREG(NVIC_EN2) |= BIT31HI;
    

    // enable interrupts globally
    __enable_irq();
    
    // start the timer and set to stall with debugger
    HWREG(WTIMER0_BASE+TIMER_O_CTL) |= (TIMER_CTL_TBEN | TIMER_CTL_TBSTALL);
}

static void InitControlLoop(void)
{
  printf(",,,CLOSED LOOP TEST PGAIN=%f, IGAIN =%f DGAIN = %f\n\r",pGain,iGain,dGain);
  
  // we will use Port B pin 4 as a debug aid for our ISR testing/timing
  // enable the clock to Port

  HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R1;
  // make sure that the Port B module clock has gotten going
  while 
  ((HWREG(SYSCTL_PRGPIO) & SYSCTL_PRGPIO_R1) != SYSCTL_PRGPIO_R1);
  // Enable pin 4 on Port B for digital I/O and make it an output
  HWREG(GPIO_PORTB_BASE+GPIO_O_DEN) |= BIT4HI;HWREG(GPIO_PORTB_BASE+GPIO_O_DIR) |= BIT4HI;
  /* Set up periodic interrupt to time the loop updates */
  InitPeriodicInt();
  /* We will use the PWM hardware to generate the control signal */
  PWMrun();
  /* now set up to measure the period of the encoder signal */
  InitInputCapture();
}


void ControlLaw(void) 
{
  
  static float IntegralTerm=0.0;   /* integrator control effort */
  static float RPMError;           /* make static for speed */
  static float LastError;          /* for Derivative Control */
  static uint32_t ThisPeriod;      /* make static for speed */
  
  // start by clearing the source of the interrupt
  HWREG(WTIMER0_BASE+TIMER_O_ICR) = TIMER_ICR_TBTOCINT; // to allow timing this routine raise a line on entry
  HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) |= BIT4HI;
  
  RPMError = TargetRPM - GetSpeed();
  IntegralTerm += iGain * RPMError;
  IntegralTerm = clamp(IntegralTerm, MIND, MAXD);  /* anti-windup */
  
  
  RequestedDuty = (pGain * ((RPMError)+IntegralTerm+(dGain * (RPMError-LastError))));
  RequestedDuty = clamp(RequestedDuty, MIND, MAXD);
  
  LastError = RPMError; // update last error
  
  SetDuty(RequestedDuty); // output calculated control
  
  HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) &= BIT4LO; // end of exe
  
  
}

static float clamp(float IntegralTerm, uint8_t min, uint8_t max)
{
  if(IntegralTerm > max)
  {
    return max;
  }
  else if(IntegralTerm < min)
  {
    return min;
  }
  else
  {
    return IntegralTerm;
  }
}



static float GetSpeed(void)
{
  return NowSpeed;
}










