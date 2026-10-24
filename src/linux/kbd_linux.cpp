/*
 * evdev key events -> HID boot keyboard reports, the Linux counterpart of
 * drivers/ps2kbd (Ps2Kbd_Mrmltr): every press or release updates one report
 * (modifier keys appear both in the modifier byte and in keycode[], as the
 * PS/2 driver puts them) and calls process_kbd_report(current, previous).
 */
#include "kbd_linux.h"
#include "hid_keys.h"

#include <linux/input.h>

void process_kbd_report(hid_keyboard_report_t const* report, hid_keyboard_report_t const* prev_report);

static hid_keyboard_report_t report = {};

static uint8_t linux_to_hid(int code) {
    switch (code) {
        case KEY_A: return HID_KEY_A;
        case KEY_B: return HID_KEY_B;
        case KEY_C: return HID_KEY_C;
        case KEY_D: return HID_KEY_D;
        case KEY_E: return HID_KEY_E;
        case KEY_F: return HID_KEY_F;
        case KEY_G: return HID_KEY_G;
        case KEY_H: return HID_KEY_H;
        case KEY_I: return HID_KEY_I;
        case KEY_J: return HID_KEY_J;
        case KEY_K: return HID_KEY_K;
        case KEY_L: return HID_KEY_L;
        case KEY_M: return HID_KEY_M;
        case KEY_N: return HID_KEY_N;
        case KEY_O: return HID_KEY_O;
        case KEY_P: return HID_KEY_P;
        case KEY_Q: return HID_KEY_Q;
        case KEY_R: return HID_KEY_R;
        case KEY_S: return HID_KEY_S;
        case KEY_T: return HID_KEY_T;
        case KEY_U: return HID_KEY_U;
        case KEY_V: return HID_KEY_V;
        case KEY_W: return HID_KEY_W;
        case KEY_X: return HID_KEY_X;
        case KEY_Y: return HID_KEY_Y;
        case KEY_Z: return HID_KEY_Z;
        case KEY_1: return HID_KEY_1;
        case KEY_2: return HID_KEY_2;
        case KEY_3: return HID_KEY_3;
        case KEY_4: return HID_KEY_4;
        case KEY_5: return HID_KEY_5;
        case KEY_6: return HID_KEY_6;
        case KEY_7: return HID_KEY_7;
        case KEY_8: return HID_KEY_8;
        case KEY_9: return HID_KEY_9;
        case KEY_0: return HID_KEY_0;
        case KEY_ENTER: return HID_KEY_ENTER;
        case KEY_ESC: return HID_KEY_ESCAPE;
        case KEY_BACKSPACE: return HID_KEY_BACKSPACE;
        case KEY_TAB: return HID_KEY_TAB;
        case KEY_SPACE: return HID_KEY_SPACE;
        case KEY_MINUS: return HID_KEY_MINUS;
        case KEY_EQUAL: return HID_KEY_EQUAL;
        case KEY_LEFTBRACE: return HID_KEY_BRACKET_LEFT;
        case KEY_RIGHTBRACE: return HID_KEY_BRACKET_RIGHT;
        case KEY_BACKSLASH: return HID_KEY_BACKSLASH;
        case KEY_SEMICOLON: return HID_KEY_SEMICOLON;
        case KEY_APOSTROPHE: return HID_KEY_APOSTROPHE;
        case KEY_GRAVE: return HID_KEY_GRAVE;
        case KEY_COMMA: return HID_KEY_COMMA;
        case KEY_DOT: return HID_KEY_PERIOD;
        case KEY_SLASH: return HID_KEY_SLASH;
        case KEY_CAPSLOCK: return HID_KEY_CAPS_LOCK;
        case KEY_F1: return HID_KEY_F1;
        case KEY_F2: return HID_KEY_F2;
        case KEY_F3: return HID_KEY_F3;
        case KEY_F4: return HID_KEY_F4;
        case KEY_F5: return HID_KEY_F5;
        case KEY_F6: return HID_KEY_F6;
        case KEY_F7: return HID_KEY_F7;
        case KEY_F8: return HID_KEY_F8;
        case KEY_F9: return HID_KEY_F9;
        case KEY_F10: return HID_KEY_F10;
        case KEY_F11: return HID_KEY_F11;
        case KEY_F12: return HID_KEY_F12;
        case KEY_SYSRQ: return HID_KEY_PRINT_SCREEN;
        case KEY_SCROLLLOCK: return HID_KEY_SCROLL_LOCK;
        case KEY_PAUSE: return HID_KEY_PAUSE;
        case KEY_INSERT: return HID_KEY_INSERT;
        case KEY_HOME: return HID_KEY_HOME;
        case KEY_PAGEUP: return HID_KEY_PAGE_UP;
        case KEY_DELETE: return HID_KEY_DELETE;
        case KEY_END: return HID_KEY_END;
        case KEY_PAGEDOWN: return HID_KEY_PAGE_DOWN;
        case KEY_RIGHT: return HID_KEY_ARROW_RIGHT;
        case KEY_LEFT: return HID_KEY_ARROW_LEFT;
        case KEY_DOWN: return HID_KEY_ARROW_DOWN;
        case KEY_UP: return HID_KEY_ARROW_UP;
        case KEY_NUMLOCK: return HID_KEY_NUM_LOCK;
        case KEY_KPSLASH: return HID_KEY_KEYPAD_DIVIDE;
        case KEY_KPASTERISK: return HID_KEY_KEYPAD_MULTIPLY;
        case KEY_KPMINUS: return HID_KEY_KEYPAD_SUBTRACT;
        case KEY_KPPLUS: return HID_KEY_KEYPAD_ADD;
        case KEY_KPENTER: return HID_KEY_KEYPAD_ENTER;
        case KEY_KP1: return HID_KEY_KEYPAD_1;
        case KEY_KP2: return HID_KEY_KEYPAD_2;
        case KEY_KP3: return HID_KEY_KEYPAD_3;
        case KEY_KP4: return HID_KEY_KEYPAD_4;
        case KEY_KP5: return HID_KEY_KEYPAD_5;
        case KEY_KP6: return HID_KEY_KEYPAD_6;
        case KEY_KP7: return HID_KEY_KEYPAD_7;
        case KEY_KP8: return HID_KEY_KEYPAD_8;
        case KEY_KP9: return HID_KEY_KEYPAD_9;
        case KEY_KP0: return HID_KEY_KEYPAD_0;
        case KEY_KPDOT: return HID_KEY_KEYPAD_DECIMAL;
        case KEY_LEFTCTRL: return HID_KEY_CONTROL_LEFT;
        case KEY_LEFTSHIFT: return HID_KEY_SHIFT_LEFT;
        case KEY_LEFTALT: return HID_KEY_ALT_LEFT;
        case KEY_LEFTMETA: return HID_KEY_GUI_LEFT;
        case KEY_RIGHTCTRL: return HID_KEY_CONTROL_RIGHT;
        case KEY_RIGHTSHIFT: return HID_KEY_SHIFT_RIGHT;
        case KEY_RIGHTALT: return HID_KEY_ALT_RIGHT;
        case KEY_RIGHTMETA: return HID_KEY_GUI_RIGHT;
        default: return HID_KEY_NONE;
    }
}

