#define MTV
#include "collision_example.h"

struct v2circ circ = {v2v(0,0), .3};
struct v2poly *poly;

void init(void) {
	poly = v2poly(v2v(-1, -1), v2v(0, 1), v2v(1, -1));
}

void move(v2v v) {
	circ.center += v;
}

v2v collide(void) {
	return v2circ2poly(circ, poly);
}

v2v mtv_origin(void) {
	return circ.center;
}

void draw(SDL_Renderer *ren) {
	v2draw_circ(ren, circ);
	v2draw_poly(ren, poly);
}
