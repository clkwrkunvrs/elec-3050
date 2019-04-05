/*====================================================*/
/* Victor P. Nelson */
/* ELEC 3040/3050 - Lab 1, Program 1 */
/* Toggle LED1 while button pressed, with short delay inserted */
/*====================================================*/
#include "STM32L1xx.h" /* Microcontroller information */
#include <math.h>
/* Define global variables */
static uint8_t count1 = 0; /* determines where in the count the program is */
//static int direction = 0;
static uint8_t row = 0, col = 0, key = 0, shown = 0;
//static uint16_t downLED = 1,upLED = 0;
static const uint16_t keyArray[4][4] = { //this is for assigning the keypresses (row and column pair) to their appropriate outputs
  {1,2,3,0xA},
  {4,5,6,0xB},
  {7,8,9,0xC},
  {0xE,0,0xF,0xD}
};
static int displayKey = 0;
/*---------------------------------------------------*/
/* Initialize GPIO pins used in the program */
/* PA0 = push button */
/* PC8 = blue LED, PC9 = green LED */
/*---------------------------------------------------*/
void PinSetup () {
	RCC->AHBENR |= 0x07; /* Enable GPIO A,B, and C clock (bits 2:0) */
  GPIOA->MODER &= ~(0x00C); //clear Moder bits for bit 1 of GPIOA
  GPIOA ->MODER |= 0x0000; //set PA1 as an input
  GPIOB->MODER &= ~(0xFFFF); //Clear PB[7:0]
  GPIOB->MODER |= 0x5500; //set PB[7:4] (columns) as output and PB[3:0] (rows) as inputs
	GPIOC->MODER &= ~(0x0FF); /*Clear PC[3:0] Mode bits*/
  GPIOC->MODER |= (0x00055); /* General purpose output mode FOR PC[3:0]*/
  GPIOB->PUPDR &= ~(0X0FF); //clear row pupdrs
  GPIOB->PUPDR |= 0x055; //pull the rows up to 1. PB[3:0]
  GPIOB->ODR &= ~(0xF0); //clear ODR bits for columns PB[7:4]
  GPIOB->ODR |= 0x00; //drive columns low
  //optionally clear ODR bits for GPIOC
  //INTERRUPT STUFF
  SYSCFG->EXTICR[0] &= 0xFF0F; //clear EXTI1 bit field
  SYSCFG->EXTICR[0] |= 0x0000; //set EXTI1 to register from PA1, the IRQ# interrupt
  EXTI->FTSR |= 0x0002; //set PA[1] IRQ# interrupt to falling edge triggered
  EXTI->IMR |= 0x0002; //Enable EXTI1
  NVIC_EnableIRQ(EXTI1_IRQn);  //ENABLE EXTI interrupts
  NVIC_ClearPendingIRQ(EXTI1_IRQn); //Clear EXTI1 pending flag
  __enable_irq(); //enable CPU interrupts
}

/* Function to count up and down from 0 to 9 and repeat */
void counting1(void) {
  //COUNT1 done
  count1++;
  if(count1 == 0x0) {count1 = 9;}
  else if(count1 == 0xA) {count1 = 0;}
  
  //show the key for 5 delays
  if(displayKey) {
    shown++;
  }
}

/*----------------------------------------------------------*/
/* Delay function - do nothing for about 1 second */
/*----------------------------------------------------------*/
void delay () {
  int i,j,n;
  for (i=0; i<40; i++) { //outer loop
    for (j=0; j<9500; j++) { //inner loop
      n = j; //dummy operation for single-step test
    } //do nothing
  }
}

void EXTI1_IRQHandler(void) {
  for(int c = 0;c < 4;c++); //small delay between reading and writing
  //get the row
  int i = 1;
  while(i<9) {
    if((GPIOB->IDR & i) == 0) {
			row = log2(i) + 1;
      break;
    }
    i *= 2;
  }

  //now drive the rows and read columns
	//p drive rows low
	GPIOB->ODR &= ~(0xF0); //clear ODR bits for columns PB[3:0]
  GPIOB->ODR |= 0x00; //drive rows low, PB[3:0]
  //Switch I/O modes
	GPIOB->MODER &= ~(0xFFFF);
	GPIOB->MODER |= 0x0055; //make columns input and rows output PB[7:4] input and PB[3:0] output
	
 //Pullup columns and
  GPIOB->PUPDR &= ~(0xFF00); //clear column bits PB[7:4]
  GPIOB->PUPDR |= 0x5500; //set pullup resistors for column bits PB[7:0]

  for(int k = 0;k < 4;k++); //small delay between reading and writing

  //get the column
	i = 1;
  while(i < 9) {
    if(((GPIOB->IDR >> 4) & i) == 0) {
			//if(i == 1) col = 1;
      col = log2(i) + 1;
      break;
    }
    i *= 2;
  }

  //determine which key was pressed
  key = keyArray[row - 1][col - 1];
  
  displayKey = 1;
  
	//debounce loop. Hoping this equals apprx 0.8msec
  int m,j;
  for (m=0; m<20; m++) { //outer loop
    for (j=0; j<141; j++) { //inner loop
   } //do nothing
  }
	//switch the rows and column modes back to rows in columns out
  GPIOB->MODER &= ~(0xFFFF); //Clear PB[7:0]
  GPIOB->MODER |= 0x5500; //set PB[7:4] (columns) as output and PB[3:0] (rows) as inputs
	GPIOB->PUPDR &= ~(0X0FF); //clear row pupdrs
  GPIOB->PUPDR |= 0x055; //pull the rows up to 1. PB[3:0]
	GPIOB->ODR &= ~(0xF0); //clear ODR bits for columns PB[7:4]
  GPIOB->ODR |= 0x00; //drive columns low
	
	//display
	GPIOC->ODR &= ~(0x0F); //clear PC[3:0]
  GPIOC->ODR = key & 0x000F;
  //clear pending flags/registers
  EXTI->PR |= 0x0002;	//Clear EXTI1 pending status
  NVIC_ClearPendingIRQ(EXTI1_IRQn);
  
}
/*------------------------------------------------*/
/* Main program */
/*------------------------------------------------*/
int main(void) {
  PinSetup(); //Configure GPIO pins

  /* Endless loop */
  while (1) { //Can also use: for(;;)
    counting1();
    delay(); //1 second delay to count to next number

    if(!displayKey) {
      GPIOC->ODR = (uint16_t)count1 & 0x000F; //AND the count bits with 0x00FF to get binary value of count1
    }
    else {
      //display the number
			//GPIOC->ODR &= ~(0x0F); //clear PC[3:0]
      //GPIOC->ODR = key & 0x000F;
      if(shown == 3) {
        displayKey = 0;
        shown = 0;
      }
    }
  }
} /* repeat forever */
