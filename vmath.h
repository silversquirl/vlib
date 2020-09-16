/* vmath.h
 *
 * Common (and obscure!) math helpers
 */

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
#include <stdint.h>

// C11 shims {{{
#if __STDC_VERSION__ >= 201112L
#define _vmath_generic_float(value, func) _Generic((value), \
	float: func##f(value), \
	double: func##d(value), \
	long double: func##l(value), \
	default: func##d(value))
#else
#define _vmath_generic_float(value, func) \
	( sizeof (value) == sizeof (float) ? func##f(value) \
	: sizeof (value) == sizeof (double) ? func##d(value) \
	: sizeof (value) == sizeof (long double) ? func##l(value) \
	)
#endif

#if __STDC_VERSION__ >= 201112L
#define _vmath_cassert _Static_assert
#else
#define _vmath_splat(a, b) _vmath_splat2(a, b)
#define _vmath_splat2(a, b) a##b
#define _vmath_cassert(expr, msg) struct _vmath_splat(_vmath_cassert, __LINE__) { int a : (expr) ? 1 : -1; }
#endif
// }}}

#define TAU_F 6.2831853f
#define TAU 6.283185307179586
#define TAU_L 6.28318530717958647l

#define vradiansf(deg) ((TAU_F/360.0f)*(deg))
#define vdegreesf(rad) ((360.0f/TAU_F)*(rad))

#define vradiansd(deg) ((TAU/360.0)*(deg))
#define vdegreesd(rad) ((360.0/TAU)*(rad))

#define vradiansl(deg) ((TAU_L/360.0l)*(deg))
#define vdegreesl(deg) ((360.0l/TAU_L)*(deg))

#define vradians(deg) _vmath_generic_float(deg, vradians)
#define vdegrees(rad) _vmath_generic_float(rad, vdegrees)

// Floating point feature detection crap {{{
#if !defined(VMATH_SILENCE_ARCH) && !defined(__amd64__) && !defined(_M_AMD64)
#pragma message "vmath does not officially support architectures other than amd64, and has not been tested on them. Use at your own risk. Defined VMATH_SILENCE_ARCH to silence this warning."
#if !defined(__STDC_IEC_559__)
#pragma message "Your implementation does not define __STDC_IEC_559__. This is probably fine, but if you are on an obscure architecture, you may encounter issues. Define VMATH_SILENCE_ARCH to silence this warning."
#endif
#endif

_vmath_cassert(sizeof (double) <= sizeof (uint64_t), "Your C implementation's double does not fit into uint64_t, which is required by vmath.h");
_vmath_cassert(sizeof (float) <= sizeof (uint32_t), "Your C implementation's float does not fit into uint32_t, which is required by vmath.h");
_vmath_cassert(-1 == ~0, "Your C implementation does not use two's complement for negative integers, which is required by vmath.h");
// }}}

#define _vmath_ftoi(f) ((union {double f; uint64_t i;}){(f)}.i)
#define _vmath_ftoif(f) ((union {float f; uint32_t i;}){(f)}.i)
#define _vmath_itof(i) ((union {uint64_t i; double f;}){(i)}.f)
#define _vmath_itoff(i) ((union {uint32_t i; float f;}){(i)}.f)

// If you don't know what to put for acceptable_ulps, start at 1 and increase until you get the desired level of inaccuracy
// With acceptable_ulps == 0, this is equivalent to a == b
static inline _Bool vclose(double a, double b, int acceptable_ulps) {
	// See here for an explanation of how this works: https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/
	if (a == b) return 1;
	if (signbit(a) != signbit(b)) return 0;
	int64_t ai = _vmath_ftoi(a), bi = _vmath_ftoi(b);
	int64_t ulps = ai - bi;
	return ulps <= acceptable_ulps && ulps >= -acceptable_ulps;
}

static inline _Bool vclosef(float a, float b, int acceptable_ulps) {
	if (a == b) return 1;
	if (signbit(a) != signbit(b)) return 0;
	int32_t ai = _vmath_ftoif(a), bi = _vmath_ftoif(b);
	int32_t ulps = ai - bi;
	return ulps <= acceptable_ulps && ulps >= -acceptable_ulps;
}

