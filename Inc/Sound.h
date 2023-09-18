// Sound.h, 
// This is the starter file for CECS 447 Project 1 Part 2
// By Dr. Min He
// September 10, 2022
// Port B bits 5-0 outputs to the 6-bit DAC
// Port D bits 3-0 inputs from piano keys: CDEF:doe ray mi fa,negative logic connections.
// Port F is onboard LaunchPad switches and LED
// SysTick ISR: PF3 is used to implement heartbeat

#include <stdint.h>

// define music note data structure 
struct Note {
  uint32_t tone_index;
  uint8_t delay;
};
typedef const struct Note NTyp;

extern volatile uint8_t octave;         // 0: lower C, 1: middle C, 2: upper C

// Constants

// **************DAC_Init*********************
// Initialize 3-bit DAC 
// Input: none
// Output: none
void DAC_Init(void);

// **************Sound_Start*********************
// Set reload value and enable systick timer
// Input: time duration to be generated in number of machine cycles
// Output: none
void Sound_Init(void);
void Sound_Start(unsigned long period);
void Sound_stop(void);
void play_a_song(void);

// Move to the next song
void next_song(void);

// check to see if the music is on or not
uint8_t is_music_on(void);

// turn off the music
void turn_off_music(void);

// turn on the music
void turn_on_music(void);

//play notes
void play_note_C(void);
void play_note_D(void);
void play_note_E(void);
void play_note_F(void);
