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
#ifndef VSLIST_HDR_INCLUDED
#define VSLIST_HDR_INCLUDED

#ifndef VSLIST_SIZE_T
#	ifdef VSLIST_NO_STDLIB
#		define VSLIST_SIZE_T long unsigned int
#	else
#		include <stddef.h>
#		define VSLIST_SIZE_T size_t
#	endif
#endif

typedef struct vslist {
	void *head;
	struct vslist *tail;
} *vslist;

// Creation and deletion
vslist vsl_cons(void *head, vslist tail);
void vsl_free(vslist l);

// Traversal
VSLIST_SIZE_T vsl_len(const vslist l);

// Modification
void vsl_append(vslist l, vslist next);
vslist vsl_reverse(vslist l);

// Copying operations
vslist vsl_copy(const vslist l);
vslist vsl_reversed(const vslist l);

#endif // VSLIST_HDR_INCLUDED

#if !defined(VSLIST_IMPL_CREATED) && defined(VSLIST_IMPL)

#ifndef VSLIST_ALLOC
#	include <stdlib.h>
#	define VSLIST_ALLOC realloc
#endif

vslist vsl_cons(void *head, vslist tail) {
	vslist ret = VSLIST_ALLOC(NULL, sizeof *ret);
	ret->head = head;
	ret->tail = tail;
	return ret;
}

void vsl_free(vslist l) {
	while (l) {
		vslist tmp = l->tail;
		l = VSLIST_ALLOC(l, 0);
		l = tmp;
	}
}

VSLIST_SIZE_T vsl_len(const vslist l) {
	size_t len;
	for (len = 0; l; l = l->tail) len++;
	return len;
}

void vsl_append(vslist l, vslist next) {
	while (l->tail) l = l->tail;
	l->tail = next;
}

vslist vsl_reverse(vslist l) {
	vslist prev = NULL;
	while (l) {
		vslist tmp = l->tail;
		l->tail = prev;
		prev = l;
		l = tmp;
	}
	return prev;
}

vslist vsl_copy(const vslist l) {
	if (!l) return NULL;
	vslist ret = vsl_cons(l->head, NULL), tmp = ret;
	while ((l = l->tail)) {
		tmp->next = vsl_cons(l->head, NULL);
		tmp = tmp->next;
	}
	return ret;
}

vslist vsl_reversed(const vslist l) {
	vslist prev = NULL;
	while (l) {
		prev = vsl_cons(l->head, prev);
		l = l->tail;
	}
	return prev;
}

#endif // VSLIST_IMPL
