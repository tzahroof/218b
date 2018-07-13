
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
#include "TapeDetectorService.h"
#include <stdio.h>
#include "inc/hw_timer.h"
#include "MotorService.h"
#include "PWM16Tiva.h"
#include "ADMulti.h"
#include "inc/hw_pwm.h"
#include "inc/tm4c123gh6pm.h"
#include "inc/hw_nvic.h"
#include "PWMLib.h"

#include "ReloadService.h"

#define TapeMask (BIT0HI|BIT1HI|BIT2HI|BIT3HI)
#define DriveLineHighTime 15 //15 MICROseconds
#define DebounceTime 1000
//2 MILLIseconds
#define WaitTime 50 //200 Milliseconds

//HIGH is black tape

/*----------------------------- Module Defines ----------------------------*/

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine.They should be functions
   relevant to the behavior of this state machine
*/

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well.
// type of state variable should match htat of enum in header file
static TapeDetectorState_t CurrentState;

// with the introduction of Gen2, we need a module level Priority var as well
static uint8_t MyPriority;
static uint8_t PrevTapeValues = 0;
static uint8_t TapeValues = 0;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitTapeDetectorService

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, sets up the initial transition and does any
     other required initialization for this state machine
 Notes

 Author
     J. Edward Carryer, 10/23/11, 18:55
****************************************************************************/
bool InitTapeDetectorService(uint8_t Priority)
{
  ES_Event_t ThisEvent;
  MyPriority = Priority;
    
  CurrentState = HighState;
  
  //Port B, 0 - 3
  
  //Initialize Port B
  HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R1;
  while ((HWREG(SYSCTL_PRGPIO) & SYSCTL_PRGPIO_R1) != SYSCTL_PRGPIO_R1) 
  { 
  
  }
    
  HWREG(GPIO_PORTB_BASE+GPIO_O_DEN) |= TapeMask;
  HWREG(GPIO_PORTB_BASE+GPIO_O_DIR) |= TapeMask; 
    
  HWREG(GPIO_PORTB_BASE + ALL_BITS) |= TapeMask; // set all bits high for output
  ES_ShortTimerInit(MyPriority, SHORT_TIMER_UNUSED );
  ES_ShortTimerStart(TIMER_A,DriveLineHighTime); 
  
  
  // post the initial transition event
  ThisEvent.EventType = ES_INIT;
  if (ES_PostToService(MyPriority, ThisEvent) == true)
  {
      printf("Tape detector initialized \r \n");
      return true;
  }
  else
  {
     printf("Tape detector init failed \r \n");
    return false;
  }
}

/****************************************************************************
 Function
     PostTapeDetector

 Parameters
     EF_Event ThisEvent , the event to post to the queue

 Returns
     boolean False if the Enqueue operation failed, True otherwise

 Description
     Posts an event to this state machine's queue
 Notes

 Author
     J. Edward Carryer, 10/23/11, 19:25
****************************************************************************/
bool PostTapeDetectorService(ES_Event_t ThisEvent)
{
  return ES_PostToService(MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunTemplateFSM

 Parameters
   ES_Event : the event to process

 Returns
   ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   add your description here
 Notes
   uses nested switch/case to implement the machine.
 Author
   J. Edward Carryer, 01/15/12, 15:23
****************************************************************************/
ES_Event_t RunTapeDetectorService(ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no error
  
  switch(CurrentState){
      case HighState:
          if(ThisEvent.EventType == ES_SHORT_TIMEOUT) {
            HWREG(GPIO_PORTB_BASE + GPIO_O_DIR) &= ~TapeMask; //set as input line
              ES_ShortTimerStart(TIMER_A,DebounceTime); 
              //ES_Timer_InitTimer(LFTimer, DebounceTime);
            //printf("Short timeout received \r\n");
            CurrentState = DebounceState; //switch states
          }
          break;
      case DebounceState:
          if(ThisEvent.EventType == ES_SHORT_TIMEOUT) {
        //if(ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == LFTimer) {
            PrevTapeValues = TapeValues; //store old Tape Values
            TapeValues = HWREG(GPIO_PORTB_BASE + ALL_BITS) & TapeMask;
            //printf(" TapeValues = %d \r\n", TapeValues);
//            printf( " PB0 : %d \t PB1: %d \t PB2: %d \t PB3: %d \r\n", (HWREG(GPIO_PORTB_BASE + ALL_BITS) & BIT0HI), 
//                    (HWREG(GPIO_PORTB_BASE + ALL_BITS) & BIT1HI),
//                    (HWREG(GPIO_PORTB_BASE + ALL_BITS) & BIT2HI),
//                    (HWREG(GPIO_PORTB_BASE + ALL_BITS) & BIT3HI));  
            if(PrevTapeValues == 0 && TapeValues != 0 && TapeValues !=6) {
                ES_Event_t ThisEvent;
                ThisEvent.EventType = EV_LINE_DETECTED;
                printf("EV_LINE_DETECTED being pass from Tape Service  \r\n");
                PostReload(ThisEvent);
            }else if (TapeValues ==6) 
            {
              printf("On the Tape \r\n");
               ES_Event_t ThisEvent;
               ThisEvent.EventType = EV_ON_TAPE;
               PostReload(ThisEvent);
            }  
            ES_Timer_InitTimer(LFTimer, WaitTime);
            CurrentState = WaitState;
          }
          break;
      case WaitState:
          if(ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == LFTimer) {
            HWREG(GPIO_PORTB_BASE + GPIO_O_DIR) |= TapeMask;
            HWREG(GPIO_PORTB_BASE + ALL_BITS) |= TapeMask;
            ES_ShortTimerStart(TIMER_A,DriveLineHighTime);
            //printf("Wait timeout received \r\n");  
            CurrentState = HighState;
          }
          break;
  }
 
  return ReturnEvent;
}

uint8_t GetTapeValues(void) {
    return TapeValues;
}
