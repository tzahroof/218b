/*****************************************************************************
 Module
   SPI.c
   

 Description
   Module for running SPI  

 Author
	E. Krimsky, ME218B
****************************************************************************/



#include "SPI.h"
#include "TemplateFSM.h"
#include "MasterSM.h"


//#define TEST
#define PDIV 200 // SSI clock divisor 
#define SCR 19
#define TICKS_PER_MS 40000

#define STATUS_CMD 0x3F
#define SCORE_CMD 0xC3
#define DEFAULT_CMD 0x00

#define PERIODIC_TIMEOUT 10


#define INTD_SHIFT 29
#define STATUS_MASK (BIT0HI|BIT1HI|BIT2HI)
#define POS_MASK (BIT4HI|BIT5HI)

static uint8_t counter = 0;
static uint8_t StatusIn[4];
static uint8_t ScoreIn[4];
static uint8_t PrevSSByte;
static uint8_t Team;


// ------- Private Functions --------------------//
static void SPI_InitPeriodic( void );

/****************************************************************************
 Function
     SPI_Init

 Parameters
     none

 Returns
     non
     
 Description
    Initializes hardware for SPI module 0 (on port A) and configures a debug
    line on PA6
****************************************************************************/

void SPI_Init( void )
{
	// See page 340, 346, 965, 1351 (table 23.5 for pins)	
	/*
    SSI0 - Port A , SSI1 - Port D or F, SSI2 - Port B, SSI3 - Port D
	For port A, SSI...
		CLK - PA2, Alt. Fun - SSI0Clk
		Fss - PA3, Alt Fun - SSI0Fss
		Rx - PA4, Alt. Fun - SSI0Rx
		Tx - PA5, Alt. Fun - SSI0Tx
	*/
  printf("entering SPI Init \r\n");
	//1. Enable the clock to the GPIO port
	HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R0; 

	// 2. Enable the clock to SSI module - p. 347
	HWREG(SYSCTL_RCGCSSI) |= SYSCTL_RCGCSSI_R0; //BIT0HI for SSI0

    // 3. Wait for the GPIO port A to be ready
    while ((HWREG(SYSCTL_PRGPIO) & SYSCTL_PRGPIO_R0) != SYSCTL_PRGPIO_R0)
    	;
    
	// 4. Program the GPIO to use the alternate functions on the SSI pins 
	// see p.1351 (table 23.5)
	HWREG(GPIO_PORTA_BASE+GPIO_O_AFSEL) |= (BIT2HI | BIT3HI | BIT4HI | BIT5HI); 


	// 5. Set mux position in GPIOPCTL to select the SSI use of the pins 
	//  p. 650 mux value is two
	uint8_t BitsPerNibble = 4;
	uint8_t mux = 2; 
    
	// 2, 3, 4, and 5 because those are the bit numbers on the port 
	HWREG(GPIO_PORTA_BASE+GPIO_O_PCTL) = (HWREG(GPIO_PORTA_BASE+GPIO_O_PCTL) 
                                           & 0xff0000ff) + 
	                                     (mux<<(2*BitsPerNibble)) + 
	                                     (mux<<(3*BitsPerNibble)) +
	                                     (mux<<(4*BitsPerNibble)) +
	                                     (mux<<(5*BitsPerNibble));

	// 6. Program the port lines for digital I/O
	HWREG(GPIO_PORTA_BASE+GPIO_O_DEN) |= (BIT2HI | BIT3HI | BIT4HI | BIT5HI); 
	// 7. Program the required data directions on the port lines
	HWREG(GPIO_PORTA_BASE+GPIO_O_DIR) |= (BIT2HI | BIT3HI | BIT5HI);
	HWREG(GPIO_PORTA_BASE+GPIO_O_DIR) &= ~BIT4HI; // Rx needs to be an input 

  // 8. If using SPI mode 3, program the pull-up on the clock line
    /* This pin is configured as a GPIO by default but is locked and 
    can only be reprogrammed by unlocking the pin in the GPIOLOCK
    register and uncommitting it by setting the GPIOCR register. */
    // see p. 1329, p. 650, p. 684 (LOCK) - handled by calling PortFunctions

	HWREG(GPIO_PORTA_BASE+GPIO_O_PUR) |= BIT2HI; // pullup on clock line 

	// 9. Wait for the SSI0 to be ready, see p. 412
	while ((HWREG(SYSCTL_PRSSI) & SYSCTL_PRSSI_R0) != SYSCTL_PRSSI_R0)
		;  

    
	// 10. Make sure that the SSI is disabled before programming mode bits
	// see p. 965
	HWREG(SSI0_BASE+SSI_O_CR1) &= BIT1LO; //SSE; 

	// 11. Select master mode (MS) & TXRIS indicating End of Transmit (EOT)
	HWREG(SSI0_BASE+SSI_O_CR1) &= BIT2LO; // MS; //clear bit to be master 
	HWREG(SSI0_BASE+SSI_O_CR1) |= BIT4HI; // EOT;

	// 12. Configure the SSI clock source to the system clock
	// CC for 'Clock Source'
	HWREG(SSI0_BASE+SSI_O_CC) &= (0xffffffff << 4); // Clear the 4 LSBs

	// 13. Configure the clock pre-scaler

	// bits 8:32 are read only so we should be able to write directly the 
	// 8 LSBs
	HWREG(SSI0_BASE+SSI_O_CPSR) = (HWREG(SSI0_BASE+SSI_O_CPSR) & 
                                                        0xffffff00) | PDIV; 


	//14. Configure clock rate (SCR), phase & polarity (SPH, SPO), mode (FRF),
	// data size (DSS)

	// see p. 969 - 970

	// SCR is bitfields 15:8 
	// first clear the SCR bits then sent them
	HWREG(SSI0_BASE+SSI_O_CR0) = (HWREG(SSI0_BASE+SSI_O_CR0) & ~0x0000ff00) |
											(SCR << 8); 
	// SPH is bitfield 7 
	HWREG(SSI0_BASE+SSI_O_CR0) |= BIT7HI; 	

	// SPO is bitfield 6 
	HWREG(SSI0_BASE+SSI_O_CR0) |= BIT6HI; 												

	// FRF - bitfields 5:4, write 00 for freescale
	HWREG(SSI0_BASE+SSI_O_CR0) &= (BIT4LO & BIT5LO);

	// DSS - bitfields 3:0, // 0x7 for 8 bit data 
	uint8_t DSS = (BIT0HI | BIT1HI | BIT2HI | BIT3HI);
	HWREG(SSI0_BASE+SSI_O_CR0) = (HWREG(SSI0_BASE+SSI_O_CR0) & ~DSS)| (0x7); 

	// 15. Locally enable interrupts (TXIM in SSIIM)
	HWREG(SSI0_BASE+SSI_O_IM) &= ~BIT3HI; // Enable TXIM - Check this 

	// 16. Make sure that the SSI is enabled for operation
	HWREG(SSI0_BASE+SSI_O_CR1) |= BIT1HI; //SSE; 

	//17. Enable the NVIC interrupt for the SSI when starting to transmit
	// see p. 104, table 2-9
	// SSI0 is vector number 23, int num 7 
	// p. 140 use EN0 for int 0 - 31 
	HWREG(NVIC_EN0) |= BIT7HI;
    
	// Erez added: enable interrupts globally
	__enable_irq();
    SPI_InitPeriodic();
    
    //set value of team (red or blue) by polling team switch
    printf("Before PF3 initialization \r \n.");
    // Initialize PF3 as input
    HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R5;
    while ((HWREG(SYSCTL_PRGPIO) & SYSCTL_PRGPIO_R5) != SYSCTL_PRGPIO_R5)
    	; 
    HWREG(GPIO_PORTF_BASE+GPIO_O_DEN) |= (BIT3HI);
    HWREG(GPIO_PORTF_BASE+GPIO_O_DIR) &= ~(BIT3HI);
    
    printf("PF3 initialized as input \r\n");
    
    if ((HWREG(GPIO_PORTF_BASE+ALL_BITS))&BIT3HI)
    {
      Team = BLUE_TEAM;
      printf("We are team BLUE/ \r \n");
    }
    else
    {
      Team = RED_TEAM;
      printf("We are team RED/ \r \n");
    }
        
    printf(" SPI Initialized \r \n");
    
    //Initializing PastStatus
    PrevSSByte = SPI_GetStatus();
}



