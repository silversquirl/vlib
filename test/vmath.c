#include "vtest.h"
#include "../vmath.h"

#define TEST_RADIANS(type, func_suffix, float_suffix, tau_suffix) do { \
		V_ENSURE_TYPE(vradians##func_suffix(0.0##float_suffix), type); \
		vassert_eq(vradians##func_suffix(0.0##float_suffix), 0.0##float_suffix); \
		vassert_eq(vradians##func_suffix(90.0##float_suffix), TAU##tau_suffix/4); \
		vassert_eq(vradians##func_suffix(180.0##float_suffix), TAU##tau_suffix/2); \
		vassert_eq(vradians##func_suffix(360.0##float_suffix), TAU##tau_suffix); \
	} while (0)

VTEST(test_radiansf) {
	TEST_RADIANS(float, f, f, _F);
}

VTEST(test_radiansd) {
	TEST_RADIANS(double, d,,);
}

VTEST(test_radiansl) {
	TEST_RADIANS(long double, l, l, _L);
}

VTEST(test_radians_generic) {
	TEST_RADIANS(float,, f, _F);
	TEST_RADIANS(double,,,);
	TEST_RADIANS(long double,, l, _L);
}


#define TEST_DEGREES(type, func_suffix, float_suffix, tau_suffix) do { \
		V_ENSURE_TYPE(vdegrees##func_suffix(0.0##float_suffix), type); \
		vassert_eq(vdegrees##func_suffix(0.0##float_suffix), 0.0##float_suffix); \
		vassert_eq(vdegrees##func_suffix(TAU##tau_suffix/4), 90.0##float_suffix); \
		vassert_eq(vdegrees##func_suffix(TAU##tau_suffix/2), 180.0##float_suffix); \
		vassert_eq(vdegrees##func_suffix(TAU##tau_suffix), 360.0##float_suffix); \
	} while (0)

VTEST(test_degreesf) {
	TEST_DEGREES(float, f, f, _F);
}

VTEST(test_degreesd) {
	TEST_DEGREES(double, d,,);
}

VTEST(test_degreesl) {
	TEST_DEGREES(long double, l, l, _L);
}

VTEST(test_degrees_generic) {
	TEST_DEGREES(float,, f, _F);
	TEST_DEGREES(double,,,);
	TEST_DEGREES(long double,, l, _L);
}

#define TEST_CLOSE(suffix) do { \
		vassert(vclose##suffix(0.5, close##suffix##_values[-1], 1)); \
		vassert(vclose##suffix(0.5, close##suffix##_values[1], 1)); \
		\
		vassertn(vclose##suffix(0.5, close##suffix##_values[-2], 1)); \
		vassertn(vclose##suffix(0.5, close##suffix##_values[2], 1)); \
		\
		vassert(vclose##suffix(0.5, close##suffix##_values[-2], 2)); \
		vassert(vclose##suffix(0.5, close##suffix##_values[2], 2)); \
	} while (0)

double close_values_[] = {
	0.49999999999999988897769753748434595763683319091796875,
	0.499999999999999944488848768742172978818416595458984375,
	0.5,
	0.50000000000000011102230246251565404236316680908203125,
	0.5000000000000002220446049250313080847263336181640625,
};
double *close_values = close_values_+2;

VTEST(test_close) {
	TEST_CLOSE();
}

float closef_values_[] = {
	0.499999940395355224609375,
	0.4999999701976776123046875,
	0.5,
	0.500000059604644775390625,
	0.50000011920928955078125,
};
float *closef_values = closef_values_+2;

VTEST(test_closef) {
	vassert(vclosef(0.5, closef_values[-1], 1));
	vassert(vclosef(0.5, closef_values[1], 1));

	vassertn(vclosef(0.5, closef_values[-2], 1));
	vassertn(vclosef(0.5, closef_values[2], 1));

	vassert(vclosef(0.5, closef_values[-2], 2));
	vassert(vclosef(0.5, closef_values[2], 2));
}

VTEST(test_rsqrtf) {
	for (float f = 1; f < 100; f++) {
		float res = 1.0f / f;
		float sq = f*f;
		float res1 = rsqrtf(sq);
		float res2 = _vmath_fisrf(sq);
		vassert_msg(vclosef(res1, res, 5000),  "rsqrtf(%g)         %.8f != %.8f", sq, res1, res);
		vassert_msg(vclosef(res2, res, 28401), "_vmath_fisrf(%g)   %.8f != %.8f", sq, res2, res);
	}
}

VTEST(test_rsqrt) {
	for (double f = 1; f < 100; f++) {
		double res = 1.0 / f;
		double sq = f*f;
		double res1 = rsqrt(sq);
		vassert_msg(vclosef(res1, res, 28385), "rsqrt(%g)   %.16f != %.16f", sq, res1, res);
	}
}

VTEST(test_debruijn) {
	uint8_t size = 1;
	for (int i = 1; i <= 1500; i++) {
		if (i >= (1<<size)) size++;
		uint8_t computed = _vmath_debruijn(i);
		if (!vassert_msg(computed == size, "_vmath_debruijn(%d): %d != %d", i, computed, size)) {
			break;
		}
	}
}

VTESTS_BEGIN
	test_radiansf,
	test_radiansd,
	test_radiansl,
	test_radians_generic,

	test_degreesf,
	test_degreesd,
	test_degreesl,
	test_degrees_generic,

	test_close,
	test_closef,

	test_rsqrtf,
	test_rsqrt,

	test_debruijn,
VTESTS_END
