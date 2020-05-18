#include <SDL2/SDL.h>

#define V2DRAW_IMPL
#define V2COL_IMPL
#include "../v2col.h"
#include "../v2draw.h"
#include "../v2vec.h"

#define trace(msg, offset) exit((fprintf(stderr, "%s:%d(%s) %s\n", __FILE__, __LINE__ + (offset), __func__, (msg)), 1))
#define sdlerr(offset) trace(SDL_GetError(), offset)

void init(void);
void move(v2v v);
void set(v2v v);
_Bool collide(void);
void draw(SDL_Renderer *ren);

int main() {
	SDL_Init(SDL_INIT_VIDEO);
	atexit(SDL_Quit);

	SDL_Window *win = SDL_CreateWindow("Polygon collision", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN);
	if (!win) sdlerr(-1);
	SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (!ren) sdlerr(-1);

	init();
	_Bool collides = collide();

	SDL_Event ev;
	for (;;) {
		while (SDL_PollEvent(&ev)) {
			switch (ev.type) {
			case SDL_QUIT:
				return 0;

			case SDL_MOUSEMOTION:
#ifndef ALL_MOVEMENT
				if (!(ev.motion.state & SDL_BUTTON_LMASK)) break;
#endif
#ifdef MOVE_CONST
				move(_v2conj(v2v(ev.motion.x, ev.motion.y) - v2draw_translation(ren)) / 100.0);
#else
				move(v2v(ev.motion.xrel, -ev.motion.yrel) / 100.0);
#endif
				collides = collide();
				break;

#ifdef SET
			case SDL_MOUSEBUTTONDOWN:
				if (ev.button.button == SDL_BUTTON_LEFT) {
					set(_v2conj(v2v(ev.button.x, ev.motion.y) - v2draw_translation(ren)) / 100.0);
					collides = collide();
				}
				break;
#endif
			}
		}

		SDL_SetRenderDrawColor(ren, 0, 0, 0, SDL_ALPHA_OPAQUE);
		SDL_RenderClear(ren);
		if (collides) {
			SDL_SetRenderDrawColor(ren, 255, 0, 255, SDL_ALPHA_OPAQUE);
		} else {
			SDL_SetRenderDrawColor(ren, 255, 255, 255, SDL_ALPHA_OPAQUE);
		}
		draw(ren);
		SDL_RenderPresent(ren);
	}
}
