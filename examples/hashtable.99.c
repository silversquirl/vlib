#include <stdio.h>
#define VTABLE_IMPL
#include "../vtable.h"

int main() {
	struct vtable *tbl = vtable_new(8);
	printf("foo:  %s\n", vtable_put(&tbl, "foo", "foo1"));
	printf("foo:  %s\n", vtable_put(&tbl, "foo", "foo2"));
	printf("foo:  %s\n", vtable_put(&tbl, "foo", "foo3"));
	printf("bar:  %s\n", vtable_put(&tbl, "bar", "bar1"));
	printf("foo:  %s\n", vtable_get(tbl, "foo"));
	printf("baz:  %s\n", vtable_put(&tbl, "baz", "baz1"));
	printf("quux: %s\n", vtable_put(&tbl, "quux", "quux1"));
	printf("frob: %s\n", vtable_put(&tbl, "frob", "frob1"));
	printf("foo:  %s\n", vtable_get(tbl, "foo"));
}
