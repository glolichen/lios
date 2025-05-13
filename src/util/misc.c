// this file contains functions from both libgcc and any libc
// this is not meant to implement everything, just some useful utilities for the kernel
// I am not responsible for most/all of the functions here, check the comments for attribution

#include "misc.h"
#include "const.h"

// from libgcc https://github.com/gcc-mirror/gcc/blob/master/libgcc/memcpy.c
void *memcpy(void *dest, const void *src, u64 len) {
	char *d = dest;
	const char *s = src;
	while (len--)
		*d++ = *s++;
	return dest;
}

// from libgcc https://github.com/gcc-mirror/gcc/blob/master/libgcc/memset.c
void *memset(void *dest, u32 val, u64 len) {
	unsigned char *ptr = dest;
	while (len-- > 0)
		*ptr++ = val;
	return dest;
}

// https://stackoverflow.com/a/1733294
u64 strlen(const char *str) {
	const char *s;
	for (s = str; *s; ++s);
	return (s - str);
}

// from glibc https://github.com/zerovm/glibc/blob/master/string/strcmp.c
u32 strcmp(const char *p1, const char *p2) {
	register const unsigned char *s1 = (const unsigned char *) p1;
	register const unsigned char *s2 = (const unsigned char *) p2;
	unsigned char c1, c2;

	do {
		c1 = (unsigned char) *s1++;
		c2 = (unsigned char) *s2++;
		if (c1 == '\0')
			return c1 - c2;
	} while (c1 == c2);

	return c1 - c2;
}

// from glibc https://github.com/zerovm/glibc/blob/master/string/strncmp.c
u32 strncmp(const char *s1, const char *s2, u64 n) {
	unsigned char c1 = '\0';
	unsigned char c2 = '\0';

	if (n >= 4) {
		u64 n4 = n >> 2;
		do {
			c1 = (unsigned char) *s1++;
			c2 = (unsigned char) *s2++;
			if (c1 == '\0' || c1 != c2)
				return c1 - c2;
			c1 = (unsigned char) *s1++;
			c2 = (unsigned char) *s2++;
			if (c1 == '\0' || c1 != c2)
				return c1 - c2;
			c1 = (unsigned char) *s1++;
			c2 = (unsigned char) *s2++;
			if (c1 == '\0' || c1 != c2)
				return c1 - c2;
			c1 = (unsigned char) *s1++;
			c2 = (unsigned char) *s2++;
			if (c1 == '\0' || c1 != c2)
				return c1 - c2;
		} while (--n4 > 0);
		n &= 3;
	}

	while (n > 0) {
		c1 = (unsigned char) *s1++;
		c2 = (unsigned char) *s2++;
		if (c1 == '\0' || c1 != c2)
			return c1 - c2;
		n--;
	}

	return c1 - c2;
}

char toupper(char c) {
	if ('a' <= c && c <= 'z')
		return c + 'A' - 'a';
	return c;
}
