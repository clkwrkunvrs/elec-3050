/*====================================================*/
/* Victor P. Nelson */
/* ELEC 3040/3050 - Lab 1, Program 1 */
/* Toggle LED1 while button pressed, with short delay inserted */
/*====================================================*/
#include "STM32L1xx.h" /* Microcontroller information */
/* Define global variables */
static uint8_t count1 = 0,count2 = 0; /* determines where in the count the program is */
static int direction = 0;
static uint16_t downLED = 1,upLED = 0;
/*---------------------------------------------------*/
/* Initialize GPIO pins used in the program */
/* PA0 = push button */
/* PC8 = blue LED, PC9 = green LED */
/*---------------------------------------------------*/
void PinSetup () {
 /* Configure PA0 as input pin to read push button */
 RCC->AHBENR |= 0x01; /* Enable GPIOA clock (bit 0) */
 GPIOA->MODER &= ~(0x000003); /*Clear PA[0] Mode bits (extraneous in this case) */	
 //Do I configure this as a GPIO AND an EXTI???
 GPIOA->MODER &= ~(0x00000003); /* General purpose input mode for PA1 AND PA0 */
 RCC->AHBENR |= 0x04; /* Enable GPIOC clock (bit 2) */
 GPIOC->MODER &= ~(0x0FFFFF); /*Clear PC[9:0] Mode bits*/
 GPIOC->MODER |= (0x00055555); /* General purpose output mode FOR PC[9:0]*/
 SYSCFG->EXTICR[0] &= 0XFF00; //clear EXTI1 and EXTI0 bit fields
 SYSCFG->EXTICR[0] |= 0x0000; //set EXTI0 and EXTI1 to select PA0 and PA1	
 EXTI->RTSR |= 0x0003; //Bits 0 and 1 = 1 to make EXTI1 and EXTI0 rising-edge trig
 EXTI->IMR |= 0x0003; //Enable EXTI1 and EXTI0
 //EXTI->PR |= 0x0003;	//Clear EXTI1 and EXTI0 pending status (this goes in interrupt handler)
 NVIC_EnableIRQ(EXTI0_IRQn);
 NVIC_EnableIRQ(EXTI1_IRQn);
 NVIC_ClearPendingIRQ(EXTI0_IRQn);
 NVIC_ClearPendingIRQ(EXTI1_IRQn);	
 __enable_irq(); //enable CPU interrupts
	//using default IRQ priorities
}

/* Function to count up and down from 0 to 9 and repeat */
void counting1(void) {
	//COUNT1 done
	count1++;
	if(count1 == 0x0) count1 = 9;
	else if(count1 == 0xA) count1 = 0;
}
	void counting2(void) {
		//COUNT2
	if((~direction) & (count2 == 0x0)) count2 = 9;
	else if((direction) && (count2 == 9)) count2 = 0;
	else direction ? count2++ : count2--; 
	}
/*----------------------------------------------------------*/
/* Delay function - do nothing for about 1/2 second */
/*----------------------------------------------------------*/
void delay () {
 int i,j,n;
 for (i=0; i<20; i++) { //outer loop
 for (j=0; j<9500; j++) { //inner loop
 n = j; //dummy operation for single-step test
	} //do nothing
 }
}


void EXTI0_IRQHandler(void) {
//set pending flag
	EXTI->PR |= 0x0003;	//Clear EXTI1 and EXTI0 pending status (this goes in interrupt handler)
	//NVIC_ClearPendingIRQ(EXTI0_IRQn);	
	//change direction
	direction = 0;
	//change LED
	downLED = 1;
	upLED = 0;
	
	}

void EXTI1_IRQHandler(void) {
//set pending flag
  EXTI->PR |= 0x0003;	//Clear EXTI1 and EXTI0 pending status (this goes in interrupt handler)
	//NVIC_ClearPendingIRQ(EXTI1_IRQn);
	//change direction
	direction = 1;
	//change LED
	downLED = 0;
	upLED = 1;
	}
/*------------------------------------------------*/
/* Main program */
/*------------------------------------------------*/
int main(void) {
 int twice = 0;
 PinSetup(); //Configure GPIO pins
 
 /* Endless loop */
 while (1) { //Can also use: for(;;) {
	 			counting1();
			//make 2nd counter count at 1/2 the speed of the 1st
			if(twice){
				counting2();
				twice = 0;
			}
			else twice = 1;
			//this might blow up because count is only 8 bits and ODR is 16; hence casting it as a short
			GPIOC->ODR = (upLED<<9 | downLED<<8 | (((uint16_t)count2)<<4) |(uint16_t)count1) & 0x03FF; //AND the count bits with 0x00FF to get binary value of count1 and count2
			//alternative could be a loop with (count >> k) & 1. Also, don't care about upper ODR bits for this project
		  delay(); //1/2 second delay to count to next number
 } /* repeat forever */
} 