/****************************************************************************
 Function
     SPI_GetStatus

 Parameters
     none

 Returns
     SSByte
 
****************************************************************************/
uint8_t SPI_GetStatus ( void )
{
    return StatusIn[3];
}

/****************************************************************************
 Function
     SPI_GetShotClock

 Parameters
     none

 Returns
     SCByte
 
****************************************************************************/
uint8_t SPI_GetShotClock ( void ) //TODO: ensure that this is in milliseconds
{
    return StatusIn[2];
}

/****************************************************************************
 Function
     SPI_GetRedScore

 Parameters
     none

 Returns
     SSByte
 
****************************************************************************/

uint8_t SPI_GetUsScore ( void )
{
    
  if (Team)
  {
    return ScoreIn[3];
  }
  else
  {
    return ScoreIn[2];
  }
}

/****************************************************************************
 Function
     SPI_GetBlueScore

 Parameters
     none

 Returns
     BBByte
 
****************************************************************************/
uint8_t SPI_GetThemScore ( void )
{
  if (Team)
  {
    return ScoreIn[2];
  }
  else
  {
    return ScoreIn[3];
  }
}

/****************************************************************************
 Function
     SPI_IntResponse

 Parameters
     none 

 Returns
     none
     
 Description
    Function that runs when EOT timeout is detected. Assigns the new byte to 
    module level variable "byteIn"
****************************************************************************/
void SPI_IntResponse ( void )
{
    // Disable (mask the interrupt) until next write 
    // see p. 973
    HWREG(SSI0_BASE+SSI_O_IM) &= ~BIT3HI; // Disable TXIM
    
    switch (counter)
    {
        case 0:
            for (int i=0; i<4; i++)
            {
                StatusIn[i] = (uint8_t)HWREG(SSI0_BASE+SSI_O_DR); 
            }
            break;
        
        case 1:
            for (int i=0; i<4; i++)
            {
                ScoreIn[i] = (uint8_t)HWREG(SSI0_BASE+SSI_O_DR); 
            }
            break;
    }
    
    //printf("%d",StatusIn[3]);
    //Increment Counter
    counter = (counter+1)%2;
    
    	
}


