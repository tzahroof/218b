/*****************************************************************************
 Module
   ShooterService.c 

 Description
   Raises and lowers servo arm to shoot ball 

 Author
	E. Krimsky, ME218B
****************************************************************************/



// Configuration Headers 
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "ES_DeferRecall.h"
#include "ES_ShortTimer.h"
#include "PWMLib.h"

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"  // Define PART_TM4C123GH6PM in project
#include "driverlib/gpio.h"


// Additional Header Files for PWM + TIMER
#include "PWMLib.h"
#include "ShooterService.h"


#define SERVO_START_PW 3000  // DOWN
#define SERVO_SHOOT_PW 1900  // UP 

#define SERVO_NUM 6 // PC4
#define SERVO_GROUP 3
#define SERVO_FREQ 50 

#define MIN_PULSE_WIDTH 1000 // microseconds 
#define MAX_PULSE_WIDTH 2000 // microseconds 
#define MICRO_PER_SEC 1e6

static uint8_t MyPriority; 


/* ----------------- Private Function Prototypes --------------------------*/
static void Servo_Set_Pulse_Width(uint16_t Pulse_Width);


/****************************************************************************
 Function
     InitShooter

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, and does any 
     other required initialization for this service

****************************************************************************/
bool InitShooter( uint8_t Priority )
{
	  //Assign local priority variable MyPriority
    MyPriority = Priority;
  
    // Setup a PWM line 
    PWM_Init(SERVO_NUM); // TODO double check 
    PWM_SetFreq(SERVO_FREQ, SERVO_GROUP); // NOTE Dhruv is confident about this 
    
    RaiseServo(); 
    return true;
}

/****************************************************************************
 Function
     PostShooter

 Parameters
     ES_Event_t: the event to post

 Returns
     bool, false if error in posting

 Description
     Post function
****************************************************************************/
bool PostShooter( ES_Event_t ThisEvent)
{
	return ES_PostToService( MyPriority, ThisEvent);
}

/****************************************************************************
 Function
     RunShooter

 Parameters
     uint8_t : the priorty of this service

 Returns
     ES_Event

 Description
     Runs the state machine for this module
****************************************************************************/
ES_Event_t RunShooter( ES_Event_t ThisEvent )
{
	  //Create ES_Event ReturnEvent
    ES_Event_t ReturnEvent;
    ReturnEvent.EventType = ES_NO_EVENT; 
  
    if (ThisEvent.EventType == EV_SHOOT)
    {
        printf("\r\nLowering Servo in Shooter Service");        // Lower the servo
        LowerServo(); 
      
        // Set a timer 
        ES_Timer_InitTimer(SHOOT_TIMER, shootTime); 
    }
    else if (ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == SHOOT_TIMER)
    {
        printf("\r\nRaising Servo in Shooter Service");
        // Raise the servo 
        RaiseServo(); 
    }      
    return ReturnEvent;
}

// Raise the shooting servo 
void RaiseServo( void )
{
    Servo_Set_Pulse_Width(SERVO_SHOOT_PW);
}

// Lower the shooting servo
void LowerServo( void )
{
    Servo_Set_Pulse_Width(SERVO_START_PW);
}

// Used for raising or lowering shoot servo 
static void Servo_Set_Pulse_Width(uint16_t Pulse_Width)
{
  uint8_t new_duty = 100 * (SERVO_FREQ * Pulse_Width)/MICRO_PER_SEC;
  PWM_SetDuty(new_duty, SERVO_NUM); 
}


