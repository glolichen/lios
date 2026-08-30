#ifndef SYSCALL_H
#define SYSCALL_H

#include "../int/interrupt.h"

void syscall_routine(const struct InterruptData *data);

#endif

