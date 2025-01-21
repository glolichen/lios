#ifndef KMATH_H
#define KMATH_H

#include "const.h"

u32 absolute(i32 num);
u32 round_u32_div(u32 dividend, u32 divisor);
u64 ceil_u64_div(u64 dividend, u64 divisor);
u64 floor_u64_div(u64 dividend, u64 divisor);
u32 ceil_u32_div(u32 dividend, u32 divisor);
u32 floor_u32_div(u32 dividend, u32 divisor);

u64 u64_max(u64 a, u64 b);

#endif
