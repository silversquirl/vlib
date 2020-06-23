#include <stdio.h>
#define VSET_IMPL
#include "../vset.h"

int main() {
	struct vset *set = vset_new(8); // A really quite small hash set
	printf("foo:  %2d\n", vset_put(&set, "foo"));
	printf("foo:  %2d\n", vset_put(&set, "foo"));
	printf("foo:  %2d\n", vset_put(&set, "foo"));
	printf("bar:  %2d\n", vset_put(&set, "bar"));
	printf("foo:  %2d\n", vset_get(set, "foo"));
	printf("baz:  %2d\n", vset_put(&set, "baz"));
	printf("quux: %2d\n", vset_put(&set, "quux"));
	printf("frob: %2d\n", vset_put(&set, "frob"));
	printf("foo:  %2d\n", vset_get(set, "foo"));
	return 0;
}
