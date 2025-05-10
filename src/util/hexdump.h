#ifndef HEXDUMP_H
#define HEXDUMP_H

#include <stdbool.h>
#include "const.h"

void hexdump(const void *src, u64 len, bool find_readable);

#endif
