// ButtonLed.c: starter file for CECS447 Project 1 Part 1
// Runs on TM4C123, 
// Dr. Min He
// September 10, 2022
// Port B bits 5-0 outputs to the 6-bit DAC
// Port D bits 3-0 inputs from piano keys: CDEF:doe ray mi fa,negative logic connections.
// Port F is onboard LaunchPad switches and LED
// SysTick ISR: PF3 is used to implement heartbeat

#include "tm4c123gh6pm.h"
#include <stdint.h>
#include "ButtonLed.h"

// Constants
#define SW1 0x10  // bit position for onboard switch 1(left switch)
#define SW2 0x01  // bit position for onboard switch 2(right switch)
#define NVIC_EN0_PORTF 0x40000000  // enable PORTF edge interrupt
#define NVIC_EN0_PORTD 0x00000008  // enable PORTD edge interrupt

// Golbals
volatile uint8_t curr_mode=PIANO;  // 0: piano mode, 1: auto-play mode
volatile uint8_t major_idx = 0;
volatile uint8_t note_C = 0, note_D = 0, note_E = 0, note_F = 0;

//---------------------Switch_Init---------------------
// initialize onboard switch and LED interface
// Input: none
// Output: none 

void ButtonLed_Init(void) {
	SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOF; // activate port F
	while ((SYSCTL_RCGC2_R&SYSCTL_RCGC2_GPIOF)!=SYSCTL_RCGC2_GPIOF);

	GPIO_PORTF_LOCK_R = GPIO_LOCK_KEY;	// unlock GPIO Port F
	GPIO_PORTF_CR_R |= 0x1F;			// allow changes to PF4-0
	GPIO_PORTF_AMSEL_R &= ~0x1F;		// disable analog on PF4-0
	GPIO_PORTF_PCTL_R &= ~0x000FFFFF;	// PCTL GPIO on PF4-0
	GPIO_PORTF_DIR_R &= ~0x11;			// PF4,PF0 in, PF3-1 out
	GPIO_PORTF_AFSEL_R &= ~0x1F;		// disable alt funct on PF4-0
	GPIO_PORTF_PUR_R |= 0x11;			// enable pull-up on PF0 and PF4
	GPIO_PORTF_DEN_R |= 0x1F;			// enable digital I/O on PF4-0
	GPIO_PORTF_IS_R &= ~0x11;			// PF4,PF0 is edge-sensitive
	GPIO_PORTF_IBE_R &= ~0x11;			// PF4,PF0 is not both edges
	GPIO_PORTF_IEV_R &= ~0x11;			// PF4,PF0 falling edge event
	GPIO_PORTF_ICR_R = 0x11;			// clear flag0,4
	GPIO_PORTF_IM_R |= 0x11;			// enable interrupt on PF0,PF4
	NVIC_PRI7_R = (NVIC_PRI7_R&0xFF1FFFFF)|0x00400000; // (g) bits:23-21 for PORTF, set priority to 2
	NVIC_EN0_R |= 0x40000000;			// (h) enable interrupt 30 in NVIC
}
//---------------------PianoKeys_Init---------------------
// initialize onboard Piano keys interface: PORT D 0 - 3 are used to generate 
// tones: CDEF:doe ray mi fa
// No need to unlock. Only PD7 needs to be unlocked.
// Input: none
// Output: none 
void PianoKeys_Init(void){
	SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOD; // activate port D
	while ((SYSCTL_RCGC2_R&SYSCTL_RCGC2_GPIOD)!=SYSCTL_RCGC2_GPIOD);

	GPIO_PORTD_LOCK_R = GPIO_LOCK_KEY;	// unlock GPIO Port D
	GPIO_PORTD_CR_R |= 0x1F;			// allow changes to PD3-0
	GPIO_PORTD_AMSEL_R &= ~0x1F;		// disable analog on PD3-0
	GPIO_PORTD_PCTL_R &= ~0x000FFFFF;	// PCTL GPIO on PD4-0
	GPIO_PORTD_DIR_R &= ~0x1F;			// PD4,PD0 in, PD3-1 out
	GPIO_PORTD_AFSEL_R &= ~0x1F;		// disable alt funct on PD4-0
	GPIO_PORTD_PUR_R |= 0x1F;			// enable pull-up on PF0 and PD4
	GPIO_PORTD_DEN_R |= 0x1F;			// enable digital I/O on PD4-0
	GPIO_PORTD_IS_R &= ~0x1F;			// PD4,PD0 is edge-sensitive
	GPIO_PORTD_IBE_R &= ~0x1F;			// PD4,PD0 is not both edges
	GPIO_PORTD_IEV_R &= ~0x1F;			// PD4,PD0 falling edge event
	GPIO_PORTD_ICR_R = 0x1F;			// clear flag0,4
	GPIO_PORTD_IM_R |= 0x1F;			// enable interrupt on PD0,PD4
	NVIC_PRI0_R = (NVIC_PRI0_R&0x1FFFFFFF)|0x80000000; // PORTD priority bits 31-29, set priority to 3
	NVIC_EN0_R |= 0x00000008; 			// (h) enable interrupt in NVIC
}


uint8_t get_current_mode(void)
{
	return curr_mode;
}

uint8_t play_note_C(void){
	return note_C;
}

uint8_t play_note_D(void){
	return note_D;
}

uint8_t play_note_E(void){
	return note_E;
}

uint8_t play_note_F(void){
	return note_F;
}



// Dependency: Requires PianoKeys_Init to be called first, assume at any time only one key is pressed
// Inputs: None
// Outputs: None
// Description: Rising/Falling edge interrupt on PD4-PD0. Whenever any 
// button is pressed, or released the interrupt will trigger.
void GPIOPortD_Handler(void){  
  // simple debouncing code: generate 20ms to 30ms delay
	for (uint32_t time=0;time<72724;time++) {}
		if(curr_mode == PIANO){ //if in piano mode
			
			if(GPIO_PORTD_RIS_R&0x01){	// if pd0 pressed
				note_C ^=0x01;
				GPIO_PORTD_ICR_R = 0x01;	// acknowledge flag0
			}
			if(GPIO_PORTD_RIS_R&0x02){	// if pd1 pressed
				note_D ^=0x02;
				GPIO_PORTD_ICR_R = 0x02;	// acknowledge flag1
			}
			if(GPIO_PORTD_RIS_R&0x04){	// if pd2 pressed
				note_E ^=0x04;
				GPIO_PORTD_ICR_R = 0x04;	// acknowledge flag2
			}
			if(GPIO_PORTD_RIS_R&0x08){	// if pd3 pressed
				note_F ^=0x08;
				GPIO_PORTD_ICR_R = 0x08;	// acknowledge flag3
			}			
		}	 
}
