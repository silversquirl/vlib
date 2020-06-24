/* vgl.h
 *
 * OpenGL helper library
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

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

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
	const char *data;
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

static inline GLfloat v3dot(vec3_t a, vec3_t b) {
	return a.x*b.x + a.y*b.y + a.z*b.z;
}

static inline vec3_t v3norm(vec3_t v) {
	GLfloat mag2 = v3dot(v, v);
	if (mag2 == 0.0f || mag2 == 1.0f) return v;
	GLfloat inv_mag = rsqrtf(mag2);
	return v3sop(v, *, inv_mag);
}

static inline vec3_t v3cross(vec3_t a, vec3_t b) {
	return vec3(
		a.x*b.y - a.y*b.x,
		a.y*b.z - a.z*b.y,
		a.z*b.x - a.x*b.z
	);
}
// }}}

// 4x4 matrix {{{
typedef union {
	GLfloat m[4][4];
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
	for (int x = 0; x < 4; x++) {
		for (int y = 0; y < 4; y++) {
			a.m[y][x] = 0;
			for (int k = 0; k < 4; k++) {
				m.m[y][x] += a.m[k][x] * b.m[y][k];
			}
		}
	}
	return m;
}

// `dir` must be normalized
mat44_t vgl_look(vec3_t pos, vec3_t dir, vec3_t up);
mat44_t vgl_perspective(GLfloat fov, GLfloat aspect, GLfloat near, GLfloat far);
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

#endif

#ifdef VGL_IMPL
#undef VGL_IMPL

const char *vgl_strerror(void) {
	switch (glGetError()) {
	case GL_NO_ERROR:
		break;

#define _vgl_pe_error(err) case GL_##err: return #err;
	_vgl_pe_error(INVALID_ENUM);
	_vgl_pe_error(INVALID_VALUE);
	_vgl_pe_error(INVALID_OPERATION);
	_vgl_pe_error(INVALID_FRAMEBUFFER_OPERATION);
	_vgl_pe_error(OUT_OF_MEMORY);
	_vgl_pe_error(STACK_UNDERFLOW);
	_vgl_pe_error(STACK_OVERFLOW);
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
		fprintf(stderr, "Error compiling GLSL shader:\n%*s\n", log_len, buf);
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

#endif
