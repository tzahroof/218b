
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
#include "MotorService.h"
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
#define TimerTicksPerMS 40000

#define ALL_LED_ON 255
#define MAX32VALUE 0xffffffff
#define iGain 0.065 //0.1
#define pGain .4//0.2//0.1
#define dGain 3.5 // 2.5
#define MAXD 100
#define MIND 1
#define PWM_FREQ 1000


#define TURN_DUTY 50
#define CPR 23 // Counter Per Revolution
#define MS_PER_SEC 1000
#define SEC_PER_MIN 60
#define MAX_PERIOD 4e6
#define INTD_SHIFT 29
#define CONTROL_LAW_TIME_MS 5                    
#define MAX_REPEAT 300
#define INIT_TIMER_RATE 1000
#define PRINT_TIMER_RATE 500
#define LEFT_PID 4
#define RIGHT_PID 5
// Module Level variables Prototypes
static uint8_t MyPriority;
static uint8_t direction = 1 ;
// add a deferral queue for up to 3 pending deferrals +1 to allow for ovehead



//Motor 1 static level variables 
//static uint32_t repeat = 0; 
static uint32_t Period;
static uint32_t EncoderCountM1 = 0;
static uint32_t LastCapture;
static float NowSpeed;
static float RequestedDuty;
static float TargetRPM1 = 0;  //TODO RE-ENABLE
static uint32_t TIMER_RATE = 5000;
static uint32_t LastCount = 0;
static uint32_t LastCount2 = 0;
static uint32_t repeat = 0; 
static float IntegralTerm=0.0;  
static float IntegralTerm2=0.0;  
/* integrator control effort */

//Motor 2 static level variables 
static uint32_t Period2;
static uint32_t EncoderCountM2 = 0;
static uint32_t LastCapture2;
static float NowSpeed2;
static float RequestedDuty2;
static uint32_t repeat2 = 0; 
static float TargetRPM2 = 0; //TODO REENAABLE

// Private Function prototypes
float GetSpeed(void);
float GetSpeed2(void);
void InitPeriodicInt( void );
void SetRPM1(float);
void SetRPM2(float);
void STOP_MOTORS(void);
void DRIVE_STRAIGHT(uint32_t,uint32_t);


bool initializeStepService(uint8_t Priority)
{ 
    ES_Event_t ThisEvent;
    MyPriority = Priority;

    //initialize PWM
    PWM_Init(0);
    PWM_Init(1);
    PWM_Init(2);
    PWM_Init(3);
    PWM_SetFreq(PWM_FREQ, 0);
    PWM_SetFreq(PWM_FREQ, 1);  
  
    //Initialize control  output
    InitPeriodicInt();
    /* now set up to measure the period of the encoder signal */
    InitInputCapture();
    InitInputCapture2();
    // post the initial transition event
    ES_Timer_InitTimer(PIDTimer, INIT_TIMER_RATE);
    ES_Timer_InitTimer(PrintTimer, INIT_TIMER_RATE);
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
    
    if (ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == PIDTimer)
     
    {
      // SETRPM(-100,100);
      //ES_Timer_InitTimer(PIDTimer, TIMER_RATE);
    }     
    if (ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == PrintTimer)
    {
      ES_Timer_InitTimer(PrintTimer, PRINT_TIMER_RATE);
//      printf("requested speed %0.1f\t",TargetRPM1);
//      printf("Now speed m1 %0.1f\t",NowSpeed);
//      printf("Now speed m2 %0.1f \r\n",NowSpeed2);
    }

    return ReturnValue;
    
} // End of RunADService




