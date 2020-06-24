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
#ifndef VTEST_H
#define VTEST_H

#include <inttypes.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

// Color codes {{{
#define _vtest_color_black "0"
#define _vtest_color_red "1"
#define _vtest_color_green "2"
#define _vtest_color_yellow "3"
#define _vtest_color_blue "4"
#define _vtest_color_magenta "5"
#define _vtest_color_cyan "6"
#define _vtest_color_white "7"

#define _v_fg(color) "\x1b[3" _vtest_color_##color "m"
#define _v_fg_bright(color) "\x1b[9" _vtest_color_##color "m"
#define _v_bg(color) "\x1b[4" _vtest_color_##color "m"
#define _v_bg_bright(color) "\x1b[10" _vtest_color_##color "m"
#define _v_reset "\x1b[0m"
// }}}

#define _vmsg_in0(func, line, class_clr, class, fmt, ...) \
	fprintf(stderr, \
		_v_fg(class_clr) "[" class "]" \
		_v_fg(green) "   %s" \
		_v_fg(cyan) ":%d" \
		_v_reset " \t" fmt "%s", \
		func, line, \
		__VA_ARGS__)
#define _vmsg_in(...) _vmsg_in0(__VA_ARGS__, "\n")
#define _vmsg(...) _vmsg_in(__func__, __LINE__, __VA_ARGS__)

enum {
	VTEST_PASS,
	VTEST_FAIL,
	VTEST_SKIP,
};

#define vskip(...) do { _vmsg(cyan, "SKIP", "" __VA_ARGS__); *_vtest_status = VTEST_SKIP; return; } while (0)
#define _vfail_in(func, line, ...) (_vmsg_in(func, line, red, "FAIL", __VA_ARGS__), *_vtest_status = VTEST_FAIL, 0)
#define vfail(...) _vfail_in(__func__, __LINE__, __VA_ARGS__)

// Assertions {{{
#define vassert_msg(cond, ...) ((cond) || vfail(__VA_ARGS__))
#define vassert(cond) vassert_msg(cond, #cond " == false")
#define vassertn(cond) vassert_msg(!(cond), #cond " == true")
#define vassert_not_null(expr) vassert_msg((expr), #expr " == NULL")
#define vassert_null(expr) vassert_msg(!(expr), #expr " != NULL")

#if __STDC_VERSION__ >= 201112L
#define vassert_eq(a, b) _Generic((a), \
	signed char:  vassert_eq_i(a, b), \
	signed short: vassert_eq_i(a, b), \
	signed int:   vassert_eq_i(a, b), \
	signed long:  vassert_eq_i(a, b), \
	\
	unsigned char:  vassert_eq_u(a, b), \
	unsigned short: vassert_eq_u(a, b), \
	unsigned int:   vassert_eq_u(a, b), \
	unsigned long:  vassert_eq_u(a, b), \
	\
	float:       vassert_eq_f(a, b), \
	double:      vassert_eq_f(a, b), \
	long double: vassert_eq_f(a, b), \
	\
	default: _vassert_eq(a, b))
#else
#define vassert_eq _vassert_eq
#endif
#define _vassert_eq(a, b) vassert_msg((a) == (b), "%s != %s", #a, #b)

#define _vassert_wrap0(func, a, b, ...) func(a, b, __VA_ARGS__, #b, __func__, __LINE__, _vtest_status)
#define _vassert_wrap(func, a, ...) _vassert_wrap0(func, a, __VA_ARGS__, #a)
#define _vassert_fail0(detail, ...) _vfail_in(func, line, "%s != %s  (" detail "%c", as, bs, __VA_ARGS__)
#define _vassert_fail(...) _vassert_fail0(__VA_ARGS__, ')')

#define _vassert_typed_args const char *as, const char *bs, const char *func, int line, int *_vtest_status

static inline int _vassert_eq_i(intmax_t a, intmax_t b, _vassert_typed_args) {
	return a == b || _vassert_fail("%"PRIdMAX" != %"PRIdMAX, a, b);
}
#define vassert_eq_i(a, b) _vassert_wrap(_vassert_eq_i, a, b)

static inline int _vassert_eq_u(uintmax_t a, uintmax_t b, _vassert_typed_args) {
	return a == b || _vassert_fail("%"PRIuMAX" != %"PRIuMAX, a, b);
}
#define vassert_eq_u(a, b) _vassert_wrap(_vassert_eq_u, a, b)

static inline int _vassert_eq_f(long double a, long double b, _vassert_typed_args) {
	return a == b || _vassert_fail("%Lg != %Lg", a, b);
}
#define vassert_eq_f(a, b) _vassert_wrap(_vassert_eq_f, a, b)

static inline int _vassert_eq_s(const char *a, const char *b, _vassert_typed_args) {
	return !strcmp(a, b) || _vassert_fail("\"%s\" != \"%s\"", a, b);
}
#define vassert_eq_s(a, b) _vassert_wrap(_vassert_eq_s, a, b)
// }}}

typedef void (*vtest_func_t)(int *_vtest_status);
#define VTEST(name) void name(int *_vtest_status)
#define VTESTS_BEGIN \
	int main() { \
		vtest_func_t tests[] = {
#define VTESTS_END \
		0}; \
		int result[3] = {0}, total = 0; \
		for (vtest_func_t *func = tests; *func; func++) { \
			int status = VTEST_PASS; \
			(*func)(&status); \
			result[status]++; \
			total++; \
		} \
		if (result[VTEST_FAIL]) fputc('\n', stderr); \
		fprintf(stderr, \
			_v_reset "[TOTAL %d TESTS]\n" \
			_v_fg_bright(green) "%2d passed\n" \
			_v_fg_bright(red) "%2d failed\n" \
			_v_fg_bright(cyan) "%2d skipped\n" \
			_v_reset "\n", \
			total, \
			result[VTEST_PASS], \
			result[VTEST_FAIL], \
			result[VTEST_SKIP]); \
		return result[VTEST_FAIL]; \
	}

// Compile-time assertions {{{
#if __STDC_VERSION__ >= 201112L
#define _vtest_cassert _Static_assert
#else
#define _vmath_cassert(expr, msg) struct _vmath_cassert##__LINE__ { int a : (expr) ? 1 : -1; }
#endif

#if __STDC_VERSION__ >= 201112L
#define V_ENSURE_TYPE(expr, type) _Generic((expr), type: (void)0)
#else
#warning "Pre-C11, V_ENSURE_TYPE is quite inaccurate, as it can only check the size"
#define V_ENSURE_TYPE(expr, type) _vtest_cassert(sizeof (expr) == sizeof (type), "Size of types do not match")
#endif
// }}}

#endif
