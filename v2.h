/*
 * v2.h
 *
 * 2D vector math and collision detection. Requires C99 or higher.
 *
 * Vectors are implemented on top of complex numbers. The advantage of this is that vector
 * addition/subtraction and multiplication/division by scalars is automatically defined and
 * uses normal arithmetic syntax.
 *
 * WARNING: Multiplying/dividing two vectors will use complex number semantics. You probably don't want that.
 *
 * In one source file, define V2_IMPL before including this header.
 *
 * Authors:
 *  - vktec
 *
 * Special thanks:
 *  - mlugg, for being actually good at maths unlike me
 *
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

#ifndef V2_H
#define V2_H

#include <complex.h>
#include <math.h>
#include <string.h>
#include <limits.h>

// Macro garbage {{{
#if __STDC_VERSION__ >= 201112L
#define _v2_cassert _Static_assert
#else
#define _v2_cassert(expr, msg) struct _v2_cassert##__LINE__ { int a : (expr) ? 1 : -1; }
#endif

#define _v2_argcount_0(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, x, ...) (x)
#define _v2_argcount(...) _v2_argcount_0(__VA_ARGS__, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)
// }}}

// Scalar type {{{
#ifdef V2_SINGLE_PRECISION
typedef float v2s;
#else
typedef double v2s;
#endif

#ifdef V2_NO_CLOSE
#pragma message ("You have chosen to omit the v2close function. This may cause compile errors.")
#else

#ifndef __STDC_IEC_559__
#error "Your C implementation does not support IEEE-754 floating point, which is required for v2close. If you're sure you don't need it, you can omit this function by defining V2_OMIT_CLOSE."
#endif
_v2_cassert(sizeof (v2s) <= sizeof (long), "v2s does not fit into a long, which is required for v2close. If you're sure you don't need it, you can omit this function by defining V2_OMIT_CLOSE.");

// If you don't know what to put for acceptable_ulps, start at 1 and increase until you get the desired level of inaccuracy
// With acceptable_ulps == 0, this is equivalent to a == b
static inline _Bool v2close(v2s a, v2s b, int acceptable_ulps) {
	// See here for an explanation of how this works: https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/
	if (signbit(a) != signbit(b)) return 0;
	union {v2s f; int i;} ai = {a}, bi = {b};
	long ulps = ai.i - bi.i;
	return ulps <= acceptable_ulps && ulps >= -acceptable_ulps;
}

#endif
// }}}

// Vector type {{{
#ifdef V2_SINGLE_PRECISION
typedef float _Complex v2v;
#else
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

// Check whether the vector is NAN
#define v2nan(v) isnan(v2x(v))

// Complex conjugate (flips sign of Y axis)
#ifdef V2_SINGLE_PRECISION
#define v2conj conjf
#else
#define v2conj conj
#endif
// }}}

// Shapes {{{
// Circle
struct v2circ {
	v2v center;
	v2s radius;
};

// Convex polygon
struct v2poly {
	unsigned sides;
	v2v points[];
};

// Polygon constructors
// Returned value must be freed with free()
struct v2poly *v2poly_v(unsigned sides, v2v *vertices);
struct v2poly *v2poly_n(unsigned sides, ...);
// This constructor only works with up to 32 points. If you need more than that, use v2poly_n
#define v2poly(...) v2poly_n(_v2_argcount(__VA_ARGS__), __VA_ARGS__)

// Polygon helpers
void v2_move_poly(struct v2poly *poly, v2v delta);
// }}}

// Collision detection
// Functions return a MTV (minimum translation vector), or NAN in the event of no collision
// If shapes touch without intersecting, 0 is returned
v2v v2circ2circ(struct v2circ a, struct v2circ b);
v2v v2poly2poly(struct v2poly *a, struct v2poly *b);
v2v v2circ2poly(struct v2circ a, struct v2poly *b);

// Raycasting
struct v2ray {
	v2v start, direction;
};
// Raycasting functions return a positive finite distance to the shape, or NAN if the ray does not intersect the shape
v2s v2ray2circ(struct v2ray r, struct v2circ circ);
v2s v2ray2poly(struct v2ray r, struct v2poly *poly);

// Broad-phase collision detection {{{
// Axis-aligned bounding box
// a must be smaller than b in both dimensions
struct v2box {
	v2v a, b;
};

// Create bounding boxes for shapes
struct v2box v2circbox(struct v2circ circ); // O(1)
struct v2box v2polybox(struct v2poly *poly); // O(n)

// Check AABB collision
_Bool v2box2box(struct v2box a, struct v2box b);

// Quadtree algorithm for subdividing the world into sections with higher resolution around objects.
//
// The ideal max height depends on the number and distribution of objects in your scene: more objects in a
// small area requires a higher max height.
//
// Bear in mind that memory consumption grows exponentially with respect to the max height. A max height of 40
// will consume 1TiB of RAM. A max height of 51 will overflow the physical address bus of an amd64 processor.
//
// For a max height of n, this representation has O(4^n) space complexity. While this is exponential, the
// constant in this case is quite small. Additionally, a tree of height n can store up to 4^n nodes, so the
// space complexity on the number of nodes is actually linear.
struct v2qt;
struct v2qt *v2qt(v2v dimensions, unsigned char height);
void v2qt_populate(struct v2qt *t, struct v2box *boxes, size_t count);
void v2qt_addbox(struct v2qt *t, struct v2box box);
// }}}

#endif

#ifdef V2_IMPL
#undef V2_IMPL

#include <stdarg.h>
#include <stdint.h>

static inline struct v2poly *_v2_make_poly(unsigned sides) {
	return malloc(offsetof(struct v2poly, points) + sizeof (v2v) * sides);
}

struct v2poly *v2poly_v(unsigned sides, v2v *vertices) {
	struct v2poly *p = _v2_make_poly(sides);
	p->sides = sides;
	memcpy(p->points, vertices, sizeof *vertices * sides);
	return p;
}

struct v2poly *v2poly_n(unsigned sides, ...) {
	struct v2poly *p = _v2_make_poly(sides);
	p->sides = sides;

	va_list args;
	va_start(args, sides);
	for (unsigned i = 0; i < p->sides; i++) {
		p->points[i] = va_arg(args, v2v);
	}
	va_end(args);

	return p;
}

void v2_move_poly(struct v2poly *poly, v2v delta) {
	for (unsigned i = 0; i < poly->sides; i++) {
		poly->points[i] += delta;
	}
}

// Collision {{{
v2v v2circ2circ(struct v2circ a, struct v2circ b) {
	// TODO: Go through all this maths and check for optimizations. There's probably
	//       some stuff that can be improved, but it's 1am and I'm too tired right now

	// Collision distance
	v2s cd = a.radius + b.radius;
	// Actual distance (squared)
	v2s d2 = v2mag2(b.center - a.center);
	if (d2 > cd*cd) return NAN;
	if (d2 == cd*cd) return 0;

	v2s d = sqrt(d2);
	// Resize vector from b to a to the size of the intersection
	v2v dir = (a.center - b.center)/d;
	v2s sect = cd - d;
	return dir * sect;
}

// Projects the polygon onto the axis
// x is min, y is max
static v2v _v2_project_poly(v2v axis, struct v2poly *poly) {
	v2s min = INFINITY, max = -INFINITY, x;
	for (unsigned i = 0; i < poly->sides; i++) {
		x = v2dot(axis, poly->points[i]);
		if (x < min) min = x;
		if (x > max) max = x;
	}
	return v2v(min, max);
}

static v2v _v2_project_circ(v2v axis, struct v2circ circ) {
	v2s m = v2mag(axis);
	v2s c = v2dot(axis, circ.center);
	v2s r = m*circ.radius;
	return v2v(c - r, c + r);
}

static inline v2v _v2_projection_overlap(v2v pa, v2v pb, v2v *axis) {
	v2s minmax = v2y(pb) - v2x(pa);
	v2s maxmin = v2y(pa) - v2x(pb);
	if (minmax < maxmin) {
		return minmax;
	} else {
		*axis *= -1;
		return maxmin;
	}
}

static v2v _v2_poly2poly_sat(struct v2poly *a, struct v2poly *b) {
	v2v min_axis = NAN;
	v2s min_overlap = INFINITY;
	for (unsigned i = 0; i < a->sides; i++) {
		v2v from = a->points[i], to = a->points[(i+1) % a->sides];

		// The normal can be computed by multiplying by i
		// This is equivalent to a rotation by 90deg, as cos(90) = 0 and sin(90) = 1
		// For a clockwise vertex order, this normal faces inwards. However, that doesn't
		// matter here because it's just used as an axis
		// TODO: try to find a way to avoid normalizing here, sqrt is expensive
		v2v axis = v2norm(I * (to-from));

		// Project polygons a and b against the axis
		v2v pa = _v2_project_poly(axis, a);
		v2v pb = _v2_project_poly(axis, b);

		// Check the collision
		v2s overlap = _v2_projection_overlap(pa, pb, &axis);
		if (overlap < min_overlap) {
			if (overlap < 0) return NAN;
			min_overlap = overlap;
			min_axis = axis;
		}
	}
	return min_axis*min_overlap;
}

v2v v2poly2poly(struct v2poly *a, struct v2poly *b) {
	// We use the Separating Axis Theorem (SAT) to determine collisions between convex polygons
	v2v ab = _v2_poly2poly_sat(a, b);
	if (v2nan(ab)) return ab;
	v2v ba = _v2_poly2poly_sat(b, a);
	return v2mag2(ab) < v2mag2(ba) ? -ab : ba;
}

v2v v2circ2poly(struct v2circ circ, struct v2poly *poly) {
	// SAT again but a bit different because circle
	v2v min_axis = NAN;
	v2s min_overlap = INFINITY;

	v2v circ_axis = NAN;
	v2s circ_distance = INFINITY;

	for (unsigned i = 0; i < poly->sides; i++) {
		v2v from = poly->points[i], to = poly->points[(i+1) % poly->sides];

		// Find circle's axis
		v2v axis = circ.center - from;
		v2s distance = v2mag2(axis);
		if (distance < circ_distance) {
			circ_distance = distance;
			// TODO: Here's another normalize it'd be nice to avoid
			circ_axis = v2norm(axis);
		}

		// Test polygon face's axis
		// TODO: More normalization
		axis = v2norm(I * (to-from));
		v2v pcirc = _v2_project_circ(axis, circ);
		v2v ppoly = _v2_project_poly(axis, poly);

		v2s overlap = _v2_projection_overlap(pcirc, ppoly, &axis);
		if (overlap < min_overlap) {
			if (overlap < 0) return NAN;
			min_overlap = overlap;
			min_axis = axis;
		}
	}

	// Test circle's axis
	v2v pcirc = _v2_project_circ(circ_axis, circ);
	v2v ppoly = _v2_project_poly(circ_axis, poly);

	v2s overlap = _v2_projection_overlap(pcirc, ppoly, &circ_axis);
	if (overlap < min_overlap) {
		if (overlap < 0) return NAN;
		min_overlap = overlap;
		min_axis = circ_axis;
	}

	return min_axis*min_overlap;
}
// }}}

// Raycasting {{{
v2s v2ray2circ(struct v2ray r, struct v2circ circ) {
	// Translate so the ray is at the origin
	circ.center -= r.start;

	// Project circle centre
	v2s proj = v2dot(r.direction, circ.center);
	if (proj < -circ.radius) return NAN;

	// Inverse magnitude squared
	v2s mag2 = v2mag2(r.direction);
	v2s inv_mag2 = 1/mag2;

	// If distance to center > radius, no collision
	v2s dist2 = v2mag2(circ.center - proj*r.direction*inv_mag2);
	v2s rad2 = circ.radius*circ.radius;
	if (dist2 > rad2) return NAN;

	// Else, we calculate the distance between the projected point and the collision using
	// Pythagoras' theorem and subtract it from the projected distance
	v2s delta = sqrt((rad2 - dist2) * mag2);
	// If the ray begins inside the circle, we need to flip the direction
	if (v2mag2(circ.center) < rad2) delta = -delta;
	proj -= delta;
	if (proj < 0) return NAN;

	return proj*inv_mag2;
}

v2s v2ray2poly(struct v2ray r, struct v2poly *poly) {
	// TODO: Go through this maths too, hopefully future vktec can find a way to remove the sqrt

	// Normalize the ray
	v2s inv_mag = 1/v2mag(r.direction);
	r.direction = v2conj(r.direction * inv_mag);

	// Check against every face. O(n)
	v2s min = INFINITY;
	for (unsigned i = 0; i < poly->sides; i++) {
		v2v a = poly->points[i], b = poly->points[(i+1) % poly->sides];
		// This transform changes the problem from a line intersection to an axis-intercept
		a = (a - r.start) * r.direction;
		b = (b - r.start) * r.direction;

		v2v d = b - a;
		v2s t = -v2y(a) / v2y(d);
		if (t < 0 || t > 1) continue;
		v2s x = v2x(a) + t*v2x(d);
		if (x < 0) continue;
		if (x < min) min = x;
	}

	return min * inv_mag;
}
// }}}

// Broad-phase collision detection {{{
struct v2box v2circbox(struct v2circ circ) {
	v2v radv = v2v(circ.radius, circ.radius);
	return (struct v2box){circ.center - radv, circ.center + radv};
}

struct v2box v2polybox(struct v2poly *poly) {
	v2s minx = INFINITY, maxx = -INFINITY;
	v2s miny = INFINITY, maxy = -INFINITY;

	for (unsigned i = 0; i < poly->sides; i++) {
		v2v point = poly->points[i];
		v2s x = v2x(point), y = v2y(point);

		if (x < minx) minx = x;
		if (x > maxx) maxx = x;
		if (y < miny) miny = y;
		if (y > maxy) maxy = y;
	}

	return (struct v2box){v2v(minx, miny), v2v(maxx, maxy)};
}

_Bool v2box2box(struct v2box a, struct v2box b) {
	if (v2x(a.b) < v2x(b.a) || v2x(b.b) < v2x(a.a)) return 0;
	if (v2y(a.b) < v2y(b.a) || v2y(b.b) < v2y(a.a)) return 0;
	return 1;
}

// Bit buffer functions
static inline _Bool _v2_bbg(unsigned char *bb, unsigned char bit) {
	return (bb[bit/CHAR_BIT] >> (bit%CHAR_BIT)) & 1;
}
static inline _Bool _v2_bbs(unsigned char *bb, unsigned char bit, _Bool value) {
	// I can't think of a way to avoid this branch
	// Luckily, the compiler will optimize it out for constant values when inlining
	// Hooray for inlining! \o/
	if (value) bb[bit/CHAR_BIT] |= 1 << (bit%CHAR_BIT);
	else bb[bit/CHAR_BIT] &= ~(1 << (bit%CHAR_BIT));
	return value;
}

// Math helpers
#if defined(V2_NOGNU) || !defined(__GNUC__)
// From this very nice page http://graphics.stanford.edu/~seander/bithacks.html#IntegerLogDeBruijn
static inline uint8_t _v2_isize(uint32_t x) {
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
#else
#define _v2_isize(x) ((uint8_t)((CHAR_BIT * sizeof x) - __builtin_clz(x)))
#endif

#define _v2_floorlog4(x) ((_v2_isize(x)+1)/2 - 1) // May be undef for x = 0
#define _v2_4exp(e) (1<<(2*(e)))

// Quadtree with a fancy bitwise representation
struct v2qt {
	// Size of the world
	v2v dim;
	// Maximum number of objects in a cell where it is acceptable to stop subdividing
	unsigned threshold;
	// Maximum height of the tree
	unsigned char height;
	// Tree bit data (may or may not also have some not-bit-data at the end for storing lists of rects)
	unsigned char bb[];
};

struct v2qt *v2qt(v2v dimensions, unsigned objects_per_cell, unsigned char max_tree_height) {
	// The size of bb is composed of two parts:
	// - One bit per non-leaf node
	// - `threshold` pointers to v2box per leaf node
	//
	// The number of non-leaf nodes can be found using the sum k = 0 -> height-1 of 4^k
	// The number of leaf nodes is simply 4^height
	//
	// h-1      4ʰ - 1
	//  ∑  4ᵏ = ──────
	// k=0      4  - 1
	//
	// 4ʰ - 1       4ʰ⁻³-1
	// ────── / 8 = ──────
	// 4  - 1       4  - 1
	//
	// One space optimization strategy could be to store an array of boxes and index into that instead
	// of using pointers. This would likely reduce the memory consumption of the leaves' box arrays
	// because one box will often intersect with multiple leaf nodes.

	// One bit per non-leaf node
	size_t bytes = !max_tree_height || (_v2_4exp(max_tree_height) - 3) / 3;
	// One pointer per leaf node
	// This is actually slightly more complex than the 4^height I quoted above: due to alignment we need
	// to round up to the nearest multiple of sizeof (struct v2box *)
	// We'll assume this size is a power of two. If it isn't, well, this code may run into some minor issues
	bytes += ((_v2_4exp(max_tree_height) + 1) * sizeof (struct v2box *)) & ~(sizeof (struct v2box *) - 1);

	struct v2qt *t = malloc(offsetof(struct v2qt, bb) + bytes);
	t.dim = dimensions;
	t.threshold = objects_per_cell;
	t.height = max_tree_height;
	return t;
}

static inline struct v2box **_v2qt_get_boxes(struct v2qt *t, uint32_t cell) {
	cell /= 8; // bits -> bytes
	cell /= sizeof (struct v2box *); // bytes -> v2boxes
	return (struct v2box **)(t->bb)[cell];
}

static _Bool _v2qt_box_in_cell(struct v2qt *t, struct v2box box, uint32_t cell) {
	// ty mlugg
	uint8_t bottom_rank = _v2_floorlog4(3*cell + 1);
	uint32_t bottom_width = _v2_4exp(bottom_rank);
	uint32_t target = cell - (bottom_width - 1)/3;

	v2v pos = 0;
	v2s side_length = 1;
	for (int rank = 1; rank <= bottom_rank; rank++) {
		uint32_t width = _v2_4exp(rank);
		uint32_t next = target*width / bottom_width;
		uint8_t quad = val % 4;

		side_length = 1/(1<<rank);
		v2v dim = v2v((quad % 2) * side_length, (quad / 2) * side_length);
		pos += dim;
	}

	pos = v2v(v2x(pos) * v2x(t->dim), v2y(pos) * v2y(t->dim));
	v2v dim = side_length * t->dim;

	return v2box2box(box, (struct v2box){pos, pos+dim});
}

void v2qt_populate(struct v2qt *t, struct v2box *boxes, size_t count) {
	for (size_t i = 0; i < count; i++) {
		v2qt_addbox(t, boxes[i]);
	}

	return t;
}

void v2qt_addbox(struct v2qt *t, struct v2box box) {
	// ty mlugg
	uint8_t bottom_rank = _v2_floorlog4(3*cell + 1);
	uint32_t bottom_width = _v2_4exp(bottom_rank);
	uint32_t target = cell - (bottom_width - 1)/3;

	v2v pos = 0;
	v2s side_length = 1;
	for (int rank = 1; rank <= bottom_rank; rank++) {
		uint32_t width = _v2_4exp(rank);
		uint32_t next = target*width / bottom_width;
		uint8_t quad = val % 4;

		side_length = 1/(1<<rank);
		v2v dim = v2v((quad % 2) * side_length, (quad / 2) * side_length);
		pos += dim;
	}

	pos = v2v(v2x(pos) * v2x(t->dim), v2y(pos) * v2y(t->dim));
	v2v dim = side_length * t->dim;

	return v2box2box(box, (struct v2box){pos, pos+dim});
}
// }}}

#endif
