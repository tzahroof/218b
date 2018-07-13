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


#include "ReloadService.h"
#include "TapeDetectorService.h"
#include "Detecter_Service.h"
#include "MotorService.h"
#include "MasterSM.h"

#define DRIVE_TO_LINE_RPM 80
#define pLineGain 0.24//0.24 //.04
//0.15 //0.3 //2//10//0.3
#define dLineGain 0//0.05 //0.05 //0.2
#define iLineGain 0//0.0001 //0.05//0.0002
static float error = 0;
static float prev_error = 0;
static float RPM_Correction = 0.0;
#define MAX_RPM 100 
#define MIN_RPM 0

#define CONTROL_LAW_PERIOD 60
#define TimerTicksPerMS 40000

#define ONE_SEC 976
#define TIMEBALLGET (ONE_SEC)

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well.
// type of state variable should match htat of enum in header file

typedef enum {Wait, DriveToLine, AlignWithLine, FollowTheLine, EmitState, GameOver} ReloadState_t;
static ReloadState_t CurrentState;
static uint8_t MyPriority;

static uint8_t DEFAULT_FORWARD_RPM = 60;
static bool FaceOffFlag = true;

// with the     introduction of Gen2, we need a module level Priority var as well

/****************************************************************************
 Function
     InitReload

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

bool InitReload(uint8_t Priority){
    MyPriority = Priority;
  
    CurrentState = Wait;
    
  printf("\r\nReload state machine initialized"); 
    return true; 
}


bool PostReload(ES_Event_t ThisEvent) {
    return ES_PostToService(MyPriority, ThisEvent);
}

/****************************************************************************
 Function
     DisableControlPeriodicInt

 Parameters
     void

 Returns
     void
 Description
     Disables the Control Law Interrupt

****************************************************************************/
void DisableControlPeriodicInt(void) {
  HWREG(WTIMER2_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TBEN;
}

/****************************************************************************
 Function
     InitControlPeriodicInt

 Parameters
     void

 Returns
     void
 Description
     Initializes the Control Law

****************************************************************************/
void InitControlPeriodicInt( void)
{
    // start by enabling the clock to the timer (Wide Timer 2)
	HWREG(SYSCTL_RCGCWTIMER) |=SYSCTL_RCGCWTIMER_R2;
	// kill a few cycles to let the clock get going
	while((HWREG(SYSCTL_PRWTIMER) & SYSCTL_PRWTIMER_R2) != SYSCTL_PRWTIMER_R2)
	{
	}
	// make sure that timer (Timer B)is disabled before configuring
	HWREG(WTIMER2_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TBEN;
	// set it up in 32bit wide (individual, not concatenated) mode
	HWREG(WTIMER2_BASE+TIMER_O_CFG) = TIMER_CFG_16_BIT;
	// set up timer B in periodic mode so that itrepeats the time-outs
	HWREG(WTIMER2_BASE+TIMER_O_TBMR) = 
	(HWREG(WTIMER2_BASE+TIMER_O_TBMR)& ~TIMER_TBMR_TBMR_M)| 
	TIMER_TBMR_TBMR_PERIOD;
	// set timeout to control law period
	HWREG(WTIMER2_BASE+TIMER_O_TBILR) = TimerTicksPerMS * CONTROL_LAW_PERIOD;
	// enable a local timeout interrupt
	HWREG(WTIMER2_BASE+TIMER_O_IMR) |= TIMER_IMR_TBTOIM;
    //Setting priority - interrupt 99 - 25*4-1
  HWREG(NVIC_PRI24) = (HWREG(NVIC_PRI24)& ~NVIC_PRI24_INTD_M)| (7 << NVIC_PRI24_INTD_S);
	// enable the Timer B in Wide Timer 2 interrupt in the NVIC
	// it is interrupt number 99 so appears in EN3 at bit 3
	HWREG(NVIC_EN3) |= BIT3HI;
	// make sure interruptsare enabled globally
	__enable_irq();
	// now kick the timer offby enabling it and enabling the timer to
	// stall while stopped by the debugger
	HWREG(WTIMER2_BASE+TIMER_O_CTL) |= (TIMER_CTL_TBEN | TIMER_CTL_TBSTALL);
}


/****************************************************************************
 Function
     position

 Parameters
     void

 Returns
     float
 Description
     Returns a proportional gain term depending on the tape value.
     Bit0hi = leftmost, Bit3hi = rightmost for the tape values
     Negative means turn Right, Positive means turn left

****************************************************************************/
static float position ( void )
{
    uint8_t SensorData = GetTapeValues();
    uint32_t num = 0;
    uint32_t den = 0;

//    printf( " Left-most : %d \t Left: %d \t Right: %d \t Rightmost: %d \r\n", (SensorData & BIT0HI) , 
//            (SensorData & BIT1HI),
//            (SensorData & BIT2HI),
//            (SensorData & BIT3HI));   
    
  
  if (SensorData == 6) // Perfectly Aligned 
  {
    return 0;
  }
  else if (SensorData == 2|| SensorData == 7) // turn left mild
  {
    return 0.5;
  }
  else if (SensorData == 4|| SensorData == 14) // turn right mild
  {
    return -0.5;
  }    
  else if (SensorData == 3) // turn left mod
  {
    return 0.75;
  }
  else if (SensorData == 12) // turn right mod
  {
    return -0.75;
  }
  else if (SensorData == 1) // Turn Left Extreme
  {
    
    return 1.2; //1
  }
  else if (SensorData == 8) // turn right extreme
  {
    return -1.2;//1
  }
  else
    return 10;
  
}    

/****************************************************************************
 Function
     LFControlLaw

 Parameters
     void

 Returns
     void

 Description
     Control Law that alters the Line follower speed based on tape reading values
     Response function; needs to be enabled.

****************************************************************************/

void LFControlLaw (void)
{

	  HWREG(WTIMER2_BASE+TIMER_O_ICR) = TIMER_ICR_TBTOCINT;
    float pos = position();
    
    
   
    if(pos!=10)
    {
      RPM_Correction = pLineGain * DEFAULT_FORWARD_RPM * pos;
    }
        
    prev_error = error;

    
    uint32_t beaconFreq= getDetecterFreq();
    if(beaconFreq <=2000 && beaconFreq >=1000) { //if we see the beacon, we should slow down to ensure a better line follow
        DEFAULT_FORWARD_RPM = 30;
    }
    
    
    if (RPM_Correction > 0)
    {
      SETRPM(DEFAULT_FORWARD_RPM, DEFAULT_FORWARD_RPM - RPM_Correction);
    }
    else
    {
      SETRPM(DEFAULT_FORWARD_RPM + RPM_Correction, DEFAULT_FORWARD_RPM);
    }
     
}

/****************************************************************************
 Function
     DriveToLineAction

 Parameters
     void

 Returns
     void
 Description
     Drives straight until it encounters the line

****************************************************************************/
void DriveToLineAction(void) {
  if (FaceOffFlag)
  {
    DRIVE_STRAIGHT(DRIVE_TO_LINE_RPM, FORWARD_DIRECTION);
    FaceOffFlag = false;
  }
  else{
    SETRPM(55,50);
  }
}


/****************************************************************************
 Function
     AlignWithLineAction

 Parameters
     void

 Returns
     void

 Description
     Turns right until we align perfectly with line

****************************************************************************/
void AlignWithLineAction(void) { //TODO: Add timer to stop
   SETRPM(45,-50);
}

/****************************************************************************
 Function
     FollowTheLineAction

 Parameters
     void

 Returns
     void

 Description
     Enables the Control Law. Tape Following

****************************************************************************/
void FollowTheLineAction(void) {
   InitControlPeriodicInt(); // start control law
}

/****************************************************************************
 Function
     EmitStateAction

 Parameters
     void

 Returns
     void

 Description
     Disables Control Law, Stops the Motors, and emits the appropriate IR freq
        to the loading station. Initializes Ball_Get_Timer to wait for a ball before
        posting a finished reload to Master

****************************************************************************/
void EmitStateAction(void) {
   DisableControlPeriodicInt();
   STOP_MOTORS();
   blastBeaconFreq();
   ES_Timer_InitTimer(Ball_Get_Timer, TIMEBALLGET);
}

/****************************************************************************
 Function
     detStartReloadState

 Parameters
     void

 Returns
     ReloadState_t

 Description
     Essentially, an entry function that determines which reload should start at,
        depending on where it is in regards to the line and the tower.

****************************************************************************/

ReloadState_t detStartReloadState(void) {
  uint8_t tapeValues = GetTapeValues();
  ReloadState_t ReturnState;

   if(tapeValues == 0){ //we are not on tape
      ReturnState = DriveToLine;
      //Do Drive to Line code here
      DriveToLineAction();
   } 
   else { //we are on tape!
     
     if(tapeValues !=6) { //we are not perfectly aligned
        ReturnState = AlignWithLine;
        //Do Align with Line code here
        AlignWithLineAction();
     } 
     else {
        if( !GetAreWeAtTower() ){ //we are not at the tower!
          ReturnState = FollowTheLine;
          //Do Follow the Line code here
          FollowTheLineAction();
        }
        else { //we are at the tower!
          ReturnState = EmitState;
          //Do emit code here
          EmitStateAction();
        }
    }
  }
   
  return ReturnState;
}

ES_Event_t RunReload(ES_Event_t ThisEvent) {
    ES_Event_t ReturnEvent;
    ReturnEvent.EventType = ES_NO_EVENT;
    ReloadState_t NextState = CurrentState;
    
    switch(CurrentState) {
      case Wait:
        if(ThisEvent.EventType == EV_INIT_RELOAD) {
          printf("\r\nReceived EV_INIT_RELOAD from Wait State in Reload");
          NextState = detStartReloadState();
        }
        if(ThisEvent.EventType == EV_STOP_RELOAD) {
          STOP_MOTORS();
          NextState = GameOver;
        }
        break;
      case DriveToLine:
        if(ThisEvent.EventType == EV_LINE_DETECTED) {
          printf("\r\nEV_Line_DETECTED received in DriveToLine state in Reload");
          NextState = AlignWithLine;
          AlignWithLineAction();
        }
        if(ThisEvent.EventType == EV_STOP_RELOAD) {
          STOP_MOTORS();
          NextState = GameOver;
        }
        break;
      case AlignWithLine:
        if(ThisEvent.EventType == EV_ON_TAPE) {
          printf("\r\nEV_ON_TAPE received in AlignWithLine State in Reload");
          NextState = FollowTheLine;
          FollowTheLineAction();
        }
        if(ThisEvent.EventType == EV_STOP_RELOAD) {
          STOP_MOTORS();
          NextState = GameOver;
        }
        break;
      case FollowTheLine:
        if(ThisEvent.EventType == EV_TOWER_DETECTED) {
          printf("\r\nEV_TOWER_DETECTED received in FollowTheLine State in Reload");
          NextState = EmitState;
          EmitStateAction();
        }
        if(ThisEvent.EventType == EV_STOP_RELOAD) {
          STOP_MOTORS();
          NextState = GameOver;
        }
        break;
      case EmitState:
        if(ThisEvent.EventType == EV_BALL_DETECTED) { //TODO: Implement in Hardware
          printf("\r\nEV_BALL_DETECTED received in EmitState in Reload");
          NextState = Wait;
          IncrementBallsInPoss();
          ES_Event_t MasterEvent;
          MasterEvent.EventType = EV_RELOAD_COMPLETE;
          PostMasterSM(MasterEvent);
        }
        if(ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == Ball_Get_Timer){
          printf("\r\nBall_Get_Timer TIMEOUT received in EmitState in Reload");
          NextState = Wait;
          IncrementBallsInPoss(); //TODO: Remove this once ball detecter switch/IR implemented in hardware
          ES_Event_t MasterEvent;
          MasterEvent.EventType = EV_RELOAD_COMPLETE;
          PostMasterSM(MasterEvent);
        }
        if(ThisEvent.EventType == EV_STOP_RELOAD) {
          STOP_MOTORS();
          NextState = GameOver;
        }
        break;
      case GameOver:
        printf("\r\n Reload Service is in the Game Over State. Stop passing events here lmao");
      break;
    }
    CurrentState = NextState;
    return ReturnEvent;
}
