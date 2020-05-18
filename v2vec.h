/*
 * v2vec.h
 *
 * 2D vector math implemented on top of complex numbers. Requires C11 or higher.
 * The advantage of using complex numbers is that vector addition and multiplication/division
 * by scalars is automatically defined and uses regular arithmetic syntax.
 *
 * WARNING: Multiplying/dividing two vectors will use complex number semantics. You probably don't want that.
 *
 */
#ifndef V2VEC_H
#define V2VEC_H

#include <complex.h>

#ifdef V2_SINGLE_PRECISION
typedef float v2s;
typedef float _Complex v2v;
#else
typedef double v2s;
typedef double _Complex v2v;
#endif

// Constructor
#if defined(CMPLX)
#define v2v(x, y) ((v2v)CMPLX(x, y))
#else
#define v2v(x, y) ((v2v)((x) + (y)*I))
#endif

// Components
#ifdef V2_SINGLE_PRECISION
#define v2x(v) crealf(v)
#define v2y(v) cimagf(v)
#else
#define v2x(v) creal(v)
#define v2y(v) cimag(v)
#endif

// Magnitude
#ifdef V2_SINGLE_PRECISION
#define v2mag(v) cabsf(v)
#else
#define v2mag(v) cabs(v)
#endif
// Squared magnitude
static inline v2s v2mag2(v2v v) { 
	v2s x = v2x(v), y = v2y(v);
	return x*x + y*y;
}

// Normalize
static inline v2v v2norm(v2v v) {
	if (v == 0) return 0;
	return v/v2mag(v);
}

// Dot product
static inline v2s v2dot(v2v a, v2v b) {
	return v2x(a)*v2x(b) + v2y(a)*v2y(b);
}

// Rotate by angle (radians)
static inline v2v v2rot(v2v v, v2s theta) {
	return v * v2v(cos(theta), sin(theta));
}

// Complex conjugate (flips sign of Y axis)
#ifdef V2_SINGLE_PRECISION
#define v2conj conjf
#else
#define v2conj conj
#endif

#endif
