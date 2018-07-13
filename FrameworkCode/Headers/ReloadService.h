/****************************************************************************

  Header file for template Flat Sate Machine
  based on the Gen2 Events and Services Framework

 ****************************************************************************/

#ifndef ReloadService_H
#define ReloadService_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */
#include "ES_Events.h"



// Public Function Prototypes

bool InitReload(uint8_t Priority);
bool PostReload(ES_Event_t ThisEvent);
ES_Event_t RunReload(ES_Event_t ThisEvent);

#endif /* end of ReloadService */
