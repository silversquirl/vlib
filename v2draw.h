/*
 * v2draw.h
 *
 * Debug drawing functions for v2col types. Depends on v2col.h, v2vec.h and SDL2, requires C11 or higher.
 *
 * In one source file, define V2DRAW_IMPL before including this header.
 *
 */
#ifndef V2DRAW_H
#define V2DRAW_H

#include "SDL2/SDL.h"
#include "v2col.h"

#ifndef V2DRAW_SCALE
#define V2DRAW_SCALE 100
#endif

void v2draw_circ(SDL_Renderer *ren, struct v2circ circ);
void v2draw_poly(SDL_Renderer *ren, struct v2poly *poly);

#endif

#ifdef V2DRAW_IMPL
#undef V2DRAW_IMPL

#include <complex.h>

#ifdef V2_SINGLE_PRECISION
#define _v2conj conjf
#else
#define _v2conj conj
#endif

void v2draw_circ(SDL_Renderer *ren, struct v2circ circ) {
	int w, h;
	SDL_GetRendererOutputSize(ren, &w, &h);
	v2v translate = v2v(w, h) * 0.5;

	int rad = circ.radius * V2DRAW_SCALE;
	v2v center = _v2conj(circ.center) * V2DRAW_SCALE + translate;

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

	int w, h;
	SDL_GetRendererOutputSize(ren, &w, &h);
	v2v translate = v2v(w, h) * 0.5;

	for (unsigned i = 0; i < poly->sides; i++) {
		v2v point = _v2conj(poly->points[i]) * V2DRAW_SCALE + translate;
		points[i] = (SDL_Point){v2x(point), v2y(point)};
	}
	points[count-1] = points[0];

	SDL_RenderDrawLines(ren, points, count);
}

#endif
