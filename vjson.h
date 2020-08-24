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

enum vjson_type vjson_value(const char **src, const char *end);
enum vjson_type vjson_key(const char **src, const char *end);
enum vjson_type vjson_item(const char **src, const char *end);

enum vjson_type vjson_number(const char **src, const char *end);
enum vjson_type vjson_bool(const char **src, const char *end);
enum vjson_type vjson_null(const char **src, const char *end);
enum vjson_type vjson_string(const char **src, const char *end);
enum vjson_type vjson_array(const char **src, const char *end);
enum vjson_type vjson_object(const char **src, const char *end);

// Skip the opening delimiter of an array or object
const char *vjson_enter(const char *src);

long double vjson_get_number(const char *src);
_Bool vjson_get_bool(const char *src);
char *vjson_get_string(const char *src);
// Get the number of items in an array or object
size_t vjson_get_size(const char *src);

#endif

#ifdef VJSON_IMPL
#undef VJSON_IMPL

#include <stdlib.h>
#include <string.h>

static inline void _vjson_whitespace(const char **src, const char *end) {
	while (*src < end && strchr(" \t\n", **src)) {
		++*src;
	}
}

static _Bool _vjson_keyword(const char **src, const char *end, const char *kw) {
	size_t kwlen = strlen(kw);
	if (*src + kwlen >= end) return 0;
	if (strncmp(*src, kw, kwlen)) return 0;

	*src += kwlen;
	return 1;
}

static int _vjson_utf8_len(unsigned long cp) {
	if (cp > 0x10FFFF) return 0; // Invalid
	if (cp > 0xFFFF) return 4;
	if (cp > 0x7FF) return 3;
	if (cp > 0x7F) return 2;
	return 1;
}

static enum vjson_type _vjson_string(const char **src, const char *end, char **val) {
	if (**src != '"') return VJSON_ERROR;
	++*src;

	size_t slen = 0;
	const char *start = *src;

	while (**src != '"') {
		if (end && start >= end) return VJSON_ERROR;

		if (**src == '\\') {
			++*src;

			if (**src == 'u') {
				++*src;

				char *cpend;
				long cp = strtol(*src, &cpend, 16);
				if (*src == cpend) return VJSON_ERROR;
				size_t cp_strlen = cpend - *src - 1;

				*src += cp_strlen;

				size_t cplen = _vjson_utf8_len(cp);
				if (!cplen) return VJSON_ERROR;

				slen += cplen - 1;
			}
		}

		++*src;
		++slen;
	}

	// Skip ending quote
	++*src;

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
					char *cpend;
					long cp = strtol(start + 1, &cpend, 16);
					start = cpend - 1;

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

static _Bool _vjson_delimited(const char **src, const char *end, char sdelim, char edelim) {
	_vjson_whitespace(src, end);
	if (*src >= end) return 0;
	if (**src != sdelim) return 0;

	// Skip starting delimiter
	++*src;

	unsigned level = 1;
	while (level) {
		if (*src >= end) return 0;

		if (**src == '"') {
			vjson_string(src, end);
		} else {
			if (**src == sdelim) {
				level++;
			} else if (**src == edelim) {
				level--;
			}

			++*src;
		}
	}

	return 1;
}

enum vjson_type vjson_value(const char **src, const char *end) {
	_vjson_whitespace(src, end);
	if (*src >= end) return VJSON_EOF;

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
		return vjson_number(src, end);

	case 't':
	case 'f':
		return vjson_bool(src, end);

	case 'n':
		return vjson_null(src, end);

	case '"':
		return vjson_string(src, end);

	case '[':
		return vjson_array(src, end);

	case '{':
		return vjson_object(src, end);
	}

	return VJSON_ERROR;
}

enum vjson_type vjson_key(const char **src, const char *end) {
	enum vjson_type t = vjson_string(src, end);
	if (t == VJSON_ERROR || t == VJSON_EOF) return t;

	_vjson_whitespace(src, end);
	if (*src >= end) return VJSON_EOF;

	if (**src != ':') return VJSON_ERROR;
	++*src;

	return t;
}

enum vjson_type vjson_item(const char **src, const char *end) {
	enum vjson_type t = vjson_value(src, end);
	if (t == VJSON_ERROR || t == VJSON_EOF) return t;

	_vjson_whitespace(src, end);
	if (*src >= end) return VJSON_EOF;

	if (**src == ',') {
		++*src;

		_vjson_whitespace(src, end);
	} else if (**src != ']' && **src != '}') {
		return VJSON_ERROR;
	}

	return t;
}

enum vjson_type vjson_number(const char **src, const char *end) {
	_vjson_whitespace(src, end);
	if (*src >= end) return VJSON_ERROR;

	char *nend;
	strtold(*src, &nend);
	if (nend == *src) return VJSON_ERROR;
	*src = nend;

	return VJSON_NUMBER;
}

enum vjson_type vjson_bool(const char **src, const char *end) {
	_vjson_whitespace(src, end);

	if (_vjson_keyword(src, end, "true")) return VJSON_BOOL;
	if (_vjson_keyword(src, end, "false")) return VJSON_BOOL;
	return VJSON_ERROR;
}

enum vjson_type vjson_null(const char **src, const char *end) {
	_vjson_whitespace(src, end);

	if (_vjson_keyword(src, end, "null")) return VJSON_NULL;
	return VJSON_ERROR;
}

enum vjson_type vjson_string(const char **src, const char *end) {
	_vjson_whitespace(src, end);
	if (*src >= end) return VJSON_ERROR;
	return _vjson_string(src, end, NULL);
}

enum vjson_type vjson_array(const char **src, const char *end) {
	return _vjson_delimited(src, end, '[', ']') ? VJSON_ARRAY : VJSON_ERROR;
}

enum vjson_type vjson_object(const char **src, const char *end) {
	return _vjson_delimited(src, end, '{', '}') ? VJSON_OBJECT : VJSON_ERROR;
}

const char *vjson_enter(const char *src) {
	while (strchr(" \t\n", *src)) src++;
	src++;
	while (strchr(" \t\n", *src)) src++;
	return src;
}

long double vjson_get_number(const char *src) {
	while (strchr(" \t\n", *src)) src++;
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

size_t vjson_get_size(const char *src) {
	while (strchr(" \t\n", *src)) src++;

	// Skip starting delimiter
	src++;

	while (strchr(" \t\n", *src)) src++;
	if (*src == ']' || *src == '}') return 0;

	size_t count = 1;
	unsigned level = 1;
	while (level) {
		if (*src == '"') {
			_vjson_string(&src, NULL, NULL);
		} else {
			if (*src == '[' || *src == '{') {
				level++;
			} else if (*src == ']' || *src == '}') {
				level--;
			} else if (level == 1 && *src == ',') {
				count++;
			}

			src++;
		}
	}

	return count;
}

#endif
