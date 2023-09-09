#ifndef MUSIC_H
#define MUSIC_H
#include <stdint.h>

// define music note data structure 
struct Note {
	uint8_t tone_index;
	uint8_t delay;
};
typedef const struct Note NTyp;

// Constant definitions
#define NUM_SONGS 3

// Function prototypes
// play the current song once
void play_a_song(void);

// Initialize music output pin:
// Make PA3 an output to the speaker, 
// enable digital I/O, ensure alt. functions off
void Music_Init(void);

// Move to the next song
void next_song(void);

// check to see if the music is on or not
uint8_t is_music_on(void);

// turn off the music
void turn_off_music(void);

// turn on the music
void turn_on_music(void);


#endif
