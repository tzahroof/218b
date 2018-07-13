#ifndef DETECTER_SERVICE_H
#define DETECTER_SERVICE_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */
#include "MotorService.h"
#include "ES_Events.h"

void InitDetecterService(void);
uint32_t getDetecterFreq(void); //Returns (2*the detected signal) in hertz
void blastBeaconFreq(void); //Turns on the signal we IR Emit


#endif /* DETECTER_SERVICE_H */
