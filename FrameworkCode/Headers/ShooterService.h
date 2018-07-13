#ifndef SHOOTER_TEST
#define SHOOTER_TEST

#include <stdint.h>
#include <stdbool.h>

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */
#include "ES_Events.h" // Erez Added 
#include "ES_Framework.h"

#define shootTime 1500 //ms

bool InitShooter(uint8_t Priority);
bool PostShooter(ES_Event_t ThisEvent);
ES_Event_t RunShooter(ES_Event_t ThisEvent);
void LowerServo( void );
void RaiseServo( void );

#endif
