/****************************************************************************

  Header file for template Flat Sate Machine
  based on the Gen2 Events and Services Framework

 ****************************************************************************/

#ifndef MasterSM_H
#define MasterSM_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */
#include "ES_Events.h"



// Public Function Prototypes

bool InitMasterSM(uint8_t Priority);
bool PostMasterSM(ES_Event_t ThisEvent);
ES_Event_t RunMasterSM(ES_Event_t ThisEvent);
void IncrementBallsInPoss(void) ;
void DecrementBallsInPoss(void);
uint8_t GetBallsInPoss(void);
bool checkTowerDetected(void);
bool GetAreWeAtTower(void);

#endif /* MasterSM_H */
