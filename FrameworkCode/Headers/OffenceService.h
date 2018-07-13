/****************************************************************************

  Header file for template Flat Sate Machine
  based on the Gen2 Events and Services Framework

 ****************************************************************************/

#ifndef OffenceService_H
#define OffenceService_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */
#include "ES_Events.h"



// Public Function Prototypes

bool InitOffence(uint8_t Priority);
bool PostOffence(ES_Event_t ThisEvent);
ES_Event_t RunOffence(ES_Event_t ThisEvent);

#endif /* end of OffenceService */
