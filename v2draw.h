/*
 * v2draw.h
 *
 * Debug drawing functions for v2 types. Depends on v2.h and SDL2, requires C11 or higher.
 *
 * In one source file, define V2DRAW_IMPL before including this header.
 *
 */
#ifndef V2DRAW_H
#define V2DRAW_H

#include "SDL2/SDL.h"
#include "v2.h"

#ifndef V2DRAW_SCALE
#define V2DRAW_SCALE 100
#endif
#ifndef V2DRAW_POINT_SIZE
#define V2DRAW_POINT_SIZE 8
#endif

void v2draw_circ(SDL_Renderer *ren, struct v2circ circ);
void v2draw_poly(SDL_Renderer *ren, struct v2poly *poly);
void v2draw_ray(SDL_Renderer *ren, struct v2ray ray);

#endif

#ifdef V2DRAW_IMPL
#undef V2DRAW_IMPL

#include <complex.h>

v2v v2draw_translation(SDL_Renderer *ren) {
	int w, h;
	SDL_GetRendererOutputSize(ren, &w, &h);
	return v2v(w, h) * 0.5;
}

void v2draw_circ(SDL_Renderer *ren, struct v2circ circ) {
	int rad = circ.radius * V2DRAW_SCALE;
	v2v center = v2conj(circ.center) * V2DRAW_SCALE + v2draw_translation(ren);

	// Midpoint circle algorithm stolen from https://en.wikipedia.org/wiki/Midpoint_circle_algorithm#C_example
	int x0 = v2x(center);
	int y0 = v2y(center);
	int x = rad - 1;
	int y = 0;
	int dx = 1;
	int dy = 1;
	int diam = rad*2;
	int err = dx - diam;

	while (x >= y) {
		SDL_RenderDrawPoint(ren, x0 + x, y0 + y);
		SDL_RenderDrawPoint(ren, x0 + y, y0 + x);
		SDL_RenderDrawPoint(ren, x0 - y, y0 + x);
		SDL_RenderDrawPoint(ren, x0 - x, y0 + y);
		SDL_RenderDrawPoint(ren, x0 - x, y0 - y);
		SDL_RenderDrawPoint(ren, x0 - y, y0 - x);
		SDL_RenderDrawPoint(ren, x0 + y, y0 - x);
		SDL_RenderDrawPoint(ren, x0 + x, y0 - y);

		if (err <= 0) {
			y++;
			err += dy;
			dy += 2;
		} else {
			x--;
			dx += 2;
			err += dx - diam;
		}
	}
}

void v2draw_poly(SDL_Renderer *ren, struct v2poly *poly) {
	unsigned count = poly->sides+1;
	SDL_Point points[count];

	v2v translate = v2draw_translation(ren);
	for (unsigned i = 0; i < poly->sides; i++) {
		v2v point = v2conj(poly->points[i]) * V2DRAW_SCALE + translate;
		points[i] = (SDL_Point){v2x(point), v2y(point)};
	}
	points[count-1] = points[0];

	SDL_RenderDrawLines(ren, points, count);
}

void v2draw_ray(SDL_Renderer *ren, struct v2ray ray) {
	int w, h;
	SDL_GetRendererOutputSize(ren, &w, &h);
	v2v translate = v2v(w, h) * 0.5;

	v2v a = v2conj(ray.start) * V2DRAW_SCALE + translate;
	v2v b = v2conj(ray.start + ray.direction * (w+h)) * V2DRAW_SCALE + translate;

	SDL_RenderDrawLine(ren, v2x(a), v2y(a), v2x(b), v2y(b));
}

void v2draw_point(SDL_Renderer *ren, v2v v) {
	v = v2conj(v) * V2DRAW_SCALE + v2draw_translation(ren);
	SDL_Rect r = {v2x(v)-V2DRAW_POINT_SIZE/2, v2y(v)-V2DRAW_POINT_SIZE/2, V2DRAW_POINT_SIZE, V2DRAW_POINT_SIZE};
	SDL_RenderFillRect(ren, &r);
}

#endif
