//#define VMATH_NOGNU
#include "vtest.h"
#define VGL_IMPL
#include "../vgl.h"

VTEST(test_mapfile) {
	struct vgl_mbuf m = vgl_mapfile("data/vgl/hello.txt");
	vassert_msg(!strncmp(m.data, "Hello, world!\n", m.len), "Incorrect file data: '''\n%.*s\n'''", (int)m.len, m.data);
	vgl_unmap(m);
}

static inline int _assert_eq_v3(vec3_t a, vec3_t b, int ulps, _vassert_typed_args) {
	return
		(vclosef(a.x, b.x, ulps) && vclosef(a.y, b.y, ulps) && vclosef(a.z, b.z, ulps)) ||
		_vassert_fail("[%f, %f, %f] != [%f, %f, %f]", a.x, a.y, a.z, b.x, b.y, b.z);
}
#define assert_eq_v3(a, b, ulps) _vassert_wrap(_assert_eq_v3, a, b, ulps)

static inline int _assert_closef(float a, float b, int ulps, _vassert_typed_args) {
	return vclosef(a, b, ulps) || _vassert_fail("%f != %f", a, b);
}
#define assert_closef(a, b, ulps) _vassert_wrap(_assert_closef, a, b, ulps)

VTEST(test_v3op) {
	assert_eq_v3(v3v3op(vec3(1, 2, 3), +, vec3(3, 4, 5)), vec3(4, 6, 8), 1);
	assert_eq_v3(v3sop(vec3(1, 2, 3), *, 3), vec3(3, 6, 9), 1);
}

VTEST(test_v3dot) {
	assert_closef(v3dot(vec3(1, 2, 3), vec3(4, 5, 6)), 4+10+18, 2);
}

#ifdef VMATH_RSQRT_SSE
#define RSQRTF_ULPS 2084
#else
#define RSQRTF_ULPS 28401
#endif
VTEST(test_v3norm) {
	assert_eq_v3(v3norm(vec3(4, 4, 7)), vec3(4.0f/9.0f, 4.0f/9.0f, 7.0f/9.0f), RSQRTF_ULPS);
}

VTEST(test_v3cross) {
	assert_eq_v3(v3cross(vec3(1, 2, 3), vec3(4, 5, 6)), vec3(1*5 - 2*4, 2*6 - 3*5, 3*4 - 1*6), 2);
}

// TODO: test_m4mul
// TODO: test_load_farbfeld

VTESTS_BEGIN
	test_mapfile,
	test_v3op,
	test_v3dot,
	test_v3norm,
	test_v3cross,
VTESTS_END
