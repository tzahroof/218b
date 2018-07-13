/****************************************************************************
 
  Header file for Motor Service
  based on the Gen 2 Events and Services Framework

 ****************************************************************************/

#ifndef MotorService_H
#define MotorService_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */
#include "MotorService.h"
#include "ES_Events.h"

#define FORWARD_DIRECTION 1
#define BACKWARD_DIRECTION 0
#define TURN_LEFT 2
#define TURN_RIGHT 3


// Public Function Prototypes

bool initializeStepService(uint8_t Priority);
bool PostStepService( ES_Event_t ThisEvent );
ES_Event_t RunStepService( ES_Event_t ThisEvent );

void InitInputCapture( void );
void InitInputCapture2( void );


void STOP_MOTORS(void);
void TURN(uint32_t Direction);
void DRIVE_STRAIGHT(uint32_t RPM, uint32_t Direction);
void SETRPM(float RPM1, float RPM2); 
float GetSpeed(void);// Get speed of Right Motor
float GetSpeed2(void);// Get speed of Left Motor



// Directions: 0- Front, 1- back, 2- right, 3 - left 
//RPM1 - Right Motor, RPM2 - Left Motor

#endif /* MotorService_H */

