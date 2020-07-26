#include <stdio.h>
#define VCO_IMPL
#include "../vco.h"

int co_a(int arg, void *data) {
	struct vco *c = data;

	for (int i = 0; i < 20; i += arg) {
		printf("%d (+%d)\n", i, arg);
		if (i % 2) vco_call(c, i);
		arg = vco_yield(1);
	}
	printf("i got too big (+%d)\n", arg);
	return 0;
}

int co_b(int arg, void *data) {
	struct vco *c = data;

	puts("a");
	vco_yield(1);
	puts("b");
	vco_yield(1);
	printf("b called c: ");
	int ret = vco_call(c, 1337);
	vco_yield(ret);
	puts("c");
	vco_yield(1);
	puts("d");
	vco_yield(1);
	puts("d");
	vco_yield(1);
	puts("e");
	vco_yield(1);
	puts("f");
	return 0;
}

int co_c(int arg, void *data) {
	for (;;) {
		printf("Hey guys c coroutine here, %d\n", arg);
		arg = vco_yield(1);
	}
}

int main() {
	struct vco *c = vco_new(co_c, NULL);
	struct vco *a = vco_new(co_a, c);
	struct vco *b = vco_new(co_b, c);

	for (int i = 0;; i++) {
		int aret = vco_call(a, i);
		int bret = vco_call(b, i);
		if (!aret) {
			puts("a done, exiting");
			break;
		} else if (!bret) {
			puts("b done, exiting");
			break;
		}
	}

	vco_del(a);
	vco_del(b);
	vco_del(c);
	return 0;
}
