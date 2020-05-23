/*
 * This is free and unencumbered software released into the public domain.
 *
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.
 *
 * In jurisdictions that recognize copyright laws, the author or authors
 * of this software dedicate any and all copyright interest in the
 * software to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and
 * successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * software under copyright law.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * For more information, please refer to <http://unlicense.org/>
 */

#ifndef VMATH_H
#define VMATH_H

#include <math.h>
#include <limits.h>

#if __STDC_VERSION__ >= 201112L
#define _vmath_cassert _Static_assert
#else
#define _vmath_cassert(expr, msg) struct _vmath_cassert##__LINE__ { int a : (expr) ? 1 : -1; }
#endif

#ifndef __STDC_IEC_559__
#error "Your C implementation does not support IEEE-754 floating point, which is required by vmath.h"
#endif
_vmath_cassert(sizeof (double) <= sizeof (long), "Your C implementation's double does not fit into a long, which is required by vmath.h");
_vmath_cassert(-1 == ~0, "Your C implementation does not use two's complement for negative integers, which is required by vmath.h");

// If you don't know what to put for acceptable_ulps, start at 1 and increase until you get the desired level of inaccuracy
// With acceptable_ulps == 0, this is equivalent to a == b
static inline _Bool vclose(double a, double b, int acceptable_ulps) {
	// See here for an explanation of how this works: https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/
	if (signbit(a) != signbit(b)) return 0;
	union {double f; int i;} ai = {a}, bi = {b};
	long ulps = ai.i - bi.i;
	return ulps <= acceptable_ulps && ulps >= -acceptable_ulps;
}

static inline _Bool vclosef(float a, float b, int acceptable_ulps) {
	if (signbit(a) != signbit(b)) return 0;
	union {float f; int i;} ai = {a}, bi = {b};
	long ulps = ai.i - bi.i;
	return ulps <= acceptable_ulps && ulps >= -acceptable_ulps;
}

// TODO: implement vclosel, but there's no 80-bit int so idk how tf to do that

#if defined(VMATH_NOGNU) || !defined(__GNUC__)
_vmath_cassert(sizeof (unsigned) == 32/CHAR_BIT, "Your C implementation's int is not 32 bits, which is required by vmath.h");
// From this very nice page http://graphics.stanford.edu/~seander/bithacks.html#IntegerLogDeBruijn
static inline unsigned char visize(unsigned x) {
	static const unsigned char debruijn[32] = {
		0, 9, 1, 10, 13, 21, 2, 29, 11, 14, 16, 18, 22, 25, 3, 30,
		8, 12, 20, 28, 15, 17, 24, 7, 19, 27, 23, 6, 26, 5, 4, 31
	};

	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;

	return 1 + debruijn[(unsigned)(x * 0x07C4ACDDU) >> 27];
}
#else
#define visize(x) ((unsigned char)((CHAR_BIT * sizeof x) - __builtin_clz(x)))
#endif

#endif
