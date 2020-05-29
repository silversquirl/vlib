/*
 * vtable.h
 *
 * Hash table for strings
 * You may change the hash function by defining VTABLE_HASH_FUNC. It defaults to djb2a
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
#ifndef VTABLE_H
#define VTABLE_H

struct vtable;
struct vtable *vtable_new(int start_size);
void *vtable_get(struct vtable *tbl, const char *key);
void *vtable_put(struct vtable **tbl, const char *key, void *val);

#endif

#ifdef VTABLE_IMPL
#undef VTABLE_IMPL

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef VTABLE_HASH_FUNC
static inline unsigned long _vtable_djb2a(const unsigned char *s) {
	unsigned long hash = 5381;
	int ch;
	while ((ch = *s++)) hash = hash*33 ^ ch;
	return hash;
}
#define VTABLE_HASH_FUNC(str) _vtable_djb2a((const unsigned char *)(str))
#endif

#ifndef VTABLE_THRESHOLD
#define VTABLE_THRESHOLD 2 // This is the ratio of total slots to used slots at which a rehash is performed
#endif

struct _vtable_slot {
	const char *key;
	void *val;
};

struct vtable {
	int size, occupied;
	struct _vtable_slot slots[];
};

struct vtable *vtable_new(int start_size) {
	struct vtable *tbl = calloc(1, offsetof(struct vtable, slots) + start_size * sizeof *tbl->slots);
	if (!tbl) return NULL;
	tbl->size = start_size;
	return tbl;
}

void vtable_printdebug(struct vtable *tbl) {
	int seg = 0;
	for (int i = 0; i < tbl->size; i++) {
		if (tbl->slots[i].key) {
			if (seg != i) {
				if (seg == i-1) {
					printf("[%d] EMPTY\n", seg);
				} else {
					printf("[%d..%d] EMPTY\n", seg, i-1);
				}
			}
			printf("[%d] '%s'\n", i, tbl->slots[i].key);
			seg = i+1;
		}
	}

	if (seg != tbl->size) {
		if (seg == tbl->size-1) {
			printf("[%d] EMPTY\n", seg);
		} else {
			printf("[%d..%d] EMPTY\n", seg, tbl->size-1);
		}
	}
}

void *vtable_get(struct vtable *tbl, const char *key) {
	unsigned long hash = VTABLE_HASH_FUNC(key);
	hash %= tbl->size;
#if VTABLE_THRESHOLD == 1
	// Threshold of 100% requires an additional check as all slots may be filled
	unsigned long start = hash;
#endif
	while (tbl->slots[hash].key) {
		if (!strcmp(key, tbl->slots[hash].key)) return tbl->slots[hash].val;
		if (++hash >= tbl->size) hash = 0;
#if VTABLE_THRESHOLD == 1
		if (hash == start) break;
#endif
	}
	return NULL;
}

static void *_vtable_put(struct vtable *tbl, struct _vtable_slot slot) {
	unsigned long hash = VTABLE_HASH_FUNC(slot.key);
	hash %= tbl->size;
	while (tbl->slots[hash].key) {
		if (!strcmp(tbl->slots[hash].key, slot.key)) return tbl->slots[hash].val;
		if (++hash >= tbl->size) hash = 0;
	}
	tbl->slots[hash] = slot;
	tbl->occupied++;
	return slot.val;
}

void *vtable_put(struct vtable **tbl, const char *key, void *val) {
	if ((*tbl)->size / VTABLE_THRESHOLD <= (*tbl)->occupied) {
		// Rehash
		struct vtable *new_tbl = vtable_new((*tbl)->size * (VTABLE_THRESHOLD < 2 ? 2 : VTABLE_THRESHOLD));
		if (!new_tbl) return NULL;
		for (int i = 0; i < (*tbl)->size; i++) {
			if (!(*tbl)->slots[i].key) continue;
			_vtable_put(new_tbl, (*tbl)->slots[i]);
		}
		free(*tbl);
		*tbl = new_tbl;
	}

	return _vtable_put(*tbl, (struct _vtable_slot){key, val});
}

#endif
