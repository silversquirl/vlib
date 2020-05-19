#define MTV
#include "collision_example.h"

struct v2poly *poly1;
struct v2poly *poly2;

void init(void) {
	poly1 = v2poly(v2v(-1, -1), v2v(0, 1), v2v(1, -1));
	poly2 = v2poly(v2v(-1, 0), v2v(0, -1), v2v(1, 0));
}

void move(v2v v) {
	v2_move_poly(poly2, v);
}

v2v collide(void) {
	return v2poly2poly(poly1, poly2);
}

v2v mtv_origin(void) {
	v2v origin = 0;
	for (unsigned i = 0; i < poly2->sides; i++) {
		origin += poly2->points[i];
	}
	origin /= poly2->sides;
	return origin;
}

void draw(SDL_Renderer *ren) {
	v2draw_poly(ren, poly1);
	v2draw_poly(ren, poly2);
}
