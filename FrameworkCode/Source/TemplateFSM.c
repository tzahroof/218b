/****************************************************************************
 Module
   TemplateFSM.c

 Revision
   1.0.1

 Description
   This is a template file for implementing flat state machines under the
   Gen2 Events and Services Framework.

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 01/15/12 11:12 jec      revisions for Gen2 framework
 11/07/11 11:26 jec      made the queue static
 10/30/11 17:59 jec      fixed references to CurrentEvent in RunTemplateSM()
 10/23/11 18:20 jec      began conversion from SMTemplate.c (02/20/07 rev)
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "TemplateFSM.h"
#include "SPI.h"
#include "PWMLib.h"


/*----------------------------- Module Defines ----------------------------*/

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine.They should be functions
   relevant to the behavior of this state machine
*/

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well.
// type of state variable should match htat of enum in header file
static TemplateState_t CurrentState;

// with the introduction of Gen2, we need a module level Priority var as well
static uint8_t MyPriority;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitTemplateFSM

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
bool InitTemplateFSM(uint8_t Priority)
{
    
  ES_Event_t ThisEvent;
  //Enable port B
  HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R1;
  while ((HWREG(SYSCTL_PRGPIO) & SYSCTL_PRGPIO_R1) != SYSCTL_PRGPIO_R1)
  {
      ;
  }
    
  //Make B3,B2,
  HWREG(GPIO_PORTB_BASE+GPIO_O_DEN) |=  (BIT2HI) | (BIT3HI) ;
  HWREG(GPIO_PORTB_BASE+GPIO_O_DIR) |=  (BIT2HI) | (BIT3HI) ;
    

  MyPriority = Priority;
  PWM_Init(0);
  PWM_Init(1);
  PWM_Init(2);
  PWM_Init(3);
    
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

/****************************************************************************
 Function
     PostTemplateFSM

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
bool PostTemplateFSM(ES_Event_t ThisEvent)
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
ES_Event_t RunTemplateFSM(ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors
  printf("reach sm");
  

  // end switch on Current State
    
  if(ThisEvent.EventType == EV_STATUS_CHANGE)
  {
      if(ThisEvent.EventParam == 1)
      {
          printf("sdsdsdsds");
          // drive the motors
          PWM_SetDuty(50,0);
          PWM_SetDuty(0,1);
          PWM_SetDuty(50,2);
          PWM_SetDuty(0,3);
          
          HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) &= BIT2LO;
          HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) &= BIT3LO;
          
      }
      else if(ThisEvent.EventParam == 2)
      {
          
          PWM_SetDuty(0,0);
          PWM_SetDuty(0,1);
          PWM_SetDuty(0,2);
          PWM_SetDuty(0,3);
          // blue possesion
          printf("Blue in possession \r \n");
          HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) |= BIT2HI;
          HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) &= BIT3LO;
          
      }
      else if(ThisEvent.EventParam == 3)
      {
          PWM_SetDuty(0,0);
          PWM_SetDuty(0,1);
          PWM_SetDuty(0,2);
          PWM_SetDuty(0,3);
          // red possesion
          printf("Red in possession \r \n");
          HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) |= BIT3HI;
          HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) &= BIT2LO;
      }          
      
  }
  return ReturnEvent;
}

/****************************************************************************
 Function
     QueryTemplateSM

 Parameters
     None

 Returns
     TemplateState_t The current state of the Template state machine

 Description
     returns the current state of the Template state machine
 Notes

 Author
     J. Edward Carryer, 10/23/11, 19:21
****************************************************************************/
TemplateState_t QueryTemplateFSM(void)
{
  return CurrentState;
}

/***************************************************************************
 private functions
 ***************************************************************************/

