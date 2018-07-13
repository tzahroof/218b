/****************************************************************************
 Module
   EventCheckers.c

 Revision
   1.0.1

 Description
   This is the sample for writing event checkers along with the event
   checkers used in the basic framework test harness.

 Notes
   Note the use of static variables in sample event checker to detect
   ONLY transitions.

 History
 When           Who     What/Why
 -------------- ---     --------
 08/06/13 13:36 jec     initial version
****************************************************************************/

// this will pull in the symbolic definitions for events, which we will want
// to post in response to detecting events
#include "ES_Configure.h"
// this will get us the structure definition for events, which we will need
// in order to post events in response to detecting events
#include "ES_Events.h"
// if you want to use distribution lists then you need those function
// definitions too.
#include "ES_PostList.h"
// This include will pull in all of the headers from the service modules
// providing the prototypes for all of the post functions
#include "ES_ServiceHeaders.h"
// this test harness for the framework references the serial routines that
// are defined in ES_Port.c
#include "ES_Port.h"
// include our own prototypes to insure consistency between header &
// actual functionsdefinition
#include "EventCheckers.h"

#include "SPI.h" //include for pound defines

// This is the event checking function sample. It is not intended to be
// included in the module. It is only here as a sample to guide you in writing
// your own event checkers
#if 0
/****************************************************************************
 Function
   Check4Lock
 Parameters
   None
 Returns
   bool: true if a new event was detected
 Description
   Sample event checker grabbed from the simple lock state machine example
 Notes
   will not compile, sample only
 Author
   J. Edward Carryer, 08/06/13, 13:48
****************************************************************************/
bool Check4Lock(void)
{
  static uint8_t  LastPinState = 0;
  uint8_t         CurrentPinState;
  bool            ReturnVal = false;

  CurrentPinState = LOCK_PIN;
  // check for pin high AND different from last time
  // do the check for difference first so that you don't bother with a test
  // of a port/variable that is not going to matter, since it hasn't changed
  if ((CurrentPinState != LastPinState) &&
      (CurrentPinState == LOCK_PIN_HI)) // event detected, so post detected event
  {
    ES_Event ThisEvent;
    ThisEvent.EventType   = ES_LOCK;
    ThisEvent.EventParam  = 1;
    // this could be any of the service post function, ES_PostListx or
    // ES_PostAll functions
    ES_PostList01(ThisEvent);
    ReturnVal = true;
  }
  LastPinState = CurrentPinState; // update the state for next time

  return ReturnVal;
}

#endif

