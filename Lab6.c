/*====================================================*/
/* Victor P. Nelson */
/* ELEC 3040/3050 - Lab 1, Program 1 */
/* Toggle LED1 while button pressed, with short delay inserted */
/*====================================================*/
#include "STM32L1xx.h" /* Microcontroller information */
#include <math.h>
/* Define global variables */
static uint8_t row = 0, col = 0, key = 0, ten = 0, sec = 0, startStop = 0;
static const uint16_t keyArray[4][4] = { //this is for assigning the keypresses (row and column pair) to their appropriate outputs
  {1,2,3,0xA},
  {4,5,6,0xB},
  {7,8,9,0xC},
  {0xE,0,0xF,0xD}
};

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
  GPIOC->MODER |= (0x05555); /* General purpose output mode FOR PC[3:0]*/
  GPIOB->PUPDR &= ~(0X0FF); //clear row pupdrs
  GPIOB->PUPDR |= 0x055; //pull the rows up to 1. PB[3:0]
  GPIOB->ODR &= ~(0xF0); //clear ODR bits for columns PB[7:4]
  GPIOB->ODR |= 0x00; //drive columns low
  GPIOC->ODR &= ~(0xFF);
  GPIOC->ODR |= 0x00; //Initialize output to all zeroes
  //optionally clear ODR bits for GPIOC
  //keypad INTERRUPT STUFF
  SYSCFG->EXTICR[0] &= 0xFF0F; //clear EXTI1 bit field
  SYSCFG->EXTICR[0] |= 0x0000; //set EXTI1 to register from PA1, the IRQ# interrupt
  EXTI->FTSR |= 0x0002; //set PA[1] IRQ# interrupt to falling edge triggered
  EXTI->IMR |= 0x0002; //Enable EXTI1
  NVIC_EnableIRQ(EXTI1_IRQn);  //ENABLE EXTI interrupts
  NVIC_ClearPendingIRQ(EXTI1_IRQn); //Clear EXTI1 pending flag
  //timer interrupt STUFF
  RCC->APB2ENR|= RCC_APB2ENR_TIM10EN; // enable clock source
  //TIM10->DIER |= TIM_DIER_UIE; //optional, same as next instruction
  
  TIM10->PSC= 2097; //divide into tenths of a second
  TIM10->ARR = 99; //Auto reload register. how many PSC intervals before tick (= ARR + 1)
  TIM10->DIER |= 0x01; //enable timer 10 interrupts
	NVIC_EnableIRQ(TIM10_IRQn); //ENABLE timer interrupts
  NVIC_ClearPendingIRQ(TIM10_IRQn);

  __enable_irq(); //enable CPU interrupts
}

void TIM10_IRQHandler() {
  if(startStop){ //only if counter is started
    ten++;
    if(ten == 0xA) {
      ten = 0;
      if(sec < 9) {
        sec++;
      }
      else sec = 0;
    }
  }
  //display the time on PC[7:0]
  GPIOC->ODR &= ~(0XFF);
  GPIOC->ODR |= ((sec << 4) | ten) & 0x00FF;
  //clear UIFlag
  TIM10->SR &= ~(0x01);
	//TIM10->SR |= 0x00;
	NVIC_ClearPendingIRQ(TIM10_IRQn);
}

void EXTI1_IRQHandler(void) { //keypad handler
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
	//drive rows low
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
			col = log2(i) + 1;
      break;
    }
    i *= 2;
  }

  //determine which key was pressed
  key = keyArray[row - 1][col - 1];

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
  if((key == 0x1) && (startStop == 0x0)) {//if clear button is pressed, clear if counter is stopped
    //reset value of counters
    ten = 0;
    sec = 0;
    //set LEDs to 0
    GPIOC->ODR &= 0x00;
  }
  else if(key == 0x0){ //start and pause timer
    startStop = (startStop == 0x0) ? 0x1 : 0x0;
    if(startStop == 1) {
      TIM10->CR1 |= 0x01; //enable timer counting
    }
    else TIM10->CR1 &= ~0X01; //disable timer counting
  }

  //clear pending flags/registers
  EXTI->PR |= 0x0002;	//Clear EXTI1 pending status
  NVIC_ClearPendingIRQ(EXTI1_IRQn);

}
/*------------------------------------------------*/
/* Main program */
/*------------------------------------------------*/
int main(void) {
  PinSetup(); //Configure GPIO pins
  ten = 0;
  sec = 0;

  /* Endless loop */
  while (1) { //Can also use: for(;;)
  //wait for interrupt
  }
} /* repeat forever */
