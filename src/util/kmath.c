#include "kmath.h"
#include "const.h"

// some math functions that have to implemented manually
u32 absolute(i32 num) {
	return num > 0 ? num : -num;
}
u32 round_u32_div(u32 dividend, u32 divisor) {
	u32 low = dividend / divisor;
	u32 high = low + 1;
	u32 low_error = absolute(low * divisor - dividend);
	u32 high_error = absolute(high * divisor - dividend);
	return low_error < high_error ? low : high;
}
u64 ceil_u64_div(u64 dividend, u64 divisor) {
	u64 floor = dividend / divisor;
	return floor * divisor == dividend ? floor : floor + 1;
}
u64 floor_u64_div(u64 dividend, u64 divisor) {
	u64 floor = dividend / divisor;
	return floor * divisor == dividend ? floor : floor - 1;
}

u32 ceil_u32_div(u32 dividend, u32 divisor) {
	u32 floor = dividend / divisor;
	return floor * divisor == dividend ? floor : floor + 1;
}
u32 floor_u32_div(u32 dividend, u32 divisor) {
	u32 floor = dividend / divisor;
	return floor * divisor == dividend ? floor : floor - 1;
}

u64 u64_max(u64 a, u64 b) {
	return a > b ? a : b;
}

