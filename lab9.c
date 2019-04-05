/*====================================================*/
/* Victor P. Nelson */
/* ELEC 3040/3050 - Lab 1, Program 1 */
/* Toggle LED1 while button pressed, with short delay inserted */
/*====================================================*/
#include "STM32L1xx.h" /* Microcontroller information */
#include <math.h>
/* Define global variables */
static uint8_t row = 0, col = 0, key = 0, second = 0;
static uint16_t interval = 0, count = 0;
static float dutyCycle = 0;
static const uint16_t keyArray[4][4] = { //this is for assigning the keypresses (row and column pair) to their appropriate outputs
  {1,2,3,0xA},
  {4,5,6,0xB},
  {7,8,9,0xC},
  {0xE,0,0xF,0xD}
};
static const float dutyArray[11] = {0,.1,.2,.3,.4,.5,.6,.7,.8,.9,1}; //duty cycle percentages

/*---------------------------------------------------*/
/* Initialize GPIO pins used in the program */
/* PA0 = push button */
/* PC8 = blue LED, PC9 = green LED */
/*---------------------------------------------------*/
void PinSetup () {
	RCC->AHBENR |= 0x07; /* Enable GPIO A,B, and C clock (bits 2:0) */
  GPIOA->MODER &= ~(0xF00C); //clear Moder bits for bit 1,6, and 7 of GPIOA
  GPIOA ->MODER |= 0xA000; //set PA1 as an input and PA6 and PA7 as alternate function
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
  EXTI->FTSR |= 0x002; //set PA[1] IRQ# interrupt to falling edge triggered
	EXTI->RTSR |= 0x80;
  EXTI->IMR |= 0x0002; //Enable EXTI1
  NVIC_EnableIRQ(EXTI1_IRQn);  //ENABLE EXTI interrupts
  NVIC_ClearPendingIRQ(EXTI1_IRQn); //Clear EXTI1 pending flag
  //timer interrupt STUFF
  RCC->APB2ENR|= RCC_APB2ENR_TIM10EN; // enable clock source
  RCC->CR |= RCC_CR_HSION; // Turn on 16MHz HSI oscillator
  while ((RCC->CR & RCC_CR_HSIRDY) == 0); // Wait until HSI ready
  RCC->CFGR |= RCC_CFGR_SW_HSI; // Select HSI as system clock
  //TIMER 10 STUFF
  //TIM10->DIER |= TIM_DIER_UIE; //optional, same as next instruction
  TIM10->PSC= 0; //prescale = 1
  //set ARR to 1599 to get 1kHz period
  TIM10->ARR = 16000; //Auto reload register. how many PSC intervals before tick (= ARR + 1).
  TIM10->DIER |= 0x11; //enable timer 10 interrupts and capture/compare interrupts
  //PWM Setup
  GPIOA->AFR[0] &= ~(0xFF000000);
  GPIOA->AFR[0] |= 0x33000000; //PA6 = AF3 (TIM10), PA7 = AF3 (TIM11)
  TIM10->CR1 &= ~(0x01);
  TIM10->CR1 |= 0x01; //Enable CCR counter
  TIM10->CCR1 &= 0x0; //SELECT DUTY CYCLE
  TIM10->CCR1 |= 0x1F40; //Default duty cycle of 50%
  TIM10->CCMR1 &= ~(0x73);
  TIM10->CCMR1 |= 0x60;  //capture/compare mode active to inactive and channel 1 as output
  TIM10->CCER &= ~(0x03);
  TIM10->CCER |= 0x01; //enable output OC1 to drive pin and define OC1 as active high
  NVIC_EnableIRQ(TIM10_IRQn); //ENABLE timer interrupts
  NVIC_ClearPendingIRQ(TIM10_IRQn);
  //TIMER 11 STUFF
  RCC->APB2ENR |= RCC_APB2ENR_TIM11EN;//enable timer 11 clock
  TIM11->PSC = 59;
  TIM11->ARR = 0xFFFF;
  TIM11->CNT = 0;
  TIM11->CCMR1 &= 0x0;
  TIM11->CCMR1 |= 0xF1; //samples 1, prescale 0 (capture on every event), input IC1 = T11 selected. if need more samples, increase bits [7:4]; channel 1 selected (bits 7:0)
  TIM11->CCER &= ~(0x11);	/* enable capture on Rising edge of input */
  TIM11->CCER |= 0x01; //enable capture cc1 for rising edge and cc2 for falling edge; IF SOMETHING BREAKS, CHECK THIS!!!!!!!!!!!!!!!!!!
  GPIOA->PUPDR &= ~(0xC000);//configure pullup resistor for comparator output
  GPIOA->PUPDR |= 0x00004000;
  TIM11->CCR1 &= ~(0x00000001); //duty cycle set to zero  											
  TIM11->DIER |= 0x3; //enable timer 11 interrupts and capture/compare interrupts
	TIM11->CR1 &= ~(0x01);
  TIM11->CR1 |= 0x01; //Enable CCR counter
 
  

  //***extra***//
  /*step 1: connect channel TIM11->CH1 //pin pa7
  step 2: activate capture compare: TIM11->CCR1
  step 3: reset TIM11->CNT to zero after each capture event (somehow) TIM11->CNT = 0;
  step 4: set ARR to value higher than expected period. TIM11->ARR = ?
  step 5: set MODER pin PA7 to alternate function: GPIOA->MODER |= 0x00008000;
  step 6: In the GPIO AF register, select TIMx_Chy: GPIOA->AFR[0] |= 0x30000000;
  step 7: Configure GPIO PUPDR register if pull-up or pull-down desired: GPIOA->PUPDR &= ~(0x0000C000); GPIOA->PUPDR |= 0x00004000; 
  step 8: set up PSC, ARR, DIER, CR1 to 1, and don't forget to clear the SR bit after interrupt handler.
  step 9: enable CCR counter with CR1 = 0x01
  step 10: select channel 1 with input and default settings for CCMR1. TIM11->CCMR1 |= 0x1;  */
  
  NVIC_EnableIRQ(TIM11_IRQn); //ENABLE timer interrupts
  NVIC_ClearPendingIRQ(TIM11_IRQn);

  __enable_irq(); //enable CPU interrupts
}

