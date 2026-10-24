#pragma once

/*
 * Linux counterpart of drivers/nespad/nespad.h: the gamepad state the
 * emulator reads is the combined state of the evdev game controllers
 * (input_linux.c) in the NES/SNES pad bit layout.
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DPAD_LEFT   0x001000
#define DPAD_RIGHT  0x004000
#define DPAD_DOWN   0x000400
#define DPAD_UP     0x000100
#define DPAD_START  0x000040
#define DPAD_SELECT 0x000010
#define DPAD_B      0x000004   //Y on SNES
#define DPAD_A      0x000001   //B on SNES

extern uint32_t nespad_state;  // (S)NES Joystick1

/* Latch the current controller state into nespad_state. */
void nespad_read(void);

#ifdef __cplusplus
}
#endif
