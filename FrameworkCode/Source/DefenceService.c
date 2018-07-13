 /*
 include header files for hardware access
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
#include "ADMulti.h"
#include "inc/hw_pwm.h"
#include "inc/tm4c123gh6pm.h"
#include "inc/hw_nvic.h"
#include "PWMLib.h"


#include "DefenceService.h"
#include "MasterSM.h"
#include "MotorService.h"

#define MOVE_BACK_TIME 1000

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well.
// type of state variable should match htat of enum in header file
typedef enum {Wait, MovingBack, ChillWithTheBois} DefenceState_t;
static DefenceState_t CurrentState;
static uint8_t MyPriority;

bool InitDefence(uint8_t Priority){
    MyPriority = Priority;
  
    CurrentState = Wait;
    printf("\r\n Finished InitDefence");
    return true; 
}

bool PostDefence(ES_Event_t ThisEvent) {
    return ES_PostToService(MyPriority, ThisEvent);
}

ES_Event_t RunDefence(ES_Event_t ThisEvent) {
    ES_Event_t ReturnEvent;
    ReturnEvent.EventType = ES_NO_EVENT;
    DefenceState_t NextState = CurrentState;
    ES_Event_t ReloadEvent;
    ReloadEvent.EventType = EV_INIT_RELOAD;

    
    switch(CurrentState) {
      case Wait:
        printf("\r\nIn the Wait State of Defence Service");
        if(ThisEvent.EventType == EV_INIT_DEFENCE) {
//          if(GetBallsInPoss() <=2) {
//            ES_Event_t ReloadEvent;
//            ReloadEvent.EventType = EV_INIT_RELOAD;
//            PostMasterSM(ReloadEvent);
//          } else {
//            NextState = ChillWithTheBois;
//          }
          ES_Timer_InitTimer(DEFENCE_TIMER, MOVE_BACK_TIME);
          SETRPM(-50,-55);
          NextState = MovingBack;
        }
        if(ThisEvent.EventType == EV_STOP_DEFENCE)
        {
          STOP_MOTORS(); 
          PostMasterSM(ReloadEvent);
          NextState = Wait;
        }
        break;
        
      case MovingBack:
        printf("\r\nIn the MovingBack State of Defence Service");
        if(ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == DEFENCE_TIMER)
        {
          printf("\r\nReceived DEFENCE_TIMER TIMEOUT in DefenceService");
          STOP_MOTORS();
          NextState = ChillWithTheBois;
        }
        if(ThisEvent.EventType == EV_STOP_DEFENCE)
        {
          printf("\r\nReceived EV_STOP_DEFENCE in DefenceService");
          STOP_MOTORS();
          NextState = Wait;          
          PostMasterSM(ReloadEvent);

        }
      break;
      
      case ChillWithTheBois:
        if(ThisEvent.EventType == EV_STOP_DEFENCE){
          //TODO: implement code to return to wait safely
          printf("\r\nReceived EV_STOP_DEFENCE in the ChillWithTheBois State of DefenceService");
          NextState = Wait;
        }
        break;
    }
    CurrentState = NextState;
    
    return ReturnEvent;
}
