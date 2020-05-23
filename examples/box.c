#include "collision_example.h"

struct v2circ circ;
struct v2poly *poly;
struct v2box box1;
struct v2box box2;

void init(void) {
	circ = (struct v2circ){v2v(2, 1), 1};
	poly = v2poly(v2v(-1, -1), v2v(-2, 0), v2v(-1, 1), v2v(0, 1), v2v(1, 0), v2v(0, -1));

	box1 = v2circbox(circ);
	box2 = v2polybox(poly);
}

void move(v2v v) {
	circ.center += v;
	box1.a += v;
	box1.b += v;
}

_Bool collide(void) {
	return v2box2box(box1, box2);
}

void draw(SDL_Renderer *ren) {
	v2draw_circ(ren, circ);
	v2draw_poly(ren, poly);
	v2draw_box(ren, box1);
	v2draw_box(ren, box2);
}
