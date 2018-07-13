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


#include "MasterSM.h"
#include "SPI.h"
#include "Detecter_Service.h"
#include "ReloadService.h"
#include "OffenceService.h"
#include "DefenceService.h"

#define SwitchPort GPIO_PORTD_BASE
#define LeftSwitchHi BIT6HI
#define RightSwitchHi BIT7HI

/*----------------------------- Module Defines ----------------------------*/

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine.They should be functions
   relevant to the behavior of this state machine
*/

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well.
// type of state variable should match htat of enum in header file

typedef enum {Wait, Reload, Offence, Defence, GameOver} MasterState_t;
static MasterState_t CurrentState;
static uint8_t MyPriority;
static uint8_t Balls_In_Poss = 0;

// with the introduction of Gen2, we need a module level Priority var as well

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitMasterSM

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
bool InitMasterSM(uint8_t Priority)
{
  printf("\r\n Entered InitMasterSM");
  MyPriority = Priority;
    
  CurrentState = Wait; 
  
  // Initialize SPI 
  SPI_Init(); 
  
  
  // Initialize PWM line to flash game status lights -> PD0 -> channel 8 -> group 4
    PWM_Init(8);
 // Set duty cycle to 100
    PWM_SetDuty(100,8);
 
    
  
  // Initialize Beacon Detect
  InitDetecterService(); 

  printf("\r\n Finished InitMasterSM");
  
  return true; 
}

