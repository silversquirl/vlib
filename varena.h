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
int varena_register_malloced(struct varena *arena, void *p);
#if defined(POSIX_C_SOURCE) && POSIX_C_SOURCE >= 200112L
int varena_register_mmapped(struct varena *arena, void *p, size_t len);
#endif
void *aalloc(struct varena **arena, size_t size);

#endif

#ifdef VARENA_IMPL

#include <stdlib.h>

#if __STDC_VERSION__ < 201112L
// This is a guess, but will be correct on most systems
#include <stdint.h>
typedef union {long double f; uintmax_t i;} max_align_t;
#endif

#define _varena_ceildiv(num, div) (((num) - 1) / (div) + 1)

// For all blocks not in use, block type is encoded like this:
//  p != 0, any size  - Normal block, no extra work  needed
//  p == 0, size == 0 - `data` stores a pointer to a malloced area, which must be freed
//  p == 0, size != 0 - `data` stores a pointer to a mmapped area, which must be munmapped
struct varena {
	unsigned p, size;
	struct varena *prev;
	max_align_t data[];
};

static struct varena *_varena_new(unsigned size) {
	struct varena *arena = malloc(offsetof(struct varena, data) + size*sizeof (max_align_t));
	if (!arena) return NULL;
	arena->p = 0;
	arena->size = size;
	arena->prev = NULL;
	return arena;
}

struct varena *varena_new(size_t size) {
	size -= offsetof(struct varena, data);
	size = _varena_ceildiv(size, sizeof (max_align_t));
	if ((unsigned)size != size) return NULL;
	return _varena_new(size);
}

static int _varena_register(struct varena *arena, void *p, size_t len) {
	struct varena *node = malloc(offsetof(struct varena, data) + sizeof p);
	if (!node) return -1;
	node->size = len;
	*(void **)node->data = p;

	node->prev = arena->prev;
	arena->prev = node;
	return 0;
}

int varena_register_malloced(struct varena *arena, void *p) {
	return _varena_register(arena, p, 0);
}

#if defined(POSIX_C_SOURCE) && POSIX_C_SOURCE >= 200112L
int varena_register_mmapped(struct varena *arena, void *p, size_t len) {
	if (!len) return -1;
	return _varena_register(arena, p, len);
}
#endif

void varena_free(struct varena *arena) {
	struct varena *prev;
	while (arena) {
		if (!arena->p) {
			if (!arena->size) {
				free(*(void **)arena->data);
			} else {
#if defined(POSIX_C_SOURCE) && POSIX_C_SOURCE >= 200112L
				munmap(*(void **)arena->data, arena->size);
#endif
			}
		}

		prev = arena->prev;
		free(arena);
		arena = prev;
	}
}

void *aalloc(struct varena **arena, size_t size) {
	if (!size) return NULL;

	size = _varena_ceildiv(size, sizeof (max_align_t));

	if (size > (*arena)->size) {
		void *p = malloc(size);
		if (!p) return NULL;
		varena_register_malloced(*arena, p);
		return p;
	}

	if ((*arena)->p + size > (*arena)->size) {
		struct varena *a = _varena_new((*arena)->size);
		if (!a) return NULL;

		a->prev = *arena;
		*arena = a;
		if ((*arena)->p + size > (*arena)->size) return NULL;
	}

	void *mem = (*arena)->data + (*arena)->p;
	(*arena)->p += size;
	return mem;
}

#endif
