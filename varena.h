/* varena.h
 *
 * A simple arena-based allocator.
 * Define VARENA_IMPL in one translation unit.
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
#ifndef VARENA_H
#define VARENA_H

#include <stddef.h>

struct varena *varena_new(size_t size);
void varena_free(struct varena *arena);
void *aalloc(struct varena **arena, size_t size);

#endif

#ifdef VARENA_IMPL

#include <stdlib.h>

struct varena {
	unsigned p, size;
	struct varena *prev;
	unsigned char data[];
};

struct varena *varena_new(size_t size) {
	struct varena *arena = malloc(size);
	if (!arena) return NULL;
	arena->p = offsetof(struct varena, data);
	arena->size = size;
	arena->prev = NULL;
	return arena;
}

void varena_free(struct varena *arena) {
	struct varena *prev;
	while (arena) {
		prev = arena->prev;
		free(arena);
		arena = prev;
	}
}

static inline _Bool _varena_ok(struct varena *arena, size_t size) {
	return arena->p + size <= arena->size;
}

void *aalloc(struct varena **arena, size_t size) {
	if (size > (*arena)->size - offsetof(struct varena, data)) {
		return NULL;
	}

	if (!_varena_ok(*arena, size)) {
		struct varena *a = varena_new((*arena)->size);
		if (!a) return NULL;

		a->prev = (*arena);
		(*arena) = a;
		if (!_varena_ok(*arena, size)) return NULL;
	}

	void *mem = (*arena)->data + (*arena)->p;
	(*arena)->p += size;
	return mem;
}

#endif
