/* vjson.h
 *
 * Define VJSON_IMPL in one translation unit
 */

/*
 * This is free and unencumbered software released into the public domain.
 *
 * Anyone is free to copy, modify, publish, use, compile, sell, or distribute
 * this software, either in source code form or as a compiled binary, for any
 * purpose, commercial or non-commercial, and by any means.
 *
 * In jurisdictions that recognize copyright laws, the author or authors of
 * this software dedicate any and all copyright interest in the software to the
 * public domain. We make this dedication for the benefit of the public at
 * large and to the detriment of our heirs and successors. We intend this
 * dedication to be an overt act of relinquishment in perpetuity of all present
 * and future rights to this software under copyright law.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * For more information, please refer to <http://unlicense.org/>
 */
#ifndef VJSON_H
#define VJSON_H

#include <stddef.h>

enum vjson_type {
	VJSON_ERROR,
	VJSON_EOF,

	VJSON_NUMBER,
	VJSON_BOOL,
	VJSON_NULL,
	VJSON_STRING,

	VJSON_ARRAY,
	VJSON_OBJECT,
};

enum vjson_type vjson_value(const char **src, size_t *len);
enum vjson_type vjson_number(const char **src, size_t *len);
enum vjson_type vjson_bool(const char **src, size_t *len);
enum vjson_type vjson_null(const char **src, size_t *len);
enum vjson_type vjson_string(const char **src, size_t *len);
enum vjson_type vjson_array(const char **src, size_t *len);
enum vjson_type vjson_object(const char **src, size_t *len);

long double vjson_get_number(const char *src);
_Bool vjson_get_bool(const char *src);
char *vjson_get_string(const char *src);

#endif

#ifdef VJSON_IMPL
#undef VJSON_IMPL

#include <stdlib.h>
#include <string.h>

static inline void _vjson_whitespace(const char **src, size_t *len) {
	while (len && strchr(" \t\n", **src)) {
		++*src;
		--*len;
	}
}

static _Bool _vjson_keyword(const char **src, size_t *len, const char *kw) {
	size_t kwlen = strlen(kw);
	if (*len < kwlen) return 0;
	if (strncmp(*src, kw, kwlen)) return 0;

	*src += kwlen;
	*len -= kwlen;
	return 1;
}

static int _vjson_utf8_len(unsigned long cp) {
	if (cp > 0x10FFFF) return 0; // Invalid
	if (cp > 0xFFFF) return 4;
	if (cp > 0x7FF) return 3;
	if (cp > 0x7F) return 2;
	return 1;
}

static enum vjson_type _vjson_string(const char **src, size_t *len, char **val) {
	if (**src != '"') return VJSON_ERROR;
	++*src;
	if (len) --*len;

	size_t slen = 0;
	const char *start = *src;

	while (**src != '"') {
		if (len && !*len) return VJSON_ERROR;

		if (**src == '\\') {
			++*src;
			if (len) --*len;

			if (**src == 'u') {
				++*src;
				if (len) --*len;

				char *end;
				long cp = strtol(*src, &end, 16);
				if (*src == end) return VJSON_ERROR;
				size_t cp_strlen = end - *src - 1;

				*src += cp_strlen;
				if (len) *len -= cp_strlen;

				size_t cplen = _vjson_utf8_len(cp);
				if (!cplen) return VJSON_ERROR;

				slen += cplen - 1;
			}
		}

		++*src;
		if (len) --*len;
		++slen;
	}

	// Skip ending quote
	++*src;
	if (len) --*len;

	if (val) {
		char *p = malloc(slen + 1);
		p[slen] = 0;
		*val = p;

		while (*start != '"') {
			if (*start == '\\') {
				++start;
				switch (*start) {
				case 'b':
					*p++ = '\b';
					break;

				case 'f':
					*p++ = '\f';
					break;

				case 'n':
					*p++ = '\n';
					break;

				case 'r':
					*p++ = '\r';
					break;

				case 't':
					*p++ = '\t';
					break;

				case 'u':;
					char *end;
					long cp = strtol(start + 1, &end, 16);
					start = end - 1;

					// Encode codepoint as UTF-8
					size_t cplen = _vjson_utf8_len(cp);
					if (cplen == 1) {
						*p++ = cp;
					} else {
						*p = (0xf0 << (4-cplen)) & 0xff;
						*p++ |= cp >> (6*--cplen);
						while (cplen) *p++ = 0x80 | ((cp >> (6*--cplen)) & 0x3f);
					}
					break;

				default:
					*p++ = *start;
					break;
				}
			} else {
				*p++ = *start;
			}

			++start;
		}
	}

	return VJSON_STRING;
}

static _Bool _vjson_delimited(const char **src, size_t *len, char start, char end) {
	_vjson_whitespace(src, len);
	if (!*len) return 0;
	if (**src != start) return 0;

	// Skip starting delimiter
	++*src;
	--*len;

	unsigned level = 1;
	while (level) {
		if (!*len) return 0;

		if (**src == '"') {
			vjson_string(src, len);
		} else {
			if (**src == start) {
				level++;
			} else if (**src == end) {
				level--;
			}

			++*src;
			--*len;
		}
	}

	// Skip ending delimiter
	++*src;
	--*len;

	return 1;
}

enum vjson_type vjson_value(const char **src, size_t *len) {
	_vjson_whitespace(src, len);
	if (!*len) return VJSON_EOF;

	switch (**src) {
	case '-':
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		return vjson_number(src, len);

	case 't':
	case 'f':
		return vjson_bool(src, len);

	case 'n':
		return vjson_null(src, len);

	case '"':
		return vjson_string(src, len);

	case '[':
		return vjson_array(src, len);

	case '{':
		return vjson_object(src, len);
	}

	return VJSON_ERROR;
}

enum vjson_type vjson_number(const char **src, size_t *len) {
	_vjson_whitespace(src, len);
	if (!*len) return VJSON_ERROR;

	char *end;
	strtold(*src, &end);
	if (end == *src) return VJSON_ERROR;
	*len -= end - *src;
	*src = end;

	return VJSON_NUMBER;
}

enum vjson_type vjson_bool(const char **src, size_t *len) {
	_vjson_whitespace(src, len);

	if (_vjson_keyword(src, len, "true")) return VJSON_BOOL;
	if (_vjson_keyword(src, len, "false")) return VJSON_BOOL;
	return VJSON_ERROR;
}

enum vjson_type vjson_null(const char **src, size_t *len) {
	_vjson_whitespace(src, len);

	if (_vjson_keyword(src, len, "null")) return VJSON_NULL;
	return VJSON_ERROR;
}

enum vjson_type vjson_string(const char **src, size_t *len) {
	_vjson_whitespace(src, len);
	if (!*len) return VJSON_ERROR;
	return _vjson_string(src, len, NULL);
}

enum vjson_type vjson_array(const char **src, size_t *len) {
	return _vjson_delimited(src, len, '[', ']') ? VJSON_ARRAY : VJSON_ERROR;
}

enum vjson_type vjson_object(const char **src, size_t *len) {
	return _vjson_delimited(src, len, '{', '}') ? VJSON_OBJECT : VJSON_ERROR;
}

long double vjson_get_number(const char *src) {
	return strtold(src, NULL);
}

_Bool vjson_get_bool(const char *src) {
	while (strchr(" \t\n", *src)) src++;
	return *src == 't';
}

char *vjson_get_string(const char *src) {
	while (strchr(" \t\n", *src)) src++;
	char *val;
	_vjson_string(&src, NULL, &val);
	return val;
}

#endif
