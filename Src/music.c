// This is startup program for CECS447 Project 1 part 1.
// Hardware connection: 
// Positive logic Speaker/Headset is connected to PA3.
// onboard two switches are used for music box control.
// Switch 1 toggle between music on/off, switch 2 toggle through the three songs.
// Authors: Min He
// Date: August 28, 2022

#include "tm4c123gh6pm.h"
#include "music.h"
#include "SysTick.h"
#include <stdint.h>
#include <stdbool.h>

void Delay(void);

// initail values for piano major notes: assume SysTick clock is 16MHz.
const uint32_t Tone_Tab[] =
// initial values for three major notes for 16MHz system clock
// Note name: C, D, E, F, G, A, B
// Offset:0, 1, 2, 3, 4, 5, 6
{30534,27211,24242,22923,20408,18182,16194, // C4 major notes
 15289,13621,12135,11454,10204,9091,8099, // C5 major notes
 7645,6810,6067,5727,5102,4545,4050};// C6 major notes

// Index for notes used in music scores
#define C6 0+2*7
#define D6 1+2*7
#define E6 2+2*7
#define F6 3+2*7
#define G6 4+2*7

enum note_names{C4, D4, E4, F4, G4, A4, B4, C5, D5, E5, F5, G5, A5, B5};
 
#define PAUSE 255				// assume there are less than 255 tones used in any song
#define MAX_NOTES 50  // assume maximum number of notes in any song is 100. You can change this value if you add a long song.
 
const NTyp happybirthday[] = {
	G5,2,G5,2,A5,4,G5,4,C6,4,B5,4,
	// pause so   so   la   so   ray' doe'
	PAUSE,4,  G5,2,G5,2,A5,4,G5,4,D6,4,C6,4,
	// pause so   so   so'  mi'  doe' ti   la
	PAUSE,4,  G5,2,G5,2,G6,4,E6,4,C6,4,B5,4,A5,8, 
	// pause fa'  fa'   mi'  doe' ray' doe' stop
	PAUSE,4,  F6,2,F6,2, E6,4,C6,4,D6,4,C6,8,0,0
};

const NTyp MaryHadALittleLamb[] = 
{E4, 4, D4, 4, C4, 4, D4, 4, E4, 4, E4, 4, E4, 8, 
 D4, 4, D4, 4, D4, 8, E4, 4, G4, 4, G4, 8,
 E4, 4, D4, 4, C4, 4, D4, 4, E4, 4, E4, 4, E4, 8, 
 D4, 4, D4, 4, E4, 4, D4, 4, C4, 8, 0, 0 };

 const NTyp TwinkleTwinkleLittleStars[] = {
C4,4,C4,4,G4,4,G4,4,A4,4,A4,4,G4,8,F4,4,F4,4,E4,4,E4,4,D4,4,D4,4,C4,8, 
G4,4,G4,4,F4,4,F4,4,E4,4,E4,4,D4,8,G4,4,G4,4,F4,4,F4,4,E4,4,E4,4,D4,8, 
C4,4,C4,4,G4,4,G4,4,A4,4,A4,4,G4,8,F4,4,F4,4,E4,4,E4,4,D4,4,D4,4,C4,8,0,0};
 
const NTyp* songs[NUM_SONGS] = {happybirthday, MaryHadALittleLamb, TwinkleTwinkleLittleStars};
uint8_t songIdx = 0;

bool play = false;

// play the current song once
void play_a_song(void) {
	uint8_t prevIdx = songIdx;
	uint8_t i=0, j;
	const NTyp* notetab = songs[songIdx];
	

	while (notetab[i].delay) { // delay==0 indicate end of the song
		if (notetab[i].tone_index==PAUSE) // index = 255 indicate a pause: stop systick
			SysTick_stop(); // silence tone, turn off SysTick timer
		else { // play a regular note
			SysTick_Set_Current_Note(Tone_Tab[notetab[i].tone_index]);
			SysTick_start();
		}
		
		// tempo control: 
		// play current note for specified duration
		for (j=0;j<notetab[i].delay;j++) {
			// if song changed or power off stop playing song
			if (songIdx != prevIdx || !is_music_on()) {
				SysTick_stop();
				return;
			}
			Delay();
		}
		
		SysTick_stop();
		i++;  // move to the next note
	}
	
	// pause after each play
	for (j=0;j<15;j++) 
		Delay();
}

// Move to the next song
void next_song(void) {
	if (0 <= songIdx && songIdx <= NUM_SONGS-2)
		songIdx++;
	else
		songIdx = 0;
}

// check to see if the music is on or not
uint8_t is_music_on(void) {
	return play;
}

// turn off the music
void turn_off_music(void) {
	play = false;
	songIdx = 0;
}

// turn on the music
void turn_on_music(void) {
	play = true;
}

// Initialize music output pin:
// Make PA3 an output to the speaker, 
// enable digital I/O, ensure alt. functions off
void Music_Init(void) {
	volatile uint32_t delay;
	SYSCTL_RCGC2_R |= 0x01;				// 1) activate clock for Port A
	delay = SYSCTL_RCGC2_R;				// allow time for clock to start
										// 2) no need to unlock PA3
	GPIO_PORTA_PCTL_R &= ~0x00000F00;	// 3) regular GPIO
	GPIO_PORTA_AMSEL_R &= ~0x08;		// 4) disable analog function on PA3
	GPIO_PORTA_DIR_R |= 0x08;			// 5) set direction to output
	GPIO_PORTA_AFSEL_R &= ~0x08;		// 6) regular port function
	GPIO_PORTA_DEN_R |= 0x08;			// 7) enable digital port
	GPIO_PORTA_DR8R_R |= 0x08;			// 8) optional: enable 8 mA drive on PA3 to increase the voice volumn
}

// Subroutine to wait 0.1 sec
// Inputs: None
// Outputs: None
// Notes: ...
void Delay(void){
	uint32_t volatile time;
	time = 727240*20/91;  // 0.1sec for 16MHz
	//  time = 727240*100/91;  // 0.1sec for 80MHz
	while(time){
		time--;
	}
}
