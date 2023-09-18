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
#define C_POSITION 0
#define D_POSITION 1
#define E_POSITION 2
#define F_POSITION 3
#define G_POSITION 4
#define A_POSITION 5
#define B_POSITION 6

unsigned char Index;
uint8_t songIdx = 0;


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
enum noteNames { 
	C3, D3, E3, F3, G3, A3, B3,
	C4, D4, E4, F4, G4, A4, B4, 
	C5, D5, E5, F5, G5, A5, B5, 
	C6, D6, E6, F6, G6, A6, B6
};

#define PAUSE 255
#define MAX_NOTES 255 // maximum number of notes for a song to be played in the program
#define NUM_SONGS 3   // number of songs in the play list.
#define NUM_OCT  3   // number of octave defined in tontab[]
#define NUM_NOTES_PER_OCT 7  // number of notes defined for each octave in tonetab
#define NVIC_EN0_PORTF 0x40000000  // enable PORTF edge interrupt
#define NVIC_EN0_PORTD 0x00000008  // enable PORTD edge interrupt
#define NUM_SAMPLES 64  // number of sample in one sin wave period

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
	G4,2,G4,2,A4,4,G4,4,C5,4,B4,4,
	// pause so   so   la   so   ray' doe'
	PAUSE,4,  G4,2,G4,2,A4,4,G4,4,D5,4,C5,4,
	// pause so   so   so'  mi'  doe' ti   la
	PAUSE,4,  G4,2,G4,2,G5,4,E5,4,C5,4,B4,4,A4,8, 
	// pause fa'  fa'   mi'  doe' ray' doe' stop
	PAUSE,4,  F5,2,F5,2, E5,4,C5,4,D5,4,C4,8,0,0
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

void Sound_Init(void){
  Index = 0;
  NVIC_ST_CTRL_R = 0;         // disable SysTick during setup
  NVIC_SYS_PRI3_R = (NVIC_SYS_PRI3_R&0x00FFFFFF)|0x20000000; // priority 1      
  NVIC_ST_CTRL_R |= NVIC_ST_CTRL_CLK_SRC|NVIC_ST_CTRL_INTEN;  // enable SysTick with core clock and interrupts
}


void Sound_Start(unsigned long period){
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
	DAC = SineWave[Index]; 		   // output to DAC: 6-bit data
}


// Move to the next song
void next_song(void) {
	songIdx = (songIdx +1) % NUM_SONGS;
}

// Move to the next song
void next_octave(void) {
	octave = (octave +1) % NUM_OCT;
}

void GPIOPortF_Handler(void){
	// simple debouncing code: generate 20ms to 30ms delay
	for (uint32_t time=0;time<72724;time++) {}
	if(GPIO_PORTF_RIS_R & SW1) { //if sw1 pressed
		if(curr_mode == PIANO){
			GPIO_PORTF_ICR_R = SW1; //ackowledge flag for sw1
			curr_mode = AUTO_PLAY;
		}
		else if(curr_mode == AUTO_PLAY){
			GPIO_PORTF_ICR_R = SW1; //ackowledge flag for sw1
			curr_mode = PIANO;
		}	
	}
	if(GPIO_PORTF_RIS_R & SW2){ 	 //if sw2 pressed
		if(curr_mode == PIANO){ 		 //piano mode
			GPIO_PORTF_ICR_R = SW2; 	 // acknowledge flag for SW2
			next_octave();
		}

		else if(curr_mode == AUTO_PLAY){ 		//autoplay mode
			GPIO_PORTF_ICR_R = SW2; 					// acknowledge flag for SW2
			next_song();
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

const NTyp* songs[NUM_SONGS] = {MaryHadALittleLamb, TwinkleTwinkleLittleStars, happybirthday};

// play the current song once
void play_a_song(void) {
	uint8_t prevIdx = songIdx;
	uint8_t i=0, j;
	const NTyp* notetab = songs[songIdx];

	while ((notetab[i].delay) && (curr_mode == AUTO_PLAY)) { // delay==0 indicate end of the song
		if (notetab[i].tone_index==PAUSE) // index = 255 indicate a pause: stop systick
			Sound_stop(); // silence tone, turn off SysTick timer
		else { // play a regular note
			Sound_Start(Tone_Tab[notetab[i].tone_index + octave*NUM_NOTES_PER_OCT]/NUM_SAMPLES);
		}
		// tempo control: 
		// play current note for specified duration
		for (j=0;j<notetab[i].delay;j++) {
			// if song changed or piano mode
			if (songIdx != prevIdx || curr_mode == PIANO) {
				Sound_stop();
				return;
			}
			Delay();
		}
		
		Sound_stop();
		i++;  // move to the next note
	}
	
	// pause after each play
	for (j=0;j<15;j++) 
		Delay();
}

void play_note_C(void){
	uint8_t c_idx = C_POSITION+ octave*NUM_NOTES_PER_OCT;
	Sound_Start(Tone_Tab[c_idx]/NUM_SAMPLES);      // Play 8 major notes
	Delay();
	Sound_stop();
}

void play_note_D(void){
	uint8_t d_idx = D_POSITION + octave*NUM_NOTES_PER_OCT;
	Sound_Start(Tone_Tab[d_idx]/NUM_SAMPLES);      // Play 8 major notes
	Delay();
	Sound_stop();
}

void play_note_E(void){
	uint8_t e_idx = E_POSITION + octave*NUM_NOTES_PER_OCT;
	Sound_Start(Tone_Tab[e_idx]/NUM_SAMPLES);      // Play 8 major notes
	Delay();
	Sound_stop();	
}

void play_note_F(void){
	uint8_t f_idx = F_POSITION + octave*NUM_NOTES_PER_OCT;
	Sound_Start(Tone_Tab[f_idx]/NUM_SAMPLES);      // Play 8 major notes
	Delay();
	Sound_stop();
}

void play_note_G(void){
	uint8_t g_idx = G_POSITION + octave*NUM_NOTES_PER_OCT;
	Sound_Start(Tone_Tab[g_idx]/NUM_SAMPLES);      // Play 8 major notes
	Delay();
	Sound_stop();
}

void play_note_A(void){
	uint8_t a_idx = A_POSITION + octave*NUM_NOTES_PER_OCT;
	Sound_Start(Tone_Tab[a_idx]/NUM_SAMPLES);      // Play 8 major notes
	Delay();
	Sound_stop();
}

void play_note_B(void){
	uint8_t b_idx = B_POSITION + octave*NUM_NOTES_PER_OCT;
	Sound_Start(Tone_Tab[b_idx]/NUM_SAMPLES);      // Play 8 major notes
	Delay();
	Sound_stop();
}