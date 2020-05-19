#include <stdio.h>
#define V2_SINGLE_PRECISION
#include "../v2.h"

// nextafterf exists, but seems to not do what I expected, so fuck it I'm writing my own
// Needless to say, this has a lot of UB in it
float nextfloat(float f) {
	int i = 0;
	memcpy(&i, &f, sizeof f);
	i++;
	memcpy(&f, &i, sizeof f);
	return f;
}

float nextfloat2(float f) {
	union { float f; int i; } u = {f};
	u.i++;
	return u.f;
}

int main() {
	v2s numbers[] = {
		1.0f,
		//16777216,
		1e20f,
		1e-20f,
		NAN
	};

	for (int i = 0; !v2nan(numbers[i]); i++) {
		v2s a = numbers[i], b = nextfloat(a);
		printf("%g == %g: %d %d\n", a, b, a == b, v2close(a, b, 1));
	}
	return 0;
}
