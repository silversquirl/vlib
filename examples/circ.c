#define MTV
#include "collision_example.h"

struct v2circ circ1 = {v2v(0,0), 2};
struct v2circ circ2 = {v2v(2,1), .5};

void init(void) {}

void move(v2v v) {
	circ1.center += v;
}

v2v collide(void) {
	return v2circ2circ(circ1, circ2);
}

v2v mtv_origin(void) {
	return circ1.center;
}

void draw(SDL_Renderer *ren) {
	v2draw_circ(ren, circ1);
	v2draw_circ(ren, circ2);
}
