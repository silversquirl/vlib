#ifndef VDLIST_HDR_INCLUDED
#define VDLIST_HDR_INCLUDED

#ifndef VDLIST_SIZE_T
#	ifdef VDLIST_NO_STDLIB
#		define VDLIST_SIZE_T long unsigned int
#	else
#		include <stddef.h>
#		define VDLIST_SIZE_T size_t
#	endif
#endif

typedef struct vdnode {
	void *value;
	struct vdnode *prev, *next;
} *vdnode;

typedef struct vdlist {
	vdnode first, last;
} vdlist;

// Creation and deletion
vdlist vdl_new(void);
vdlist vdl_single(void *value);
void vdl_free(vdlist l);
vdnode vdn_cons(vdnode prev, void *value, vdnode next);
void vdl_free_from(vdnode n);

// Traversal
VDLIST_SIZE_T vdl_len(const vdlist l);
VDLIST_SIZE_T vdl_len_from(const vdnode n);

// Modification
void vdl_appendl(vdlist *l, vdlist other);
void vdl_appendr(vdlist *l, vdlist other);
void vdl_reverse(vdlist *l);

#endif // VDLIST_HDR_INCLUDED

#if !defined(VDLIST_IMPL_CREATED) && defined(VDLIST_IMPL)

#ifndef VDLIST_ALLOC
#	include <stdlib.h>
#	define VDLIST_ALLOC realloc
#endif

vdlist vdl_new(void) {
	return (vdlist){NULL, NULL};
}

vdlist vdl_single(void *value) {
	vdnode n = vdn_cons(NULL, value, NULL);
	return (vdlist){n, n};
}

vdlist vdl_free(vdlist l) {
	vdl_free_from(l->first);
	l->first = NULL;
	l->last = NULL;
}

vdnode vdn_cons(vdnode prev, void *value, vdnode next) {
	vdnode ret = VDLIST_ALLOC(NULL, sizeof *ret);
	ret->prev = prev;
	ret->next = next;
	return ret;
}

void vdl_free_from(vdnode n) {
	while (n) {
		vdnode tmp = n->next;
		n = VDLIST_ALLOC(n, 0);
		n = tmp;
	}
}

VDLIST_SIZE_T vdl_len(const vdlist l) {
	return vdl_len_from(l.first);
}

VDLIST_SIZE_T vdl_len_from(const vdnode n) {
	size_t len;
	for (len = 0; n; n = n->next) len++;
	return len;
}

void vdl_appendl(vdlist *l, vdlist other) {
	other.last->next = l->first;
	l->first.prev = other.last;
	l->first = other.first;
}

void vdl_appendr(vdlist *l, vdlist other) {
	other.first->prev = l->last;
	l->last.next = other.first;
	l->last = other.last;
}

void vdl_reverse(vdlist *l) {
	// Swap the links
	vdnode n = l->first;
	while (n) {
		vdnode tmp = n->next;
		n->next = n->prev;
		n->prev = tmp;
		n = tmp;
	}

	// Swap the ends
	n = l->first;
	l->first = l->last;
	l->last = n;
}

vdlist vdl_copy(const vdlist l) {
	vdlist ret = {NULL, NULL};
	vdnode n = l->first, tmp;
	tmp = ret->first = vdn_cons(NULL, n->value, NULL);
	while ((n = n->next)) {
		tmp->next = vdn_cons(tmp, n->value, NULL);
		tmp = tmp->next;
	}
	return ret;
}

// Same as vdl_copy, but starting at the end of l
vdlist vdl_reversed(const vdlist l) {
	vdlist ret = {NULL, NULL};
	vdnode n = l->last, tmp;
	tmp = ret->first = vdn_cons(NULL, n->value, NULL);
	while ((n = n->prev)) {
		tmp->next = vdn_cons(tmp, n->value, NULL);
		tmp = tmp->next;
	}
	return ret;
}

#endif // VDLIST_IMPL