// TODO: implement vclosel, but there's no 80-bit int so idk how tf to do that

static inline float _vmath_fisrf(float f) {
	// Fast inverse square root for float
	uint32_t fi = _vmath_ftoif(f);
	fi = 0x5f3759df - (fi >> 1);
	float ff = _vmath_itoff(fi);
	return ff * (1.5f - (0.5f * f * ff * ff));
}

static inline float _vmath_fisrd(double f) {
	// Fast inverse square root for double
	uint64_t fi = _vmath_ftoi(f);
	fi = 0x5fe6eb50c7b537a9 - (fi >> 1);
	double ff = _vmath_itof(fi);
	return ff * (1.5 - (0.5 * f * ff * ff));
}

#if !defined(VMATH_NOGNU) && defined(__SSE__) && (defined(__GNUC__) || defined(__clang__) || defined(__TINYC__))
#define VMATH_RSQRT_SSE
static inline float rsqrtf(float f) {
	// Inline asm for rsqrtss
	float ret;
	__asm__ ("rsqrtss %0, %1" : "=x" (ret) : "Ex" (f));
	return ret;
}
#else
// Fast inverse square root
// Not as fast or accurate as rsqrtss, but it's not too bad and works anywhere that uses IEEE754
#define rsqrtf _vmath_fisrf
#endif

// No rsqrt instruction for double, strangely
#define rsqrt _vmath_fisrd

// From this very nice page http://graphics.stanford.edu/~seander/bithacks.html#IntegerLogDeBruijn
static inline uint8_t _vmath_debruijn(uint32_t x) {
	static const uint8_t debruijn[32] = {
		0, 9, 1, 10, 13, 21, 2, 29, 11, 14, 16, 18, 22, 25, 3, 30,
		8, 12, 20, 28, 15, 17, 24, 7, 19, 27, 23, 6, 26, 5, 4, 31
	};

	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;

	return 1 + debruijn[(uint32_t)(x * 0x07C4ACDDU) >> 27];
}

#if defined(VMATH_NOGNU) || !defined(__GNUC__)
#define visize _vmath_debruijn
#else
#define visize(x) ((uint8_t)((CHAR_BIT * sizeof x) - __builtin_clz(x)))
#endif

struct vmath_rand {uint64_t a, b;};
// Initialize a random number generator
struct vmath_rand vmath_srand(uint32_t seed);
// Random 32-bit number
uint32_t vmath_rand32(struct vmath_rand *r);
// Random number between min and max, inclusive
uint32_t vmath_randr(struct vmath_rand *r, uint32_t min, uint32_t max);

#endif

#ifdef VMATH_IMPL

// PCG PRNG https://www.pcg-random.org/
// Implementation adapted from https://github.com/mattiasgustavsson/libs/blob/main/rnd.h
// {{{
static uint64_t vmath_murmur_fmix64(uint64_t x) {
	x ^= x >> 33;
	x *= 0xff51afd7ed558ccd;
	x ^= x >> 33;
	x *= 0xc4ceb9fe1a85ec53;
	x ^= x >> 33;
	return x;
}

struct vmath_rand vmath_srand(uint32_t seed) {
	uint64_t seed_ = (uint64_t)seed << 1 | 1;
	seed_ = vmath_murmur_fmix64(seed_);

	struct vmath_rand r;
	r.a = 0;
	r.b = seed_ << 1 | 1;

	vmath_rand32(&r);
	r.a += vmath_murmur_fmix64(seed_);
	vmath_rand32(&r);

	return r;
}

uint32_t vmath_rand32(struct vmath_rand *r) {
	uint64_t old = r->a;
	r->a = old * 0x5851f42d4c957f2d + r->b;
	uint32_t xsh = ((old >> 18) ^ old) >> 27;
	uint32_t rot = old >> 59;
	return xsh >> rot | xsh << ((~rot + 1) & 31);
}

uint32_t vmath_randr(struct vmath_rand *r, uint32_t min, uint32_t max) {
	uint32_t bound = max - min + 1;
	if (!bound) return vmath_rand32(r);
	uint32_t thres = (~bound + 1) % bound;

	uint32_t val;
	do {
		val = vmath_rand32(r);
	} while (val < thres);

	return val%bound + min;
}
// }}}

#endif
