#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdbool.h>
#include "interrupt.h"

struct KeyboardRecordingList {
	u8 *pressed_keys;
	u64 length;
};

void keyboard_routine(const struct InterruptData *data);

void keyboard_start_recording(void);
bool keyboard_is_recording(void);
struct KeyboardRecordingList keyboard_get_recording(void);
void keyboard_end_recording(void);

#endif
