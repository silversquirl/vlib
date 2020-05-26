#define V2_IMPL
#include "../v2.h"

int main() {
	struct v2qt *t = v2qt(v2v(1024, 1024), 8, 5);
	v2qt_addbox(t, (struct v2box){
		v2v(0, 0),
		v2v(128, 128),
	});
	v2qt_addbox(t, (struct v2box){
		v2v(800, 600),
		v2v(810, 610),
	});

	v2qt_printranks(t);
	v2qt_printdiagram(t);
	return 0;
}
