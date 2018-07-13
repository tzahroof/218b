/****************************************************************************
 
  Header file for AD Service
  based on the Gen 2 Events and Services Framework

 ****************************************************************************/

#ifndef ADService_H
#define ADService_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */
#include "ADService.h"
#include "ES_Events.h"

// typedefs for the states
// State definitions for use with the query function

// Public Function Prototypes

bool initializeADService(uint8_t Priority);
bool PostADService( ES_Event_t ThisEvent );
ES_Event_t RunADService( ES_Event_t ThisEvent );

uint32_t getADValue(void);

#endif /* ADService_H */

