// Sound.c
// This is the starter file for CECS 447 Project 1 Part 2
// By Dr. Min He
// September 10, 2022
// Port B bits 5-0 outputs to the 6-bit DAC
// Port D bits 3-0 inputs from piano keys: CDEF:doe ray mi fa,negative logic connections.
// Port F is onboard LaunchPad switches and LED
// SysTick ISR: PF3 is used to implement heartbeat

#include "tm4c123gh6pm.h"
#include "sound.h"
#include <stdint.h>
#include <stdbool.h>
#include "ButtonLed.h"

// define bit addresses for Port B bits 0,1,2,3,4,5 => DAC inputs 
#define DAC (*((volatile unsigned long *)0x400051FC)) //6 bits or 0x400053FC for 8 bits (use later)
#define SW1 0x10  // bit position for onboard switch 1(left switch)
#define SW2 0x01  // bit position for onboard switch 2(right switch)	
// 6-bit: value range 0 to 2^6-1=63, 64 samples
const uint8_t SineWave[64] = {32,35,38,41,44,47,49,52,54,56,58,59,61,62,62,63,63,63,62,62,
															61,59,58,56,54,52,49,47,44,41,38,35,32,29,26,23,20,17,15,12,
															10, 8, 6, 5, 3, 2, 2, 1, 1, 1, 2, 2, 3, 5, 6, 8,10,12,15,17,
															20,23,26,29};

// initial values for piano major tones.
// Assume SysTick clock frequency is 16MHz.
const uint32_t Tone_Tab[] =
// C, D, E, F, G, A, B
// 1, 2, 3, 4, 5, 6, 7
// lower C octave:130.813, 146.832,164.814,174.614,195.998, 220,246.942
// calculate reload value for the whole period:Reload value = Fclk/Ft = 16MHz/Ft
{122137,108844,96970,91429,81633,72727,64777,							// C3
 30534*2,27211*2,24242*2,22923*2,20408*2,18182*2,16194*2, // C4 Major notes
 15289*2,13621*2,12135*2,11454*2,10204*2,9091*2,8099*2,   // C5 Major notes (piano mode c3-c5), happy birthday will need c3-c6
 7645*2,6810*2,6067*2,5727*2,5102*2,4545*2,4050*2};        // C6 Major notes

// Constants
// index definition for tones used in happy birthday.
//#define G4 4 //change to enumeration definition 28 note names
//#define A4 5
//#define B4 6
//#define C5 0+7
//#define D5 1+7
//#define E5 2+7
//#define F5 3+7
//#define G5 4+7
//#define A5 5+7		

// Index for notes used in music scores
#define C6 0+2*7
#define D6 1+2*7
#define E6 2+2*7
#define F6 3+2*7
#define G6 4+2*7

enum note_names{C4, D4, E4, F4, G4, A4, B4, C5, D5, E5, F5, G5, A5, B5};

#define PAUSE 255
#define MAX_NOTES 255 // maximum number of notes for a song to be played in the program
#define NUM_SONGS 3   // number of songs in the play list.
#define SILENCE MAX_NOTES // use the last valid index to indicate a silence note. The song can only have up to 254 notes. 
#define NUM_OCT  3   // number of octave defined in tontab[]
#define NUM_NOTES_PER_OCT 7  // number of notes defined for each octave in tonetab
#define NVIC_EN0_PORTF 0x40000000  // enable PORTF edge interrupt
#define NVIC_EN0_PORTD 0x00000008  // enable PORTD edge interrupt
#define NUM_SAMPLES 64  // number of sample in one sin wave period

unsigned char Index;

const NTyp MaryHadALittleLamb[] = 
{E4, 4, D4, 4, C4, 4, D4, 4, E4, 4, E4, 4, E4, 8, 
 D4, 4, D4, 4, D4, 8, E4, 4, G4, 4, G4, 8,
 E4, 4, D4, 4, C4, 4, D4, 4, E4, 4, E4, 4, E4, 8, 
 D4, 4, D4, 4, E4, 4, D4, 4, C4, 8, 0, 0 };

const NTyp TwinkleTwinkleLittleStars[] = {
C4,4,C4,4,G4,4,G4,4,A4,4,A4,4,G4,8,F4,4,F4,4,E4,4,E4,4,D4,4,D4,4,C4,8, 
G4,4,G4,4,F4,4,F4,4,E4,4,E4,4,D4,8,G4,4,G4,4,F4,4,F4,4,E4,4,E4,4,D4,8, 
C4,4,C4,4,G4,4,G4,4,A4,4,A4,4,G4,8,F4,4,F4,4,E4,4,E4,4,D4,4,D4,4,C4,8,0,0};

