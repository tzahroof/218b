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


#include "OffenceService.h"
#include "MasterSM.h"
#include "SPI.h"
#include "ShooterService.h"

#define MIN_SHOTCLOCK_THRESHOLD 2
#define RESHOOT_TIME 1500

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well.
// type of state variable should match htat of enum in header file
typedef enum {Wait, Shoot} OffenceState_t;
static OffenceState_t CurrentState;
static uint8_t MyPriority;

bool InitOffence(uint8_t Priority){
    MyPriority = Priority;
  
    CurrentState = Wait;
    printf("\r\n Finished InitOffence");
    return true; 
}

bool PostOffence(ES_Event_t ThisEvent) {
    return ES_PostToService(MyPriority, ThisEvent);
}
/****************************************************************************
 Function
     detStartOffenceState

 Parameters
     void

 Returns
     ReloadState_t

 Description
     Entry Function for OffenceState
     If( we have more than 0 balls AND there's enough time on the shotclock)
        Shoot!
     Otherwise
        Reload!

****************************************************************************/
OffenceState_t detStartOffenceState(void) {
    if(GetBallsInPoss() > 0 && SPI_GetShotClock() > MIN_SHOTCLOCK_THRESHOLD) 
    {
      printf("\r\nWe are going to shoot a ball in the Offence Service\r\n");
      //TODO: Robot can transition into reload state when the shotclock is low (should go to defence)
      ES_Event_t ShootEvent;
      ShootEvent.EventType = EV_SHOOT;
      PostShooter(ShootEvent);
      DecrementBallsInPoss();
      ES_Timer_InitTimer(ReshootTimer, RESHOOT_TIME + 100);
      return Shoot;
    }
    else { //Transition into Reload instead
      printf("\r\nWe are going to post Reload to MasterSM from Offence Service");
      ES_Event_t NeedToReload;
      NeedToReload.EventType = EV_INIT_RELOAD;
      PostMasterSM(NeedToReload);
      return Wait;
    }
    
}

ES_Event_t RunOffence(ES_Event_t ThisEvent) {
    ES_Event_t ReturnEvent;
    ReturnEvent.EventType = ES_NO_EVENT;
    OffenceState_t NextState = CurrentState;
    
    switch(CurrentState) {
      case Wait:
        if(ThisEvent.EventType == EV_INIT_OFFENCE) {
          printf("\r\nReceived EV_INT_OFFENCE in Wait State of Offence Service");
          NextState = detStartOffenceState();
        }
        break;
      case Shoot:
        printf("\r\nIn the Shoot State of Offence");
        if(ThisEvent.EventType == EV_STOP_OFFENCE) { // we have swapped possession
          printf("\r\nEV_STOP_OFFENCE received in Shoot Service.");
          NextState = Wait;
        }
        if(ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == ReshootTimer) { //enough time has passed for the servo to successfully shoot
          printf("\r\nReshootTimer TIMEOUT received in Shoot State of Offence Service");
          NextState = detStartOffenceState();
          
        }
        break;
    }
    CurrentState = NextState;
    
    return ReturnEvent;
}
