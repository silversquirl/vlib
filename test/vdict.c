#include <string.h>
#define VMATH_IMPL
#include "../vmath.h"
#include "vtest.h"

#define VDICT_NAME vdict_s2s
#define VDICT_KEY const char *
#define VDICT_VAL const char *
#define VDICT_HASH vdict_hash_string
#define VDICT_EQUAL vdict_eq_string
#define VDICT_IMPL
#include "../vdict.h"

#define VDICT_NAME vdict_i2i
#define VDICT_KEY uint32_t
#define VDICT_VAL uint32_t
#define VDICT_HASH vdict_hash_int
#define VDICT_EQUAL vdict_eq_int
#define VDICT_IMPL
#include "../vdict.h"

struct vdict_s2s *d = NULL;
struct vdict_i2i *di = NULL;

VTEST(test_new) {
	d = vdict_s2s_new();
	vassert_not_null(d);

	di = vdict_i2i_new();
	vassert_not_null(di);
}

VTEST(test_put) {
	if (!d) vskip();

	vassert_eq(vdict_s2s_put(d, "foo", "bar"), 0);
	vassert_eq(vdict_s2s_put(d, "bar", "quux"), 0);

	vassert_eq(vdict_s2s_put(d, "bar", "frob"), 1);
	vassert_eq(vdict_s2s_put(d, "foo", "baz"), 1);
}

VTEST(test_del) {
	if (!d) vskip();

	const char *v;
	vassert_eq(vdict_s2s_put(d, "delete me", "sekrit data"), 0);
	vassert(vdict_s2s_get(d, "delete me", &v));
	vassert_eq_s(v, "sekrit data");

	vassert(vdict_s2s_del(d, "delete me", &v));
	vassert_eq_s(v, "sekrit data");
	vassertn(vdict_s2s_get(d, "delete me", NULL));

	vassertn(vdict_s2s_del(d, "delete me", NULL));
	vassertn(vdict_s2s_get(d, "delete me", NULL));
}

#define get_expect(k, v) do { \
		const char *s = NULL; \
		vassert(vdict_s2s_get(d, k, &s)); \
		if (vassert_msg(s, "d[\"%s\"] == NULL", k)) { \
			vassert_eq_s(s, v); \
		} \
	} while (0)
VTEST(test_get) {
	if (!d) vskip();

	get_expect("foo", "baz");
	get_expect("bar", "frob");
}

VTEST(test_rehash) {
	if (!d) vskip();

	vassert_eq(d->ecap_e, 4);
	vassert_eq(d->mcap_e, 5);

	vassert_eq(vdict_s2s_put(d, "1", "one"), 0);
	vassert_eq(vdict_s2s_put(d, "2", "two"), 0);
	vassert_eq(vdict_s2s_put(d, "3", "three"), 0);
	vassert_eq(vdict_s2s_put(d, "4", "four"), 0);
	vassert_eq(vdict_s2s_put(d, "5", "five"), 0);
	vassert_eq(vdict_s2s_put(d, "6", "six"), 0);
	vassert_eq(vdict_s2s_put(d, "7", "seven"), 0);
	vassert_eq(vdict_s2s_put(d, "8", "eight"), 0);

	get_expect("1", "one");
	get_expect("2", "two");
	get_expect("3", "three");
	get_expect("4", "four");
	get_expect("5", "five");
	get_expect("6", "six");
	get_expect("7", "seven");
	get_expect("8", "eight");

	vassert_eq(d->ecap_e, 4);
	vassert_eq(d->mcap_e, 5);

	vassert_eq(vdict_s2s_put(d, "9", "nine"), 0);
	vassert_eq(vdict_s2s_put(d, "10", "ten"), 0);
	vassert_eq(vdict_s2s_put(d, "11", "eleven"), 0);
	vassert_eq(vdict_s2s_put(d, "12", "twelve"), 0);
	vassert_eq(vdict_s2s_put(d, "13", "thirteen"), 0);
	vassert_eq(vdict_s2s_put(d, "14", "fourteen"), 0);
	vassert_eq(vdict_s2s_put(d, "15", "fifteen"), 0);
	vassert_eq(vdict_s2s_put(d, "16", "sixteen"), 0);

	get_expect("1", "one");
	get_expect("2", "two");
	get_expect("3", "three");
	get_expect("4", "four");
	get_expect("5", "five");
	get_expect("6", "six");
	get_expect("7", "seven");
	get_expect("8", "eight");

	get_expect("9", "nine");
	get_expect("10", "ten");
	get_expect("11", "eleven");
	get_expect("12", "twelve");
	get_expect("13", "thirteen");
	get_expect("14", "fourteen");
	get_expect("15", "fifteen");
	get_expect("16", "sixteen");

	vassert_eq(d->ecap_e, 5);
	vassert_eq(d->mcap_e, 6);
}