// note table for Happy Birthday
// doe ray mi fa so la ti 
// C   D   E  F  G  A  B
const NTyp happybirthday[] = {
	G5,2,G5,2,A5,4,G5,4,C6,4,B5,4,
	// pause so   so   la   so   ray' doe'
	PAUSE,4,  G5,2,G5,2,A5,4,G5,4,D6,4,C6,4,
	// pause so   so   so'  mi'  doe' ti   la
	PAUSE,4,  G5,2,G5,2,G6,4,E6,4,C6,4,B5,4,A5,8, 
	// pause fa'  fa'   mi'  doe' ray' doe' stop
	PAUSE,4,  F6,2,F6,2, E6,4,C6,4,D6,4,C6,8,0,0
};

// File scope golbal
volatile uint8_t curr_song=0;      // 0: Mary Had A Little Lamb. 1: Twinkle Twinkle Little Stars, 2: Happy Birthday.
volatile uint8_t stop_play=1;      // 0: continue playing a song, 1: stop playing a song
volatile uint8_t octave=0;         // 0: lower C, 1: middle C, 2: upper C

																		// **************DAC_Init*********************
// Initialize 6-bit DAC 
// Input: none
// Output: none
void DAC_Init(void){unsigned long volatile delay;
  SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOB; // activate port B
  delay = SYSCTL_RCGC2_R;    // allow time to finish activating
  GPIO_PORTB_AMSEL_R &= ~0x3F;      // no analog 
  GPIO_PORTB_PCTL_R &= ~0x00FFFFFF; // regular function
  GPIO_PORTB_DIR_R |= 0x3F;      // make PB5-0 out
  GPIO_PORTB_AFSEL_R &= ~0x3F;   // disable alt funct on PB5-0
  GPIO_PORTB_DEN_R |= 0x3F;      // enable digital I/O on PB5-0
  GPIO_PORTB_DR8R_R |= 0x3F;        // enable 8 mA drive on PB5-0
}

// **************Sound_Start*********************
// Set reload value and enable systick timer
// Input: time duration to be generated in number of machine cycles
// Output: none


void Sound_Start(uint32_t period){
  NVIC_ST_RELOAD_R = period-1;// reload value maybe 2x
  NVIC_ST_CURRENT_R = 0;      // any write to current clears it
	NVIC_ST_CTRL_R |= NVIC_ST_CTRL_ENABLE;
}

void Sound_stop(void)
{
	NVIC_ST_CTRL_R &= ~NVIC_ST_CTRL_ENABLE;
}

// Interrupt service routine
// Executed based on number of sampels in each period
void SysTick_Handler(void){
	GPIO_PORTF_DATA_R ^= 0x08;     // toggle PF3, debugging
  Index = (Index+1)&0x3F;        // 64 samples for each period
	DAC = SineWave[Index]; // output to DAC: 6-bit data
}

void GPIOPortF_Handler(void){
	// simple debouncing code: generate 20ms to 30ms delay
	for (uint32_t time=0;time<72724;time++) {}
		if(GPIO_PORTF_RIS_R & SW1) { //if sw1 pressed
			if(curr_mode == 0){
				curr_mode = curr_mode + 1;
				GPIO_PORTF_ICR_R = SW1; //ackowledge flag for sw1
			}
			if(curr_mode == 1){
				curr_mode = curr_mode - 1;
				GPIO_PORTF_ICR_R = SW1; //ackowledge flag for sw1
			}	
		}
			if(GPIO_PORTF_RIS_R & SW2){ 	 //if sw2 pressed
				if(curr_mode == 0){ 				 //piano mode
					octave = (octave + 1) % 3; // Increment the octave
					GPIO_PORTF_ICR_R = SW2; 	 // acknowledge flag for SW2
				}
	
				if(curr_mode == 1){ 								//autoplay mode
					curr_song =  (curr_song + 1) % 3;	// Increment the song
					GPIO_PORTF_ICR_R = SW2; 					// acknowledge flag for SW2
				}		
			}
}


//write isr for handlers
// Subroutine to wait 0.1 sec
// Inputs: None
// Outputs: None
// Notes: ...
void Delay(void){
	uint32_t volatile time;
  time = 727240*20/91;  // 0.1sec
  while(time){
		time--;
  }
}


void play_a_song()
{
	
}

