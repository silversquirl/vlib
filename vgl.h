/* vgl.h
 *
 * OpenGL helper library. Requires GLFW3, GLEW, OpenGL 3.3 or greater and C11 or greater
 * Define VGL_IMPL in exactly one translation unit
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
#ifndef VGL_H
#define VGL_H

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "vmath.h"

#define vgl_perror() _vgl_perror(vgl_strerror(), __LINE__, __func__, __FILE__)
static inline int _vgl_perror(const char *err, int line, const char *func, const char *file) {
	if (!err) return 0;
	fprintf(stderr, "%s:%d (%s): %s\n", file, line, func, err);
	return 1;
}

const char *vgl_strerror(void);

// Context creation {{{
struct vgl_window_options {
	int width, height;
	const char *title;
	struct {int maj, min;} version; // OpenGL version, default 3.3
	int msaa; // MSAA samples, default 4, -1 to disable
	_Bool resizable; // Can the window be resized or not?
};
#define vgl_init(...) vgl_init1((struct vgl_window_options){__VA_ARGS__})
GLFWwindow *vgl_init1(struct vgl_window_options opts);
// }}}

// Keyword args helpers {{{
#define VGL_DEFAULT(opt, val) ((opt) ? (opt) : ((opt) = (val)))
#define VGL_DEFAULT_NULLABLE(opt, val) ((opt) < 0 ? 0 : VGL_DEFAULT(opt, val))
// }}}

// Memory-mapped files {{{
struct vgl_mbuf {
	size_t len;
	union {
		const char *data;
		const unsigned char *udata;
	};
};

struct vgl_mbuf vgl_mapfile(const char *fn);
void vgl_unmap(struct vgl_mbuf buf);
// }}}

// 3d vector {{{
typedef union {
	GLfloat v[3];
	struct {
		GLfloat x, y, z;
	};
} vec3_t;
#define vec3(x, y, z) ((vec3_t){{x, y, z}})

#define v3v3op(a, op, b) vec3((a).x op (b).x, (a).y op (b).y, (a).z op (b).z)
#define v3sop(v, op, s) vec3((v).x op s, (v).y op s, (v).z op s)

static inline vec3_t v3neg(vec3_t v) {
	return vec3(-v.x, -v.y, -v.z);
}

static inline GLfloat v3dot(vec3_t a, vec3_t b) {
	return a.x*b.x + a.y*b.y + a.z*b.z;
}

static inline vec3_t v3norm(vec3_t v) {
	GLfloat mag2 = v3dot(v, v);
	if (mag2 == 0.0f || mag2 == 1.0f) return v;
	GLfloat inv_mag = rsqrtf(mag2);
	return v3sop(v, *, inv_mag);
}

// Slow but accurate
static inline vec3_t v3norm_slow(vec3_t v) {
	GLfloat mag2 = v3dot(v, v);
	if (mag2 == 0.0f || mag2 == 1.0f) return v;
	GLfloat mag = sqrtf(mag2);
	return v3sop(v, /, mag);
}

static inline vec3_t v3cross(vec3_t a, vec3_t b) {
	return vec3(
		a.y*b.z - a.z*b.y,
		a.z*b.x - a.x*b.z,
		a.x*b.y - a.y*b.x
	);
}
// }}}

// 4x4 matrix {{{
typedef union {
	GLfloat m[4][4];
	GLfloat a[4*4];
	struct {
		GLfloat 
			a11, a12, a13, a14,
			a21, a22, a23, a24,
			a31, a32, a33, a34,
			a41, a42, a43, a44;
	};
} mat44_t;

static inline mat44_t m4mul(mat44_t a, mat44_t b) {
	mat44_t m;
	for (int row = 0; row < 4; row++) {
		for (int col = 0; col < 4; col++) {
			m.m[row][col] = 0;
			for (int k = 0; k < 4; k++) {
				m.m[row][col] += a.m[row][k] * b.m[k][col];
			}
		}
	}
	return m;
}

const mat44_t m4id = (mat44_t){{
	{1, 0, 0, 0},
	{0, 1, 0, 0},
	{0, 0, 1, 0},
	{0, 0, 0, 1},
}};

void m4print(mat44_t m);

// `dir` must be normalized
mat44_t vgl_look(vec3_t pos, vec3_t dir, vec3_t up);
#define vaspect(width, height) ((float)(width) / (float)(height))
mat44_t vgl_perspective(GLfloat fov, GLfloat aspect, GLfloat near, GLfloat far);
// }}}

// Quaternions {{{
typedef union {
	GLfloat q[4];
	struct {
		GLfloat w, x, y, z;
	};
} quat_t;
#define quat(w, x, y, z) ((quat_t){{w, x, y, z}})

enum vgl_euler_order {
	VGL_XYZ,
	VGL_XZY,
	VGL_YXZ,
	VGL_YZX,
	VGL_ZXY,
	VGL_ZYX,
};

static inline quat_t qneg(quat_t q) {
	return quat(-q.w, -q.x, -q.y, -q.z);
}

static inline quat_t qinv(quat_t q) {
	GLfloat fac = 1.0f / (q.w*q.w + q.x*q.x + q.y*q.y + q.z*q.z);
	return quat(fac * q.w, -fac * q.x, -fac * q.y, -fac * q.z);
}

static inline quat_t qmul(quat_t a, quat_t b) {
	// Hamilton product
	return quat(
		a.w*b.w - a.x*b.x - a.y*b.y - a.z*b.z,
		a.w*b.x + a.x*b.w + a.y*b.z - a.z*b.y,
		a.w*b.y - a.x*b.z + a.y*b.w + a.z*b.x,
		a.w*b.z + a.x*b.y - a.y*b.x + a.z*b.w
	);
}
#define qmul3(a, b, c) qmul(a, qmul(b, c))

static inline vec3_t v3rot(vec3_t v, quat_t rot) {
	quat_t q = qmul3(rot, quat(0, v.x, v.y, v.z), qinv(rot));
	return vec3(q.x, q.y, q.z);
}

static inline quat_t qrot(vec3_t axis, GLfloat angle) {
	angle *= 0.5f;
	GLfloat s = sin(angle), c = cos(angle);
	return quat(c, s*axis.x, s*axis.y, s*axis.z);
}

static inline quat_t qeuler(vec3_t angles, enum vgl_euler_order order) {
	quat_t x = qrot(vec3(1, 0, 0), angles.x);
	quat_t y = qrot(vec3(0, 1, 0), angles.y);
	quat_t z = qrot(vec3(0, 0, 1), angles.z);

	switch (order) {
	case VGL_XYZ:
		return qmul3(z, y, x);
	case VGL_XZY:
		return qmul3(y, z, x);
	case VGL_YXZ:
		return qmul3(z, x, y);
	case VGL_YZX:
		return qmul3(x, z, y);
	case VGL_ZXY:
		return qmul3(y, x, z);
	case VGL_ZYX:
		return qmul3(x, y, z);
	}
}
// }}}

// Shaders {{{
// If vert_len or frag_len is negative, the string is assumed to be null-terminated
GLuint vgl_shader_source(const char *vert_src, GLint vert_len, const char *frag_src, GLint frag_len);
GLuint vgl_shader_file(const char *vert_fn, const char *frag_fn);
// }}}

// Image loading {{{
struct vgl_image {
	unsigned width, height;
	GLushort *data;
};
struct vgl_image vgl_load_farbfeld_data(const char *data, size_t len);
struct vgl_image vgl_load_farbfeld(const char *fn);
// }}}

// Model loading {{{
enum vgl_mesh_flag {
	VGL_MESH_W = 0x01,
	VGL_MESH_NORMAL = 0x02,
	VGL_MESH_UV = 0x04,
	VGL_MESH_UVW = 0x08,
};

struct vgl_mesh {
	GLuint flag;
	GLsizei nvert, ntri;
	GLfloat *verts;
	union {
		GLuint *tris_hp; // High-poly, need to use int
		GLushort *tris_lp; // Low-poly, can use short
	};
};

enum {VGL_VMESH_VERSION = 1};

#define vgl_mesh_highpoly(mesh) ((mesh)->nvert > (GLuint)~(GLushort)0)
#define vgl_mesh_tri(mesh, idx) (vgl_mesh_highpoly(mesh) ? (mesh)->tris_hp[idx] : (mesh)->tris_lp[idx])
// counted in multiples of sizeof (GLfloat)
#define vgl_mesh_vert_size(mesh) _vgl_mesh_vert_size((mesh)->flag)
static inline size_t _vgl_mesh_vert_size(GLuint flag) {
	return
		3 +
		1*!!(flag & VGL_MESH_W) +
		3*!!(flag & VGL_MESH_NORMAL) +
		2*!!(flag & VGL_MESH_UV) +
		1*!!(flag & VGL_MESH_UVW);
}
void vgl_mesh_del(struct vgl_mesh *mesh);

struct vgl_gpu_mesh {
	GLuint flag;
	GLsizei nvert, nelem;
	GLuint array, elem;
};

struct vgl_gpu_layout {
	GLint vert, norm, uv;
};

void vgl_mesh_upload_to(struct vgl_mesh *mesh, struct vgl_gpu_mesh *gmesh);
struct vgl_gpu_mesh vgl_mesh_upload(struct vgl_mesh *mesh);
void vgl_mesh_draw(struct vgl_gpu_mesh gmesh, struct vgl_gpu_layout layout);

extern const char *vgl_mesherr;
struct vgl_mesh *vgl_load_vmesh_data(const unsigned char *data, size_t len);
struct vgl_mesh *vgl_load_vmesh(const char *fn);
// }}}

#endif

#ifdef VGL_IMPL
#undef VGL_IMPL

#include <fcntl.h>
#include <regex.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

const char *vgl_strerror(void) {
	switch (glGetError()) {
	case GL_NO_ERROR:
		break;

#define _vgl_match_error(err) case GL_##err: return #err;
	_vgl_match_error(INVALID_ENUM);
	_vgl_match_error(INVALID_VALUE);
	_vgl_match_error(INVALID_OPERATION);
	_vgl_match_error(INVALID_FRAMEBUFFER_OPERATION);
	_vgl_match_error(OUT_OF_MEMORY);
	_vgl_match_error(STACK_UNDERFLOW);
	_vgl_match_error(STACK_OVERFLOW);
	}
	return NULL;
}

// Context creation {{{
void _vgl_debug(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *msg, const void *data) {
	// TODO: make this a little prettier and more comprehensive
	fprintf(stderr, "GL debug: %s\n", msg);
}

GLFWwindow *vgl_init1(struct vgl_window_options opts) {
	if (!glfwInit()) return NULL;

	VGL_DEFAULT_NULLABLE(opts.msaa, 4);
	VGL_DEFAULT(opts.version.maj, 3);
	VGL_DEFAULT(opts.version.min, 3);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, opts.version.maj);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, opts.version.min);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SAMPLES, opts.msaa);
	glfwWindowHint(GLFW_RESIZABLE, opts.resizable);

	GLFWwindow *win = glfwCreateWindow(opts.width, opts.height, opts.title, NULL, NULL);;
	if (!win) {
		glfwTerminate();
		return NULL;
	}

	glewExperimental = 1;
	glfwMakeContextCurrent(win);
	if (glewInit() != GLEW_OK) {
		glfwTerminate();
		return NULL;
	}

	if (GLEW_ARB_debug_output) {
		puts("Enabling ARB_debug_output");
		glDebugMessageCallbackARB(_vgl_debug, NULL);
	}

	return win;
}
// }}}

// Memory-mapped files {{{
struct vgl_mbuf vgl_mapfile(const char *fn) {
	struct vgl_mbuf buf = {0};

	int fd = open(fn, O_RDONLY);
	if (fd < 0) return buf;

	struct stat st;
	if (fstat(fd, &st)) goto end;

	buf.len = st.st_size;
	buf.data = mmap(NULL, buf.len, PROT_READ, MAP_PRIVATE, fd, 0);

end:
	close(fd);
	return buf;
}

void vgl_unmap(struct vgl_mbuf buf) {
	munmap((void *)buf.data, buf.len);
}
// }}}

// 4x4 matrix {{{
void m4print(mat44_t m) {
	for (int row = 0; row < 4; row++) {
		for (int col = 0; col < 4; col++) {
			printf("%4.2g ", m.m[row][col]);
		}
		putchar('\n');
	}
}

// `dir` must be normalized
mat44_t vgl_look(vec3_t pos, vec3_t dir, vec3_t up) {
	// Stolen from cglm
	vec3_t s = v3norm(v3cross(dir, up));
	vec3_t u = v3cross(s, dir);

	return (mat44_t){{
		{s.x, u.x, -dir.x, 0},
		{s.y, u.y, -dir.y, 0},
		{s.z, u.z, -dir.z, 0},
		{-v3dot(s, pos), -v3dot(u, pos), v3dot(dir, pos), 1},
	}};
}

mat44_t vgl_perspective(GLfloat fov, GLfloat aspect, GLfloat near, GLfloat far) {
	// Stolen from cglm
	GLfloat f  = 1.0f / tanf(fov * 0.5f);
	GLfloat fn = 1.0f / (near - far);
	return (mat44_t){{
		{f / aspect, 0, 0, 0},
		{0, f, 0, 0},
		{0, 0, (near+far) * fn, -1.0f},
		{0, 0, 2.0f * near * far * fn, 0},
	}};
}
// }}}

// Shaders {{{
GLuint _vgl_compile_shader(const char *src, GLint len, GLenum type) {
	GLuint shad = glCreateShader(type);
	glShaderSource(shad, 1, &src, &len);
	glCompileShader(shad);

	GLint result, log_len;
	glGetShaderiv(shad, GL_COMPILE_STATUS, &result);
	glGetShaderiv(shad, GL_INFO_LOG_LENGTH, &log_len);

	if (log_len > 0) {
		char buf[log_len];
		glGetShaderInfoLog(shad, log_len, NULL, buf);
		fprintf(stderr, "Error compiling %s shader:\n%*s\n", type == GL_VERTEX_SHADER ? "vertex" : "fragment", log_len, buf);
	}

	if (!result) {
		glDeleteShader(shad);
		return 0;
	}

	return shad;
}

GLuint vgl_shader_source(const char *vert_src, GLint vert_len, const char *frag_src, GLint frag_len) {
	GLuint vert, frag;
	vert = _vgl_compile_shader(vert_src, vert_len, GL_VERTEX_SHADER); 
	if (!vert) return 0;
	frag = _vgl_compile_shader(frag_src, frag_len, GL_FRAGMENT_SHADER);
	if (!frag) {
		glDeleteShader(vert);
		return 0;
	}

	GLuint prog = glCreateProgram();
	glAttachShader(prog, vert);
	glAttachShader(prog, frag);
	glLinkProgram(prog);

	GLint result, log_len;
	glGetProgramiv(prog, GL_LINK_STATUS, &result);
	glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &log_len);

	if (log_len > 0) {
		char buf[log_len];
		glGetProgramInfoLog(prog, log_len, NULL, buf);
		fprintf(stderr, "Error linking GLSL shader:\n%*s\n", log_len, buf);
	}

	glDetachShader(prog, vert);
	glDetachShader(prog, frag);
	glDeleteShader(vert);
	glDeleteShader(frag);

	return result ? prog : 0;
}

GLuint vgl_shader_file(const char *vert_fn, const char *frag_fn) {
	struct vgl_mbuf vert = vgl_mapfile(vert_fn);
	if (!vert.data) return 0;

	struct vgl_mbuf frag = vgl_mapfile(frag_fn);
	if (!frag.data) {
		vgl_unmap(vert);
		return 0;
	}

	GLuint prog = vgl_shader_source(vert.data, vert.len, frag.data, frag.len);

	vgl_unmap(vert);
	vgl_unmap(frag);

	return prog;
}
// }}}

// Image loading {{{
struct vgl_image vgl_load_farbfeld_data(const char *bytes, size_t len) {
	struct vgl_image img = {0};

	// 8 - magic
	// 4 - width
	// 4 - height
	if (len < 8+4+4) return img;

	size_t i = 0;
	for (; i < 8; i++) {
		if (bytes[i] != "farbfeld"[i]) return img;
	}

	for (int j = 0; j < 4; j++) {
		img.width = (img.width<<8) | bytes[i];
		img.height = (img.height<<8) | bytes[4+i];
		i++;
	}
	i += 4;

	// 4 components per pixel, 2 bytes per component
	size_t computed_len = i + img.width*img.height*4*2;
	if (len < computed_len) return img;
	len = computed_len;

	img.data = calloc(len, sizeof *img.data);
	if (!img.data) return img;
	for (int j = 0; i+1 < len; j++) {
		img.data[j] = (bytes[i]<<8) | bytes[i+1];
		i += 2;
	}

	return img;
}

struct vgl_image vgl_load_farbfeld(const char *fn) {
	struct vgl_mbuf f = vgl_mapfile(fn);
	struct vgl_image img = vgl_load_farbfeld_data(f.data, f.len);
	vgl_unmap(f);
	return img;
}
// }}}

// Model loading {{{
void vgl_mesh_del(struct vgl_mesh *mesh) {
	free(mesh->verts);
	if (vgl_mesh_highpoly(mesh)) free(mesh->tris_hp);
	else free(mesh->tris_lp);
	free(mesh);
}

void vgl_mesh_upload_to(struct vgl_mesh *mesh, struct vgl_gpu_mesh *gmesh) {
	gmesh->flag = mesh->flag;

	glBindBuffer(GL_ARRAY_BUFFER, gmesh->array);
	glBufferData(GL_ARRAY_BUFFER, mesh->nvert * vgl_mesh_vert_size(mesh) * sizeof *mesh->verts, mesh->verts, GL_STATIC_DRAW);
	gmesh->nvert = mesh->nvert;

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gmesh->elem);
	if (vgl_mesh_highpoly(mesh)) {
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->ntri * 3 * sizeof *mesh->tris_hp, mesh->tris_hp, GL_STATIC_DRAW);
	} else {
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->ntri * 3 * sizeof *mesh->tris_lp, mesh->tris_lp, GL_STATIC_DRAW);
	}
	gmesh->nelem = 3 * mesh->ntri;
}

struct vgl_gpu_mesh vgl_mesh_upload(struct vgl_mesh *mesh) {
	GLuint bufs[2];
	glGenBuffers(2, bufs);

	struct vgl_gpu_mesh gmesh = {0};
	gmesh.array = bufs[0];
	gmesh.elem = bufs[1];

	vgl_mesh_upload_to(mesh, &gmesh);
	return gmesh;
}

void vgl_mesh_draw(struct vgl_gpu_mesh gmesh, struct vgl_gpu_layout layout) {
	GLsizei stride = vgl_mesh_vert_size(&gmesh) * sizeof (GLfloat);
	glBindBuffer(GL_ARRAY_BUFFER, gmesh.array);

	float *offset = 0;

	GLint size = 3 + !!(gmesh.flag & VGL_MESH_W);
	if (layout.vert != -1) {
		glEnableVertexAttribArray(layout.vert);
		glVertexAttribPointer(layout.vert, size, GL_FLOAT, GL_FALSE, stride, offset);
	}
	offset += size;

	if (gmesh.flag & VGL_MESH_NORMAL) {
		GLint size = 3;
		if (layout.norm != -1) {
			glEnableVertexAttribArray(layout.norm);
			glVertexAttribPointer(layout.norm, size, GL_FLOAT, GL_FALSE, stride, offset);
		}
		offset += size;
	}

	if (gmesh.flag & VGL_MESH_UV) {
		GLint size = 2 + !!(gmesh.flag & VGL_MESH_UVW);
		if (layout.uv != -1) {
			glEnableVertexAttribArray(layout.uv);
			glVertexAttribPointer(layout.uv, size, GL_FLOAT, GL_FALSE, stride, offset);
		}
		offset += size;
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gmesh.elem);
	glDrawElements(GL_TRIANGLES, gmesh.nelem, vgl_mesh_highpoly(&gmesh) ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT, 0);

	if (gmesh.flag & VGL_MESH_UV && layout.uv != -1) glDisableVertexAttribArray(layout.uv);
	if (gmesh.flag & VGL_MESH_NORMAL && layout.norm != -1) glDisableVertexAttribArray(layout.norm);
	if (layout.vert != -1) glDisableVertexAttribArray(layout.vert);
}

static inline GLfloat _vgl_pun_i2f(GLuint i) {
	float f;
#ifdef __STDC_IEC_559__
	f = (union {GLuint i; GLfloat f;}){i}.f;
#else
	// Layout of a IEEE 754 binary32 float:
	//  +------------+----------------+--------------------+
	//  | 1-bit sign | 8-bit exponent | 23-bit significand |
	//  +------------+----------------+--------------------+
	const int FRAC_BIT = 23;
	int sign = 1 + -2*(i>>31 & 1);
	int exp = (i>>FRAC_BIT & 0xff) - 127;
	int frac = 1<<FRAC_BIT | (i & (1<<FRAC_BIT)-1);
	f = ldexpf(sign * frac, exp - FRAC_BIT);
#endif
	return f;
}

const char *vgl_mesherr = NULL;
struct vgl_mesh *vgl_load_vmesh_data(const unsigned char *data, size_t len) {
	size_t i = 0;

#define _vgl_mesherr(msg) return (vgl_mesherr = (msg), NULL)

	// Verify header len
	if (len < 16) _vgl_mesherr("Invalid header: file too short");

	// Verify magic
	while (i < 6 && data[i] == "\x7fVMESH"[i]) i++;
	if (i < 6) _vgl_mesherr("Invalid header: bad magic");

	// Verify version
	if (data[i++] != VGL_VMESH_VERSION) _vgl_mesherr("Invalid header: unsupported version");

	// Parse flags
	GLuint flag = data[i++];

	// Parse vertex and triangle counts
	GLsizei nvert = 0, ntri = 0;
	for (int j = 0; j < 4; j++) nvert |= data[i++] << (8*j);
	for (int j = 0; j < 4; j++) ntri |= data[i++] << (8*j);

	// Verify file len
	size_t vert_size = _vgl_mesh_vert_size(flag);
	size_t hdr_len = 16;
	size_t vtab_len = nvert * 4*vert_size;
	size_t ttab_len = ntri * 4*3;
	if (len < hdr_len + vtab_len + ttab_len) _vgl_mesherr("Invalid data: file too short");

	// Create mesh struct
	struct vgl_mesh *mesh = malloc(sizeof *mesh);
	mesh->flag = flag;
	mesh->nvert = nvert;
	mesh->ntri = ntri;

	mesh->verts = malloc(nvert * vert_size * sizeof *mesh->verts);
	if vgl_mesh_highpoly(mesh) {
		mesh->tris_hp = malloc(ntri * 3 * sizeof *mesh->tris_hp);
	} else {
		mesh->tris_lp = malloc(ntri * 3 * sizeof *mesh->tris_lp);
	}

#define _vgl_u32(v, section) do { \
		if (i + 4 > len) { \
			free(mesh); \
			_vgl_mesherr("Invalid data: unexpected EOF in " section " table"); \
		} \
		(v) = 0; \
		for (int j = 0; j < 4; j++) { \
			(v) |= data[i++] << (CHAR_BIT*j); \
		} \
	} while (0)

#define _vgl_f32(v, section) do { \
		GLuint vali; \
		_vgl_u32(vali, section); \
		(v) = _vgl_pun_i2f(vali); \
	} while (0)

	// Parse vertex table
	for (GLsizei verti = 0; verti < nvert; verti++) {
		for (int comp = 0; comp < vert_size; comp++) {
			_vgl_f32(mesh->verts[vert_size*verti + comp], "vertex");
		}
	}

	// Parse triangle table
	for (GLsizei trii = 0; trii < ntri; trii++) {
		for (int verti = 0; verti < 3; verti++) {
			GLuint v;
			_vgl_u32(v, "triangle");
			if (vgl_mesh_highpoly(mesh)) {
				mesh->tris_hp[3*trii + verti] = v;
			} else {
				mesh->tris_lp[3*trii + verti] = v;
			}
		}
	}

	return mesh;
}

struct vgl_mesh *vgl_load_vmesh(const char *fn) {
	struct vgl_mbuf f = vgl_mapfile(fn);
	if (!f.data) {
		vgl_mesherr = strerror(errno);
		return NULL;
	}
	struct vgl_mesh *mesh = vgl_load_vmesh_data(f.udata, f.len);
	vgl_unmap(f);
	return mesh;
}
// }}}

#endif
