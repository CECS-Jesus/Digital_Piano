# Digital Piano Project on TM4C-123 MCU

## Overview

This project aims to create a digital piano using the TM4C-123 microcontroller. The project uses various hardware components and software functions to achieve this. The piano has different modes like "Piano Mode" and "Auto-Play Mode" which can be toggled using onboard switches.

## Features

- 6-bit DAC for sound generation.
- Piano keys mapped to GPIO pins.
- Different modes for manual and auto-playing.
- Interrupt-based architecture for real-time performance.

## Components

### Hardware

- TM4C-123 microcontroller
- 6-bit DAC
- Onboard switches and LEDs

### Software

#### ButtonLed

Responsible for initializing onboard switches and LEDs. It also controls the current mode of the piano.

- `ButtonLed_Init()`: Initialize the switches and LEDs.
- `get_current_mode()`: Get the current mode of operation.

#### Sound

Handles all the sound-related functionalities like initializing the DAC, playing notes, and songs.

- `Sound_Init()`: Initialize the sound system.
- `Sound_Start()`: Start playing a note.
- `Sound_stop()`: Stop playing a note.

#### Project1P2

Main file where all initializations and main loop reside.

- `main()`: Main function where the program starts.

#### startup

Assembly file for low-level initializations and configurations.

### Utilities

- `DisableInterrupts()`: Disable all interrupts.
- `EnableInterrupts()`: Enable all interrupts.
- `WaitForInterrupt()`: Go to low power mode while waiting for the next interrupt.

## How to Run

1. Compile all the source and header files.
2. Flash the compiled program onto the TM4C-123 microcontroller.
3. Interact with the piano using the onboard switches to change modes and play notes.

## Author

Jesus Perez, Kevin Martinez, and Shane Lobslinger

## License

This project is licensed under the MIT License.