void InitInputCapture( void )
{

    // Enable the clock to WideTimer1
    HWREG(SYSCTL_RCGCWTIMER) |= SYSCTL_RCGCWTIMER_R1;
    
    // Enable the clock to port C
    HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R2;
    
    // Disable TimerA before configuring it 
    HWREG(WTIMER1_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TAEN;
    
    // Set up timer in indivudual mode (32 bits)
    // dont be confused by the syntax
    HWREG(WTIMER1_BASE+TIMER_O_CFG) = TIMER_CFG_16_BIT;
    
    // Initialize ILR to max 32 bit num
    HWREG(WTIMER1_BASE+TIMER_O_TAILR) = MAX32VALUE; 
    
    // Set up in CAPTURE mode (TAMR=3, TAAMS = 0)
    // for edge time (TACMR = 1), and up count (TACDIR = 1)
    HWREG(WTIMER1_BASE+TIMER_O_TAMR) =
        (HWREG(WTIMER1_BASE+TIMER_O_TAMR) & ~TIMER_TAMR_TAAMS) |
        (TIMER_TAMR_TACDIR | TIMER_TAMR_TACMR | TIMER_TAMR_TAMR_CAP);
        
    // Want rising edge, clear the TAAEVENT bits 
    HWREG(WTIMER1_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TAEVENT_M;
    
    // Set up port to do capture, use alternate function to set to PC6
    HWREG(GPIO_PORTC_BASE+GPIO_O_AFSEL) |= BIT6HI;
    
    // need to map alternate function for bit6 to WT1CP0
    HWREG(GPIO_PORTC_BASE+GPIO_O_PCTL) =
        (HWREG(GPIO_PORTC_BASE+GPIO_O_PCTL) & 0xf0ffffff) + (7<<24);
        
    // Digitial enable and then set at input
    HWREG(GPIO_PORTC_BASE+GPIO_O_DEN) |= BIT6HI;
    HWREG(GPIO_PORTC_BASE+GPIO_O_DIR) &= ~BIT6HI;
    
    // enable local capture interupt on timer 
    HWREG(WTIMER1_BASE+TIMER_O_IMR) |= TIMER_IMR_CAEIM;
    
    // Enable it in the NVIC
    HWREG(NVIC_EN3) |= BIT0HI; 
    
    // globally enable interupts 
    __enable_irq(); 
    
    // start timer set it to stall when using debugger
    HWREG(WTIMER1_BASE+TIMER_O_CTL) |= (TIMER_CTL_TAEN | TIMER_CTL_TASTALL);
}


void InputCaptureResponse( void )
{

  // start by clearing the source of the interrupt, the input capture event
  HWREG(WTIMER1_BASE+TIMER_O_ICR) = TIMER_ICR_CAECINT;
  uint32_t ThisCapture;
  // now grab the captured value and calculate the period
  ThisCapture = HWREG(WTIMER1_BASE+TIMER_O_TAR);
   
  Period = ThisCapture - LastCapture;
  if(Period > MAX_PERIOD)
  {
     Period =  MAX_PERIOD;
  }

  EncoderCountM1++;
  // update LastCapture to prepare for the next edge
  LastCapture = ThisCapture;
  uint32_t periodMS = Period/TimerTicksPerMS;
  NowSpeed = MS_PER_SEC * SEC_PER_MIN/(periodMS * CPR) ;

}





void InitInputCapture2( void )
{
    // Enable the clock to WideTimer3
    HWREG(SYSCTL_RCGCWTIMER) |= SYSCTL_RCGCWTIMER_R3;
    
    // Enable the clock to port D
    HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R3;
    
    // Disable TimerB before configuring it 
    HWREG(WTIMER3_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TBEN;
    
    // Set up timer in indivudual mode (32 bits)
    // dont be confused by the syntax
    HWREG(WTIMER3_BASE+TIMER_O_CFG) = TIMER_CFG_16_BIT;
    
    // Initialize ILR to max 32 bit num
    HWREG(WTIMER3_BASE+TIMER_O_TBILR) = MAX32VALUE; 
    
    // Set up in CAPTURE mode (TBMR=3, TBAMS = 0)
    // for edge time (TACMR = 1), and up count (TACDIR = 1)
    HWREG(WTIMER3_BASE+TIMER_O_TBMR) =
        (HWREG(WTIMER3_BASE+TIMER_O_TBMR) & ~TIMER_TBMR_TBAMS) |
        (TIMER_TBMR_TBCDIR | TIMER_TBMR_TBCMR | TIMER_TBMR_TBMR_CAP);
        
    // Want rising edge, clear the TBVENT bits 
    HWREG(WTIMER3_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TBEVENT_M;
    
    // Set up port to do capture, use alternate function to set to PD3
    HWREG(GPIO_PORTD_BASE+GPIO_O_AFSEL) |= BIT3HI;
    
    // need to map alternate function for bit6 to WT3CP1
    HWREG(GPIO_PORTD_BASE+GPIO_O_PCTL) =
        (HWREG(GPIO_PORTD_BASE+GPIO_O_PCTL) & 0xffff0fff) + (7<<12);
        
    // Digitial enable and then set at input
    HWREG(GPIO_PORTD_BASE+GPIO_O_DEN) |= BIT3HI;
    HWREG(GPIO_PORTD_BASE+GPIO_O_DIR) &= ~BIT3HI;
    
    // enable local capture interupt on timer 
    HWREG(WTIMER3_BASE+TIMER_O_IMR) |= TIMER_IMR_CBEIM;
    
   // HWREG(NVIC_PRI24) = (HWREG(NVIC_PRI24) & NVIC_PRI25_INTB_M) | NVIC_PRI25_INTB_S;
    
    // Enable it in the NVIC
    HWREG(NVIC_EN3) |= BIT5HI; 
    
    // globally enable interupts 
    __enable_irq(); 
    
    // start timer set it to stall when using debugger
    HWREG(WTIMER3_BASE+TIMER_O_CTL) |= (TIMER_CTL_TBEN | TIMER_CTL_TBSTALL);
     
}


void InputCaptureResponse2( void )
{
   //printf("heree");
  uint32_t ThisCapture2;
  // start by clearing the source of the interrupt, the input capture event
  HWREG(WTIMER3_BASE+TIMER_O_ICR) = TIMER_ICR_CBECINT;
  // now grab the captured value and calculate the period
  ThisCapture2 = HWREG(WTIMER3_BASE+TIMER_O_TBR);
   
  Period2 = ThisCapture2 - LastCapture2;
  if(Period2 > MAX_PERIOD)
  {
     Period2 =  MAX_PERIOD;
  }
  EncoderCountM2++;
  // update LastCapture to prepare for the next edge
  LastCapture2 = ThisCapture2;
  uint32_t periodMS2 = Period2/TimerTicksPerMS;
  NowSpeed2 = MS_PER_SEC * SEC_PER_MIN/(periodMS2 * CPR) ;

}

void InitPeriodicInt( void ){
    // enable clock to wide timer 1
    HWREG(SYSCTL_RCGCWTIMER) |= SYSCTL_RCGCWTIMER_R1;
    
    // wait for clock to get going 
    while((HWREG(SYSCTL_PRWTIMER) & SYSCTL_PRWTIMER_R1) != SYSCTL_PRWTIMER_R1)
        ;
    // Disbale timer B before configuring
    HWREG(WTIMER1_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TBEN;
    
    // set in individual (not concatenated) -- make sure this doesn't mess 
    // with other timer
    HWREG(WTIMER1_BASE+TIMER_O_CFG) = TIMER_CFG_16_BIT;
    
    // set up timer B in periodic mode 
    HWREG(WTIMER1_BASE+TIMER_O_TBMR) = (HWREG(WTIMER1_BASE+TIMER_O_TBMR) 
            & ~TIMER_TBMR_TBMR_M) | TIMER_TBMR_TBMR_PERIOD;

    // set timeout to 2 ms 
    HWREG(WTIMER1_BASE+TIMER_O_TBILR) = TimerTicksPerMS*CONTROL_LAW_TIME_MS;
    
    // enable local timeout interrupt
    HWREG(WTIMER1_BASE+TIMER_O_IMR) |= TIMER_IMR_TBTOIM;
    
    // set as priority 7
    HWREG(NVIC_PRI24) = (HWREG(NVIC_PRI24)& ~NVIC_PRI24_INTB_M)| (7 << NVIC_PRI24_INTB_S);
    
    // enable in the NVIC, Int number 95 so 95-64 = BIT31HI, NVIC_EN2
    HWREG(NVIC_EN3) |= BIT1HI;
    

    // enable interrupts globally  
    __enable_irq();
    
    // start the timer and set to stall with debugger
    HWREG(WTIMER1_BASE+TIMER_O_CTL) |= (TIMER_CTL_TBEN | TIMER_CTL_TBSTALL);
}


void ControlLaw(void) 
{

  // start by clearing the source of the interrupt
  HWREG(WTIMER1_BASE+TIMER_O_ICR) = TIMER_ICR_TBTOCINT;
    
  //static float IntegralTerm=0.0;   /* integrator control effort */
  static float RPMError;           /* make static for speed */
  static float LastError;          /* for Derivative Control */
  
 // static float IntegralTerm2=0.0;   /* integrator control effort */
  static float RPMError2;           /* make static for speed */
  static float LastError2;          /* for Derivative Control */

  
  if(LastCount == EncoderCountM1)
  {     
      repeat++;
      if (repeat > MAX_REPEAT)
      {
        NowSpeed = 0;
        repeat = 0;
      }
  }
  
  if(LastCount2 == EncoderCountM2)
  {     
      repeat2++;
      if (repeat2 > MAX_REPEAT)
      {
        NowSpeed2 = 0;
        repeat2 = 0;
      }
  }


  LastCount = EncoderCountM1;
  LastCount2 = EncoderCountM2;
  
   
  
  RPMError = TargetRPM1 - GetSpeed();
  RPMError2 = TargetRPM2 - GetSpeed2();
  
  IntegralTerm += iGain * RPMError;
  IntegralTerm2 += iGain * RPMError2;

  
   
  
  RequestedDuty = (pGain * ((RPMError)+IntegralTerm+(dGain * (RPMError-LastError))));
  RequestedDuty2 = (pGain * ((RPMError2)+IntegralTerm2+(dGain * (RPMError2-LastError2)))); 
  
  
  if (RequestedDuty > MAXD)
  {
      RequestedDuty = MAXD; // cap the DC
      IntegralTerm -= iGain * RPMError; // subtract from error sum 
  } 
  else if (RequestedDuty < MIND)
  {
      RequestedDuty = MIND;   // cap the DC
      IntegralTerm -= iGain * RPMError;  // subtract from error sum
  }
  
  if (RequestedDuty2 > MAXD)
  {
      RequestedDuty2 = MAXD; // cap the DC
      IntegralTerm2 -= iGain * RPMError; // subtract from error sum 
  } 
  else if (RequestedDuty < MIND)
  {
      RequestedDuty2 = MIND;   // cap the DC
      IntegralTerm2 -= iGain * RPMError;  // subtract from error sum
  }
  
  
    
  LastError = RPMError; // update last error
  LastError2 = RPMError2; // update last error
  
  if(direction == FORWARD_DIRECTION)
  {
      PWM_SetDuty(RequestedDuty,0);
      PWM_SetDuty(0,1);
        
      PWM_SetDuty(RequestedDuty2,2);
      PWM_SetDuty(0,3);  
  }
  else if(direction == BACKWARD_DIRECTION)
  {
      PWM_SetDuty(RequestedDuty,1);
      PWM_SetDuty(0,0);
        
      PWM_SetDuty(RequestedDuty2,3);
      PWM_SetDuty(0,2);
  }
  else if(direction == TURN_RIGHT)
  {
      PWM_SetDuty(TURN_DUTY,0);
      PWM_SetDuty(0,1);
      PWM_SetDuty(TURN_DUTY,3);
      PWM_SetDuty(0,2);
  }
  else if(direction == TURN_LEFT)  
  {
      PWM_SetDuty(TURN_DUTY,1);                                  
      PWM_SetDuty(0,0);
      PWM_SetDuty(TURN_DUTY,2);
      PWM_SetDuty(0,3);
  }
  else if(direction == RIGHT_PID)
  {
     PWM_SetDuty(0,0);
     PWM_SetDuty(RequestedDuty,1);
        
     PWM_SetDuty(RequestedDuty2,2);
     PWM_SetDuty(0,3);  
  }
  else if(direction == LEFT_PID)
  {
     PWM_SetDuty(RequestedDuty,0);
     PWM_SetDuty(0,1);
        
     PWM_SetDuty(0,2);
     PWM_SetDuty(RequestedDuty2,3); 
  }
  
}


float GetSpeed(void)
{
  return NowSpeed;
}

float GetSpeed2(void)
{
  return NowSpeed2;
}

void SetRPM1(float RPM)
{ 
  //IntegralTerm=0.0;  
  TargetRPM1 = RPM;
}

void SetRPM2(float RPM)
{
   // IntegralTerm2=0.0;  
    TargetRPM2 = RPM;
}

void STOP_MOTORS(void)
{
  SetRPM1(0);
  SetRPM2(0); 
}
void DRIVE_STRAIGHT(uint32_t RPM, uint32_t Direction )
{
  //IntegralTerm=0.0;
  //IntegralTerm2=0.0;
  direction = Direction;
  SetRPM1(RPM);
  SetRPM2(RPM); 
}

void TURN(uint32_t Direction )
{
  direction = Direction; 
}

void SETRPM(float RPM1, float RPM2)
{
   //IntegralTerm=0.0;
  // IntegralTerm2=0.0;
   if(RPM1 < 0 && RPM2 > 0)
   {
       direction = LEFT_PID;
       SetRPM1(-RPM1);
       SetRPM2(RPM2); 
   }
   else if (RPM1 > 0 && RPM2 < 0)
   {
       direction = RIGHT_PID;
       SetRPM1(RPM1);
       SetRPM2(-RPM2); 
   }
   else if (RPM1 < 0 && RPM2 < 0)
   {
       direction = BACKWARD_DIRECTION;
       SetRPM1(-RPM1);
       SetRPM2(-RPM2); 
   }
   else
   {
       direction = FORWARD_DIRECTION;
       SetRPM1(RPM1);
       SetRPM2(RPM2); 
   }
    
  
}
    









