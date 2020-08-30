#define VARENA_IMPL
#include "vtest.h"
#include "../varena.h"

VTEST(test_allocation) {
	struct varena *arena = varena_new(4096);
	vassert_not_null(arena);

	vassert_not_null(aalloc(&arena, 100));

	varena_free(arena);
}

VTEST(test_large_allocation) {
	struct varena *arena = varena_new(4096);
	vassert_not_null(arena);

	vassert_not_null(aalloc(&arena, 100000));

	varena_free(arena);
}

VTEST(test_multiple_allocations) {
	struct varena *arena = varena_new(4096);
	vassert_not_null(arena);

	for (int i = 0; i < 100; i++) {
		vassert_not_null(aalloc(&arena, 100));
	}

	varena_free(arena);
}

VTEST(test_multiple_large_allocations) {
	struct varena *arena = varena_new(4096);
	vassert_not_null(arena);

	for (int i = 0; i < 100; i++) {
		vassert_not_null(aalloc(&arena, 100000));
		vassert_not_null(aalloc(&arena, 100));
	}

	varena_free(arena);
}

VTESTS_BEGIN
	test_allocation,
	test_large_allocation,
	test_multiple_allocations,
	test_multiple_large_allocations,
VTESTS_END
