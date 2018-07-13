/****************************************************************************

  Header file for template Flat Sate Machine
  based on the Gen2 Events and Services Framework

 ****************************************************************************/

#ifndef TapeDetectorService_H
#define TapeDetectorService_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */
#include "ES_Events.h"

// typedefs for the states
// State definitions for use with the query function
typedef enum
{
    HighState, DebounceState, WaitState
}TapeDetectorState_t;

// Public Function Prototypes

bool InitTapeDetectorService(uint8_t Priority);
bool PostTapeDetectorService(ES_Event_t ThisEvent);
ES_Event_t RunTapeDetectorService(ES_Event_t ThisEvent);
uint8_t GetTapeValues(void);
//bool CheckLineDetected(void); //removed and reimplemented in WaitState of TapeDetectorService
#endif /* FSMTemplate_H */
