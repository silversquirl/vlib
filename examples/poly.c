#include <SDL2/SDL.h>

#define V2DRAW_IMPL
#define V2COL_IMPL
#include "../v2col.h"
#include "../v2draw.h"
#include "../v2vec.h"

#define trace(msg, offset) exit((fprintf(stderr, "%s:%d(%s) %s\n", __FILE__, __LINE__ + (offset), __func__, (msg)), 1))
#define sdlerr(offset) trace(SDL_GetError(), offset)

int main() {
	SDL_Init(SDL_INIT_VIDEO);
	atexit(SDL_Quit);

	SDL_Window *win = SDL_CreateWindow("Polygon collision", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN);
	if (!win) sdlerr(-1);
	SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (!ren) sdlerr(-1);

	struct v2poly *poly1 = v2poly(v2v(-1, -1), v2v(0, 1), v2v(1, -1));
	struct v2poly *poly2 = v2poly(v2v(-1, 0), v2v(0, -1), v2v(1, 0));

	_Bool collides = v2col_poly2poly(poly1, poly2);

	SDL_Event ev;
	for (;;) {
		while (SDL_PollEvent(&ev)) {
			switch (ev.type) {
			case SDL_QUIT:
				return 0;

			case SDL_MOUSEMOTION:
				if (ev.motion.state & SDL_BUTTON_LMASK) {
					v2_move_poly(poly2, v2v(ev.motion.xrel/100.0, -ev.motion.yrel/100.0));
					collides = v2col_poly2poly(poly1, poly2);
				}
				break;
			}
		}

		SDL_SetRenderDrawColor(ren, 0, 0, 0, SDL_ALPHA_OPAQUE);
		SDL_RenderClear(ren);
		if (collides) {
			SDL_SetRenderDrawColor(ren, 255, 0, 255, SDL_ALPHA_OPAQUE);
		} else {
			SDL_SetRenderDrawColor(ren, 255, 255, 255, SDL_ALPHA_OPAQUE);
		}
		v2draw_poly(ren, poly1);
		v2draw_poly(ren, poly2);
		SDL_RenderPresent(ren);
	}
}
