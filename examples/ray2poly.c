#define ALL_MOVEMENT
#define MOVE_CONST
#define SET
#include "collision_example.h"

struct v2ray ray = {v2v(0,0), v2v(0, 0)};
struct v2poly *poly;
v2v intersection = NAN;

void init(void) {
	poly = v2poly(v2v(-1, -1), v2v(0, 1), v2v(1, -1));
}

void move(v2v v) {
	ray.direction = v - ray.start;
}

void set(v2v v) {
	ray.start = v;
	ray.direction = v2v(0, 0);
}

_Bool collide(void) {
	v2s lambda = v2ray2poly(ray, poly);
	if (isfinite(lambda)) {
		intersection = ray.start + ray.direction*lambda;
		return 1;
	} else {
		intersection = NAN;
		return 0;
	}
}

void draw(SDL_Renderer *ren) {
	v2draw_ray(ren, ray);
	v2draw_poly(ren, poly);
	if (!v2nan(intersection)) {
		v2draw_point(ren, intersection);
	}
}