void TIM10_IRQHandler() {
  

  //clear UIFlag and capture/compare interrupt flag
  TIM10->SR &= ~(0x02);
	//TIM10->SR |= 0x00;
	NVIC_ClearPendingIRQ(TIM10_IRQn);
}

void TIM11_IRQHandler() {
	if(second == 0x0) {
		//interval = 1;
		TIM11->CNT = 0x0;
		//interval = TIM11->CNT;
		second = 0x1;
	}
	else {
		count = TIM11->CNT;
		TIM11->CNT = 0; //may be redundant
		//period = ???;
		second = 0x0;
	}
	TIM11->SR &= ~(0x03);
	NVIC_ClearPendingIRQ(TIM11_IRQn);
	
	/*
	1. reset count to 0;
	2. on first rising edge, start count
	3. on second rising edge, pass count value to variable 'counted'
	4. then reset count to 0
	5. use 'counted' in formula to calculate period
	Solve the wind speed
	Vwind = K÷(Crising_2 – Crising_1)×Freq_cnt
	Or, if count reset to 0 on each rising edge:
	Vwind = K÷(Crising_2)×Freq_cnt
	//what is Freq_cnt?
	//remember you have cc2 set to activate on falling edge. What do i do with that?
	*/
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


  //get the dutyCycle
	dutyCycle = dutyArray[key];
  //change the duty cycle
  TIM10->CCR1 = 16000 * dutyCycle;
  //display the duty cycle on PC[3:0]
  GPIOC->ODR &= ~(0x0F);
  GPIOC->ODR |= key & 0x000F;

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
  //wait for interrupt
  }
} /* repeat forever */
