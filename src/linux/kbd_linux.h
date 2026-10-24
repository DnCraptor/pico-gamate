#pragma once

/* evdev keyboard event (Linux input code; value 1 = press, 2 = autorepeat,
 * 0 = release) -> HID report -> process_kbd_report().  Render thread only. */
void kbd_linux_event(int code, int value);