/****************************************************************************
 Function
     PostMasterSM

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
bool PostMasterSM(ES_Event_t ThisEvent)
{
  return ES_PostToService(MyPriority, ThisEvent);
}

void TurnOffLights(void) {
  PWM_SetDuty(0,8);
}

/****************************************************************************
 Function
    RunMasterSM

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
ES_Event_t RunMasterSM(ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no error
  MasterState_t NextState = CurrentState; 
  
  switch(CurrentState)
  {
    case(Wait):
      printf("\r\nIn the Wait State of Master SM");
      if ((ThisEvent.EventType == EV_STATUS_CHANGE) && (ThisEvent.EventParam == FACE_OFF))
      {
        // Set freq to 2 Hz 
        PWM_SetFreq(2,4);
        //Set duty to 50
        PWM_SetDuty(50,8);
        NextState = Reload; 
        // Flash lights (Change duty to 50) 
        
        ES_Event_t ReloadEvent;
        ReloadEvent.EventType = EV_INIT_RELOAD;
        PostReload(ReloadEvent); 
      }
      else
      {
         printf("\r\nUnaccounted for event in Wait (MasterSM)");
      }
      break;
    case(Reload):
      printf("\r\nIn the Reload State of Master SM");
      
      if((ThisEvent.EventType == EV_STATUS_CHANGE) && (ThisEvent.EventParam == GAME_OVER)) { //If we get game-over, we want to stop everything!
          printf("\r\nReceived EV_STATUS_CHANGE -> GAME_OVER in the Reload State");
          NextState = GameOver;
          TurnOffLights();
          ES_Event_t GameOverReloadEvent;
          GameOverReloadEvent.EventType = EV_STOP_RELOAD;
          PostReload(GameOverReloadEvent);
      }
    
      if (ThisEvent.EventType == EV_RELOAD_COMPLETE)
      {
        printf("\r\nReceived EV_RELOAD_COMPLETE from Master SM");
        
        uint8_t CurrentStatus = SPI_GetStatusMessage();
        printf("\r\nCurrent Status of the game (possession) :: %d", CurrentStatus);
        switch(CurrentStatus)
        {
          case(US_POSS):
            printf("\r\nPossession: US_POSS");
            NextState = Offence;
            ES_Event_t OffenceEvent;
            OffenceEvent.EventType = EV_INIT_OFFENCE;
            PostOffence(OffenceEvent); 
            break;
          case(THEM_POSS):
            printf("\r\nPossession: THEM_POSS");
            NextState = Defence;
            ES_Event_t DefenceEvent;
            DefenceEvent.EventType = EV_INIT_DEFENCE;
            PostDefence(DefenceEvent); 
            break;
          case(TIE_BREAK):
            printf("\r\nPossession: TIE_BREAK");
            NextState = Reload; 
            ES_Event_t ReloadEvent;
            ReloadEvent.EventType = EV_INIT_RELOAD;
            PostReload(ReloadEvent); 
            break; 
          case(GAME_OVER):
            printf("\r\nPossession: GAME_OVER");
            NextState = GameOver;
            TurnOffLights();
            break;
            
        }
      }
      break;
      
    case(Offence):
      printf("\r\nIn the Offence State of Master SM");
      if(ThisEvent.EventType == EV_STATUS_CHANGE) {
          ES_Event_t OffenceEvent;
          OffenceEvent.EventType = EV_STOP_OFFENCE;
          
          switch(ThisEvent.EventParam){
            case THEM_POSS:
              NextState = Defence;
              PostOffence(OffenceEvent); //Stop offence
              
              ES_Event_t DefenceEvent;
              DefenceEvent.EventType = EV_INIT_DEFENCE;
              PostDefence(DefenceEvent); //Initialize Defence
              
              break;
            case GAME_OVER:
              NextState = GameOver;
              TurnOffLights();
              PostOffence(OffenceEvent); //Stop offence
              
              //Turn off lights
            
              break;
            case TIE_BREAK:
              NextState = Reload;
              PostOffence(OffenceEvent); //Stop Offence
            
              ES_Event_t ReloadEvent;
              ReloadEvent.EventType = EV_INIT_RELOAD;
              PostReload(ReloadEvent); //Initialize Reload
              break;
          }
      }
      
      if(ThisEvent.EventType == EV_INIT_RELOAD) {
        NextState = Reload;
//        printf("\r\nPosting EV_INIT_RELOAD to Reload Service from Offence Service");
        PostReload(ThisEvent);
      }
      break;
      
    case(Defence):
      printf("\r\nIn the Defence State of Master SM");      
      if(ThisEvent.EventType == EV_STATUS_CHANGE) {
        ES_Event_t DefenceEvent;
        DefenceEvent.EventType = EV_STOP_DEFENCE;
        
        switch(ThisEvent.EventParam) {
          case US_POSS:
            NextState = Reload;
            PostDefence(DefenceEvent); // stop defence
          
            ES_Event_t ReloadEvent;
            ReloadEvent.EventType = EV_INIT_RELOAD;
            PostReload(ReloadEvent); //Initialize Offence
            
            break;
          
          case GAME_OVER:
            NextState = GameOver;
            TurnOffLights();
            PostDefence(DefenceEvent); //stop defence
          
            //Turn off lights
          
            break;
          
          case TIE_BREAK:
            NextState = Reload;
            PostDefence(DefenceEvent); //stop defence
          
            ES_Event_t TiebreakReloadEvent;
            TiebreakReloadEvent.EventType = EV_INIT_RELOAD;
            PostReload(TiebreakReloadEvent); //Initialize Reload
            break;
        }
      }
      if(ThisEvent.EventType == EV_INIT_RELOAD) {
        NextState = Reload;
        PostReload(ThisEvent);
      }
      
      break;
      
    case(GameOver):
      printf("\r\ngot event in game over!"); 
      break;
    default:
      printf("Unaccounted for event in master SM!");
      ReturnEvent.EventType = ES_ERROR;
      break;
  }
  CurrentState = NextState;
      
  return ReturnEvent; 
}

void IncrementBallsInPoss(void) {
    Balls_In_Poss++;
}

void DecrementBallsInPoss(void) {
    Balls_In_Poss--;
}

uint8_t GetBallsInPoss(void) {
    return Balls_In_Poss;
}

static bool AreWeAtTower = false;
static uint8_t PrevLeftSwitchState = 0;
static uint8_t PrevRightSwitchState = 0;
static uint8_t LeftSwitchState = 0;
static uint8_t RightSwitchState = 0;

bool checkTowerDetected(void) {
  PrevLeftSwitchState = LeftSwitchState;
  PrevRightSwitchState = RightSwitchState;
  
  LeftSwitchState = HWREG(SwitchPort + ALL_BITS) & LeftSwitchHi;
  RightSwitchState = HWREG(SwitchPort + ALL_BITS) & RightSwitchHi;
  
//  printf("Left Switch :: %d\r\n", LeftSwitchState);
//  printf("Right Switch :: %d\r\n", RightSwitchState);
  
  
  if(PrevLeftSwitchState != LeftSwitchState || PrevRightSwitchState != RightSwitchState) { // at least one of the switches has detected something

    if(AreWeAtTower && LeftSwitchState == 0 && RightSwitchState == 0) { //if we were at the tower and both switches are now zero
      AreWeAtTower = false; //if one of the switches has changed, we can assume we're no longer at the tower

      printf("No longer at tower \r\n");
      //TODO: implement code to utilize that we're no longer at tower
    }
    else {
      if(LeftSwitchState == LeftSwitchHi || RightSwitchState ==RightSwitchHi) { //both switches activated
        printf("Tower Detected \r\n");
        ES_Event_t ReturnEvent;
        ReturnEvent.EventType = EV_TOWER_DETECTED;
        AreWeAtTower = true; //we are at the tower!
        PostReload(ReturnEvent);
      }
    }
 }
  
 return AreWeAtTower;
}

bool GetAreWeAtTower(void) {
  return AreWeAtTower;

}
