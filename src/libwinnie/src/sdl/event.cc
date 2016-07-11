#ifdef WINNIE_SDL
#include <stdlib.h>
#include <SDL/SDL.h>

#include "event.h"
#include "keyboard.h"
#include "mouse.h"
#include "wm.h"

enum {
	TIMER_EVENT = SDL_USEREVENT
};

SDL_Event sdl_event;
void process_events()
{
	wm->process_windows();
	if(!SDL_WaitEvent(&sdl_event)) {
		return;
	}

	do {
		switch(sdl_event.type) {
		case SDL_KEYDOWN:
		case SDL_KEYUP:
			process_keyboard_event();
			break;
		case SDL_MOUSEMOTION:
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			process_mouse_event();
			break;
		case SDL_QUIT:
			exit(0);

		case TIMER_EVENT:
			{
				Window *win = (Window*)sdl_event.user.data1;
				TimerFuncType func = win->get_timer_callback();
				if(func) {
					func(win);
				} else {
					fprintf(stderr, "timer gone off but window has no timer callback!\n");
				}
			}
			break;

		default:
			break;
		}
	} while(SDL_PollEvent(&sdl_event));
}

struct TimerData {
	SDL_TimerID sdl_timer;
	Window *win;
	TimerMode mode;
};

static unsigned int timer_callback(unsigned int interval, void *cls)
{
	TimerData *td = (TimerData*)cls;

	SDL_Event ev;
	ev.type = TIMER_EVENT;
	ev.user.data1 = td->win;
	SDL_PushEvent(&ev);

	if(td->mode == TIMER_ONESHOT) {
		delete td;
		return 0;
	}
	return interval;	// repeat at same interval
}

void set_window_timer(Window *win, unsigned int msec, TimerMode mode)
{
	if(!win->get_timer_callback()) {
		fprintf(stderr, "trying to start a timer without having a timer callback!\n");
		return;
	}
	TimerData *td = new TimerData;
	td->win = win;
	td->mode = mode;
	td->sdl_timer = SDL_AddTimer(msec, timer_callback, td);
}

#endif // WINNIE_SDL
