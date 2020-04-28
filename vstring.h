#ifndef VSTRING_HDR_INCLUDED
#define VSTRING_HDR_INCLUDED

#ifndef VSTRING_SIZE_T
#	ifdef VSTRING_NO_STDLIB
#		define VSTRING_SIZE_T long unsigned int
#	else
#		include <stddef.h>
#		define VSTRING_SIZE_T size_t
#	endif
#endif

typedef char *vstring;

// Creation and deletion
vstring vs_new(const char *str);
vstring vs_new_n(const char *buf, VSTRING_SIZE_T len);
vstring vs_alloc(VSTRING_SIZE_T len);
void vs_free(vstring s);

// Querying
VSTRING_SIZE_T vs_len(vstring s);

// Modification
vstring vs_append(vstring s, vstring other);
vstring vs_append_c(vstring s, char *str);
vstring vs_append_n(vstring s, char *buf, VSTRING_SIZE_T len);
vstring vs_resize(vstring s, VSTRING_SIZE_T new_len);

#endif // VSTRING_HDR_INCLUDED

#if !defined(VSTRING_IMPL_CREATED) && defined(VSTRING_IMPL)
#define VSTRING_IMPL_CREATED

#ifndef VSTRING_ALLOC
#	include <stdlib.h>
#	define VSTRING_ALLOC realloc
#endif

#ifndef VSTRING_MEMCPY
#	ifdef VSTRING_NO_STDLIB
		void *_vstring_memcpy(void *dest, const void *src, VSTRING_SIZE_T n) {
			char *d = dest;
			const char *s = src;
			VSTRING_SIZE_T i;
			for (i = 0; i < n; i++) d[i] = s[i];
			return d;
		}
#		define VSTRING_MEMCPY _vstring_memcpy
#	else
#		include <string.h>
#		define VSTRING_MEMCPY memcpy
#	endif
#endif

#ifndef VSTRING_STRLEN
#	ifdef VSTRING_NO_STDLIB
		VSTRING_SIZE_T _vstring_strlen(char *str) {
			VSTRING_SIZE_T n = 0;
			while (str[n]) n++;
			return n;
		}
#		define VSTRING_STRLEN _vstring_strlen
#	else
#		include <string.h>
#		define VSTRING_STRLEN strlen
#	endif
#endif

// INTERNAL
static void _vs_set_len(vstring s, VSTRING_SIZE_T len) {
	((VSTRING_SIZE_T *)s)[-1] = len;
	s[len] = 0; // NUL terminate
}

// EXTERNAL
vstring vs_new(const char *str) {
	return vs_new_n(str, VSTRING_STRLEN(str));
}

vstring vs_new_n(const char *buf, VSTRING_SIZE_T len) {
	vstring s = vs_resize(NULL, len);
	VSTRING_MEMCPY(s, buf, len);
	return s;
}

vstring vs_alloc(VSTRING_SIZE_T len) {
	return vs_resize(NULL, len);
}

void vs_free(vstring s) {
	s -= sizeof (VSTRING_SIZE_T);
	s = VSTRING_ALLOC(s, 0); // GCC warns about unused result of realloc
}

VSTRING_SIZE_T vs_len(vstring s) {
	if (!s) return 0;
	return ((VSTRING_SIZE_T *)s)[-1];
}

vstring vs_append(vstring s, vstring other) {
	return vs_append_n(s, other, vs_len(other));
}
vstring vs_append_c(vstring s, char *str) {
	return vs_append_n(s, str, VSTRING_STRLEN(str));
}
vstring vs_append_n(vstring s, char *buf, VSTRING_SIZE_T len) {
	VSTRING_SIZE_T oldl = vs_len(s);
	s = vs_resize(s, oldl + len);
	VSTRING_MEMCPY(s + oldl, buf, len);
	return s;
}
vstring vs_resize(vstring s, VSTRING_SIZE_T len) {
	// Allocate rather than reallocating if s is NULL
	size_t alloc = len;
	if (s) s -= sizeof (VSTRING_SIZE_T);
	alloc += sizeof (VSTRING_SIZE_T);
	alloc++; // NUL terminator

	s = VSTRING_ALLOC(s, alloc);
	s += sizeof (VSTRING_SIZE_T);
	_vs_set_len(s, len);
	return s;
}


#endif // VSTRING_IMPL
