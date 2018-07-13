
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

#include "ADService.h"
#include "PWM16Tiva.h"
#include "ADMulti.h"


#define TIMEAD 100

// Module Level variables Prototypes
static uint8_t MyPriority;
static uint32_t AnalogPin[1];
// add a deferral queue for up to 3 pending deferrals +1 to allow for ovehead
// define static variable ADValue
uint32_t ADValue = 2000;

// Module Level Functions;
uint32_t getADValue( void );


bool initializeADService(uint8_t Priority)
{ 
  ES_Event_t ThisEvent;

  MyPriority = Priority;

  //Initialize the analog pins using ADMulti (PE0)
  ADC_MultiInit(1);
  
  // Initialize ADTimer to 20ms
  ES_Timer_InitTimer(ADTimer, TIMEAD);
  
  // Initialize StepTimer to 20ms
  ES_Timer_InitTimer(StepTimer, TIMEAD);

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
bool PostADService( ES_Event_t ThisEvent )
{
  return ES_PostToService( MyPriority, ThisEvent);
}

ES_Event_t RunADService( ES_Event_t ThisEvent )
{
  //local var ReturnValue initialized to ES_NO_EVENT
  ES_Event_t ReturnValue;
  ReturnValue.EventType = ES_NO_EVENT;
    /*
  if (ThisEvent.EventType == ES_TIMEOUT)
  {
    // read the analog input pins
    ADC_MultiRead(AnalogPin);
    // store the input value corresponding to pot. input in ADValue
    ADValue = AnalogPin[0];       
    // Initialize ADTimer to 20ms
    ES_Timer_InitTimer(ADTimer, TIMEAD);
  }
  // return ReturnValue*/
  return ReturnValue;
    
} // End of RunADService

uint32_t getADValue( void )
{
    return ADValue;
}





