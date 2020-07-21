//#define VMATH_NOGNU
#include "../examples/glad/glad.h"
#include "../examples/glad/glad.c"

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
		_vassert_fail("[%.200g, %.200g, %.200g] != [%.200g, %.200g, %.200g]", a.x, a.y, a.z, b.x, b.y, b.z);
}
#define assert_eq_v3(a, b, ulps) _vassert_wrap(_assert_eq_v3, a, b, ulps)

static inline int _assert_closef(float a, float b, int ulps, _vassert_typed_args) {
	return vclosef(a, b, ulps) || _vassert_fail("%g != %g", a, b);
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
#define RSQRTF_ULPS 5000
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

static inline int _assert_eq_quat(quat_t a, quat_t b, int ulps, _vassert_typed_args) {
	return
		(vclosef(a.w, b.w, ulps) && vclosef(a.x, b.x, ulps) && vclosef(a.y, b.y, ulps) && vclosef(a.z, b.z, ulps)) ||
		_vassert_fail("[%.200g, %.200g, %.200g, %.200g] != [%.200g, %.200g, %.200g, %.200g]", a.w, a.x, a.y, a.z, b.w, b.x, b.y, b.z);
}
#define assert_eq_quat(a, b, ulps) _vassert_wrap(_assert_eq_quat, a, b, ulps)

VTEST(test_qinv) {
	vec3_t axis = v3norm_slow(vec3(1, 1, 1));
	quat_t q = qrot(axis, vradiansf(120));
	quat_t iq = qrot(axis, -vradiansf(120));
	assert_eq_quat(qinv(q), iq, 1);
	assert_eq_quat(qinv(iq), q, 1);
}

VTEST(test_qmul) {
	quat_t r = quat(1, 0, 0, 0);
	quat_t i = quat(0, 1, 0, 0);
	quat_t j = quat(0, 0, 1, 0);
	quat_t k = quat(0, 0, 0, 1);

	// real * basis elements
	assert_eq_quat(qmul(r, r), r, 0);
	assert_eq_quat(qmul(r, i), i, 0);
	assert_eq_quat(qmul(r, j), j, 0);
	assert_eq_quat(qmul(r, k), k, 0);

	// basis elements * real
	assert_eq_quat(qmul(i, r), i, 0);
	assert_eq_quat(qmul(j, r), j, 0);
	assert_eq_quat(qmul(k, r), k, 0);

	// i^2 = j^2 = k^2 = ijk = -1
	assert_eq_quat(qmul(i, i), qneg(r), 0);
	assert_eq_quat(qmul(j, j), qneg(r), 0);
	assert_eq_quat(qmul(k, k), qneg(r), 0);
	assert_eq_quat(qmul3(i, j, k), qneg(r), 0);

	// basis elements * basis elements
	assert_eq_quat(qmul(i, j), k, 0);
	assert_eq_quat(qmul(i, k), qneg(j), 0);
	assert_eq_quat(qmul(j, i), qneg(k), 0);
	assert_eq_quat(qmul(j, k), i, 0);
	assert_eq_quat(qmul(k, i), j, 0);
	assert_eq_quat(qmul(k, j), qneg(i), 0);
}

VTEST(test_v3rot) {
	quat_t rot = quat(0.5, 0.5, 0.5, 0.5);

	vec3_t i = vec3(1, 0, 0);
	vec3_t j = vec3(0, 1, 0);
	vec3_t k = vec3(0, 0, 1);

	assert_eq_v3(v3rot(i, rot), j, 1);
	assert_eq_v3(v3rot(j, rot), k, 1);
	assert_eq_v3(v3rot(k, rot), i, 1);
}

VTEST(test_qeuler) {
	vec3_t rot = vec3(vradiansf(90), vradiansf(-90), vradiansf(90));

	assert_eq_quat(qeuler(rot, VGL_XYZ), quat(0, 1/sqrtf(2), 0, 1/sqrtf(2)), 5);
	assert_eq_quat(qeuler(rot, VGL_XZY), quat(1/sqrtf(2), 0, 0, 1/sqrtf(2)), 5);
	assert_eq_quat(qeuler(rot, VGL_YXZ), quat(1/sqrtf(2), 1/sqrtf(2), 0, 0), 5);
	assert_eq_quat(qeuler(rot, VGL_YZX), quat(0, 1/sqrtf(2), -1/sqrtf(2), 0), 5);
	assert_eq_quat(qeuler(rot, VGL_ZXY), quat(0, 0, -1/sqrtf(2), 1/sqrtf(2)), 5);
	assert_eq_quat(qeuler(rot, VGL_ZYX), quat(1/sqrtf(2), 0, -1/sqrtf(2), 0), 5);
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

	test_qinv,
	test_qmul,
	test_v3rot,
	test_qeuler,

	// TODO: test_load_farbfeld
	test_load_vmesh,
VTESTS_END
