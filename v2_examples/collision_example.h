#include <SDL2/SDL.h>

#define V2_IMPL
#define V2DRAW_IMPL
#include "../v2.h"
#include "../v2draw.h"

#define trace(msg, offset) exit((fprintf(stderr, "%s:%d(%s) %s\n", __FILE__, __LINE__ + (offset), __func__, (msg)), 1))
#define sdlerr(offset) trace(SDL_GetError(), offset)

void init(void);
void move(v2v v);
void set(v2v v);
#ifdef MTV
v2v collide(void);
v2v mtv_origin(void);
#else
_Bool collide(void);
#endif
void draw(SDL_Renderer *ren);

int main() {
	SDL_Init(SDL_INIT_VIDEO);
	atexit(SDL_Quit);

	SDL_Window *win = SDL_CreateWindow("Polygon collision", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN);
	if (!win) sdlerr(-1);
	SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (!ren) sdlerr(-1);

	init();
#ifdef MTV
	v2v mtv, origin;
#define check_collide() mtv = collide(), origin = mtv_origin()
#else
	_Bool collides;
#define check_collide() collides = collide()
#endif
	check_collide();

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
				move(v2conj(v2v(ev.motion.x, ev.motion.y) - v2draw_translation(ren)) / 100.0);
#else
				move(v2v(ev.motion.xrel, -ev.motion.yrel) / 100.0);
#endif
				check_collide();
				break;

#ifdef SET
			case SDL_MOUSEBUTTONDOWN:
				if (ev.button.button == SDL_BUTTON_LEFT) {
					set(v2conj(v2v(ev.button.x, ev.motion.y) - v2draw_translation(ren)) / 100.0);
					check_collide();
				}
				break;
#endif
			}
		}

		SDL_SetRenderDrawColor(ren, 0, 0, 0, SDL_ALPHA_OPAQUE);
		SDL_RenderClear(ren);

#ifdef MTV
		if (!v2nan(mtv)) {
			SDL_SetRenderDrawColor(ren, 255, 0, 255, SDL_ALPHA_OPAQUE);
			v2draw_vec(ren, origin, mtv);
		} else {
			SDL_SetRenderDrawColor(ren, 255, 255, 255, SDL_ALPHA_OPAQUE);
		}
#else
		if (collides) {
			SDL_SetRenderDrawColor(ren, 255, 0, 255, SDL_ALPHA_OPAQUE);
		} else {
			SDL_SetRenderDrawColor(ren, 255, 255, 255, SDL_ALPHA_OPAQUE);
		}
#endif

		draw(ren);
		SDL_RenderPresent(ren);
	}
}
