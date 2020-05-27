/*
 * vset.h
 *
 * Hash set for strings
 * You may change the hash function by defining VSET_HASH_FUNC. It defaults to djb2a
 *
 * Author: vktec
 *
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
#ifndef VSET_H
#define VSET_H

struct vset;
struct vset *vset_new(int start_size);
int vset_get(struct vset *set, const char *str);
int vset_put(struct vset **set, const char *str);

#endif

#ifdef VSET_IMPL
#undef VSET_IMPL

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#ifndef VSET_HASH_FUNC
static inline unsigned long _vset_djb2a(const unsigned char *s) {
	unsigned long hash = 5381;
	int ch;
	while ((ch = *s++)) hash = hash*33 ^ ch;
	return hash;
}
#define VSET_HASH_FUNC(str) _vset_djb2a((const unsigned char *)(str))
#endif

#ifndef VSET_THRESHOLD
#define VSET_THRESHOLD 2 // This is the ratio of total slots to used slots at which a rehash is performed
#endif

struct vset {
	int size, occupied;
	const char *slots[];
};

struct vset *vset_new(int start_size) {
	struct vset *set = calloc(1, offsetof(struct vset, slots) + start_size * sizeof *set->slots);
	if (!set) return NULL;
	set->size = start_size;
	return set;
}

int vset_get(struct vset *set, const char *str) {
	unsigned long hash = VSET_HASH_FUNC(str);
	hash %= set->size;
#if VSET_THRESHOLD == 1
	// Threshold of 100% requires an additional check as all slots may be filled
	unsigned long start = hash;
#endif
	while (set->slots[hash]) {
		if (!strcmp(str, set->slots[hash])) return hash;
		if (++hash > set->size) hash = 0;
#if VSET_THRESHOLD == 1
		if (hash == start) break;
#endif
	}
	return -1;
}

static int _vset_put(struct vset *set, const char *str) {
	unsigned long hash = VSET_HASH_FUNC(str);
	hash %= set->size;
	while (set->slots[hash]) {
		if (!strcmp(set->slots[hash], str)) return hash;
		if (++hash > set->size) hash = 0;
	}
	set->slots[hash] = str;
	set->occupied++;
	return hash;
}

int vset_put(struct vset **set, const char *str) {
	if ((*set)->size / VSET_THRESHOLD <= (*set)->occupied) {
		// Rehash
		struct vset *new_set = vset_new((*set)->size * (VSET_THRESHOLD < 2 ? 2 : VSET_THRESHOLD));
		if (!new_set) return -1;
		for (int i = 0; i < (*set)->size; i++) {
			if (!(*set)->slots[i]) continue;
			_vset_put(new_set, (*set)->slots[i]);
		}
		free(*set);
		*set = new_set;
	}

	return _vset_put(*set, str);
}

#endif
