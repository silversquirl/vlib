/* vdict.h
 *
 * A generic, ordered dictionary type inspired by Python's dict.
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
#ifndef VDICT_H
#define VDICT_H

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#define _vdict_message(fmt, ...) (fprintf(stderr, "%s: " fmt "%c", __func__, __VA_ARGS__), abort())
#ifndef VDICT_PANIC
#define VDICT_PANIC(...) (_vdict_message(__VA_ARGS__, '\n'), abort())
#endif

// Hash functions {{{
#define vhash_int(i) (i)
#define vhash_float(f) ((union {double f; uintmax_t i;}){f}.i)
static inline uintmax_t vhash_string(const char *s) {
	// djb2a
	uintmax_t hash = 5381;
	while (*s) hash = hash*33 ^ *s++;
	return hash;
}
// }}}

#define vdict_always(a, b) 1

#define vdict(name) struct _vdict__##name

#define vdict_decl(linkage, name, key_t, val_t) \
	vdict(name); \
	linkage vdict(name) *vdict_##name##_new(void); \
	linkage void vdict_##name##_free(vdict(name) *d); \
	linkage int vdict_##name##_set(vdict(name) *d, key_t k, val_t v); \
	linkage int vdict_##name##_del(vdict(name) *d, key_t k); \
	linkage val_t *vdict_##name##_get(vdict(name) *d, key_t k)

// For types where the hash function is guaranteed to be collision-free (eg. float, int), eq_func may be vdict_always
// For strings, it should be !strcmp or similar
// For other types, it should be a custom function for comparing the type
#define vdict_def(linkage, name, key_t, val_t, hash_func, eq_func) \
	vdict(name) { \
		struct _vdict_entry__##name { \
			uintmax_t hash; \
			_Bool removed; \
			key_t k; \
			val_t v; \
		} *entries; \
		uint32_t size, n_removed, e_capacity; \
		\
		/* Indices are offset by 1, with 0 representing an unused slot */ \
		uint32_t *indices; \
		uint32_t i_capacity; \
	}; \
	\
	linkage vdict(name) *vdict_##name##_new(void) { \
		vdict(name) *d = calloc(1, sizeof *d); \
		if (!d) return NULL; \
		d->e_capacity = 8; \
		d->entries = malloc(d->e_capacity * sizeof *d->entries); \
		if (!d->entries) { \
			free(d); \
			return NULL; \
		} \
		\
		d->i_capacity = 32; \
		d->indices = calloc(d->i_capacity, sizeof *d->indices); \
		if (!d->indices) { \
			free(d->entries); \
			free(d); \
			return NULL; \
		} \
		\
		return d; \
	} \
	\
	linkage void vdict_##name##_free(vdict(name) *d) { \
		free(d->entries); \
		free(d->indices); \
		free(d); \
	} \
	\
	static int _vdict_##name##_rehash(vdict(name) *d, uint32_t cap) { \
		uint32_t *indices = calloc(cap, sizeof *d->indices); \
		if (!indices) return 1; \
		int offset = 0; \
		for (uint32_t i = 0; i < d->size; i++) { \
			struct _vdict_entry__##name entry = d->entries[i]; \
			if (entry.removed) { \
				offset++; \
			} else { \
				uint32_t hash = entry.hash % cap; \
				while (indices[hash]) { \
					if (++hash == cap) hash = 0; \
				} \
				indices[hash] = i+1-offset; \
				d->entries[i-offset] = d->entries[i]; \
			} \
		} \
		d->size -= offset; \
		d->n_removed -= offset; \
		free(d->indices); \
		d->indices = indices; \
		d->i_capacity = cap; \
		return 0; \
	} \
	\
	static void _vdict_##name##_repack(vdict(name) *d) { \
		int offset = 0; \
		for (uint32_t i = 0; i < d->size; i++) { \
			struct _vdict_entry__##name entry = d->entries[i]; \
			if (entry.removed) { \
				offset++; \
			} else if (offset) { \
				uint32_t hash = entry.hash % d->i_capacity; \
				while (d->indices[hash] != i+1) { \
					if (++hash == d->i_capacity) hash = 0; \
				} \
				d->indices[hash] = i+1-offset; \
				d->entries[i-offset] = d->entries[i]; \
			} \
		} \
		d->size -= offset; \
		d->n_removed -= offset; \
	} \
	\
	static int _vdict_##name##_grow(vdict(name) *d, uint32_t count) { \
		uint32_t cap = d->e_capacity; \
		if (cap < d->size + count) { \
			cap *= 1 << (d->size + count - cap); \
			void *entries = realloc(d->entries, cap * sizeof *d->entries); \
			if (!entries) return 1; \
			d->entries = entries; \
			d->e_capacity = cap; \
		} \
		\
		cap = d->i_capacity; \
		if (cap/2 < d->size + count) { \
			cap *= 1 << (d->size + count - cap/2); \
			if (_vdict_##name##_rehash(d, cap)) return 1; \
		} \
		\
		return 0; \
	} \
	\
	static int _vdict_##name##_shrink(vdict(name) *d) { \
		uint32_t cap = d->e_capacity / 4; \
		if (cap > d->size) { \
			void *entries = realloc(d->entries, cap * sizeof *d->entries); \
			if (!entries) return 1; \
			d->entries = entries; \
			d->e_capacity = cap; \
		} \
		\
		cap = d->i_capacity / 4; \
		if (cap/4 > d->size) { \
			if (_vdict_##name##_rehash(d, cap)) return 1; \
		} else if (d->n_removed > d->e_capacity/4) { \
			_vdict_##name##_repack(d); \
		} \
		\
		return 0; \
	} \
	\
	linkage int vdict_##name##_set(vdict(name) *d, key_t k, val_t v) { \
		if (_vdict_##name##_grow(d, 1)) VDICT_PANIC("Failed to allocate memory"); \
		uintmax_t full_hash = hash_func(k); \
		uint32_t hash = full_hash % d->i_capacity; \
		while (d->indices[hash]) { \
			struct _vdict_entry__##name *entry = d->entries + d->indices[hash] - 1; \
			if (++hash == d->i_capacity) hash = 0; \
			if (full_hash != entry->hash) continue; \
			if (!(eq_func(k, entry->k))) continue; \
			entry->v = v; \
			return 0; \
		} \
		d->entries[d->size++] = (struct _vdict_entry__##name){full_hash, 0, k, v}; \
		d->indices[hash] = d->size; \
		return 1; \
	} \
	\
	static uint32_t *_vdict_##name##_get_entry(vdict(name) *d, key_t k) { \
		uintmax_t full_hash = hash_func(k); \
		uint32_t hash = full_hash % d->i_capacity; \
		while (d->indices[hash]) { \
			struct _vdict_entry__##name *entry = d->entries + d->indices[hash] - 1; \
			if (full_hash == entry->hash && (eq_func(k, entry->k))) { \
				return d->indices + hash; \
			} \
			if (++hash == d->i_capacity) hash = 0; \
		} \
		return NULL; \
	} \
	\
	linkage int vdict_##name##_del(vdict(name) *d, key_t k) { \
		uint32_t *idx = _vdict_##name##_get_entry(d, k); \
		if (!idx) return 0; \
		(d->entries + *idx - 1)->removed = 1; \
		*idx = 0; \
		d->n_removed++; \
		_vdict_##name##_shrink(d); \
		return 1; \
	} \
	\
	/* BEWARE: the pointer returned from this may be invalidated by calls to vdict_NAME_set or vdict_NAME_del */ \
	linkage val_t *vdict_##name##_get(vdict(name) *d, key_t k) { \
		uint32_t *idx = _vdict_##name##_get_entry(d, k); \
		if (!idx) return NULL; \
		return &(d->entries + *idx - 1)->v; \
	} \
	\
	typedef int _vdict_semicolon_forcer_##__LINE__

#endif