/****************************************************************************
 Function
   Check4Keystroke
 Parameters
   None
 Returns
   bool: true if a new key was detected & posted
 Description
   checks to see if a new key from the keyboard is detected and, if so,
   retrieves the key and posts an ES_NewKey event to TestHarnessService0
 Notes
   The functions that actually check the serial hardware for characters
   and retrieve them are assumed to be in ES_Port.c
   Since we always retrieve the keystroke when we detect it, thus clearing the
   hardware flag that indicates that a new key is ready this event checker
   will only generate events on the arrival of new characters, even though we
   do not internally keep track of the last keystroke that we retrieved.
 Author
   J. Edward Carryer, 08/06/13, 13:48
****************************************************************************/
bool Check4Keystroke(void)
{
 
  if ( IsNewKeyReady() ) // new key waiting?
  {
    ES_Event_t ThisEvent;
    char Key = GetNewKey();
    
    if(Key == 'a')
    {
        ThisEvent.EventType = EV_INIT_RELOAD;
        PostReload(ThisEvent); 
    }
    if(Key == 'b')
    {
        ThisEvent.EventType = EV_LINE_DETECTED;
        PostReload(ThisEvent);
    }
    if(Key == 'c')
    {
        ThisEvent.EventType = EV_ON_TAPE;
        PostReload(ThisEvent);
    }
    if(Key == 'd')
    {
        ThisEvent.EventType = EV_BALL_DETECTED;
        PostReload(ThisEvent);
    }
    if(Key == 'e')
    {
        ThisEvent.EventType = EV_RELOAD_COMPLETE;
        PostMasterSM(ThisEvent);
    }
    if(Key == 'f')
    {
        ThisEvent.EventType = EV_TOWER_DETECTED;
        PostReload(ThisEvent);
    }
    if(Key == 'g')
    {
        ThisEvent.EventType = EV_INIT_OFFENCE;
        PostOffence(ThisEvent);
    }
    if(Key == 'h')
    {
        ThisEvent.EventType = EV_STOP_OFFENCE;
        PostOffence(ThisEvent);
    }          
    if(Key == 'i')
    {
        ThisEvent.EventType = EV_SHOOT;
        PostOffence(ThisEvent);
    }
    if(Key == 'j')
    {    
        ThisEvent.EventType = EV_INIT_DEFENCE;
        PostDefence(ThisEvent);
    }
    if(Key == 'k')
    {    
        ThisEvent.EventType = EV_STOP_DEFENCE;
        PostDefence(ThisEvent);
    }
    if(Key == 'l')
    {
        ThisEvent.EventType = ES_TIMEOUT;
        ThisEvent.EventParam = DEFENCE_TIMER;
        PostDefence(ThisEvent);
    }
    if(Key == 'm')
    {    
        ThisEvent.EventType = ES_TIMEOUT;
        ThisEvent.EventParam = SHOOT_TIMER;
        PostShooter(ThisEvent);
    }
    if(Key == 'n')
    {    
        ThisEvent.EventType = ES_TIMEOUT;
        ThisEvent.EventParam = ReshootTimer;
        PostOffence(ThisEvent);
    }
    if(Key == 'o')
    {    
        ThisEvent.EventType = EV_STATUS_CHANGE;
        ThisEvent.EventParam = US_POSS;
        PostMasterSM(ThisEvent);
    }
    if(Key == 'p')
    {
        ThisEvent.EventType = EV_STATUS_CHANGE;
        ThisEvent.EventParam = THEM_POSS;
        PostMasterSM(ThisEvent);
    }
    if(Key == 'q')
    {    
        ThisEvent.EventType = EV_STATUS_CHANGE;
        ThisEvent.EventParam = TIE_BREAK;
        PostMasterSM(ThisEvent);
    }
    if(Key == 'r')
    {    
        ThisEvent.EventType = EV_STATUS_CHANGE;
        ThisEvent.EventParam = GAME_OVER;
        PostMasterSM(ThisEvent);
    }
    if(Key == 's')
    {
        ThisEvent.EventType = ES_TIMEOUT;
        ThisEvent.EventParam = Ball_Get_Timer;
        PostReload(ThisEvent);
    }
    if(Key == 't')
    {    
        ThisEvent.EventType = ES_TIMEOUT;
        ThisEvent.EventParam = LFTimer;
        PostTapeDetectorService(ThisEvent);
    }
    if(Key == 'u')
    {    
        ThisEvent.EventType = EV_STATUS_CHANGE;
        ThisEvent.EventParam = FACE_OFF;
        PostMasterSM(ThisEvent);
        printf("\r\nPosted Faceoff to master");
    }
    if(Key == 'v')
    {    
        printf("\r\n");
        SPI_SetStatus(34); //Assume Red Team
        printf("\r\nSet US_POSS in SPI");
    }
    if(Key == 'w')
    {
        printf("\r\n");
        SPI_SetStatus(18); //Assume Blue Team
        printf("\r\nSet THEM_POSS in SPI");
    }
    if(Key == 'x')
    {    
        printf("\r\n");
        SPI_SetStatus(3); //TIE_BREAK
        printf("\r\nSet TIE_BREAK in SPI");
    }
    if(Key == 'y')
    {    
        printf("\r\n");
        SPI_SetStatus(200); //GAME_OVER
        printf("\r\nSet GAME_OVER in SPI");
    }
    return true;
  }
  return false;
}

