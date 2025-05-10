#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdbool.h>
#include "../int/interrupt.h"

void keyboard_routine(const struct InterruptData *data);
void keyboard_start_recording(void);
bool keyboard_is_recording(void);
// u8 *keyboard_get_recording(void);
u32 keyboard_get_recording(void);

#endif