static uint8_t hid_key_to_modifier(uint8_t hid) {
    switch (hid) {
        case HID_KEY_CONTROL_LEFT: return KEYBOARD_MODIFIER_LEFTCTRL;
        case HID_KEY_SHIFT_LEFT: return KEYBOARD_MODIFIER_LEFTSHIFT;
        case HID_KEY_ALT_LEFT: return KEYBOARD_MODIFIER_LEFTALT;
        case HID_KEY_GUI_LEFT: return KEYBOARD_MODIFIER_LEFTGUI;
        case HID_KEY_CONTROL_RIGHT: return KEYBOARD_MODIFIER_RIGHTCTRL;
        case HID_KEY_SHIFT_RIGHT: return KEYBOARD_MODIFIER_RIGHTSHIFT;
        case HID_KEY_ALT_RIGHT: return KEYBOARD_MODIFIER_RIGHTALT;
        case HID_KEY_GUI_RIGHT: return KEYBOARD_MODIFIER_RIGHTGUI;
        default: return 0;
    }
}

static void key_press(uint8_t hid) {
    const hid_keyboard_report_t prev = report;
    for (uint8_t k : report.keycode)
        if (k == hid) return;   // already held (autorepeat)
    report.modifier |= hid_key_to_modifier(hid);
    for (uint8_t& k : report.keycode) {
        if (k == HID_KEY_NONE) {
            k = hid;
            process_kbd_report(&report, &prev);
            return;
        }
    }
    // More than 6 keys held: dropped, as by the PS/2 driver.
}

static void key_release(uint8_t hid) {
    const hid_keyboard_report_t prev = report;
    report.modifier &= (uint8_t)~hid_key_to_modifier(hid);
    for (uint8_t& k : report.keycode) {
        if (k == hid) {
            k = HID_KEY_NONE;
            process_kbd_report(&report, &prev);
            return;
        }
    }
}

void kbd_linux_event(int code, int value) {
    const uint8_t hid = linux_to_hid(code);
    if (hid == HID_KEY_NONE) return;
    if (value == 1) key_press(hid);
    else if (value == 0) key_release(hid);
    // value 2 (autorepeat): the key is held already, nothing changes.
}
