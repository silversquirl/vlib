/* v.h
 *
 * General utility functions and macros for C
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
#ifndef V_H
#define V_H

// Some useful headers
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// Macro magic {{{
#define _v_argcount(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, x, ...) (x)
#define argcount(...) _v_argcount(__VA_ARGS__, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)

#define _v_splat(a, b) _v_splat2(a, b)
#define _v_splat2(a, b) a##b

#define _v_call(f, ...) f(__VA_ARGS__)

#define with(start, end) for (_Bool _v_with_flag = 0; !_v_with_flag; end) for (start; !_v_with_flag++;)
// }}}

// Debugging {{{
#if defined(__has_builtin) && __has_builtin(__builtin_debugtrap)
#define breakpoint() __builtin_debugtrap()
#elif __GNUC__ || (defined(__has_builtin) && __has_builtin(__builtin_trap))
#define breakpoint() __builtin_trap()
#else
#define breakpoint() raise(SIGTRAP)
#endif

#ifdef RELEASE
#	define _v_abort abort
#else
#	define _v_abort breakpoint
#endif

#ifdef RELEASE
#	define dif(cond) if (0)
#else
#	define dif(cond) if (cond)
#endif

#define _v_message(class, color, file, func, line, fmt, ...) fprintf(stderr, FG(color) "  %s:%d  (%s)\t " FGB(color) class SGR0 fmt "%c", file, line, func, __VA_ARGS__)
#define _v_panic(...) (_v_message(__VA_ARGS__), _v_abort())
#define panic(...) _v_panic("PANIC:  ", RED, __FILE__, __func__, __LINE__, __VA_ARGS__, '\n')

#ifdef RELEASE
#	define TODO _Pragma("GCC error \"TODO while compiling in release mode\"") _v_ERROR_TODO_IN_RELEASE_MODE
#	define DEBUG(...) _Pragma("GCC error \"DEBUG while compiling in release mode\"") _v_ERROR_DEBUG_IN_RELEASE_MODE
#else
#	define TODO _v_panic("TODO", GREEN, __FILE__, __func__, __LINE__, "", '\n')
#	define DEBUG(...) _v_message("DEBUG:  ", GREEN, __FILE__, __func__, __LINE__, __VA_ARGS__, '\n')
#endif

#define assert(cond, ...) do { dif (!(cond)) _v_panic("ASSERT: ", YELLOW, __FILE__, __func__, __LINE__, __VA_ARGS__, '\n'); } while (0)
// }}}

// Compile-time assertions {{{
// Ensures a constant expression evaluates to non-zero at compile time
#if __STDC_VERSION__ >= 201112L
#	define ct_assert (void)_Static_assert
#else
#	define ct_assert(expr, msg) ((void)sizeof (struct { int a : (expr) ? 1 : -1; }))
#endif

// As above, but valid at top-level scope
#if __STDC_VERSION__ >= 201112L
#	define ct_tlassert _Static_assert
#else
#	define ct_tlassert(expr, msg) struct _v_splat(_v_ct_tlassert, __LINE__) { int a : (expr) ? 1 : -1; }
#endif

// Ensures an expression is constant
#define _v_ct(expr) (sizeof (struct {char a[(expr)];}){{0}}.a)
#define ct(expr) (_v_ct((expr) ? (expr) : 1) - !(expr))
// }}}

// Color {{{
#define SGR(n) "\x1b[" #n "m"
#define SGR0 SGR(0)
#define FG(color) _v_call(SGR, _v_splat(3, color))
#define BG(color) _v_call(SGR, _v_splat(4, color))
#define FGB(color) _v_call(SGR, _v_splat(9, color))
#define BGB(color) _v_call(SGR, _v_splat(10, color))

#define BLACK 0
#define RED 1
#define GREEN 2
#define YELLOW 3
#define BLUE 4
#define MAGENTA 5
#define CYAN 6
#define WHITE 7
// }}}

// Type-safeish casts {{{
// Safeishly cast a pointer to void *
#define ptrcast(p) ((struct {void *v;}){p}.v)
// Safeishly remove `const` from a pointer
#define unconst(p) ((void *)(struct {const void *v;}){p}.v)
// }}}

#define _v_alignup(x, a) (~(~(x)+1 & ~(a)+1) + 1)

static inline void *pagealloc(size_t size) {
	size_t pgsiz = sysconf(_SC_PAGESIZE);
	size = _v_alignup(size, pgsiz);
#if __STDC_VERSION__ >= 201112L
	return aligned_alloc(pgsiz, size);
#elif _POSIX_C_SOURCE >= 200112L
	void *mem;
	int err = posix_memalign(&mem, pgsiz, size)
	if (err) {
		errno = err;
		return NULL;
	}
	return mem;
#else
#	define pagealloc(size) _Pragma("GCC error \"No implementation for pagealloc on this platform\"") _v_ERROR_UNSUPPORTED_PLATFORM
	abort();
#endif
}

static inline void *mapfile(const char *fn, size_t *len) {
	int fd = open(fn, O_RDONLY);
	if (fd < 0) return NULL;

	struct stat st;
	if (fstat(fd, &st)) {
		close(fd);
		return NULL;
	}

	if (len) *len = st.st_size;
	void *mem = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	close(fd);
	if (mem == MMAP_FAILED) return NULL;
	return mem;
}

typedef unsigned char ubyte;

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))

#define make(type_or_val) malloc(sizeof (type_or_val))
// TODO: overflow detection
#define maken(count, type_or_val) malloc(count * sizeof (type_or_val))
#define makev(type, field, count) malloc(offsetof(type, field) + count*sizeof (*(type){}.field))
#define makez(type_or_val) calloc(1, sizeof (type_or_val))
#define makenz(count, type_or_val) calloc(count, sizeof (type_or_val))
#define makevz(type, field, count) calloc(1, offsetof(type, field) + count*sizeof (*(type){}.field))

#endif
