/****************************************************************************

  Header file for template Flat Sate Machine
  based on the Gen2 Events and Services Framework

 ****************************************************************************/

#ifndef DefenceService_H
#define DefenceService_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */
#include "ES_Events.h"



// Public Function Prototypes

bool InitDefence(uint8_t Priority);
bool PostDefence(ES_Event_t ThisEvent);
ES_Event_t RunDefence(ES_Event_t ThisEvent);

#endif /* end of DefenceService */
