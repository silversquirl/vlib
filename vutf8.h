/* vutf8.h
 *
 * Encoding and decoding algorithms for the one true Unicode encoding
 * Define VUTF8_IMPL in exactly one translation unit
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
#ifndef VUTF8_H
#define VUTF8_H

#include <stdint.h>

// Returns the length of a code point, or 0 if out of range
int vutf8_len(uint32_t cp);

// Writes a code point to the specified buffer
// Invalid code points are replaced by the Unicode replacement character U+FFFD
// buf must have enough remaining space to store the code point
// Returns buf + the number of code units written
char *vutf8_write(char *buf, uint32_t cp);

// Reads a code point from the specified buffer
// Invalid code unit sequences will be decoded as the Unicode replacement character U+FFFD
// If cp is non-null, the decoded code point will be written to it
// This function is safe to call on any null-terminated buf
// Returns a pointer to the start of the next code point
char *vutf8_next(const char *buf, uint32_t *cp);

// Backtracks to the previous code point
// If cp is non-null, the decoded code point will be written to it
// If start is non-null, the algorithm will consider it the start of the buffer and not progress past it
// Returns a pointer to the start of the previous code point
char *vutf8_prev(const char *buf, const char *start, uint32_t *cp);

#define VUTF8_REPLACEMENT 0xFFFD

#endif

#ifdef VUTF8_IMPL
#undef VUTF8_IMPL

int vutf8_len(uint32_t cp) {
	if (cp > 0x10FFFF) return 0; // Invalid
	if (cp > 0xFFFF) return 4;
	if (cp > 0x7FF) return 3;
	if (cp > 0x7F) return 2;
	return 1;
}

char *vutf8_write(char *buf, uint32_t cp) {
	int l = vutf8_len(cp);
	if (!l) {
		cp = VUTF8_REPLACEMENT;
		l = vutf8_len(cp);
	}

	if (l == 1) {
		*buf++ = cp;
		return buf;
	}

	*buf = (0xf0 << (4-l)) & 0xff;

	*buf++ |= cp >> (6*--l);
	while (l) *buf++ = 0x80 | ((cp >> (6*--l)) & 0x3f);

	return buf;
}

char *vutf8_next(const char *buf, uint32_t *cp) {
	// Replace invalid bytes
	if ((*buf & 0xC0) == 0x80) {
		buf++;
		*cp = VUTF8_REPLACEMENT;
		return (char *)buf;
	}

	// Compute the length of the sequence
	int l = 1;
	if ((*buf & 0xE0) == 0xC0) l = 2;
	else if ((*buf & 0xF0) == 0xE0) l = 3;
	else if ((*buf & 0xF8) == 0xF0) l = 4;

	// Single-byte characters are easy
	if (l == 1) {
		if (cp) *cp = *buf;
		return (char *)++buf;
	}

	// Decode multi-byte sequence
	if (cp) *cp = *buf++ & (0x7f>>l);
	while (--l) {
		if ((*buf & 0xC0) != 0x80) {
			// Invalid sequence
			if (cp) *cp = VUTF8_REPLACEMENT;
			return (char *)buf;
		}

		if (cp) {
			*cp <<= 6;
			*cp |= *buf & 0x3f;
		}

		buf++;
	}

	return (char *)buf;
}

char *vutf8_prev(const char *buf, const char *start, uint32_t *cp) {
	const char *init_buf = buf;

	// Attempt to backtrack to the start of the previous code point
	do {
		buf--;
		if (start && buf < start) {
			buf = init_buf - 1;
			if (buf < start) buf = start;
			if (cp) *cp = VUTF8_REPLACEMENT;
			return (char *)buf;
		}
	} while ((*buf & 0xC0) == 0x80);

	if (cp) vutf8_next(buf, cp);
	return (char *)buf;
}

#endif
