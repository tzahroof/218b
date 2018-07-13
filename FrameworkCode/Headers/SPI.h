#ifndef SPI_H
#define SPI_H

#include <stdint.h>
#include <stdbool.h>

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */
#include "ES_Events.h" // Erez Added 
#include "ES_Framework.h"

// Configuration Headers 
#include "ES_Framework.h"
#include "ES_DeferRecall.h"
#include "ES_ShortTimer.h"
#include "termio.h"
#include "EnablePA25_PB23_PD7_PF0.h"


#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_ssi.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"  // Define PART_TM4C123GH6PM in project
#include "driverlib/gpio.h"
#include "inc/hw_timer.h"
#include "inc/hw_nvic.h"

#define RED_TEAM 0
#define BLUE_TEAM 1

#define WAITING_FOR_START 5
#define FACE_OFF 0
#define US_POSS 1
#define THEM_POSS 2
#define TIE_BREAK 3
#define GAME_OVER 4

void SPI_Init( void );
uint8_t SPI_GetStatus( void ); // SS - game status + posession
uint8_t SPI_GetShotClock( void ); // SC - shot clock value
uint8_t SPI_GetUsScore ( void );
uint8_t SPI_GetThemScore ( void );
bool GameStatusEventChecker ( void );
uint8_t SPI_GetTeam (void); //0 - red 1   1- blue
uint8_t SPI_GetStatusMessage(void);

//debugging methods
void SPI_SetStatus(uint8_t CurrentSSByte);
#endif



