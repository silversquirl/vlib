#include <string.h>
#include "vtest.h"
#include "../vdict.h"

vdict_decl(static, s2s, const char *, const char *);
vdict_def(static, s2s, const char *, const char *, vhash_string, !strcmp);

vdict(s2s) *d = NULL;

VTEST(test_new) {
	d = vdict_s2s_new();
	vassert_not_null(d);
}

VTEST(test_set) {
	if (!d) vskip();

	vassert(vdict_s2s_set(d, "foo", "bar"));
	vassertn(vdict_s2s_set(d, "foo", "baz"));

	vassert(vdict_s2s_set(d, "bar", "quux"));
	vassertn(vdict_s2s_set(d, "bar", "frob"));
}

VTEST(test_del) {
	if (!d) vskip();

	vassert(vdict_s2s_set(d, "delete me", "sekrit data"));
	vassert_not_null(vdict_s2s_get(d, "delete me"));
	vassert(vdict_s2s_del(d, "delete me"));
	vassert_null(vdict_s2s_get(d, "delete me"));
	vassertn(vdict_s2s_del(d, "delete me"));
	vassert_null(vdict_s2s_get(d, "delete me"));
}

#define get_expect(k, v) do { \
		const char **s = vdict_s2s_get(d, k); \
		if (vassert_msg(s, "d[\"%s\"] == NULL", k)) { \
			vassert_eq_s(*s, v); \
		} \
	} while (0)
VTEST(test_get) {
	if (!d) vskip();

	get_expect("foo", "baz");
	get_expect("bar", "frob");
}

VTEST(test_rehash) {
	if (!d) vskip();

	vassert_eq(d->e_capacity, 8);
	vassert_eq(d->i_capacity, 32);

	vassert(vdict_s2s_set(d, "1", "one"));
	vassert(vdict_s2s_set(d, "2", "two"));
	vassert(vdict_s2s_set(d, "3", "three"));
	vassert(vdict_s2s_set(d, "4", "four"));
	vassert(vdict_s2s_set(d, "5", "five"));
	vassert(vdict_s2s_set(d, "6", "six"));
	vassert(vdict_s2s_set(d, "7", "seven"));
	vassert(vdict_s2s_set(d, "8", "eight"));

	get_expect("1", "one");
	get_expect("2", "two");
	get_expect("3", "three");
	get_expect("4", "four");
	get_expect("5", "five");
	get_expect("6", "six");
	get_expect("7", "seven");
	get_expect("8", "eight");

	vassert_eq(d->e_capacity, 16);
	vassert_eq(d->i_capacity, 32);

	vassert(vdict_s2s_set(d, "9", "nine"));
	vassert(vdict_s2s_set(d, "10", "ten"));
	vassert(vdict_s2s_set(d, "11", "eleven"));
	vassert(vdict_s2s_set(d, "12", "twelve"));
	vassert(vdict_s2s_set(d, "13", "thirteen"));
	vassert(vdict_s2s_set(d, "14", "fourteen"));
	vassert(vdict_s2s_set(d, "15", "fifteen"));
	vassert(vdict_s2s_set(d, "16", "sixteen"));

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

	vassert_eq(d->e_capacity, 32);
	vassert_eq(d->i_capacity, 64);
}

VTEST(test_repack) {
	if (!d) vskip();

	vassert_eq(d->n_removed, 0);

	get_expect("1", "one");
	vassert(vdict_s2s_del(d, "1"));
	get_expect("2", "two");
	vassert(vdict_s2s_del(d, "2"));
	get_expect("3", "three");
	vassert(vdict_s2s_del(d, "3"));
	get_expect("4", "four");
	vassert(vdict_s2s_del(d, "4"));

	vassert_eq(d->n_removed, 4);

	get_expect("5", "five");
	vassert(vdict_s2s_del(d, "5"));
	get_expect("6", "six");
	vassert(vdict_s2s_del(d, "6"));
	get_expect("7", "seven");
	vassert(vdict_s2s_del(d, "7"));
	get_expect("8", "eight");
	vassert(vdict_s2s_del(d, "8"));

	vassert_eq(d->n_removed, 8);

	get_expect("9", "nine");
	vassert(vdict_s2s_del(d, "9"));

	vassert_eq(d->n_removed, 0);

	get_expect("10", "ten");
	vassert(vdict_s2s_del(d, "10"));
	get_expect("11", "eleven");
	vassert(vdict_s2s_del(d, "11"));
	get_expect("12", "twelve");
	vassert(vdict_s2s_del(d, "12"));
	get_expect("13", "thirteen");
	vassert(vdict_s2s_del(d, "13"));
	get_expect("14", "fourteen");
	vassert(vdict_s2s_del(d, "14"));
	get_expect("15", "fifteen");
	vassert(vdict_s2s_del(d, "15"));
	get_expect("16", "sixteen");
	vassert(vdict_s2s_del(d, "16"));

	vassert_eq(d->n_removed, 7);
}

VTEST(test_free) {
	if (!d) vskip();
	vdict_s2s_free(d);
	d = NULL;
}

VTESTS_BEGIN
	test_new,
	test_set,
	test_del,
	test_get,
	test_rehash,
	test_repack,
	test_free,
VTESTS_END