VTEST(test_iter) {
	vskip();
#if 0
	const char *order[] = {
		"foo", "baz",
		"bar", "frob",

		"1", "one",
		"2", "two",
		"3", "three",
		"4", "four",
		"5", "five",
		"6", "six",
		"7", "seven",
		"8", "eight",

		"9", "nine",
		"10", "ten",
		"11", "eleven",
		"12", "twelve",
		"13", "thirteen",
		"14", "fourteen",
		"15", "fifteen",
		"16", "sixteen",
		NULL, NULL,
	};
	const char **p = order;

	vdict_iter (s2s, d, const char *k, const char *v) {
		if (!*p) {
			vfail("Reached end of array too early");
			return;
		}

		vassert_eq_s(k, *p++);
		vassert_eq_s(v, *p++);
	}

	if (*p) {
		vfail("Reached end of dict too early");
		return;
	}
#endif
}

// Property-based/PRNG-driven tests {{{
enum {
	PROP_RANDOM_SEED = 1,
	PROP_ITER_COUNT = 5000,
};

VTEST(test_put_prop) {
	struct vmath_rand r = vmath_srand(PROP_RANDOM_SEED);
	for (int i = 0; i < PROP_ITER_COUNT; i++) {
		uint32_t k = vmath_rand32(&r);
		uint32_t v = vmath_rand32(&r);

		// Ensure key isn't in dict already
		vassertn(vdict_i2i_get(di, k, NULL));

		// Ensure put inserts
		vassert_eq(vdict_i2i_put(di, k, v), 0);

		// Ensure value matches
		uint32_t v2;
		vassert(vdict_i2i_get(di, k, &v2));
		vassert_eq(v, v2);
	}
}

VTEST(test_get_prop) {
	struct vmath_rand r = vmath_srand(PROP_RANDOM_SEED);
	for (int i = 0; i < PROP_ITER_COUNT; i++) {
		uint32_t k = vmath_rand32(&r);
		uint32_t v = vmath_rand32(&r);

		// Ensure values match put values
		uint32_t v2;
		vassert(vdict_i2i_get(di, k, &v2));
		vassert_eq(v, v2);
	}

	for (int i = 0; i < PROP_ITER_COUNT; i++) {
		uint32_t k = vmath_rand32(&r);

		// Ensure never-used keys are not in the dict
		vassertn(vdict_i2i_get(di, k, NULL));
	}
}

VTEST(test_del_prop) {
	struct vmath_rand r = vmath_srand(PROP_RANDOM_SEED);
	for (int i = 0; i < PROP_ITER_COUNT; i++) {
		uint32_t k = vmath_rand32(&r);
		uint32_t v = vmath_rand32(&r);

		// Ensure deletion succeeds
		uint32_t v2;
		vassert(vdict_i2i_del(di, k, &v2));
		// Ensure value matches
		vassert_eq(v, v2);
		// Ensure value does not exist any more
		vassertn(vdict_i2i_get(di, k, NULL));
	}

	// Double ensure values do not exist any more
	r = vmath_srand(PROP_RANDOM_SEED);
	for (int i = 0; i < PROP_ITER_COUNT; i++) {
		uint32_t k = vmath_rand32(&r);
		uint32_t v = vmath_rand32(&r);

		vassertn(vdict_i2i_get(di, k, NULL));
	}
}
// }}}

VTEST(test_free) {
	if (!d && !di) vskip();

	vdict_s2s_free(d);
	d = NULL;

	vdict_i2i_free(di);
	di = NULL;
}

VTESTS_BEGIN
	test_new,

	// Simple case-by-case tests
	test_put,
	test_del,
	test_get,
	test_rehash,
	//test_iter,

	// Property-based tests
	test_put_prop,
	test_get_prop,
	test_del_prop,

	test_free,
VTESTS_END