/****************************************************************************
 Function
     SPI_InitPeriodic

 Parameters
     none

 Returns
     none

 Description
     Initializes Periodic timer for updating the SPI read value 

****************************************************************************/
static void SPI_InitPeriodic( void )
{
    // enable clock to wide timer 0 
    HWREG(SYSCTL_RCGCWTIMER) |= SYSCTL_RCGCWTIMER_R0;
    
    // wait for clock to get going 
    while((HWREG(SYSCTL_PRWTIMER) & SYSCTL_PRWTIMER_R0) != SYSCTL_PRWTIMER_R0)
        ;
    // Disbale timer B before configuring
    HWREG(WTIMER0_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TBEN;
    
    // set in individual (not concatenated) -- make sure this doesn't mess 
    // with other timer
    HWREG(WTIMER0_BASE+TIMER_O_CFG) = TIMER_CFG_16_BIT;
    
    // set up timer B in periodic mode 
    HWREG(WTIMER0_BASE+TIMER_O_TBMR) = (HWREG(WTIMER0_BASE+TIMER_O_TBMR) 
            & ~TIMER_TBMR_TBMR_M) | TIMER_TBMR_TBMR_PERIOD;

    // set timeout to 3 ms 
    HWREG(WTIMER0_BASE+TIMER_O_TBILR) = TICKS_PER_MS * PERIODIC_TIMEOUT;
    
    // enable local timeout interrupt
    HWREG(WTIMER0_BASE+TIMER_O_IMR) |= TIMER_IMR_TBTOIM;
    
    // enable in the NVIC, Int number 95 so 95-64 = BIT31HI, NVIC_EN2
    HWREG(NVIC_EN2) |= BIT31HI;
    
    // set as priority 1 ( encoder edge will have default priority 0) 
    // int 95 so PRI23 and using INTD
    // left shift by 29 because intD 
    HWREG(NVIC_PRI23) = (HWREG(NVIC_PRI23) & NVIC_PRI23_INTD_M) | 
                                ( 0 << INTD_SHIFT);
    
    // enable interrupts globally
    __enable_irq();
    
    // start the timer and set to stall with debugger
    HWREG(WTIMER0_BASE+TIMER_O_CTL) |= (TIMER_CTL_TBEN | TIMER_CTL_TBSTALL);
    
}


/****************************************************************************
 Function
     SPI_PeriodicResponse

 Parameters
     none

 Returns
     none

 Description
     Interrupt response routine for 2ms Periodic interupt

****************************************************************************/
void SPI_PeriodicResponse ( void )
{
    // clear the source of the interupt
    HWREG(WTIMER0_BASE+TIMER_O_ICR) = TIMER_ICR_TBTOCINT;   
    
    switch (counter)
    {
        case 0:
            HWREG(SSI0_BASE+SSI_O_DR) = STATUS_CMD;
            for (int i=0; i<3; i++)
            {
                HWREG(SSI0_BASE+SSI_O_DR) = DEFAULT_CMD;
            }
            break;
        
        case 1:
            HWREG(SSI0_BASE+SSI_O_DR) = SCORE_CMD;
            for (int i=0; i<3; i++)
            {
                HWREG(SSI0_BASE+SSI_O_DR) = DEFAULT_CMD;
            }
            break;
    }
    
    HWREG(SSI0_BASE+SSI_O_IM) |= BIT3HI; // Enable TXIM
}

/****************************************************************************
 Function
     GameStatusEventChecker

 Parameters
     none

 Returns
     bool

 Description
     Posts status change events

****************************************************************************/
//TODO: Re-enable GameStatusEventChecker

bool GameStatusEventChecker ( void )
{
    //local RetrunVal = False, CurrentInputState
    bool ReturnVal = false;
    uint8_t CurrentSSByte = SPI_GetStatus();
    //printf("%d",CurrentSSByte);
    
    ES_Event_t GameStatusChange;
    
    //printf("prev:%d \t curr:%d \r \n", PrevGameStatus, CurrentGameStatus);
 
    if (  CurrentSSByte != PrevSSByte )
    {
        
        GameStatusChange.EventType = EV_STATUS_CHANGE;
        switch (CurrentSSByte)
        {
            case (0):
                printf("Status changed to: Waiting For Start : %d \t %d \r \n", CurrentSSByte, PrevSSByte);
                GameStatusChange.EventParam = WAITING_FOR_START;
                
                break; 
        
            case (1):
                printf("Status changed to: Face-Off : %d \r \n", CurrentSSByte);
                GameStatusChange.EventParam = FACE_OFF;
                printf("FACE_OFF \r \n");
                break;
            
            case (34):
                printf("Status changed to: Playing : Blue : %d \r \n", CurrentSSByte);
                if (Team)
                {
                  GameStatusChange.EventParam = US_POSS;
                  printf("We have possession. \r \n");
                }
                else
                {
                  GameStatusChange.EventParam = THEM_POSS;
                  printf("They have possession. \r \n");
                }
                
                break;
            
            case (18):
                printf("Status changed to: Playing : Red :%d \r \n", CurrentSSByte);
                if (Team)
                {
                  GameStatusChange.EventParam = THEM_POSS;
                  printf("They have possession. \r \n");
                }
                else
                {
                  GameStatusChange.EventParam = US_POSS;
                  printf("We have possession. \r \n");
                }
               
                break;
            
            case (3):
                printf("Status changed to: Tie break :%d \r \n", CurrentSSByte);
                GameStatusChange.EventParam = TIE_BREAK;
                printf("TIE BREAK \r \n");
                break;
            
            case (255):
              break;
            
            default : 
                printf("Status changed to: Game Over (by GameStatusChecker) : %d \r \n", CurrentSSByte);
                GameStatusChange.EventParam = GAME_OVER;
                printf("GAME OVER \r \n");
        }
        
        //Post GameStatusChange to relevant service
        /* Insert code here */
        PostMasterSM(GameStatusChange);
        
        //Update ReturnVal to true
        ReturnVal = true;
    }
    
    // Update PrevSSByte
    PrevSSByte = CurrentSSByte;
    
    // return ReturnValue
    return ReturnVal;
  
}

uint8_t SPI_GetTeam (void)
{
  return Team;
}

uint8_t SPI_GetStatusMessage(void)
{
  uint8_t CurrentSSByte = SPI_GetStatus();
  uint8_t ReturnValue = 255;
  switch (CurrentSSByte)
  {
    case (0):
        ReturnValue = WAITING_FOR_START;        
                break; 
        
    case (1):
         ReturnValue = FACE_OFF;    
                break;
            
    case (34):
                
        if (Team)
        {
          ReturnValue = US_POSS;
        }
        else
        {
          ReturnValue = THEM_POSS;
        }
        
        break;
            
    case (18):
                
        if (Team)
        {
          ReturnValue = THEM_POSS;
        }
        else
        {
          ReturnValue = US_POSS;
        }
       
        break;
            
     case (3):
         ReturnValue = TIE_BREAK;
         break;
            
     case (255):
          break;
            
     default :
         ReturnValue = GAME_OVER;
  }
  
  return ReturnValue;
}


//TEST METHOD FOR HARNESS TESTING\
//assume Team  = True
void SPI_SetStatus(uint8_t CurrentSSByte) {

    //printf("%d",CurrentSSByte);
    StatusIn[3] = CurrentSSByte;
    StatusIn[2] = 10; //arbitrarily set the shot clock high
    ES_Event_t GameStatusChange;
    Team=true;
    
    //printf("prev:%d \t curr:%d \r \n", PrevGameStatus, CurrentGameStatus);
 
    if (  CurrentSSByte != PrevSSByte )
    {
        
        GameStatusChange.EventType = EV_STATUS_CHANGE;
        switch (CurrentSSByte)
        {
            case (0):
                printf("Status changed to: Waiting For Start : %d \t %d \r \n", CurrentSSByte, PrevSSByte);
                GameStatusChange.EventParam = WAITING_FOR_START;
                
                break; 
        
            case (1):
                printf("Status changed to: Face-Off : %d \r \n", CurrentSSByte);
                GameStatusChange.EventParam = FACE_OFF;
                printf("FACE_OFF \r \n");
                break;
            
            case (34):
                printf("Status changed to: Playing : Blue : %d \r \n", CurrentSSByte);
                if (Team)
                {
                  GameStatusChange.EventParam = US_POSS;
                  printf("We have possession. \r \n");
                }
                else
                {
                  GameStatusChange.EventParam = THEM_POSS;
                  printf("They have possession. \r \n");
                }
                
                break;
            
            case (18):
                printf("Status changed to: Playing : Red :%d \r \n", CurrentSSByte);
                if (Team)
                {
                  GameStatusChange.EventParam = THEM_POSS;
                  printf("They have possession. \r \n");
                }
                else
                {
                  GameStatusChange.EventParam = US_POSS;
                  printf("We have possession. \r \n");
                }
               
                break;
            
            case (3):
                printf("Status changed to: Tie break :%d \r \n", CurrentSSByte);
                GameStatusChange.EventParam = TIE_BREAK;
                printf("TIE BREAK \r \n");
                break;
            
            case (255):
              break;
            
            default : 
                printf("Status changed to: Game Over (by GameStatusChecker) : %d \r \n", CurrentSSByte);
                GameStatusChange.EventParam = GAME_OVER;
                printf("GAME OVER \r \n");
        }
      }
        
        //Post GameStatusChange to relevant service
        /* Insert code here */
        PostMasterSM(GameStatusChange);
        PrevSSByte = CurrentSSByte;
        
}

#ifdef TEST
int main (void)
{
    // Set the clock to run at 40MhZ using the PLL and 16MHz external crystal
    SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN
                            | SYSCTL_XTAL_16MHZ);
    TERMIO_Init();
    PortFunctionInit();
    printf("\x1b[2J"); // clear screen 

    // When doing testing, it is useful to announce just which program
    // is running.
    puts("\rStarting Test Harness for SPI\r");
    SPI_Init(); 
    
    return 1; 
}
#endif
