#ifndef MISC_H
#define MISC_H

#include "const.h"

void *memcpy(void *dest, const void *src, u64 n);
void *memset(void *dest, u32 val, u64 len);

u64 strlen(const char *str);
u32 strcmp(const char *p1, const char *p2);
u32 strncmp(const char *s1, const char *s2, u64 n);

char toupper(char c);

#endif
