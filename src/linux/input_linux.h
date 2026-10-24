#ifndef ORANGE_GAMATE_INPUT_LINUX_H
#define ORANGE_GAMATE_INPUT_LINUX_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * evdev keyboards and game controllers (/dev/input/event*), the Linux
 * counterpart of the PS/2 keyboard, USB HID and NES/SNES gamepad drivers of
 * the RP2350 build.  Keyboards are grabbed exclusively so key presses do not
 * also reach the text console underneath the KMS output.  Devices are
 * hot-plugged by periodic rescans.
 */

void input_init(void);
void input_shutdown(void);

/* Rescan for new devices (rate-limited) and read pending events.  Keyboard
 * events are passed to on_key (Linux input code; value 1 = press,
 * 2 = autorepeat, 0 = release) in the order they arrived. */
void input_poll(void (*on_key)(int code, int value));

/* Current state of all game controllers, ORed together, as nespad_state
 * bits (DPAD_* in nespad.h). */
uint32_t input_gamepad_state(void);

#ifdef __cplusplus
}
#endif

#endif /* ORANGE_GAMATE_INPUT_LINUX_H */
