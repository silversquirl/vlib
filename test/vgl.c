//#define VMATH_NOGNU
#include "vtest.h"
#define VGL_IMPL
#include "../vgl.h"

VTEST(test_mapfile) {
	struct vgl_mbuf m = vgl_mapfile("data/vgl/hello.txt");
	if (!vassert_not_null(m.data)) return;
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
	assert_eq_v3(v3neg(vec3(1, 2, 3)), vec3(-1, -2, -3), 0);
	assert_eq_v3(v3neg(vec3(0, 0, 0)), vec3(-0.0f, -0.0f, -0.0f), 0);
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
	assert_eq_v3(v3cross(vec3(1, 2, 3), vec3(4, 5, 6)), vec3(2*6 - 3*5, 3*4 - 1*6, 1*5 - 2*4), 2);
}

static inline _Bool m4eq(mat44_t a, mat44_t b, int ulps) {
	for (int y = 0; y < 4; y++) {
		for (int x = 0; x < 4; x++) {
			if (!vclosef(a.m[y][x], b.m[y][x], ulps)) return 0;
		}
	}
	return 1;
}

VTEST(test_m4mul) {
	mat44_t a = (mat44_t){{
		{1,   2,  3,  4},
		{5,   6,  7,  8},
		{9,  10, 11, 12},
		{13, 14, 15, 16},
	}};

	mat44_t b = (mat44_t){{
		{1, 0, 1, 0},
		{1, 0, 1, 0},
		{1, 0, 1, 0},
		{1, 0, 1, 0},
	}};

	mat44_t ab = (mat44_t){{
		{1+2+3+4, 0, 1+2+3+4, 0},
		{5+6+7+8, 0, 5+6+7+8, 0},
		{9+10+11+12, 0, 9+10+11+12, 0},
		{13+14+15+16, 0, 13+14+15+16, 0},
	}};

	mat44_t ba = (mat44_t){{
		{1+9, 2+10, 3+11, 4+12},
		{1+9, 2+10, 3+11, 4+12},
		{1+9, 2+10, 3+11, 4+12},
		{1+9, 2+10, 3+11, 4+12},
	}};

	vassert(m4eq(m4mul(a, m4id), a, 0));
	vassert(m4eq(m4mul(m4id, a), a, 0));
	vassert(m4eq(m4mul(a, b), ab, 0));
	vassert(m4eq(m4mul(b, a), ba, 0));
}

// TODO: test_load_farbfeld

VTEST(test_load_vmesh) {
	struct vgl_mesh *mesh = vgl_load_vmesh("data/vgl/cube.vmsh");
	if (!vassert_not_null(mesh)) return;

	vassert_eq(mesh->flag, VGL_MESH_NORMAL | VGL_MESH_UV);
	vassert_eq(mesh->nvert, 24);
	vassert_eq(mesh->ntri, 12);
	vassertn(vgl_mesh_highpoly(mesh));

	// TODO: validate actual mesh data
}

VTESTS_BEGIN
	test_mapfile,
	test_v3op,
	test_v3dot,
	test_v3norm,
	test_v3cross,
	test_m4mul,
	test_load_vmesh,
VTESTS_END
